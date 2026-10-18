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

    /// Reads a race mask column (see RaceMask::FromString). Logs and returns no value if it is malformed.
    static Optional<RaceMask> ReadRaceMask(Field const& field, std::string_view table, std::string_view column);

    /// Races players cannot create or change to, from CharacterCreating.Disabled.RaceMask.
    static RaceMask GetCreationDisabledRaceMask();
private:
    static uint16 _maxRaces; // Max playable race + 1

    static RaceMask _playableRaceMask;
    static RaceMask _allianceRaceMask;
    static RaceMask _hordeRaceMask;
};

#define sRaceMgr RaceMgr::instance()

#endif
