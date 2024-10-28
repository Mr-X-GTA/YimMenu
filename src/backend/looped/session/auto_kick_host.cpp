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

	static bool bLastKickHost   = false;
	static int lastKickedHostID = -1; // Letzter gekickter Host
	void looped::session_auto_kick_host()
	{
		// Aktivität der Sitzung überprüfen
		if (!*g_pointers->m_gta.m_is_session_started)
		{
			return;
		}

		// Lokalen Spieler und aktuellen Host abrufen
		int localPlayerID = PLAYER::PLAYER_ID();
		int hostPlayerID  = NETWORK::NETWORK_GET_HOST_PLAYER_INDEX();

		// Bedingung: Kick aktivieren, wenn ein Host gesetzt ist und er nicht der letzte gekickte Host war
		if (hostPlayerID != -1 && hostPlayerID != localPlayerID && hostPlayerID != lastKickedHostID)
		{
			g_player_service->iterate([&](auto& player) {
				// Bedingungen zum Überspringen bestimmter Spieler (Freunde, Modder, vertrauenswürdige Spieler)
				if (player.second->is_trusted || (g.session.trust_friends && player.second->is_friend())
				    || (player.second->is_modder && g.session.exclude_modders_from_kick_host))
				{
					return; // Spieler überspringen
				}

				// Prüfen, ob der Spieler der Host ist und ihn kicken
				if (player.second->is_host()) // Wenn der Spieler-Host geprüft wird
				{
					player_command::get("smartkick"_J)->call(player.second, {});

					// Letzten gekickten Host speichern und Status aktualisieren
					lastKickedHostID = hostPlayerID;
					bLastKickHost    = true;
				}
			});
		}
		else
		{
			// Wenn die Bedingung für den Kick nicht erfüllt ist, zurücksetzen
			bLastKickHost = false;
		}
	}
}
