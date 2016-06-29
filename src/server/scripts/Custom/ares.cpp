/* HyperionCore */
#include "Chat.h"
#include "Player.h"
#include "ScriptMgr.h"

using namespace std;

static const vector<string> modifierList = {"str", "agi", "sta", "int", "spt", "cha", "hp", "fort", "will", "rfx"};

const char* NO_MODIFIERS_MSG = "It seems your modifiers were never generated. Visit our web portal and go to your character sheet in order to resolve this issue.";
const char* ARES_SELECT_QUERY = "SELECT %s FROM ares_modifiers WHERE guid='%u'";

struct AresPlayerStats {
	int STR, AGI, STA, INT, SPT, CHA, HP, WIL, REF;
};

class AresLoader {
	public:
	AresLoader();

};
class Ares_CommandScript : public CommandScript {
	public:
	Ares_CommandScript() : CommandScript( "Ares_CommandScript" ) {}
	std::vector<ChatCommand> GetCommands() const override {
		static std::vector<ChatCommand> commandTable = {
			{"roll",   rbac::RBAC_PERM_COMMAND_BARBERSHOP,   false,  &HandleRollCommand,   ""},
		};

		return commandTable;
	}

	static bool HandleRollCommand( ChatHandler* handler, const char* args ) {
		int stat = -1;
		int mod = 0;
		uint32 dice;

		if(!*args)
			return false;

		char* c_rollStat = strtok( (char*) args, " " );
		char* c_dice = strtok( NULL, " " );
		char* c_mod = strtok( NULL, " " );

		if(!c_rollStat || !c_dice)
			return false;

		dice = (int) atoi( c_dice );
		// if unspecificed, additional modifiers = 0
		!c_mod ? mod = 0 : mod = (int)atoi( c_mod );

		for(uint32 i = 0; i < modifierList.size(); i++) {
			if(c_rollStat == modifierList[i]) {
				stat = i;
				break;
			}
		}

		// the roll type isn't a valid modifier.
		if(stat == -1)
			return false;

		Player* player = handler->GetSession()->GetPlayer();
		QueryResult result = CharacterDatabase.PQuery( ARES_SELECT_QUERY, c_rollStat, player->GetGUID() );
		if(!result) {
			ChatHandler( player->GetSession() ).SendSysMessage( NO_MODIFIERS_MSG );
			return false;
		}

		uint32 playerStat = result->Fetch()[0].GetUInt32();
		uint32 roll = urand( 0, dice ) + playerStat + mod;
		return true;
	}
};