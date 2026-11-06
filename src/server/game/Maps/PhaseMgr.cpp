/*
 * Zeppelin custom content — Cata-style zone phasing progression (F-194).
 * See PhaseMgr.h for the design.
 */

#include "PhaseMgr.h"
#include "ConditionMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldPacket.h"

void PhaseMgr::Recalculate()
{
    if (!_player || !sWorld->getBoolConfig(CONFIG_GOBLIN_START_ZONE))
        return;

    PhaseDefinitionStore const& store = sObjectMgr->GetPhaseDefinitionStore();
    PhaseDefinitionStore::const_iterator itr = store.find(_player->GetZoneId());
    if (itr == store.end())
    {
        if (_terrainSwap)
            SendTerrainSwap(0); // left a zone with swapped terrain
        return; // this zone is not phased — leave the phasemask as-is (auras / other systems own it)
    }

    if (_player->IsGameMaster())
    {
        if (_player->GetPhaseMask() != uint32(PHASEMASK_ANYWHERE))
            _player->SetPhaseMask(PHASEMASK_ANYWHERE, true);
        if (_terrainSwap)
            SendTerrainSwap(0); // GMs see every phase, on the base terrain
        return;
    }

    uint32 phasemask = 0;
    uint32 terrainSwap = 0; // WHE F-102: the terrainswapmap of the definition that set the phasemask last
    for (PhaseDefinition const& def : itr->second)
    {
        ConditionList const conditions = sConditionMgr->GetConditionsForPhaseDefinition(def.zoneId, def.entry);
        if (!sConditionMgr->IsObjectMeetToConditions(_player, conditions))
            continue;

        if (def.IsOverwritingExistingPhases())
        {
            phasemask = def.phasemask;
            terrainSwap = def.terrainswapmap;
        }
        else if (def.terrainswapmap && !def.IsNegatingPhasemask())
            terrainSwap = def.terrainswapmap;
        else if (def.IsNegatingPhasemask())
            phasemask &= ~def.phasemask;
        else
            phasemask |= def.phasemask;

        if (def.IsLastDefinition())
            break;
    }

    uint32 const finalMask = phasemask ? phasemask : uint32(PHASEMASK_NORMAL);
    if (_player->GetPhaseMask() != finalMask)
        _player->SetPhaseMask(finalMask, true);

    if (terrainSwap != _terrainSwap)
        SendTerrainSwap(terrainSwap);
}

void PhaseMgr::SendTerrainSwap(uint32 swapMapId)
{
    _terrainSwap = swapMapId;
    WorldPacket data(SMSG_SET_PHASE_SHIFT, 12);
    data << uint32(_player->GetPhaseMask());
    data << uint32(swapMapId);
    data << int32(_player->GetMapId());
    _player->SendDirectMessage(&data);
}

uint32 TerrainSwapInfo::Resolve(uint32 phaseMask) const
{
    // A player's mask is always exactly one definition's (every swap zone's rows OVERWRITE), so an
    // exact match is the same pick PhaseMgr::Recalculate sends the client.
    for (TerrainSwapRule const& rule : rules)
        if (rule.phasemask == phaseMask)
            return rule.swapMapId;

    // Creatures/GOs: one also visible in the normal world is seen on base terrain too - keep it.
    if (phaseMask & 0x1) // PHASEMASK_NORMAL
        return 0;

    // Otherwise swap only when every definition the mask overlaps shows the same terrain
    // (e.g. a creature in phase 8 is seen by 16392/32776/32808 players, all on 659).
    uint32 found = 0;
    bool any = false;
    for (TerrainSwapRule const& rule : rules)
    {
        if (!(rule.phasemask & phaseMask))
            continue;

        if (any && rule.swapMapId != found)
            return 0;

        found = rule.swapMapId;
        any = true;
    }

    return found;
}
