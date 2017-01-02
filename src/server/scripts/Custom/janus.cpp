/* HyperionCore
 * Code Name: Helios
 * Desc: Gameobject placement and building system
 */

#include "ScriptMgr.h"
#include "ObjectMgr.h"
#include "Chat.h"
#include "AccountMgr.h"
#include "Player.h"
#include "GameObject.h"
#include "Language.h"

#define JANUS_OBJECT 540120

const char* JANUS_LOAD = "SELECT map, x, y, z, o FROM janus WHERE guid='%u' LIMIT 1";
const char* JANUS_LOOKUP = "SELECT objectID FROM janus WHERE guid='%u'";
const char* JANUS_UPDATE = "UPDATE janus SET map='%u', x='%f', y='%f', o='%f', WHERE guid='%u'";
const char* JANUS_INSERT = "INSERT INTO janus (guid, map, x, y, z, o) VALUES (%u, %u, %f, %f, %f)";
const char* JANUS_DELETE = "DELETE FROM janus WHERE guid='%u'";


class JanusCommandScript : public CommandScript {
    public:
        JanusCommandScript() : CommandScript( "JanusCommandScript" ) {}
        std::vector<ChatCommand> GetCommands() const override
        {
            static std::vector<ChatCommand> JanusCommandTable =
            {
                { "link",   rbac::RBAC_PERM_COMMAND_JANUS_LINK,     false,  &HandleLinkCommand,   "" },
                { "delete", rbac::RBAC_PERM_COMMAND_JANUS_DELETE,   false,  &HandleDeleteCommand, "" },
            };

            static std::vector<ChatCommand> commandTable =
            {
                {"janus",   rbac::RBAC_PERM_COMMAND_JANUS, false,  NULL, "", JanusCommandTable},
            };

            return commandTable;
        }

        static bool HandleLinkCommand(ChatHandler* handler, const char* args) {

            char* id = handler->extractKeyFromLink((char*)args, "Hgameobject");
            if (!id)
                return false;

            ObjectGuid::LowType guidLow = strtoull(id, nullptr, 10);
            if (!guidLow)
                return false;

            GameObject* object = NULL;
            if(GameObjectData const* god = sObjectMgr->GetGOData(guidLow))
                object = handler->GetObjectGlobalyWithGuidOrNearWithDbGuid(guidLow, god->id);

            if (!object) {
                handler->PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, guidLow);
                handler->SetSentErrorMessage(true);
                return false;
            }

            if(object->GetEntry() != JANUS_OBJECT) {
                handler->PSendSysMessage("Not a Janus Gameobject. Object entry must be 540120", guidLow);
                handler->SetSentErrorMessage(true);
                return false;
            }

            Player* player = handler->GetSession()->GetPlayer();
            float x = player->GetPositionX();
            float y = player->GetPositionY();
            float z = player->GetPositionZ();
            float o = player->GetOrientation();
            uint32 map = player->GetMapId();

            QueryResult result = WorldDatabase.PQuery(JANUS_LOOKUP, guidLow);

            if(result)
                WorldDatabase.PExecute(JANUS_UPDATE, map, x, y, z, o, guidLow);
            else
                WorldDatabase.PExecute(JANUS_INSERT, guidLow, map, x, y, z, o);

            handler->SendSysMessage("A link has been added at your location to the selected object.");
            return true;
        }

        static bool HandleDeleteCommand(ChatHandler* handler, const char* args) {
            char* id = handler->extractKeyFromLink((char*)args, "Hgameobject");
            if (!id)
                return false;

            ObjectGuid::LowType guidLow = strtoull(id, nullptr, 10);
            if (!guidLow)
                return false;

            GameObject* object = NULL;

            // by DB guid
            if (GameObjectData const* gameObjectData = sObjectMgr->GetGOData(guidLow))
                object = handler->GetObjectGlobalyWithGuidOrNearWithDbGuid(guidLow, gameObjectData->id);

            if (!object)
            {
                handler->PSendSysMessage(LANG_COMMAND_OBJNOTFOUND, guidLow);
                handler->SetSentErrorMessage(true);
                return false;
            }

            if(object->GetEntry() != JANUS_OBJECT)
            {
                handler->PSendSysMessage("This is NOT aJanus Object. Please delete it as a regular gameobject.");
                handler->SetSentErrorMessage(true);
                return false;
            }

            ObjectGuid ownerGuid = object->GetOwnerGUID();
            if (!ownerGuid.IsEmpty())
            {
                Unit* owner = ObjectAccessor::GetUnit(*handler->GetSession()->GetPlayer(), ownerGuid);
                if (!owner || !ownerGuid.IsPlayer())
                {
                    handler->PSendSysMessage(LANG_COMMAND_DELOBJREFERCREATURE, ownerGuid.ToString().c_str(), object->GetGUID().ToString().c_str());
                    handler->SetSentErrorMessage(true);
                    return false;
                }

                owner->RemoveGameObject(object, false);
            }

            object->SetRespawnTime(0); // not save respawn time
            object->Delete();
            object->DeleteFromDB();

            WorldDatabase.PExecute(JANUS_DELETE, guidLow); // Delete the shit from janus' table.

            handler->PSendSysMessage(LANG_COMMAND_DELOBJMESSAGE, object->GetGUID().ToString().c_str());

            return true;
        }
};

class Janus : public GameObjectScript {
    public:
        Janus() : GameObjectScript("janus"){}

        bool OnGossipHello( Player* player, GameObject* go ) {
            uint64 guid = go->GetSpawnId();
            QueryResult result = WorldDatabase.PQuery(JANUS_LOAD, guid);
            if(!result)
                return false;

            Field* fields = result->Fetch();

            uint32 map = fields[0].GetUInt32();
            float x = fields[1].GetFloat();
            float y = fields[2].GetFloat();
            float z = fields[3].GetFloat();
            float o = fields[4].GetFloat();

            player->TeleportTo( map, x, y, z, o );
            player->PlayerTalkClass->SendCloseGossip();
            return true;
        }
};

void AddSC_Janus() {
    new Janus();
    new JanusCommandScript();
}
