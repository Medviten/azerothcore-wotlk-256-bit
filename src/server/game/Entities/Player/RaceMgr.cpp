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
    }
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
