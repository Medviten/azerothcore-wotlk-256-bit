/*
 * Custom: F-137 Exile's Reach scripts, ported from TrinityCore's zone_exiles_reach.cpp (retail's new player island,
 * map 2175, its ships 2261 / 2369).
 *
 * Retail plays most of the island's story as "conversations" (a talking head per line) and runs its scripted steps on
 * personal copies of the NPCs, spells and phases 3.3.5a lacks. Here a conversation line is the NPC saying the line's
 * broadcast text (F-137 writes retail's broadcast texts at their ids: SayLine), the NPCs act in the shared world, and
 * personal summons (the followers) are visible to their summoner only. What retail does by spell (bandaging, summoning
 * a follower) happens on talking to the NPC or on accepting the quest. Lines are named by broadcast text id; each
 * pair is (Alliance, Horde), as retail's conversations carry both factions' lines.
 *
 * The creatures with a ScriptName here are F-137's SCRIPTED (tools/races/retail_world_port.py); the quest events
 * (ExilesReachQuestScript) need none.
 */

#include "CreatureScript.h"
#include "DBCStores.h"
#include "GameTime.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "PlayerScript.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "SpellInfo.h"
#include "TemporarySummon.h"

enum ExilesReachData
{
    // quests (Alliance, Horde)
    QUEST_WARMING_UP_A                  = 56775,
    QUEST_WARMING_UP_H                  = 59926,
    QUEST_STAND_YOUR_GROUND_A           = 58209,
    QUEST_STAND_YOUR_GROUND_H           = 59927,
    QUEST_BRACE_FOR_IMPACT_A            = 58208,
    QUEST_BRACE_FOR_IMPACT_H            = 59928,
    QUEST_MURLOC_MANIA_A                = 55122,
    QUEST_MURLOC_MANIA_H                = 59929,
    QUEST_FINDING_THE_LOST_EXPEDITION_A = 54952,
    QUEST_FINDING_THE_LOST_EXPEDITION_H = 59931,
    QUEST_COOKING_MEAT_A                = 55174,
    QUEST_COOKING_MEAT_H                = 59932,
    QUEST_ENHANCED_COMBAT_TACTICS_A     = 59254,
    QUEST_ENHANCED_COMBAT_TACTICS_H     = 59933,

    // the ships
    NPC_GARRICK_SHIP                    = 156280,
    NPC_GRIMAXE_SHIP_LOWER              = 166573,
    NPC_GRIMAXE_SHIP_LOWER2             = 166824,
    NPC_GRIMAXE_SHIP_UPPER              = 166827,
    NPC_COLE_SHIP                       = 160664,
    NPC_THROG_SHIP                      = 166583,
    CREDIT_SPAR                         = 155607,

    // the beach and the camp
    NPC_GARRICK_BEACH                   = 156626,
    NPC_GRIMAXE_BEACH                   = 166782,
    NPC_GARRICK_FOLLOWER                = 165359,
    NPC_GRIMAXE_FOLLOWER                = 166805,
    NPC_ALARIA_CAMP                     = 156607,
    NPC_GARRICK_CAMP                    = 156651,
    NPC_WONSA_CAMP                      = 166854,
    NPC_GRIMAXE_CAMP                    = 166906,
    NPC_TRAINER_A                       = 164577,   // also the quest's credit, for both factions
    NPC_TRAINER_H                       = 166916,

