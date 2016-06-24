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

class JanusCommandScript : public CommandScript {
public:
	JanusCommandScript() : CommandScript( "janus_commandscript" ) {}
	std::vector<ChatCommand> GetCommands() const {
		static std::vector<ChatCommand> JanusCommandTable = {
			{ "link",   rbac::RBAC_PERM_COMMAND_JANUS,   true,  &HandleLinkCommand,   "" },
			{ "delete", rbac::RBAC_PERM_COMMAND_JANUS,   true,  &HandleDeleteCommand, "" },
			{ "",     0,                                 false, NULL,                 "" }
		};
		static std::vector<ChatCommand> commandTable = {
			{"janus",   rbac::RBAC_PERM_COMMAND_JANUS, true,  NULL, "", JanusCommandTable},
		};

		return commandTable;
	}

	static bool HandleLinkCommand( ChatHandler* handler, const char* args ) {
		return true;
	}

	static bool HandleDeleteCommand( ChatHandler* handler, const char* args ) {
		return true;
	}
};

class Janus : public GameObjectScript {
public: 
	Janus() : GameObjectScript("janus"){}

	bool OnGossipHello( Player* player, GameObject* go ) {
		uint64 guid = go->GetSpawnId();
		QueryResult result = WorldDatabase.PQuery( "SELECT map, x, y, z, o FROM janus WHERE objectID='%u' LIMIT 1", guid );
		if(!result)
			return false;

		Field* fields = result->Fetch();

		uint32 map = fields[0].GetUInt32();
		float x = fields[1].GetFloat();
		float y = fields[2].GetFloat();
		float z = fields[3].GetFloat();
		float o = fields[4].GetFloat();

		player->TeleportTo( map, x, y, z, o );

		return true;
	}
};

void AddSC_Janus() {
	new Janus();
	new JanusCommandScript();
}