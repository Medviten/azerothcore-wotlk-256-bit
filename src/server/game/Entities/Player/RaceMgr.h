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

#ifndef __ACORE_RACEMGR_H
#define __ACORE_RACEMGR_H

#include "DatabaseEnv.h"
#include "ObjectGuid.h"
#include "RaceMask.h"
#include "SharedDefines.h"
#include <map>
#include <unordered_map>

class Player;
class WorldPacket;

class RaceMgr
{
private:
    RaceMgr();
    ~RaceMgr();

public:
    static RaceMgr* instance();

    static void LoadRaces();
    static uint16 GetMaxRaces() { return _maxRaces; }
    static void SetMaxRaces(uint16 max) { _maxRaces = max; }
    static RaceMask const& GetPlayableRaceMask() { return _playableRaceMask; }
    static RaceMask const& GetAllianceRaceMask() { return _allianceRaceMask; }
    static RaceMask const& GetHordeRaceMask() { return _hordeRaceMask; }

    /// Races without a female model: ChrRaces gives them a female display that uses the male display's model.
    /// Characters of these races are always created male.
    static bool IsMaleOnlyRace(uint8 race) { return _maleOnlyRaceMask.HasRace(race); }

    /**
     * @brief Reads a race mask stored as four BIGINT columns (see RaceMask::FromWords64()).
     * @param lowIndex Column of races 1-64: the original race mask column, so SQL bitwise math on it keeps working.
     * @param highIndex First of the three consecutive columns of races 65-128, 129-192 and 193-256.
     */
    static RaceMask ReadRaceMask(Field const* fields, uint32 lowIndex, uint32 highIndex);

    /// Races players cannot create or change to, from CharacterCreating.Disabled.RaceMask.
    static RaceMask GetCreationDisabledRaceMask();

    /// Loads player_race_template, which the race modpaks fill, so adding a race needs no code.
    static void LoadRaceTemplates();
    /// The stock race whose start area a race shares. A race without a template is its own home.
    static uint8 GetHomeRace(uint8 race);
    /// The race whose looks and language a race shares, or 0 for none. A race without a template is its own kin.
    static uint8 GetKinRace(uint8 race);
    /// The stock race a race can pass for: its kin, or its home if its kin is none or has no stock player model.
    static uint8 GetLookalikeRace(uint8 race);
    /// Whether a race's shapeshift forms follow skin (fur) color instead of hair color.
    static bool UsesSkinColorForms(uint8 race);

    /// Custom: the race mask layout of the loaded DBCs (32 or 256), set by LoadDBCStores()
    static void SetDBCRaceMaskBits(uint32 bits) { _dbcRaceMaskBits = bits; }
    /// Custom: uint32 fields per race mask in the item query and who packets: RaceMask.PacketBits, or the DBCs' layout
    /// (the matching client reads the same DBCs). 1 is the stock layout, RaceMask::FIELD_COUNT the 256-bit one.
    static uint32 GetPacketRaceMaskFields();
    static void WritePacketRaceMask(WorldPacket& data, RaceMask const& mask);
    static RaceMask ReadPacketRaceMask(WorldPacket& data);
private:
    struct RaceTemplate
    {
        uint8 homeRace;
        uint8 kinRace;
        bool shapeshiftSkinColor;
    };

    static uint16 _maxRaces; // Max playable race + 1

    static RaceMask _playableRaceMask;
    static RaceMask _allianceRaceMask;
    static RaceMask _hordeRaceMask;
    static RaceMask _maleOnlyRaceMask;
    static std::unordered_map<uint8, RaceTemplate> _raceTemplates;
    static uint32 _dbcRaceMaskBits;
};

#define sRaceMgr RaceMgr::instance()

#endif
