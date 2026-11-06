/*
 * Zeppelin custom content — Cata-style zone phasing progression (F-194).
 *
 * Server-side, definition-driven phasing backported from TrinityCore 4.3.4 (Project
 * Neltharion) so a zone's world-state progresses as the player quests. Per-zone ordered
 * phase_definitions are gated by conditions (CONDITION_SOURCE_TYPE_PHASE) on quest state;
 * the resolved phasemask is applied via Player::SetPhaseMask (visibility). The CLIENT terrain
 * swap (terrainswapmap) is sent with SMSG_SET_PHASE_SHIFT (WHE F-102); the SERVER side of it (heights, liquid,
 * area, navmesh) is TerrainSwapInfo below, consumed by Map/GridTerrainLoader/PathGenerator (I-385).
 * phaseId is ignored on 3.3.5a.
 */

#ifndef ZEPPELIN_PHASEMGR_H
#define ZEPPELIN_PHASEMGR_H

#include "Define.h"
#include <list>
#include <unordered_map>
#include <vector>

class Player;

enum PhasingFlags
{
    PHASE_FLAG_OVERWRITE_EXISTING = 0x01, // replace the accumulated phasemask instead of OR-ing
    PHASE_FLAG_NO_MORE_PHASES     = 0x02, // stop after this definition matches
    PHASE_FLAG_NEGATE_PHASE       = 0x04  // clear these bits instead of setting them
};

struct PhaseDefinition
{
    uint32 zoneId         = 0;
    uint32 entry          = 0;
    uint32 phasemask      = 0;
    uint32 phaseId        = 0;   // client-side (terrain-swap era) — ignored on 3.3.5a
    uint32 terrainswapmap = 0;   // swap map id; server terrain via TerrainSwapInfo (I-385)
    uint32 originmap      = 0;
    uint8  flags          = 0;

    [[nodiscard]] bool IsOverwritingExistingPhases() const { return (flags & PHASE_FLAG_OVERWRITE_EXISTING) != 0; }
    [[nodiscard]] bool IsLastDefinition() const { return (flags & PHASE_FLAG_NO_MORE_PHASES) != 0; }
    [[nodiscard]] bool IsNegatingPhasemask() const { return (flags & PHASE_FLAG_NEGATE_PHASE) != 0; }
};

typedef std::list<PhaseDefinition> PhaseDefinitionContainer;
typedef std::unordered_map<uint32 /*zoneId*/, PhaseDefinitionContainer> PhaseDefinitionStore;

// I-385 server-side terrain swap. For a map that hosts a zone with terrainswapmap definitions:
// every definition of those zones (swapMapId 0 = that phase keeps the base terrain), and the
// distinct swap maps whose maps/<swap><gx><gy>.map overlay the base grids. Which grids actually
// swap is decided by which overlay files exist, so a phase bit only changes terrain where a
// swap tile was shipped.
struct TerrainSwapRule
{
    uint32 phasemask = 0;
    uint32 swapMapId = 0;
};

struct TerrainSwapInfo
{
    std::vector<TerrainSwapRule> rules;   // definition (entry) order
    std::vector<uint32> swapMapIds;

    // Swap map for an object's phasemask, 0 = base terrain.
    [[nodiscard]] uint32 Resolve(uint32 phaseMask) const;
};

typedef std::unordered_map<uint32 /*mapId*/, TerrainSwapInfo> TerrainSwapStore;

class PhaseMgr
{
public:
    explicit PhaseMgr(Player* player) : _player(player) { }

    // Recompute the player's definition-driven phasemask for their current zone and apply it.
    // Safe to call when the current zone has no phase_definitions (no-op).
    void Recalculate();

    // Fired when something that a phase condition could depend on changed (zone change,
    // quest accept/complete/reward/abandon, login). Currently just recalculates.
    void NotifyConditionChanged() { Recalculate(); }

private:
    // WHE F-102: tell the client which map's tiles replace the base terrain (0 = none). Our client reads the
    // swap map and its base map after SMSG_SET_PHASE_SHIFT's phase; stock clients only read the phase.
    void SendTerrainSwap(uint32 swapMapId);

    Player* _player;
    uint32 _terrainSwap = 0;
};

#endif // ZEPPELIN_PHASEMGR_H