    // conversation lines and texts (broadcast text ids)
    LINE_WARMING_UP_ACCEPT_A            = 291036,
    LINE_WARMING_UP_ACCEPT_H            = 290957,
    LINE_WARMING_UP_DONE_A              = 291038,
    LINE_WARMING_UP_DONE_H              = 290960,
    LINE_SPAR_PREFIGHT_A                = 177651,
    LINE_SPAR_PREFIGHT_H                = 195852,
    LINE_SPAR_AGGRO_A                   = 184223,
    LINE_SPAR_AGGRO_H                   = 195853,
    LINE_SPAR_FACE_A                    = 177657,
    LINE_SPAR_FACE_H                    = 195854,
    LINE_SPAR_YIELD_A                   = 177677,
    LINE_SPAR_YIELD_H                   = 195857,
    LINE_BELOW_DECKS_A                  = 177674,
    LINE_BELOW_DECKS_H                  = 195893,
    LINE_STORM_A                        = 177675,
    LINE_STORM_H                        = 195892,
    LINE_BEACH_ARRIVAL_A                = 178506,
    LINE_BEACH_ARRIVAL_H                = 195918,
    LINE_MURLOC_MANIA_A                 = 291041,
    LINE_MURLOC_MANIA_H                 = 290964,
    LINE_EXPEDITION_A                   = 178509,
    LINE_EXPEDITION_H                   = 195958,
    LINE_CAMP_HUNGRY_A                  = 178598,
    LINE_CAMP_HUNGRY_H                  = 195968,
    LINE_CAMP_LEADER_A                  = 191634,
    LINE_CAMP_LEADER_H                  = 195969,
    LINE_COOKING_DONE_A                 = 184798,
    LINE_COOKING_DONE_H                 = 196667,
    LINE_COOKING_LEADER_A               = 192440,
    LINE_COOKING_LEADER_H               = 196668,
    LINE_TRAINING_WALK_A                = 191072,
    LINE_TRAINING_WALK_H                = 195981,
    LINE_TRAINING_READY_A               = 189218,
    LINE_TRAINING_READY_H               = 195985,
    LINE_TRAINING_RANGED_A              = 189404,
    LINE_TRAINING_RANGED_H              = 196061,
    LINE_TRAINING_CHARGE_A              = 189312,
    LINE_TRAINING_CHARGE_H              = 195989,
    LINE_TRAINING_CHARGE1_A             = 189308,
    LINE_TRAINING_CHARGE1_H             = 195993,
    LINE_TRAINING_CHARGE2_A             = 189315,
    LINE_TRAINING_CHARGE2_H             = 195997,
    LINE_TRAINING_DONE_A                = 189266,
    LINE_TRAINING_DONE_H                = 195999,
};

// Finding the Lost Expedition's goal: the abandoned camp (TrinityCore's camp point)
static Position const AbandonedCamp = { -249.059f, -2492.52f, 18.0742f, 0.0f };

// A conversation line: the speaker says the broadcast text to the player, after a delay (the speaker looked up again
// then, so it may have despawned or moved away meanwhile)
static void SayLine(Player* player, std::initializer_list<uint32> entries, uint32 broadcastTextId,
    Milliseconds delay = 0ms)
{
    std::vector<uint32> speakers(entries);
    ObjectGuid playerGuid = player->GetGUID();
    auto say = [playerGuid, speakers, broadcastTextId]()
    {
        Player* p = ObjectAccessor::FindPlayer(playerGuid);
        if (!p)
            return;
        Creature* best = nullptr;
        for (uint32 entry : speakers)
            if (Creature* c = p->FindNearestCreature(entry, 60.0f, true))
                if (!best || p->GetDistance(c) < p->GetDistance(best))
                    best = c;
        if (best)
            best->Say(broadcastTextId, p);
    };
    if (delay == 0ms)
        say();
    else
        player->m_Events.AddEventAtOffset(std::move(say), delay);
}

static bool IsAlliance(Player* player) { return player->GetTeamId() == TEAM_ALLIANCE; }

// ---- quest events ----------------------------------------------------------------------------------------------

class ExilesReachQuestScript : public PlayerScript
{
public:
    ExilesReachQuestScript() : PlayerScript("ExilesReachQuestScript",
        { PLAYERHOOK_ON_PLAYER_QUEST_ACCEPT, PLAYERHOOK_ON_PLAYER_COMPLETE_QUEST }) { }

