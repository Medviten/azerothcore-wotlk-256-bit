/*
 * Zeppelin custom content — Cata-style zone phasing progression (F-194).
 *
 * Server-side, definition-driven phasing backported from TrinityCore 4.3.4 (Project
 * Neltharion) so a zone's world-state progresses as the player quests. Per-zone ordered
 * phase_definitions are gated by conditions (CONDITION_SOURCE_TYPE_PHASE) on quest state;
 * the resolved phasemask is applied via Player::SetPhaseMask (visibility). Terrain swaps
 * (terrainswapmap/phaseId) are NOT applied here — deferred until the swap terrains are
 * ported; those columns are loaded but ignored.
 */

#ifndef ZEPPELIN_PHASEMGR_H
#define ZEPPELIN_PHASEMGR_H

#include "Define.h"
#include <list>
#include <unordered_map>

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
    uint32 terrainswapmap = 0;   // deferred (needs swap terrain ported) — ignored here
    uint32 originmap      = 0;
    uint8  flags          = 0;

    [[nodiscard]] bool IsOverwritingExistingPhases() const { return (flags & PHASE_FLAG_OVERWRITE_EXISTING) != 0; }
    [[nodiscard]] bool IsLastDefinition() const { return (flags & PHASE_FLAG_NO_MORE_PHASES) != 0; }
    [[nodiscard]] bool IsNegatingPhasemask() const { return (flags & PHASE_FLAG_NEGATE_PHASE) != 0; }
};

typedef std::list<PhaseDefinition> PhaseDefinitionContainer;
typedef std::unordered_map<uint32 /*zoneId*/, PhaseDefinitionContainer> PhaseDefinitionStore;

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
    Player* _player;
};

#endif // ZEPPELIN_PHASEMGR_H
