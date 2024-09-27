#include "backend/player_command.hpp"
#include "gta_util.hpp"
#include "pointers.hpp"
#include "services/battleye/battleye_service.hpp"

#include <network/Network.hpp>

#include "packet.hpp"
#include <network/snSession.hpp>

namespace big
{
	class battleye_update_kick : player_command
	{
		using player_command::player_command;

		virtual CommandAccessLevel get_access_level() override
		{
			return CommandAccessLevel::TOXIC;
		}

		virtual void execute(player_ptr player, const command_arguments& _args, const std::shared_ptr<command_context> ctx) override
		{
			unsigned char data[] = {0x00, 0x50, 0x31, 0x4A, 0xC0, 0x1A, 0x13, 0xFF, 0xFF, 0xFF};
			player->tampered_with_be = true;
			for (int i = 0; i < 20; i++)
			{
				data[0] = i;
				g_battleye_service.send_message_to_client(player->get_net_game_player()->get_host_token(), &data, sizeof(data));
			}
		}
	};

	battleye_update_kick g_battleye_update_kick("battleupdate", "BATTLEYE_UPDATE_KICK", "BATTLEYE_UPDATE_KICK_DESC", 0);
}