    void OnPlayerQuestAccept(Player* player, Quest const* quest) override
    {
        switch (quest->GetQuestId())
        {
            case QUEST_WARMING_UP_A:
                SayLine(player, { NPC_GARRICK_SHIP }, LINE_WARMING_UP_ACCEPT_A);
                break;
            case QUEST_WARMING_UP_H:
                SayLine(player, { NPC_GRIMAXE_SHIP_LOWER, NPC_GRIMAXE_SHIP_LOWER2 }, LINE_WARMING_UP_ACCEPT_H);
                break;
            case QUEST_BRACE_FOR_IMPACT_A:
                SayLine(player, { NPC_GARRICK_SHIP }, LINE_BELOW_DECKS_A);
                SayLine(player, { NPC_COLE_SHIP }, LINE_STORM_A, 3s);
                break;
            case QUEST_BRACE_FOR_IMPACT_H:
                SayLine(player, { NPC_GRIMAXE_SHIP_UPPER, NPC_GRIMAXE_SHIP_LOWER }, LINE_BELOW_DECKS_H);
                SayLine(player, { NPC_THROG_SHIP }, LINE_STORM_H, 3s);
                break;
            case QUEST_MURLOC_MANIA_A:
                SayLine(player, { NPC_GARRICK_BEACH }, LINE_MURLOC_MANIA_A);
                break;
            case QUEST_MURLOC_MANIA_H:
                SayLine(player, { NPC_GRIMAXE_BEACH }, LINE_MURLOC_MANIA_H);
                break;
            case QUEST_FINDING_THE_LOST_EXPEDITION_A:
            case QUEST_FINDING_THE_LOST_EXPEDITION_H:
                SummonFollower(player, IsAlliance(player) ? NPC_GARRICK_FOLLOWER : NPC_GRIMAXE_FOLLOWER);
                break;
            case QUEST_ENHANCED_COMBAT_TACTICS_A:
            case QUEST_ENHANCED_COMBAT_TACTICS_H:
                SummonFollower(player, IsAlliance(player) ? NPC_TRAINER_A : NPC_TRAINER_H);
                break;
            default:
                break;
        }
    }

    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        switch (quest->GetQuestId())
        {
            case QUEST_WARMING_UP_A:
                SayLine(player, { NPC_GARRICK_SHIP }, LINE_WARMING_UP_DONE_A);
                break;
            case QUEST_WARMING_UP_H:
                SayLine(player, { NPC_GRIMAXE_SHIP_LOWER, NPC_GRIMAXE_SHIP_LOWER2 }, LINE_WARMING_UP_DONE_H);
                break;
            case QUEST_BRACE_FOR_IMPACT_A:      // shipwrecked: the turn-in lands the player on the beach
                SayLine(player, { NPC_GARRICK_BEACH }, LINE_BEACH_ARRIVAL_A, 4s);
                break;
            case QUEST_BRACE_FOR_IMPACT_H:
                SayLine(player, { NPC_GRIMAXE_BEACH }, LINE_BEACH_ARRIVAL_H, 4s);
                break;
            case QUEST_COOKING_MEAT_A:
                SayLine(player, { NPC_ALARIA_CAMP }, LINE_COOKING_DONE_A);
                SayLine(player, { NPC_GARRICK_CAMP }, LINE_COOKING_LEADER_A, 4s);
                break;
            case QUEST_COOKING_MEAT_H:
                SayLine(player, { NPC_WONSA_CAMP }, LINE_COOKING_DONE_H);
                SayLine(player, { NPC_GRIMAXE_CAMP }, LINE_COOKING_LEADER_H, 4s);
                break;
            default:
                break;
        }
    }

private:
    // a personal summon beside the player: Finding the Lost Expedition's leader (npc_er_expedition_leader),
    // Enhanced Combat Tactics' trainer (npc_er_combat_trainer)
    static void SummonFollower(Player* player, uint32 entry)
    {
        Position pos = player->GetNearPosition(2.0f, static_cast<float>(M_PI) / 2);
        player->SummonCreature(entry, pos, TEMPSUMMON_MANUAL_DESPAWN, 0, 0, nullptr, true);
    }
};

// ---- the ships --------------------------------------------------------------------------------------------------

