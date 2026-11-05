/*
 * Zeppelin custom content — F-011 Goblin Starting Zone (Lost Isles / Kezan).
 *
 * Quest-item spell scripts ported from Cataclysm (Project Neltharion). The source
 * effects are DUMMY/SCRIPT_EFFECT whose behaviour lives in core C++; here we
 * reimplement the quest-credit behaviour for 3.3.5a.
 *
 * Registered spell_script_names live in the zep-goblin-start zpak SQL.
 */

#include "Creature.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"

#include <algorithm>
#include <cmath>
#include <list>

enum ZepGoblinCredits
{
    SPELL_WEED_WHACKER            = 68211, // DUMMY
    SPELL_WEED_WHACKER_AURA       = 68212, // whirlwind state, periodic 68213 mow pulse
    SPELL_KTC_SNAPFLASH           = 68280, // DUMMY (photograph target -> credit that entry)

    // q24942 "Zombies vs. Super Booster Rocket Boots" (I-340)
    QUEST_ZOMBIES_VS_BOOTS        = 24942,
    SPELL_SUPER_BOOSTER_ROCKET_BOOTS = 72891, // SCRIPT_EFFECT, item 84320 on-use
    SPELL_BOOTS_SUMMON            = 72889, // SUMMON 38802 (SummonProperties 827, vehicle)
    SPELL_BOOTS_CONTROL_VEHICLE   = 72887, // CONTROL_VEHICLE on the boots + DUMMY on the rider
    SPELL_BOOTS_BURNING_AURA      = 72885, // 500ms periodic -> 72886, the 8yd fire nova

    // Gallywix Labor Mine chain (I-357)
    QUEST_KAJACOLA_IDEAS          = 25110, // turn-in fires the one and only summon
    QUEST_WILD_MINE_CART_RIDE     = 25184, // end of the window; the companion AI despawns on it
    NPC_ASSISTANT_GREELY_FOLLOWER = 39199,
    SPELL_RESUMMON_GREELY         = 73648, // SUMMON 39199, SummonProperties 2989, permanent
    AREA_GALLYWIX_LABOR_MINE      = 4924
};

