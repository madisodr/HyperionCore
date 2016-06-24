/* HyperionCore */

#define HELIOS_SPELL 29173
#define START_ZONE 725

#include "Chat.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellScript.h"

struct HeliosObject {
	uint32 item;
	uint32 object;
};

static std::vector<HeliosObject*> HeliosObjectList;

class HeliosHandler : public WorldScript {
public:
	HeliosHandler() : WorldScript( "HeliosHandler" ) {}
	void OnStartup() {
		if(loadHyperion()) {
			TC_LOG_INFO( "server.loading", "Loading Helios Item/Object Pairs" );
		} else {
			TC_LOG_INFO( "server.loading", "Failed to load Helios Item/Object Pairs" );
			exit( 1 );
		}
	}

	// Cleanup the helios objects
	void OnShutdown() {
		for(HeliosObject* i : HeliosObjectList) {
			delete i;
		}
	}

	bool loadHyperion() {
		Field* f;
		QueryResult r = WorldDatabase.PQuery( "SELECT item, object FROM helios" );
		if(!r) return false;

		do {
			f = r->Fetch();
			HeliosObject* i = new HeliosObject;
			i->item = f[0].GetUInt32();
			i->object = f[1].GetUInt32();
			HeliosObjectList.push_back( i );
		} while(r->NextRow());
	}
	
};

class HeliosItem : public ItemScript {
public:
	static uint32 itemEntry;
	static ObjectGuid itemGUID;
	static const WorldLocation* pos;

	HeliosItem() : ItemScript( "hyperion_helios_item" ) {}

	bool OnUse( Player* p, Item* i, SpellCastTargets const& targets ) override {
		if(isGoodMap( p->GetMapId() )) {
			itemEntry = i->GetEntry();
			itemGUID = i->GetGUID();
			pos = targets.GetDstPos();
			p->CastSpell( p, HELIOS_SPELL, true );

		} else {
			ChatHandler( p->GetSession() ).SendSysMessage( "You are not able to spawn gameobjects in this zone." );
			return false;
		}
	}

	bool isGoodMap( uint32 map ) {
		if(map == START_ZONE)
			return false;

		return true;
	}
};


uint32 HeliosItem::itemEntry;
ObjectGuid HeliosItem::itemGUID;
const WorldLocation* HeliosItem::pos;

class HeliosSpell : public SpellScriptLoader {
public:
	HeliosSpell() : SpellScriptLoader( "hyperion_helios_spell" ) {}
	class HeliosSpell_SpellScript : public SpellScript {
		PrepareSpellScript( HeliosSpell_SpellScript );

		void HandleAfterCast() {
			if(!GetCaster() || GetCaster()->GetTypeId() != TYPEID_PLAYER)
				return;

			Player* p = GetCaster()->ToPlayer();
			const WorldLocation* pos = HeliosItem::pos;
			for(HeliosObject* i : HeliosObjectList) {

			}
		}

		void Register() override {
			AfterCast += SpellCastFn( HeliosSpell_SpellScript::HandleAfterCast );
		}
	};
};


void AddSC_Helios() {
	new HeliosHandler();
    new HeliosItem();
    new HeliosSpell();

}