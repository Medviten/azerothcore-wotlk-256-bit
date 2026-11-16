/*
 * Custom: F-134 Wandering Isle (Pandaren) start zone.
 *
 * With WanderingIsleStartZone.Enable on, a new Pandaren character (a race "The Lesson of the Sandy Fist", which
 * only Pandaren may take, allows), except a Death Knight, is moved on its first login to the Wandering Isle (map 860),
 * by Master Shang Xi where Mists of Pandaria starts Pandaren, bound there, and given "Much to Learn" (MoP offers it
 * with a spell this port doesn't have). playercreateinfo stays as it is, so switching it off gives the races their
 * normal start back. The island's NPCs, objects and quests are the modpak's (F-134); its scripted events are not
 * ported yet.
 */

#include "ObjectMgr.h"
#include "Player.h"
#include "PlayerScript.h"
#include "World.h"

enum WanderingIsleStart
{
    QUEST_MUCH_TO_LEARN             = 29404,
    QUEST_LESSON_OF_THE_SANDY_FIST  = 29406,  // Pandaren races only
    MAP_WANDERING_ISLE              = 860,
    ZONE_WANDERING_ISLE             = 5736,
};

static Position const WanderingIsleStartPos = { 1466.09f, 3465.98f, 181.86f, 2.87962f };

class WanderingIsleStartPlayerScript : public PlayerScript
{
public:
    WanderingIsleStartPlayerScript() : PlayerScript("WanderingIsleStartPlayerScript", { PLAYERHOOK_ON_FIRST_LOGIN }) { }

    void OnPlayerFirstLogin(Player* player) override
    {
        if (!sWorld->getBoolConfig(CONFIG_WANDERING_ISLE_START_ZONE) || player->getClass() == CLASS_DEATH_KNIGHT)
            return;

        Quest const* pandarenOnly = sObjectMgr->GetQuestTemplate(QUEST_LESSON_OF_THE_SANDY_FIST);
        if (!pandarenOnly || !player->SatisfyQuestRace(pandarenOnly, false))
            return;

        WorldLocation start(MAP_WANDERING_ISLE, WanderingIsleStartPos);
        player->SetHomebind(start, ZONE_WANDERING_ISLE);
        player->TeleportTo(start);

        if (Quest const* first = sObjectMgr->GetQuestTemplate(QUEST_MUCH_TO_LEARN))
            if (player->CanAddQuest(first, false))
                player->AddQuestAndCheckCompletion(first, nullptr);
    }
};

void AddSC_wandering_isle_start()
{
    new WanderingIsleStartPlayerScript();
}
