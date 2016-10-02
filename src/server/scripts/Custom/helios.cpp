/* HyperionCore */
#include "Chat.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellScript.h"

#define HELIOS_SPELL 29173
#define START_ZONE 725

struct HeliosObjectTemplate {
    uint32 item;
    uint32 object;
};

const char* HELIOS_LOAD = "SELECT item object FROM helios";
const char* HELIOS_LOOKUP = "SELECT object FROM helios_spawned WHERE item = '%u'";
static std::vector<HeliosObjectTemplate*> HeliosObjectTemplateList;

class HeliosHandler : public WorldScript {
    public:
        HeliosHandler() : WorldScript( "HeliosHandler" ) {}
        void OnStartup() {
            Field* f;
            QueryResult r = WorldDatabase.PQuery(HELIOS_LOAD);
            if(!r)
                return false;

            do {
                f = r->Fetch();
                HeliosObjectTemplate* i = new HeliosObjectTemplate;
                i->item = f[0].GetUInt32();
                i->object = f[1].GetUInt32();
                HeliosObjectTemplateList.push_back( i );
            } while(r->NextRow());
        }

        // Cleanup the helios objects
        void OnShutdown() {
            for (HeliosObjectTemplate* i : HeliosObjectTemplateList) {
                delete i;
            }
        }

        bool loadHelios() {
            Field* f;
            QueryResult r = WorldDatabase.PQuery(HELIOS_LOAD);
            if(!r)
                return false;

            do {
                f = r->Fetch();
                HeliosObjectTemplate* i = new HeliosObjectTemplate;
                i->item = f[0].GetUInt32();
                i->object = f[1].GetUInt32();
                HeliosObjectTemplateList.push_back( i );
            } while(r->NextRow());

            return true;
        }
};

class HeliosItem : public ItemScript {
    public:
        static uint32 itemEntry;
        static ObjectGuid itemGUID;
        static const WorldLocation* pos;

        HeliosItem() : ItemScript( "HeliosItem" ) {}

        bool OnUse( Player* p, Item* i, SpellCastTargets const& targets ) override {
            if(isGoodMap( p->GetMapId() )) {
                itemEntry = i->GetEntry();
                itemGUID = i->GetGUID();
                pos = targets.GetDstPos();
                p->CastSpell( p, HELIOS_SPELL, true );
                return true;
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
        HeliosSpell() : SpellScriptLoader( "HeliosSpell" ) {}
        class HeliosSpell_SpellScript : public SpellScript {
            PrepareSpellScript( HeliosSpell_SpellScript );

            void HandleAfterCast() {
                if(!GetCaster() || GetCaster()->GetTypeId() != TYPEID_PLAYER)
                    return;

                Player* p = GetCaster()->ToPlayer();
                const WorldLocation* pos = HeliosItem::pos;
                bool found = false;
                HeliosObjectTemplate* data;
                for(HeliosObjectTemplate* i : HeliosObjectTemplateList) {
                    if(i->item == HeliosItem::itemEntry) {
                        data = i;
                        found = true;
                        break;
                    }
                }

                // Didn't find the object in the list. Broken APT.
                if(found) {
                    uint32 objEntry = data->object;

                    QueryResult result = WorldDatabase.PQuery(HELIOS_LOOKUP, HeliosItem::itemGUID );
                    if(result) {
                        Field* f = result->Fetch();
                        uint32 objGUID = f[0].GetUInt32();
                        if(GameObjectData const* goData = sObjectMgr->GetGOData( objGUID )) {
                            GameObject* object = ChatHandler( p->GetSession() ).GetObjectGlobalyWithGuidOrNearWithDbGuid( objGUID, goData->id );
                            RemoveHeliosObject( p, object );
                        } else
                            PlaceHeliosObject( p, objEntry, pos );
                    }
                } else {
                    ChatHandler( p->GetSession() ).SendSysMessage( "The gameobject this is suppose to spawn appears to be missing. Please report this error if it continues." );
                    return;
                }
            }

            static void RemoveHeliosObject(Player* player, GameObject* object) {
                if (!object)
                    return;

                object->SetRespawnTime( 0 );  // not save respawn time
                object->Delete();
                object->DeleteFromDB();
            }

            static void PlaceHeliosObject( Player* p, uint32 objEntry, const WorldLocation* pos ) {
                float x = pos->GetPositionX();
                float y = pos->GetPositionX();
                float z = pos->GetPositionX();
                float o = p->GetOrientation();
                Map* map = p->GetMap();

                GameObject* object = new GameObject;
                if (!object->Create( objEntry, map, 0, x, y, z, o, 0.0f, 0.0f, 0.0f, 0.0f, 0, GO_STATE_READY )) {
                    delete object;
                    return;
                }

                object->CopyPhaseFrom( p );
                object->SaveToDB( map->GetId(), (1 << map->GetSpawnMode()), p->GetPhaseMask() );
                ObjectGuid::LowType spawnId = object->GetSpawnId();
                delete object;

                object = new GameObject();
                if (!object->LoadGameObjectFromDB( spawnId, map )) {
                    delete object;
                    return;
                }

                sObjectMgr->AddGameobjectToGrid( spawnId, ASSERT_NOTNULL( sObjectMgr->GetGOData( spawnId ) ) );
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
