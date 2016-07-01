/* HyperionCore */
#include "Chat.h"
#include "Player.h"
#include "ScriptMgr.h"

using namespace std;

static const vector<string> modifierList = {"str", "agi", "sta", "int", "spt", "cha"};

enum MODS {
	STR,
	AGI,
	STA,
	ITN,
	SPT,
	CHA,
	MAX,
};

const char* ARES_NO_MODIFIERS_MSG = "We were unable to find any modifiers for your character. Please report this error to a GM or on the forums.";
const char* ARES_USING_BASE_MODIFIERS_MSG = "Using default modifiers. Visit our web portal and go to your character sheet in order to populate your modifiers with proper stats.";
const char* ARES_SELECT_STAT_QUERY = "SELECT modified, %s FROM ares_modifiers WHERE guid='%u'";
const char* ARES_SELECT_ALL_QUERY = "SELECT * FROM ares_modifiers WHERE guid='%u'";
const char* ARES_SELECT_BASE_RACIAL = "SELECT * FROM ares_base_racial WHERE race = '%u'";
const char* ARES_SELECT_BASE_CLASS = "SELECT * FROM ares_base_class WHERE class = '%u'";
const char* ARES_NEW = "INSERT INTO ares_modifiers VALUES('%u', '%u', '%u, '%u', '%u', '%u', '%u')";

const char* ARES_STR_MSG = "Strength: %u";
const char* ARES_AGI_MSG = "Agility: %u";
const char* ARES_STA_MSG = "Stamina: %u";
const char* ARES_ITN_MSG = "Intellect: %u";
const char* ARES_SPT_MSG = "Wisdom: %u";
const char* ARES_CHA_MSG = "Charisma: %u";
typedef vector<int> AresStats;



class Ares_PlayerScript : public PlayerScript {
	public:
	Ares_PlayerScript() : PlayerScript( "Ares_PlayerScript" ) {}
	void OnCreate( Player* player ) {
		AresStats mods = {0, 0, 0, 0, 0, 0};
		LoadBaseStats( player->getRace(), player->getClass(), mods );
		WorldDatabase.PExecute( ARES_NEW, player->GetGUID().GetCounter(), mods[STR], mods[AGI], mods[STA], mods[ITN], mods[SPT], mods[CHA] );
	}

	void LoadBaseStats( uint32 Race, uint32 Class, AresStats& mods ) {
		QueryResult result = CharacterDatabase.PQuery( ARES_SELECT_BASE_RACIAL, Race );
		if(result) {
			Field* fields = result->Fetch();
			mods[STR] += fields[1].GetUInt32();
			mods[AGI] += fields[2].GetUInt32();
			mods[STA] += fields[3].GetUInt32();
			mods[ITN] += fields[4].GetUInt32();
			mods[SPT] += fields[5].GetUInt32();
			mods[CHA] += fields[6].GetUInt32();
		}
		
		result = CharacterDatabase.PQuery( ARES_SELECT_BASE_CLASS, Class );
		if(result) {
			Field* fields = result->Fetch();
			mods[STR] += fields[1].GetUInt32();
			mods[AGI] += fields[2].GetUInt32();
			mods[STA] += fields[3].GetUInt32();
			mods[ITN] += fields[4].GetUInt32();
			mods[SPT] += fields[5].GetUInt32();
			mods[CHA] += fields[6].GetUInt32();
		}
	}

};


class Ares_CommandScript : public CommandScript {
	public:
	Ares_CommandScript() : CommandScript( "Ares_CommandScript" ) {}
	std::vector<ChatCommand> GetCommands() const override {
		static std::vector<ChatCommand> commandTable = {
			{"roll",        rbac::RBAC_PERM_COMMAND_ARES_ROLL,   false,  &HandleRollCommand,   ""},
			{"viewstats",   rbac::RBAC_PERM_COMMAND_ARES_VIEW,   false,  &HandleViewCommand,   ""},
		};

		return commandTable;
	}
	static bool HandleViewCommand( ChatHandler* handler, const char* args ) {
		Player* player = handler->GetSession()->GetPlayer();
		QueryResult result = CharacterDatabase.PQuery( ARES_SELECT_ALL_QUERY, player->GetGUID() );
		if(result) {
			AresStats mods = {0, 0, 0, 0, 0, 0};
			Field* field = result->Fetch();
			uint32 guid = field[0].GetInt32();
			bool setup = field[1].GetBool();
			mods[STR] = field[2].GetInt32();
			mods[AGI] = field[3].GetInt32();
			mods[STA] = field[4].GetInt32();
			mods[ITN] = field[5].GetInt32();
			mods[SPT] = field[6].GetInt32();
			mods[CHA] = field[7].GetInt32();

			if(setup == false)
				ChatHandler( player->GetSession() ).SendSysMessage( ARES_USING_BASE_MODIFIERS_MSG );
			
			ChatHandler( player->GetSession() ).SendSysMessage( ARES_STR_MSG, mods[STR] );
			ChatHandler( player->GetSession() ).SendSysMessage( ARES_AGI_MSG, mods[STR] );
			ChatHandler( player->GetSession() ).SendSysMessage( ARES_STA_MSG, mods[STR] );
			ChatHandler( player->GetSession() ).SendSysMessage( ARES_ITN_MSG, mods[STR] );
			ChatHandler( player->GetSession() ).SendSysMessage( ARES_SPT_MSG, mods[STR] );
			ChatHandler( player->GetSession() ).SendSysMessage( ARES_CHA_MSG, mods[STR] );
		} else
			ChatHandler( player->GetSession() ).SendSysMessage( ARES_NO_MODIFIERS_MSG );
		
		return true;
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
		!c_mod ? mod = 0 : mod = (int) atoi( c_mod );

		for(uint32 i = 0; i < modifierList.size(); i++) {
			if(c_rollStat == modifierList[i]) {
				stat = i;
				break;
			}
		}

		// the roll type isn't a valid modifier.
		if(stat == -1)
			return false;

		uint32 playerStat = 0;
		bool hasPlayerSetStats = false;
		Player* player = handler->GetSession()->GetPlayer();

		QueryResult result = CharacterDatabase.PQuery( ARES_SELECT_STAT_QUERY, c_rollStat, player->GetGUID() );
		if(!result)
			ChatHandler( player->GetSession() ).SendSysMessage( ARES_NO_MODIFIERS_MSG );
		else {
			Field* f = result->Fetch();
			playerStat = f[0].GetUInt32();
			hasPlayerSetStats = f[0].GetBool();
		}

		if(!hasPlayerSetStats)
			ChatHandler( player->GetSession() ).SendSysMessage( ARES_USING_BASE_MODIFIERS_MSG );

		uint32 roll = urand( 1, dice ) + playerStat + mod;
		return true;
	}
};

void AddSC_Ares() {
	new Ares_PlayerScript();
	new Ares_CommandScript();
}