enum ShipSparringData
{
    EVENT_SPAR_ATTACK = 1,
    FACTION_SPAR      = 14,
};

// 160664 - Private Cole, 166583 - Grunt Throg: Stand Your Ground. They spar with whoever takes the quest from them,
// never taking them below 95% health, and yield at 30%, crediting the spar (TrinityCore summons a sparring copy)
struct npc_er_ship_sparring : public ScriptedAI
{
    npc_er_ship_sparring(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        _jumped = false;
        me->RestoreFaction();
    }

    bool Alliance() const { return me->GetEntry() == NPC_COLE_SHIP; }

    void sQuestAccept(Player* player, Quest const* quest) override
    {
        if (quest->GetQuestId() != QUEST_STAND_YOUR_GROUND_A && quest->GetQuestId() != QUEST_STAND_YOUR_GROUND_H)
            return;
        if (me->IsInCombat())
            return;
        _opponent = player->GetGUID();
        me->Say(Alliance() ? LINE_SPAR_PREFIGHT_A : LINE_SPAR_PREFIGHT_H, player);
        events.ScheduleEvent(EVENT_SPAR_ATTACK, 3s);
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Say(Alliance() ? LINE_SPAR_AGGRO_A : LINE_SPAR_AGGRO_H);
    }

    void DamageDealt(Unit* victim, uint32& damage, DamageEffectType /*type*/, SpellSchoolMask /*mask*/) override
    {
        if (victim->GetHealthPct() < 95.0f)
            damage = 0;
    }

    void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*type*/, SpellSchoolMask /*mask*/) override
    {
        if (!_jumped && me->HealthBelowPctDamaged(65, damage))
        {
            _jumped = true;
            me->Say(Alliance() ? LINE_SPAR_FACE_A : LINE_SPAR_FACE_H);
        }
        if (!me->HealthBelowPctDamaged(30, damage))
            return;
        damage = 0;
        Player* player = attacker ? attacker->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (player)
        {
            player->KilledMonsterCredit(CREDIT_SPAR);
            me->Say(Alliance() ? LINE_SPAR_YIELD_A : LINE_SPAR_YIELD_H, player);
        }
        me->SetFullHealth();
        EnterEvadeMode(EVADE_REASON_OTHER);
    }

    void UpdateAI(uint32 diff) override
    {
        events.Update(diff);
        if (events.ExecuteEvent() == EVENT_SPAR_ATTACK)
        {
            if (Player* player = ObjectAccessor::GetPlayer(*me, _opponent))
            {
                me->SetFaction(FACTION_SPAR);
                AttackStart(player);
            }
        }
        if (!UpdateVictim())
            return;
        DoMeleeAttackIfReady();
    }

private:
    ObjectGuid _opponent;
    bool _jumped = false;
};

// ---- the beach --------------------------------------------------------------------------------------------------

enum BeachSurvivorData
{
    NPC_BJORN_LAYING    = 156609,
    NPC_AUSTIN_LAYING   = 156610,
    NPC_KEE_LA_LAYING   = 156612,
    NPC_BO_LAYING       = 166786,
    NPC_MITHDRAN_LAYING = 166791,
    NPC_LANA_LAYING     = 166796,

    EVENT_SURVIVOR_SALUTE = 1,
    EVENT_SURVIVOR_LIE_DOWN,
};

static uint32 SurvivorLine(uint32 entry)
{
    switch (entry)
    {
        case NPC_BJORN_LAYING:    return 178943;    // "Nasty storm! It must'a hit the first boat, too."
        case NPC_AUSTIN_LAYING:   return 178945;    // "Never seen a storm like that..."
        case NPC_KEE_LA_LAYING:   return 178944;    // "Don't worry, we'll find the expedition."
        case NPC_BO_LAYING:       return 195930;    // "My thanks. We'll find the expedition yet."
        case NPC_MITHDRAN_LAYING: return 195932;    // "Mark my words, that storm was unnatural."
        case NPC_LANA_LAYING:     return 195924;    // "Quite the storm. Probably hit the first boat, too."
        default:                  return 0;
    }
}