// I-340 Super Booster Rocket Boots (q24942): the item's SCRIPT_EFFECT summons
// the boots vehicle (38802) and boards the player, who then burns Goblin Zombies
// (38753) by running over them. Beat for beat from the donor's
// Spell::EffectScriptEffect case 72891 (SpellEffects.cpp:4446): gated on the
// quest being active and on the player not already wearing a pair.
//
// It does NOT grant credit. Credit is the zombie's: 38753 carries
// KillCredit1 = 38807, so the fire damage killing it pays the objective once per
// zombie. The port previously wired this spell to a generic fixed-credit script,
// which banked exactly one credit per click and never touched a zombie.
//
// The donor gated the re-use check on `HasAura(72887)` and we deliberately do
// NOT copy that. 72887 applies CONTROL_VEHICLE to the boots (effect 0, target 25)
// and a permanent DUMMY to the player (effect 1, target 1), so the player's half
// is a separate permanent Aura object that nothing in AC removes when the ride
// ends — `Unit::ExitVehicle` only strips CONTROL_VEHICLE from the vehicle base.
// Gating on it would let one ride lock the boots out for good. `GetVehicle()` is
// the same question asked of real state. (The boots AI strips the stale dummy on
// unboard as well, so it does not sit on the buff bar either.)
class spell_zep_q24942_zombies_booster : public SpellScript
{
    PrepareSpellScript(spell_zep_q24942_zombies_booster);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_BOOTS_SUMMON, SPELL_BOOTS_CONTROL_VEHICLE });
    }

    void HandleScript(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;

        if (player->GetQuestStatus(QUEST_ZOMBIES_VS_BOOTS) != QUEST_STATUS_INCOMPLETE)
            return;

        if (player->GetVehicle())
            return;

        player->CastSpell(player, SPELL_BOOTS_SUMMON, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_zep_q24942_zombies_booster::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

// I-340 the ride itself. 72887 is cast by the player on the summoned boots and
// carries TWO effects with different targets, so AC builds two separate Aura
// objects: effect 0 CONTROL_VEHICLE on the BOOTS (target 25, the explicit
// target), effect 1 DUMMY on the RIDER (target 1, the caster). This script rides
// the rider's half and is what actually lights the boots.
//
// Retail did the same thing from the seat: 72897 "Zombies vs. Super Booster
// Rocket Boots: AICast on Enter Seat" (effect 160, TARGET_UNIT_MASTER 27)
// triggers 72885 onto the controller when the seat is entered. 3.3.5a has no
// enter-seat cast, and the obvious substitute — the boots' own
// CreatureAI::PassengerBoarded — CANNOT WORK on this vehicle:
//
//     Vehicle::AddPassenger charms the vehicle to the player for a
//     VEHICLE_SEAT_FLAG_CAN_CONTROL seat (Vehicle.cpp:432), and
//     Unit::SetCharmedBy -> CreatureAI::OnCharmed ends with
//     `me->IsAIEnabled = false` (CreatureAI.cpp:54). The PassengerBoarded call
//     40 lines later is gated on IsAIEnabled (Vehicle.cpp:496), so it never
//     fires — and Creature::Update skips UpdateAI for the same reason. EVERY
//     CreatureAI hook on a DRIVEN vehicle is dead for the whole ride.
//
// The 72887 aura, by contrast, is exactly co-terminous with the ride: AC removes
// the rider's half itself when the ride ends
// (AuraEffect::HandleAuraControlVehicle's remove branch does
// `caster->RemoveAurasDueToSpell(GetId())`), whether the player right-clicks
// off, the boots despawn, or they log out. So apply and remove are both
// guaranteed here and nowhere else.
class spell_zep_q24942_boots_ride : public AuraScript
{
    PrepareAuraScript(spell_zep_q24942_boots_ride);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_BOOTS_BURNING_AURA });
    }

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* rider = GetTarget();
        if (rider->IsPlayer())
            rider->CastSpell(rider, SPELL_BOOTS_BURNING_AURA, true);
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(SPELL_BOOTS_BURNING_AURA);
    }

    void Register() override
    {
        AfterEffectApply  += AuraEffectApplyFn(spell_zep_q24942_boots_ride::HandleApply, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_zep_q24942_boots_ride::HandleRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// I-307 Weed Whacker (q14236): the item's dummy (68211) toggles the whirlwind
// aura 68212, whose periodic pulse (68213) mows the Deadly Jungle Plants. No
// direct credit here — the plants (35897) self-credit on death.
class spell_zep_q14236_weed_whacker : public SpellScript
{
    PrepareSpellScript(spell_zep_q14236_weed_whacker);

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        if (caster->HasAura(SPELL_WEED_WHACKER_AURA))
            caster->RemoveAurasDueToSpell(SPELL_WEED_WHACKER_AURA);
        else
            caster->CastSpell(caster, SPELL_WEED_WHACKER_AURA, true);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_zep_q14236_weed_whacker::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// KTC Snapflash: photograph the targeted creature -> credit its own entry.
class spell_zep_q14031_ktc_snapflash : public SpellScript
{
    PrepareSpellScript(spell_zep_q14031_ktc_snapflash);

    void HandleCredit(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;
        if (Creature* target = GetHitCreature())
            player->KilledMonsterCredit(target->GetEntry());
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_zep_q14031_ktc_snapflash::HandleCredit, EFFECT_0, SPELL_EFFECT_ANY);
    }
};

// I-248 Fourth and Goal (q28414): score detection for Throw Footbomb (69992,
// cast by the possessed Bilgewater Buccaneer 37179). The KTC goal is the gap
// between the smokestacks — a vertical quad GM-mapped in-game: Z 110..177 above
// the line (-8935.1,-11336.4) -> (-8944.3,-11296.0). Data-side detection cannot
// express "the ball crossed the plane" (dest areas only see the landing point;
// the traj corridor is missile-width), so: intersect the shot's XY path with the
// goal line and evaluate the ball height there using the same endpoint +
// launch-angle parabola the trajectory system uses (Spell::SelectImplicitTrajTargets);
// credit goal bunny 37203 when it passes inside the frame. No-op off-quest.
class spell_zep_q28414_footbomb_goal : public SpellScript
{
    PrepareSpellScript(spell_zep_q28414_footbomb_goal);

    static constexpr float GOAL_X1 = -8935.1f;
    static constexpr float GOAL_Y1 = -11336.4f;
    static constexpr float GOAL_X2 = -8944.3f;
    static constexpr float GOAL_Y2 = -11296.0f;
    static constexpr float GOAL_Z_MIN = 106.0f;
    static constexpr float GOAL_Z_MAX = 180.0f;
    static constexpr uint32 NPC_FOURTH_AND_GOAL_TARGET = 37203;

    void CheckGoal()
    {
        Unit* caster = GetCaster();
        WorldLocation const* dest = GetExplTargetDest();
        if (!caster || !dest)
            return;

        Player* player = caster->GetCharmerOrOwnerPlayerOrPlayerItself();
        if (!player)
            return;

        float const sx = caster->GetPositionX();
        float const sy = caster->GetPositionY();
        float const sz = caster->GetPositionZ();
        float const rx = dest->GetPositionX() - sx;
        float const ry = dest->GetPositionY() - sy;
        float const qx = GOAL_X2 - GOAL_X1;
        float const qy = GOAL_Y2 - GOAL_Y1;

        float const denom = rx * qy - ry * qx;
        if (std::fabs(denom) < 0.0001f)
            return;

        float const ex = GOAL_X1 - sx;
        float const ey = GOAL_Y1 - sy;
        float const t = (ex * qy - ey * qx) / denom; // fraction along the shot
        float const s = (ex * ry - ey * rx) / denom; // fraction along the goal line
        if (t < 0.0f || t > 1.0f || s < 0.0f || s > 1.0f)
            return;

        // Ball height at the crossing: parabola through the endpoints leaving at
        // the client-sent launch angle (matches the rendered aim arc); straight
        // chord as fallback when no trajectory data reached us.
        float const dist2d = std::sqrt(rx * rx + ry * ry);
        float zCross;
        SpellCastTargets const& targets = GetSpell()->m_targets;
        if (dist2d > 1.0f && targets.HasTraj())
        {
            float const b = std::tan(targets.GetElevation());
            float const a = (dest->GetPositionZ() - sz - b * dist2d) / (dist2d * dist2d);
            float const d = t * dist2d;
            zCross = sz + b * d + a * d * d;
        }
        else
            zCross = sz + t * (dest->GetPositionZ() - sz);

        if (zCross < GOAL_Z_MIN || zCross > GOAL_Z_MAX)
            return;

        // Credit when the ball actually reaches the goal plane, not at launch:
        // flight time to the crossing from the client-sent trajectory speed
        // (fall back to the spell's missile render speed of 40 yd/s).
        float const d = t * dist2d;
        float speedXY = targets.HasTraj() ? targets.GetSpeedXY() : 40.0f;
        if (speedXY < 1.0f)
            speedXY = 40.0f;
        uint32 const delay = std::min<uint32>(uint32(d / speedXY * 1000.0f), 8000);

        ObjectGuid const playerGuid = player->GetGUID();
        player->m_Events.AddEventAtOffset([playerGuid]()
        {
            if (Player* scorer = ObjectAccessor::FindPlayer(playerGuid))
                scorer->KilledMonsterCredit(NPC_FOURTH_AND_GOAL_TARGET);
        }, Milliseconds(delay));
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_zep_q28414_footbomb_goal::CheckGoal);
    }
};

// I-259 Kaja'Cola (item 84474, spell 70478): "It gives you IDEAS! (TM)". The
// spell only applies a 5s dummy aura; on retail the drinker sometimes blurts
// out a random goblin invention idea (Cata broadcast_text 37326-37365, already
// imported with the zone). Spoken after a short "thinking" delay inside the
// aura window.
class spell_zep_item_kajacola : public AuraScript
{
    PrepareAuraScript(spell_zep_item_kajacola);

    static constexpr uint32 IDEA_TEXT_FIRST = 37326;
    static constexpr uint32 IDEA_TEXT_LAST  = 37365;

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Player* player = GetTarget()->ToPlayer();
        if (!player)
            return;

        BroadcastText const* bct = sObjectMgr->GetBroadcastText(urand(IDEA_TEXT_FIRST, IDEA_TEXT_LAST));
        if (!bct)
            return;

        ObjectGuid const playerGuid = player->GetGUID();
        std::string idea = bct->GetText(LOCALE_enUS, player->getGender());
        player->m_Events.AddEventAtOffset([playerGuid, idea = std::move(idea)]()
        {
            if (Player* drinker = ObjectAccessor::FindPlayer(playerGuid))
                drinker->Say(idea, LANG_UNIVERSAL);
        }, Milliseconds(urand(1500, 3500)));
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_zep_item_kajacola::HandleApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// I-262 Waltz Right In (q14123): carrier 67435's 5s periodic re-applies the mook
// disguise (70467). Retail gated this on being out of combat (kezan.cpp); without
// the gate the tick re-casts mid-fight and the next hit knocks it straight back
// off — buff/transform flicker for the whole scrap. While in combat the tick
// instead strips a lingering disguise (covers combat entries the aura interrupt
// flags can't see, e.g. opening with a ranged/spell attack).
class spell_zep_mook_disguise_carrier : public AuraScript
{
    PrepareAuraScript(spell_zep_mook_disguise_carrier);

    static constexpr uint32 SPELL_MOOK_DISGUISE = 70467;

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        Unit* target = GetTarget();
        if (target->IsInCombat())
        {
            PreventDefaultAction();
            target->RemoveAurasDueToSpell(SPELL_MOOK_DISGUISE);
        }
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_zep_mook_disguise_carrier::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
    }
};

/* ---------------------------------------------------------------------------
 * I-357 — Assistant Greely persistence.
 *
 * The Gallywix Labor Mine chain (q25122 Morale Boost / q25123 Throw It On the
 * Ground! / q25124 Brute Brutality / q25125 Light at the End of the Tunnel) has
 * exactly one quest giver: the FOLLOWER Greely, 39199. She is a summon, cast
 * once by 38124's SmartAI on the q25110 turn-in (SMART_EVENT_QUEST_REWARDED ->
 * SMART_ACTION_INVOKER_CAST 73648) and never again.
 *
 * A summon does not survive a logout or a worldserver restart, and
 * npc_zep_wild_cart_companion despawns her on player death or at 120 yd —
 * faithfully, the donor's npc_assistant_greedy_kajacolaAI has the same three
 * branches. Nothing brings her back: the donor's 1 s heartbeat (aura 73607 ->
 * periodic 73631 "Dummy To Assistant Greely") is a GREET ping handled by the
 * donor's SpellHit (IDEA visual + Talk), not a resummon, and we ship no script
 * for it. Any of those four events therefore stranded the character with four
 * unobtainable quests and no way to recover.
 *
 * Restore her whenever the player is standing in the mine inside the window the
 * companion AI itself considers live: q25110 rewarded, q25184 not yet finished,
 * and no Greely of ours already out. Re-casting 73648 (rather than spawning her
 * directly) keeps the donor plumbing intact — SummonProperties 2989, ownership,
 * and the AI's IsSummonedBy all run exactly as they do on the real turn-in.
 * --------------------------------------------------------------------------- */
class player_zep_greely_persistence : public PlayerScript
{
public:
    // Opt in to just the three hooks we use — an empty list enables ALL of them,
    // which would put this script on every player hook in the core for nothing.
    player_zep_greely_persistence() : PlayerScript("player_zep_greely_persistence",
        { PLAYERHOOK_ON_LOGIN, PLAYERHOOK_ON_UPDATE_AREA, PLAYERHOOK_ON_PLAYER_RESURRECT }) { }

    void OnPlayerLogin(Player* player) override
    {
        // Covers the relog and the worldserver restart: the summon is simply gone.
        TryRestore(player, player->GetAreaId());
    }

    void OnPlayerUpdateArea(Player* player, uint32 /*oldArea*/, uint32 newArea) override
    {
        // Covers the 120 yd walk-away and the corpse run back in.
        TryRestore(player, newArea);
    }

    void OnPlayerResurrect(Player* player, float /*restorePercent*/, bool& /*applySickness*/) override
    {
        // Covers a rez taken in place, where no area change ever fires.
        TryRestore(player, player->GetAreaId());
    }

private:
    static void TryRestore(Player* player, uint32 areaId)
    {
        if (!player || !player->IsInWorld() || areaId != AREA_GALLYWIX_LABOR_MINE || !player->IsAlive())
            return;

        if (player->GetQuestStatus(QUEST_KAJACOLA_IDEAS) != QUEST_STATUS_REWARDED)
            return;

        QuestStatus cart = player->GetQuestStatus(QUEST_WILD_MINE_CART_RIDE);
        if (cart == QUEST_STATUS_REWARDED || cart == QUEST_STATUS_COMPLETE)
            return;

        // Deliberately NOT gated on HasAura(73616) "Summon Assistant Greely Controller".
        // 73648 does apply that marker to the player: its effect 2 is FORCE_CAST_2 with
        // TriggerSpell 73616 and TARGET_UNIT_CASTER, so EffectForceCast has the caster
        // cast it on itself. But it is a PERMANENT player aura, so it is saved and
        // restored across the very logout or restart that killed the summon — after
        // exactly the failure this script exists for, the player holds 73616 with no
        // Greely and the aura reports her present. Real state is the creature, not the
        // marker. (Same trap as the stale 72887 boots dummy noted above.)
        //
        // Ownership matters: the nearest 39199 in a busy mine is very likely to be
        // someone else's, and FindNearestCreature would happily hand it back.
        std::list<Creature*> greelys;
        player->GetCreatureListWithEntryInGrid(greelys, NPC_ASSISTANT_GREELY_FOLLOWER, 150.0f);
        for (Creature* greely : greelys)
            if (greely->GetOwnerGUID() == player->GetGUID())
                return;

        player->CastSpell(player, SPELL_RESUMMON_GREELY, true);
    }
};

void AddSC_zeppelin_goblin_start()
{
    // q14019 Monkey Business has no C++ script: the credit is granted by the fed
    // monkey's own SmartAI (34699 SPELLHIT 67917 -> actionlist 3469900), which
    // also roots, explodes and despawns it. See zz_[I-281]_monkey_business_feed.sql.
    // q24671 Cluster Cluck likewise has no C++ script (I-318). The generic
    // fixed-credit script paid the caster on ANY effect hit, so 71170's dummy
    // banked a credit even when the item self-cast with nothing targeted, and
    // a real capture paid twice. Credit belongs to the Wild Clucker's own
    // SmartAI (38111 SPELLHIT 71170 -> actionlist 3811100), which also clears
    // the spellclick flag and flies the bird off.
    // The generic `spell_zep_fixed_credit<Entry>` template those two used is GONE
    // (I-340). It credited the caster on ANY effect hit without looking at what
    // was hit, so every spell it was ever wired to either paid for nothing or
    // paid twice — I-281, I-318, and finally q24942, whose whole mechanic it was
    // standing in for. Quest credit belongs to the thing that was actually hit.
    RegisterSpellScript(spell_zep_q14236_weed_whacker);
    RegisterSpellScript(spell_zep_q24942_zombies_booster);
    RegisterSpellScript(spell_zep_q24942_boots_ride);
    RegisterSpellScript(spell_zep_q14031_ktc_snapflash);
    RegisterSpellScript(spell_zep_q28414_footbomb_goal);
    RegisterSpellScript(spell_zep_item_kajacola);
    RegisterSpellScript(spell_zep_mook_disguise_carrier);
    new player_zep_greely_persistence();
}
