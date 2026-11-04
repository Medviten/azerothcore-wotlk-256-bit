/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "RaceMgr.h"
#include "AccountMgr.h"
#include "StringConvert.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Util.h"
#include "WorldPacket.h"
#include "World.h"
#include "WorldSession.h"

uint16 RaceMgr::_maxRaces = 0;

RaceMask RaceMgr::_playableRaceMask;
RaceMask RaceMgr::_allianceRaceMask;
RaceMask RaceMgr::_hordeRaceMask;
RaceMask RaceMgr::_maleOnlyRaceMask;
std::unordered_map<uint8, RaceMgr::RaceTemplate> RaceMgr::_raceTemplates;

RaceMgr::RaceMgr()
{
}

RaceMgr::~RaceMgr()
{
}

RaceMgr* RaceMgr::instance()
{
    static RaceMgr instance;
    return &instance;
}

void RaceMgr::LoadRaces()
{
    SetMaxRaces(0 + 1);
    _playableRaceMask = RaceMask();
    _hordeRaceMask    = RaceMask();
    _allianceRaceMask = RaceMask();
    _maleOnlyRaceMask = RaceMask();

    for (auto const& raceEntry : sChrRacesStore)
    {
        if (!raceEntry)
            continue;

        uint8 alliance = raceEntry->alliance;
        uint8 raceId = raceEntry->RaceID;

        if (raceEntry->Flags & CHRRACES_FLAGS_NOT_PLAYABLE)
            continue;

        if (GetMaxRaces() <= raceId)
            SetMaxRaces(raceId + 1);

        RaceMask raceBit = RaceMask::FromRace(raceId);

        _playableRaceMask |= raceBit;

        if (alliance == ALLIANCE_HORDE)
            _hordeRaceMask |= raceBit;
        else if (alliance == ALLIANCE_ALLIANCE)
            _allianceRaceMask |= raceBit;

        // no female model: the female display shows the male model
        CreatureDisplayInfoEntry const* maleDisplay = sCreatureDisplayInfoStore.LookupEntry(raceEntry->model_m);
        CreatureDisplayInfoEntry const* femaleDisplay = sCreatureDisplayInfoStore.LookupEntry(raceEntry->model_f);
        if (maleDisplay && femaleDisplay && maleDisplay->ModelId == femaleDisplay->ModelId)
            _maleOnlyRaceMask |= raceBit;
    }

    LoadRaceTemplates();
}

void RaceMgr::LoadRaceTemplates()
{
    _raceTemplates.clear();

    QueryResult result = WorldDatabase.Query("SELECT race, home_race, kin_race, shapeshift_skin_color FROM player_race_template");
    if (!result)
    {
        LOG_INFO("server.loading", ">> Loaded 0 race templates. DB table `player_race_template` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        _raceTemplates[fields[0].Get<uint8>()] = { fields[1].Get<uint8>(), fields[2].Get<uint8>(), fields[3].Get<uint8>() != 0 };
    } while (result->NextRow());

    LOG_INFO("server.loading", ">> Loaded {} race templates", _raceTemplates.size());
}

uint8 RaceMgr::GetHomeRace(uint8 race)
{
    auto itr = _raceTemplates.find(race);
    return itr != _raceTemplates.end() ? itr->second.homeRace : race;
}

uint8 RaceMgr::GetKinRace(uint8 race)
{
    auto itr = _raceTemplates.find(race);
    return itr != _raceTemplates.end() ? itr->second.kinRace : race;
}

uint8 RaceMgr::GetLookalikeRace(uint8 race)
{
    uint8 const kin = GetKinRace(race);
    return kin && kin != uint8(DisplayRace::Goblin) ? kin : GetHomeRace(race);
}

bool RaceMgr::UsesSkinColorForms(uint8 race)
{
    auto itr = _raceTemplates.find(race);
    return itr != _raceTemplates.end() && itr->second.shapeshiftSkinColor;
}

RaceMask RaceMgr::ReadRaceMask(Field const* fields, uint32 lowIndex, uint32 highIndex)
{
    // signed columns (item_template.AllowableRace) hold -1, read as all 64 bits set
    auto readWord = [](Field const& field) -> uint64
    {
        std::string_view value = field.Get<std::string_view>();
        if (Optional<uint64> word = Acore::StringTo<uint64>(value))
            return *word;
        return uint64(Acore::StringTo<int64>(value).value_or(0));
    };

    std::array<uint64, RaceMask::WORD64_COUNT> words;
    words[0] = readWord(fields[lowIndex]);
    for (uint32 i = 1; i < RaceMask::WORD64_COUNT; ++i)
        words[i] = readWord(fields[highIndex + i - 1]);

    return RaceMask::FromWords64(words);
}

RaceMask RaceMgr::GetCreationDisabledRaceMask()
{
    std::string_view value = sWorld->getStringConfig(CONFIG_CHARACTER_CREATING_DISABLED_RACEMASK);
    Optional<RaceMask> raceMask = RaceMask::FromString(value);
    if (!raceMask)
    {
        LOG_ERROR("server.loading", "CharacterCreating.Disabled.RaceMask has malformed value '{}', no race is disabled.",
            value);
        return RaceMask();
    }

    return *raceMask;
}
