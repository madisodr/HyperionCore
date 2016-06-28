/* HyperionCore */

#include "Chat.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "MiscPackets.h"

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
		};

		return commandTable;
	}

	static bool HandleBarbershopCommand( ChatHandler* handler, const char* args ) {
		Player* player = handler->GetSession()->GetPlayer();
		WorldPackets::Misc::EnableBarberShop packet;

		player->SendDirectMessage( packet.Write() );
		return true;
	}
};

class Prometheus : public PlayerScript {
public:
	Prometheus() : PlayerScript( "prometheus" ) {}
	void OnLogin( Player* player, bool firstLogin ) {
		if(!firstLogin) {
			QueryResult r = WorldDatabase.PQuery( "SELECT height, morph, flags FROM prometheus WHERE character = %u", player->GetGUID().GetCounter() );

			if(r) {
				Field* field = r->Fetch();
				SetHeight( player, field[0].GetFloat() );
				SetRace( player, field[1].GetUInt16() );
				SetCustomFlags( player, field[2].GetUInt32() );
			}
		} else {
			WorldDatabase.PExecute( "INSERT INTO prometheus VALUES (%u, %f, %u, %u)", player->GetGUID().GetCounter(), 1, -1, 0 );
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