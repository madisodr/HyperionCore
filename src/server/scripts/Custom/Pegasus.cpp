/* HyperionCore */

#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ObjectAccessor.h"
#include "TargetedMovementGenerator.h"

const char* PEGASUS_SELECT_ENTRY = "SELECT entry, spawn_entry FROM pegasus";

struct PegasusMount {
    uint32 entry;
    uint32 spawnId;
};

enum GossipOptions {
    DISMISS = 5,
    STAY = 10,
    FOLLOW = 15,
    BANK = 20,
    EXIT = 25
};

static std::vector<PegasusMount*> mounts;

class PegasusLoader : public WorldScript {

    public:
        PegasusLoader() : WorldScript("pegasus_loader") {}

        bool LoadMounts() {
            Field* f;
            QueryResult r = WorldDatabase.PQuery(PEGASUS_SELECT_ENTRY);
            if (!r)
                return false;

            while (r->NextRow()) {
                f = r->Fetch();
                PegasusMount* i = new PegasusMount;
                i->entry = f[0].GetUInt32();
                i->spawnId = f[1].GetUInt32();

                mounts.push_back( i );
            }
        }
};


class
PegasusHandler : public PlayerScript {

    public:
        PegasusHandler() : PlayerScript( "pegasus_handler" ) {}

        void UnsummonMountFromWorld(Player* p) {
            p->PlayerTalkClass->SendCloseGossip();

            if (p->GetPegasusMount() != NULL) {
                p->GetPegasusMount()->ToTempSummon()->UnSummon();
                p->SetPegasusMount(NULL);
            }
        }

        void OnMount(Player* p) {
            UnsummonMountFromWorld(p);
        }

        void OnDismount(Player* p, uint32 entry) {
            TempSummon* m;
            for (PegasusMount* e : mounts) {
                if (e->entry == entry) {
                    m = p->SummonCreature(e->spawnId, p->GetPositionX() + 5, p->GetPositionY() + 5, p->GetPositionZ() + 1);
                    p->SetPegasusMount(m);
                    m->GetMotionMaster()->MoveFollow(p, PET_FOLLOW_DIST, m->GetFollowAngle());
                }
            }
        }

        void OnLogout(Player* p) {
            UnsummonMountFromWorld(p);
        }

};

class PegasusGossip : public CreatureScript {
    public:
        PegasusGossip() : CreatureScript("pegasus_gossip") { }

        bool OnGossipHello(Player* p, Creature* c) {
            if (p == c->ToTempSummon()->GetSummoner()) {
                p->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Dismiss", GOSSIP_SENDER_MAIN, DISMISS);
                p->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Stay", GOSSIP_SENDER_MAIN, STAY);
                p->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Follow", GOSSIP_SENDER_MAIN, FOLLOW);
                p->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Check Storage", GOSSIP_SENDER_MAIN, BANK);
                p->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Exit", GOSSIP_SENDER_MAIN, EXIT);
                p->PlayerTalkClass->SendGossipMenu(1, c->GetGUID());
            }

            return true;
        }

        bool OnGossipSelect(Player* p, Creature* c, uint32 sender, uint32 actions) {
            p->PlayerTalkClass->ClearMenus();
            switch(actions) {
                case DISMISS:
                    if (p->GetPegasusMount() != NULL) {
                        c->ToTempSummon()->UnSummon();
                        p->SetPegasusMount( NULL );
                    }
                    break;
                case STAY: // Tell the mount to stay in one place.
                    if (p->GetPegasusMount() != NULL) {
                        c->SetPosition(c->GetPositionX(), c->GetPositionY(), c->GetPositionZ(), c->GetOrientation());
                        c->GetMotionMaster()->MovementExpired( true );
                    }
                    break;
                case FOLLOW:
                    if (p->GetPegasusMount() != NULL)
                        c->GetMotionMaster()->MoveFollow( p, PET_FOLLOW_DIST, c->GetFollowAngle() );
                    break;
                case BANK:
                    p->GetSession()->SendShowBank( p->GetGUID() );
                    break;
                case EXIT:
                    break;
            }

            p->PlayerTalkClass->SendCloseGossip();
            return true;
        }
};

void AddSC_Pegasus() {
    new PegasusLoader();
    new PegasusHandler();
    new PegasusGossip();
}
