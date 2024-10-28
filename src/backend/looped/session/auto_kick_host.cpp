#include "backend/looped/looped.hpp"
#include "backend/player_command.hpp"
#include "natives.hpp"
#include "pointers.hpp"

namespace big
{
	static bool is_next_in_queue()
	{
		uint64_t my_host_token = g_player_service->get_self()->get_net_data()->m_host_token;

		for (const auto& plyr : g_player_service->players() | std::ranges::views::values)
		{
			if (plyr->is_host())
				continue;

			if (plyr->get_net_data()->m_host_token < my_host_token)
			{
				return false;
			}
		}

		return true;
	}

	static int lastKickedHostID = -1;
	static bool bLastKickHost = false;
	void looped::session_auto_kick_host()
	{
		if (!*g_pointers->m_gta.m_is_session_started)
		{
			return;
		}

		const auto localPlayerID = PLAYER::PLAYER_ID();
		int hostPlayerID         = NETWORK::NETWORK_GET_HOST_PLAYER_INDEX();

		if (hostPlayerID != -1 && hostPlayerID != localPlayerID && hostPlayerID != lastKickedHostID)
		{
			g_player_service->iterate([&](auto& player) {
				if (player.second->is_trusted || (g.session.trust_friends && player.second->is_friend()) || (g.session.exclude_modders_from_kick_host))
				{
					return;
				}


				if (player.second->is_host())
				{
					player_command::get("smartkick"_J)->call(player.second, {});

					lastKickedHostID = hostPlayerID;
					bLastKickHost    = true;
				}
			});
		}
		else
		{
			bLastKickHost = false;
		}
	}
	
}
