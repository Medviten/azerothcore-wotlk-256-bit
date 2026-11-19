/*
 * Custom: F-137 Exile's Reach start.
 *
 * F-137 moves the start (playercreateinfo) of the races whose player_race_template row has exiles_reach = 1 (most
 * custom races; stock races and those that fit a stock or their own start keep it) to the island's beach. With
 * ExilesReachStartZone.Enable on, those new characters (not Death Knights, not those the Wandering Isle start takes)
 * start instead aboard their faction's ship sailing to the island, as retail's new player experience does: the
 * Alliance Ship (transport 13701) or the Horde Transport (13702), on retail's deck spot. The ships are F-137's
 * continent transports on map 2175; the character row is saved before this hook runs, so it gets the transport and
 * the deck offset written directly, and the first login puts the character aboard the way it restores anyone who
 * logged out on a transport (Player::LoadFromDB). With the ship missing (EnableContinentTransport off) the character
 * starts on the beach.
 *
 * The way off the island: Kalecgos, at the end of Darkmaul Citadel, credits "Speak with Kalecgos to leave" and sends
 * the player to their capital, bound there (retail's An End to Beginnings, whose givers its scripts summon, isn't
 * ported), as the Wandering Isle's finale does.
 */

#include "AreaDefines.h"
#include "CreatureScript.h"
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerScript.h"
#include "ScriptedGossip.h"
#include "World.h"

enum ExilesReachStart
{
    MAP_EXILES_REACH            = 2175,
    TRANSPORT_ALLIANCE_SHIP     = 13701,
    TRANSPORT_HORDE_SHIP        = 13702,
    QUEST_SANDY_FIST            = 29406,    // F-134: only the races starting on the Wandering Isle may take it
    CREDIT_LEAVE_ALLIANCE       = 161350,   // "Speak with Kalecgos to leave Darkmaul Citadel"
    CREDIT_LEAVE_HORDE          = 167663,
};

// retail's playercreateinfo deck spots (TrinityCore: transport guids 29 and 30), relative to the ship
static Position const AllianceDeck = { 11.1301f, -0.417182f, 5.18741f, 3.14843f };
static Position const HordeDeck = { -10.7291f, -7.14635f, 8.73113f, 1.56321f };

class ExilesReachStartPlayerScript : public PlayerScript
{
public:
    ExilesReachStartPlayerScript() : PlayerScript("ExilesReachStartPlayerScript", { PLAYERHOOK_ON_CREATE }) { }

    void OnPlayerCreate(Player* player) override
    {
        if (!sWorld->getBoolConfig(CONFIG_EXILES_REACH_START_ZONE) || player->getClass() == CLASS_DEATH_KNIGHT ||
            player->GetMapId() != MAP_EXILES_REACH)
            return;
        // the Wandering Isle start moves them on their first login
        Quest const* wanderingIsle = sObjectMgr->GetQuestTemplate(QUEST_SANDY_FIST);
        if (sWorld->getBoolConfig(CONFIG_WANDERING_ISLE_START_ZONE) && wanderingIsle &&
            player->SatisfyQuestRace(wanderingIsle, false))
            return;

        bool alliance = player->GetTeamId(true) == TEAM_ALLIANCE;
        Position const& deck = alliance ? AllianceDeck : HordeDeck;
        CharacterDatabase.Execute("UPDATE characters SET transguid = {}, trans_x = {}, trans_y = {}, trans_z = {}, "
            "trans_o = {} WHERE guid = {}", uint32(alliance ? TRANSPORT_ALLIANCE_SHIP : TRANSPORT_HORDE_SHIP),
            deck.GetPositionX(), deck.GetPositionY(), deck.GetPositionZ(), deck.GetOrientation(),
            player->GetGUID().GetCounter());
    }
};

// 156501 - Kalecgos, freed at the end of Darkmaul Citadel
class npc_darkmaul_kalecgos : public CreatureScript
{
public:
    npc_darkmaul_kalecgos() : CreatureScript("npc_darkmaul_kalecgos") { }

    bool OnGossipHello(Player* player, Creature* /*creature*/) override
    {
        CloseGossipMenuFor(player);
        player->KilledMonsterCredit(CREDIT_LEAVE_ALLIANCE);
        player->KilledMonsterCredit(CREDIT_LEAVE_HORDE);

        bool alliance = player->GetTeamId() == TEAM_ALLIANCE;
        WorldLocation home = alliance
            ? WorldLocation(MAP_EASTERN_KINGDOMS, -8833.38f, 628.63f, 94.01f, 1.07f)     // Stormwind, Trade District
            : WorldLocation(MAP_KALIMDOR, 1633.75f, -4439.39f, 15.43f, 0.0f);          // Orgrimmar, Valley of Strength
        player->SetHomebind(home, alliance ? AREA_STORMWIND_CITY : AREA_ORGRIMMAR);
        player->TeleportTo(home);
        return true;
    }
};

void AddSC_exiles_reach_start()
{
    new ExilesReachStartPlayerScript();
    new npc_darkmaul_kalecgos();
}
