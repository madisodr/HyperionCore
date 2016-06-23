/* HyperionCore
 * Code Name: Pegasus
 * Desc: Gameobject placement and building system
 */

#include "Hyperion.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "TargetedMovementGenerator.h"

struct PegasusMount {
	uint32 entry;
	uint32 spawnId;
};

static std::vector<PegasusMount*> mounts;

class PegasusLoader : public WorldScript {
public:
	PegasusLoader() : WorldScript("pegasus_loader") {}
	bool LoadMounts() {
		Field* f;
		QueryResult r = WorldDatabase.PQuery( "SELECT entry, spawn_entry FROM pegasus" );
		if(!r) return false;

		do {
			f = r->Fetch();
			PegasusMount* i = new PegasusMount;
			i->entry = f[0].GetUInt32();
			i->spawnId = f[1].GetUInt32();

			mounts.push_back( i );
		} while(r->NextRow());
	}
};

extern class Player;
class PegasusHandler : public PlayerScript {
public:
	PegasusHandler() : PlayerScript( "pegasus_handler" ) {}

	void UnsummonMountFromWorld( Player* p ) {
		p->PlayerTalkClass->SendCloseGossip();

		if(p->GetPegasusMount() != NULL) {
			p->GetPegasusMount()->ToTempSummon()->UnSummon();
			p->SetPegasusMount( NULL );
		}
	}

	void OnMount( Player* p ) {
		UnsummonMountFromWorld( p );
	}

	void OnDismount( Player* p, uint32 entry ) {
		TempSummon* m;
		for(PegasusMount* e : mounts) {
			if(e->entry == entry) {
				m = p->SummonCreature( e->spawnId, p->GetPositionX() + 5, p->GetPositionY() + 5, p->GetPositionZ() + 1 );
				p->SetPegasusMount( m );
				m->GetMotionMaster()->Movefollow( p, PET_FOLLOW_DIST, m->GetFollowAngle() );
			}
		}
	}
	void OnLogout( Player* p ) {
		UnsummonMountFromWorld( p );
	}
};

class PegasusGossip : public CreatureScript {
public:
	PegasusGossip() : CreatureScript("pegasus_gossip") { }
};


void AddSC_Pegasus() {
	new PegasusLoader();
	new PegasusHandler();
	new PegasusGossip();
}