// Emergency First Aid's survivors: retail bandages them with the quest's spell; here talking to one bandages them.
// They get up, thank the player and salute, then lie down again for the next player (retail swaps in a personal
// standing copy that walks off)
struct npc_er_beach_survivor : public ScriptedAI
{
    npc_er_beach_survivor(Creature* creature) : ScriptedAI(creature) { }

    void InitializeAI() override
    {
        _standState = me->getStandState();
        ScriptedAI::InitializeAI();
    }

    void sGossipHello(Player* player) override
    {
        CloseGossipMenuFor(player);
        if (_up)
            return;
        _up = true;
        player->KilledMonsterCredit(me->GetEntry());
        me->SetStandState(UNIT_STAND_STATE_STAND);
        me->SetFacingToObject(player);
        if (uint32 line = SurvivorLine(me->GetEntry()))
            me->Say(line, player);
        events.ScheduleEvent(EVENT_SURVIVOR_SALUTE, 3s);
    }

    void UpdateAI(uint32 diff) override
    {
        events.Update(diff);
        switch (events.ExecuteEvent())
        {
            case EVENT_SURVIVOR_SALUTE:
                me->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
                events.ScheduleEvent(EVENT_SURVIVOR_LIE_DOWN, 20s);
                break;
            case EVENT_SURVIVOR_LIE_DOWN:
                _up = false;
                me->SetStandState(_standState);
                me->SetFacingTo(me->GetHomePosition().GetOrientation());
                break;
            default:
                break;
        }
    }

private:
    uint8 _standState = UNIT_STAND_STATE_STAND;
    bool _up = false;
};

enum ExpeditionLeaderData
{
    EVENT_LEADER_START  = 1,
    EVENT_LEADER_CHECK,
    POINT_CAMP          = 1,
};

// 165359 - Captain Garrick, 166805 - Warlord Breka Grimaxe: Finding the Lost Expedition. Summoned on the quest's
// accept (visible to the player only), they send the survivors off, follow the player to the abandoned camp and, once
// there, find the expedition's last survivor (Alaria / Won'sa) and walk to her
struct npc_er_expedition_leader : public ScriptedAI
{
    npc_er_expedition_leader(Creature* creature) : ScriptedAI(creature) { }

    bool Alliance() const { return me->GetEntry() == NPC_GARRICK_FOLLOWER; }

    Player* Owner() const
    {
        TempSummon* summon = me->ToTempSummon();
        Unit* owner = summon ? summon->GetSummonerUnit() : nullptr;
        return owner ? owner->ToPlayer() : nullptr;
    }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetReactState(REACT_PASSIVE);
        events.ScheduleEvent(EVENT_LEADER_START, 1s);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type == POINT_MOTION_TYPE && id == POINT_CAMP)
            me->DespawnOrUnsummon(10s);
    }

    void UpdateAI(uint32 diff) override
    {
        events.Update(diff);
        switch (events.ExecuteEvent())
        {
            case EVENT_LEADER_START:
                if (Player* owner = Owner())
                {
                    me->Say(Alliance() ? LINE_EXPEDITION_A : LINE_EXPEDITION_H, owner);
                    me->GetMotionMaster()->MoveFollow(owner, 2.0f, static_cast<float>(M_PI) / 4);
                }
                events.ScheduleEvent(EVENT_LEADER_CHECK, 2s);
                break;
            case EVENT_LEADER_CHECK:
            {
                Player* owner = Owner();
                uint32 quest = Alliance() ? QUEST_FINDING_THE_LOST_EXPEDITION_A : QUEST_FINDING_THE_LOST_EXPEDITION_H;
                // the player left, dropped the quest or went elsewhere: no follower
                if (!owner || owner->GetQuestStatus(quest) == QUEST_STATUS_NONE || !me->IsWithinDist(owner, 80.0f))
                {
                    me->DespawnOrUnsummon();
                    break;
                }
                if (owner->GetDistance(AbandonedCamp) > 25.0f)
                {
                    events.ScheduleEvent(EVENT_LEADER_CHECK, 1s);
                    break;
                }
                SayLine(owner, { Alliance() ? uint32(NPC_ALARIA_CAMP) : uint32(NPC_WONSA_CAMP) },
                    Alliance() ? LINE_CAMP_HUNGRY_A : LINE_CAMP_HUNGRY_H);
                me->Say(Alliance() ? LINE_CAMP_LEADER_A : LINE_CAMP_LEADER_H, owner);
                me->GetMotionMaster()->Clear();
                me->GetMotionMaster()->MovePoint(POINT_CAMP, AbandonedCamp);
                break;
            }
            default:
                break;
        }
    }
};

