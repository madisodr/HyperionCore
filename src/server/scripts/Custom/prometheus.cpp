/* HyperionCore */

#include "Chat.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "MiscPackets.h"


static const char* PROMETHEUS_SELECT_CHAR = "SELECT height, morph, flags FROM prometheus WHERE character = %u";
static const char* PROMETHEUS_INSERT_NEW_CHAR = "INSERT INTO prometheus VALUES (%u, %f, %u, %u)";

enum Prometheus_Flags {
	NONE = 0x00000000,
	WORLD_PVP_ENABLED = 0x00000001,
};


class Prometheus_CommandScript : public CommandScript {
public:
	Prometheus_CommandScript() : CommandScript( "Prometheus_CommandScript" ) {}
	std::vector<ChatCommand> GetCommands() const override {
		static std::vector<ChatCommand> commandTable = {
			{"barbershop",   rbac::RBAC_PERM_COMMAND_BARBERSHOP,   false,  &HandleBarbershopCommand,   ""},
			{"warp",         rbac::RBAC_PERM_COMMAND_WARP, false, &HandleWarpCommand, "" },
      {"face",         rbac::RBAC_PERM_COMMAND_FACE, false, &HandleFaceCommand, "" },
		};

		return commandTable;
	}

	static bool HandleBarbershopCommand( ChatHandler* handler, const char* args ) {
		Player* player = handler->GetSession()->GetPlayer();
		WorldPackets::Misc::EnableBarberShop packet;

		player->SendDirectMessage( packet.Write() );
		return true;
	}

  static bool HandleFaceCommand( ChatHandler* handler, const char* args ) {
    if(!*args)
      return false;


    Player* player = handler->GetSession()->GetPlayer();
    char* dir = strtok( (char*) args, " " );
    bool intVal = false;
    float o = 0.0;

    if(isdigit( dir[0] )) {
      intVal = true;
      o = atoi( dir );
    } 

    if(!intVal) {
      if(dir == "n") 
        o = 0;
      else if(dir == "ne") 
        o = 45;
      else if(dir == "e") 
        o = 90;
      else if(dir == "se")
        o = 135;
      else if(dir == "s") 
        o = 180;
      else if(dir == "sw") 
        o = 225;
      else if(dir == "w") 
        o = 270;
      else if(dir == "nw") 
        o = 315;
      else o = 0;
    }

    o = (o * M_PI / 180.0f);
    float x = player->GetPositionX();
    float y = player->GetPositionY();
    float z = player->GetPositionZ();
    float o = player->GetOrientation();
    uint32 map = player->GetMapId();

    player->TeleportTo( map, x, y, z, o );
  }

	static bool HandleWarpCommand( ChatHandler* handler, const char* args ) {
		if(!*args)
			return false;

		Player* player = handler->GetSession()->GetPlayer();
		char* dist = strtok( (char*) args, " " );
		char* dir = strtok( NULL, " " );

		if(!dist || !dir)
			return false;

		char d = dir[0];
		float value = float( atof( dist ) );
		float x = player->GetPositionX();
		float y = player->GetPositionY();
		float z = player->GetPositionZ();
		float o = player->GetOrientation();
		uint32 map = player->GetMapId();

		switch(d) {
			case 'l':
				x += cos( o + (M_PI / 2) ) * value;
				y += sin( o + (M_PI / 2) ) * value;
				player->TeleportTo( map, x, y, z, o );
				break;
			case 'r':
				x += cos( o - (M_PI / 2) ) * value;
				y += sin( o - (M_PI / 2) ) * value;
				player->TeleportTo( map, x, y, z, o );
				break;
			case 'f':
				x += cos( o ) * value;
				y += sin( o ) * value;
				player->TeleportTo( map, x, y, z, o );
				break;
			case 'b':
				x -= cos( o ) * value;
				y -= sin( o ) * value;
				player->TeleportTo( map, x, y, z, o );
				break;
			case 'u':
				player->TeleportTo( map, x, y, z + value, o );
				break;
			case 'd':
				player->TeleportTo( map, x, y, z - value, o );
				break;
			case 'o':
				o = Position::NormalizeOrientation( (value * M_PI / 180.0f) + o );
				player->TeleportTo( map, x, y, z, o );
				break;
			default:
				return false;
		}

		return true;

	}
};


class Prometheus : public PlayerScript {
public:
	Prometheus() : PlayerScript( "prometheus" ) {}
	void OnLogin( Player* player, bool firstLogin ) {
		if(!firstLogin) {
			QueryResult r = WorldDatabase.PQuery( PROMETHEUS_SELECT_CHAR, player->GetGUID().GetCounter() );

			if(r) {
				Field* field = r->Fetch();
				SetHeight( player, field[0].GetFloat() );
				SetRace( player, field[1].GetUInt16() );
				SetCustomFlags( player, field[2].GetUInt32() );
			}
		} else {
			WorldDatabase.PExecute( PROMETHEUS_INSERT_NEW_CHAR, player->GetGUID().GetCounter(), 1, -1, 0 );
		}
	}

	inline void SetHeight( Player* player, float height ) {
		if(height <= 0.2f && height > 5.0f)
			player->SetObjectScale( 1 );

		player->SetObjectScale( height );
	}

	inline void SetRace( Player* player, uint16 morph ) {
		if(morph == -1)
			return;

		player->SetDisplayId( morph );

	}

	void SetCustomFlags( Player* player, uint32 flags ) {
		if(flags == Prometheus_Flags::NONE)
			return;

		if(flags & Prometheus_Flags::WORLD_PVP_ENABLED) {
			// Enable PVP stuff
		} else {
			player->RemoveByteFlag( UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_PVP );
			player->HasByteFlag( UNIT_FIELD_BYTES_2, 1, UNIT_BYTE2_FLAG_FFA_PVP );
			player->RemoveFlag( PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP );
			player->RemoveFlag( PLAYER_FLAGS, PLAYER_FLAGS_PVP_TIMER );
			player->RemoveFlag( PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP );
		}
	}
};

void AddSC_Prometheus() {
	new Prometheus();
	new Prometheus_CommandScript();
}