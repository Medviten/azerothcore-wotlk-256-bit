/*
 * Zeppelin custom content — Cata-style zone phasing progression (F-194).
 * See PhaseMgr.h for the design.
 */

#include "PhaseMgr.h"
#include "ConditionMgr.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "World.h"

void PhaseMgr::Recalculate()
{
    if (!_player || !sWorld->getBoolConfig(CONFIG_GOBLIN_START_ZONE))
        return;

    PhaseDefinitionStore const& store = sObjectMgr->GetPhaseDefinitionStore();
    PhaseDefinitionStore::const_iterator itr = store.find(_player->GetZoneId());
    if (itr == store.end())
        return; // this zone is not phased — leave the phasemask as-is (auras / other systems own it)

    if (_player->IsGameMaster())
    {
        if (_player->GetPhaseMask() != uint32(PHASEMASK_ANYWHERE))
            _player->SetPhaseMask(PHASEMASK_ANYWHERE, true);
        return;
    }

    uint32 phasemask = 0;
    for (PhaseDefinition const& def : itr->second)
    {
        ConditionList const conditions = sConditionMgr->GetConditionsForPhaseDefinition(def.zoneId, def.entry);
        if (!sConditionMgr->IsObjectMeetToConditions(_player, conditions))
            continue;

        if (def.IsOverwritingExistingPhases())
            phasemask = def.phasemask;
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
}