// ---- the abandoned camp -----------------------------------------------------------------------------------------

enum CombatTrainerData
{
    EVENT_TRAINER_CHECK     = 1,
    EVENT_TRAINER_LEAVE,
    POINT_TRAINER_WALK      = 1,
    POINT_TRAINER_SPOT,
    POINT_TRAINER_HOME,
    TRAINER_CREDITS         = 3,
    FACTION_TRAINER_SPAR    = 14,
};

static Position const TrainerWalkPosition = { -250.60243f, -2485.2517f, 17.787413f, 0.0f };
static Position const TrainerSpotPosition = { -231.5225f, -2480.5276f, 19.019197f, 0.0f };

// 164577 - Alliance Sparring Partner, 166916 - Horde Sparring Partner: Enhanced Combat Tactics. The camp's leader
// spars with the player (a personal summon), never taking them below 91% health and never going down. Retail checks
// each class's level 1 rotation (Charge, Crusader Strike, Primal Strike, Fire Blast...), which 3.3.5a's level 1 classes
// mostly lack: here any of the player's own class abilities landing on the trainer is a credit, one per 2.5 seconds,
// TRAINER_CREDITS of them. The warrior keeps retail's Charge lines
struct npc_er_combat_trainer : public ScriptedAI
{
    npc_er_combat_trainer(Creature* creature) : ScriptedAI(creature) { }

    bool Alliance() const { return me->GetEntry() == NPC_TRAINER_A; }
    uint32 Quest() const { return Alliance() ? QUEST_ENHANCED_COMBAT_TACTICS_A : QUEST_ENHANCED_COMBAT_TACTICS_H; }

    Player* Owner() const
    {
        TempSummon* summon = me->ToTempSummon();
        Unit* owner = summon ? summon->GetSummonerUnit() : nullptr;
        return owner ? owner->ToPlayer() : nullptr;
    }

