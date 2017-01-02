/* HyperionCore */

#include "Chat.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "MiscPackets.h"
#include "Language.h"
#define dtor(deg) (deg * M_PI / 180.0)

const char* PROMETHEUS_SELECT_CHAR = "SELECT height, morph, flags FROM prometheus WHERE character = %u";
const char* PROMETHEUS_INSERT_NEW_CHAR = "INSERT INTO prometheus VALUES (%u, %f, %u, %u)";

// We use this enough. Predefine it.
const float PI_OVER_2 = 1.57079632679;
const double MIN_HEIGHT = 0.2f;
const double MAX_HEIGHT = 2.0f;

enum Prometheus_Flags {
    NONE = 0x00000000,
    WORLD_PVP_ENABLED = 0x00000001,
};

static const char* const DIRECTIONS[] = { "n", "ne", "e", "se", "s", "sw", "w", "nw" };
static const int DIRECTION_COUNT = 8;

class PrometheusCommandScript : public CommandScript {
    public:
        PrometheusCommandScript() : CommandScript( "PrometheusCommandScript" ) {  }
        std::vector<ChatCommand> GetCommands() const override
        {
            static std::vector<ChatCommand> commandTable =
            {
                { "barbershop", rbac::RBAC_PERM_COMMAND_BARBERSHOP, false, &HandleBarbershopCommand, "" },
                { "warp",       rbac::RBAC_PERM_COMMAND_WARP,       false, &HandleWarpCommand,       "" },
                { "face",       rbac::RBAC_PERM_COMMAND_FACE,       false, &HandleFaceCommand,       "" },
            };

            return commandTable;
        }

        static bool HandleBarbershopCommand( ChatHandler* handler) {
            Player* player = handler->GetSession()->GetPlayer();
            WorldPackets::Misc::EnableBarberShop packet;

            player->SendDirectMessage( packet.Write() );
            return true;
        }

        static bool HandleFaceCommand( ChatHandler* handler, const char* args ) {
            if (!*args)
                return false;

            Player* player = handler->GetSession()->GetPlayer();
            char* dir = strtok( (char*) args, " " );
            float o;


            if (isdigit(dir[0])) {
                o = atoi(dir);
            } else {
                for (int i = 0; i < DIRECTION_COUNT; i++) {
                    if(dir == DIRECTIONS[i])
                        break;
                    o += 45;
                }
            }

            // Gotta make the orientation into radians
            o = dtor(o);
            float x = player->GetPositionX();
            float y = player->GetPositionY();
            float z = player->GetPositionZ();
            uint32 map = player->GetMapId();

            player->TeleportTo( map, x, y, z, o );
            return true;
        }

        static bool HandleWarpCommand( ChatHandler* handler, const char* args ) {
            if (!*args)
                return false;

            Player* player = handler->GetSession()->GetPlayer();
            char* dist = strtok( (char*) args, " " );
            char* dir = strtok( NULL, " " );

            if (!dist || !dir)
                return false;

            char d = dir[0];
            float value = atof(dist);
            float x = player->GetPositionX();
            float y = player->GetPositionY();
            float z = player->GetPositionZ();
            float o = player->GetOrientation();
            uint32 map = player->GetMapId();

            switch (d) {
                case 'l':
                    x += cos( o + PI_OVER_2 ) * value;
                    y += sin( o + PI_OVER_2 ) * value;
                    break;
                case 'r':
                    x += cos( o - PI_OVER_2 ) * value;
                    y += sin( o - PI_OVER_2 ) * value;
                    break;
                case 'f':
                    x += cos( o ) * value;
                    y += sin( o ) * value;
                    break;
                case 'b':
                    x -= cos( o ) * value;
                    y -= sin( o ) * value;
                    break;
                case 'u':
                    z = z + value;
                    break;
                case 'd':
                    z = z - value;
                    break;
                case 'o':
                    o = Position::NormalizeOrientation( dtor(value) + o );
                    break;
                default:
                    return false;
            }
            player->TeleportTo( map, x, y, z, o );
            return true;
        }
};


class Prometheus : public PlayerScript {
    public:
        Prometheus() : PlayerScript( "prometheus" ) {}
        void OnLogin( Player* player, bool firstLogin ) {
            if (!firstLogin) {
                QueryResult r = WorldDatabase.PQuery( PROMETHEUS_SELECT_CHAR, player->GetGUID().GetCounter() );

                if (r) {
                    Field* field = r->Fetch();
                    SetHeight( player, field[0].GetFloat() );
                    SetRace( player, field[1].GetUInt16() );
                    SetCustomFlags( player, field[2].GetUInt32() );
                } else { // Just incase there was a character that didn't have new character data
                    WorldDatabase.PExecute( PROMETHEUS_INSERT_NEW_CHAR, player->GetGUID().GetCounter(), 1, -1, 0 );
                }
            } else { // Its a new character so add in new character data
                WorldDatabase.PExecute( PROMETHEUS_INSERT_NEW_CHAR, player->GetGUID().GetCounter(), 1, -1, 0 );
            }
        }

        void SetHeight(Player* player, float height) {
            // Limit on how small or big your character can be.
            if (height >= MIN_HEIGHT && height <= MAX_HEIGHT)
               player->SetObjectScale( height );
        }

        void SetRace(Player* player, uint16 morph) {
            if (morph != -1)
                player->SetDisplayId(morph);
        }

        void SetCustomFlags(Player* player, uint32 flags) {
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
    new PrometheusCommandScript();
}