    void IsSummonedBy(WorldObject* /*summoner*/) override
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetImmuneToPC(true);
        if (Player* owner = Owner())
            me->Say(Alliance() ? LINE_TRAINING_WALK_A : LINE_TRAINING_WALK_H, owner);
        me->SetWalk(true);
        me->GetMotionMaster()->MovePoint(POINT_TRAINER_WALK, TrainerWalkPosition);
        events.ScheduleEvent(EVENT_TRAINER_CHECK, 1s);
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != POINT_MOTION_TYPE)
            return;
        switch (id)
        {
            case POINT_TRAINER_WALK:
                me->SetWalk(false);
                me->GetMotionMaster()->MovePoint(POINT_TRAINER_SPOT, TrainerSpotPosition);
                break;
            case POINT_TRAINER_SPOT:
                if (Player* owner = Owner())
                {
                    me->SetFacingToObject(owner);
                    me->SetFaction(FACTION_TRAINER_SPAR);
                    me->SetImmuneToPC(false);
                    me->SetReactState(REACT_DEFENSIVE);
                    uint8 cls = owner->getClass();
                    uint32 line = cls == CLASS_WARRIOR ? (Alliance() ? LINE_TRAINING_CHARGE_A : LINE_TRAINING_CHARGE_H)
                        : owner->getPowerType() == POWER_MANA && cls != CLASS_PALADIN && cls != CLASS_HUNTER
                        ? (Alliance() ? LINE_TRAINING_RANGED_A : LINE_TRAINING_RANGED_H)
                        : (Alliance() ? LINE_TRAINING_READY_A : LINE_TRAINING_READY_H);
                    me->Say(line, owner);
                }
                break;
            case POINT_TRAINER_HOME:
                me->DespawnOrUnsummon(2s);
                break;
            default:
                break;
        }
    }

    void SpellHit(Unit* caster, SpellInfo const* spellInfo) override
    {
        Player* owner = Owner();
        if (!owner || caster != owner || _done || GameTime::GetGameTimeMS() < _nextCredit || spellInfo->IsPassive())
            return;
        ChrClassesEntry const* cls = sChrClassesStore.LookupEntry(owner->getClass());
        if (!cls || spellInfo->SpellFamilyName != cls->spellfamily)
            return;
        _nextCredit = GameTime::GetGameTimeMS() + 2500ms;
        owner->KilledMonsterCredit(NPC_TRAINER_A);
        uint32 credits = owner->GetReqKillOrCastCurrentCount(Quest(), NPC_TRAINER_A);
        if (credits >= TRAINER_CREDITS || owner->GetQuestStatus(Quest()) == QUEST_STATUS_COMPLETE)
        {
            _done = true;
            me->Say(Alliance() ? LINE_TRAINING_DONE_A : LINE_TRAINING_DONE_H, owner);
            events.ScheduleEvent(EVENT_TRAINER_LEAVE, 2s);
        }
        else if (owner->getClass() == CLASS_WARRIOR)
            me->Say(credits == 1 ? (Alliance() ? LINE_TRAINING_CHARGE1_A : LINE_TRAINING_CHARGE1_H)
                : (Alliance() ? LINE_TRAINING_CHARGE2_A : LINE_TRAINING_CHARGE2_H), owner);
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*type*/, SpellSchoolMask /*mask*/) override
    {
        if (damage >= me->GetHealth())
            damage = me->GetHealth() - 1;
    }

    void DamageDealt(Unit* victim, uint32& damage, DamageEffectType /*type*/, SpellSchoolMask /*mask*/) override
    {
        if (victim->GetHealthPct() < 91.0f)
            damage = 0;
    }

    void EnterEvadeMode(EvadeReason why) override
    {
        if (_done)
            return;
        ScriptedAI::EnterEvadeMode(why);
    }

    void UpdateAI(uint32 diff) override
    {
        events.Update(diff);
        switch (events.ExecuteEvent())
        {
            case EVENT_TRAINER_CHECK:
            {
                Player* owner = Owner();
                if (!owner || owner->GetQuestStatus(Quest()) == QUEST_STATUS_NONE || !me->IsWithinDist(owner, 80.0f))
                {
                    me->DespawnOrUnsummon();
                    return;
                }
                events.ScheduleEvent(EVENT_TRAINER_CHECK, 1s);
                break;
            }
            case EVENT_TRAINER_LEAVE:
                events.CancelEvent(EVENT_TRAINER_CHECK);
                me->CombatStop(true);
                me->RestoreFaction();
                me->SetImmuneToPC(true);
                me->SetReactState(REACT_PASSIVE);
                me->SetFullHealth();
                me->GetMotionMaster()->Clear();
                me->GetMotionMaster()->MovePoint(POINT_TRAINER_HOME, AbandonedCamp);
                return;
            default:
                break;
        }
        if (!UpdateVictim())
            return;
        DoMeleeAttackIfReady();
    }

private:
    Milliseconds _nextCredit = 0ms;
    bool _done = false;
};

void AddSC_exiles_reach()
{
    new ExilesReachQuestScript();
    RegisterCreatureAI(npc_er_ship_sparring);
    RegisterCreatureAI(npc_er_beach_survivor);
    RegisterCreatureAI(npc_er_expedition_leader);
    RegisterCreatureAI(npc_er_combat_trainer);
}
