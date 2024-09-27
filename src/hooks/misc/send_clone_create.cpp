#include "hooking/hooking.hpp"
#include "services/players/player_service.hpp"

#include <network/CNetGamePlayer.hpp>

namespace big
{
	void hooks::send_clone_create(CNetworkObjectMgr* _this, rage::netObject* object, CNetGamePlayer* player, rage::datBitBuffer* buffer)
	{
		if (auto plyr = g_player_service->get_by_id(player->m_player_id); plyr && plyr->bad_host && (eNetObjType)object->m_object_type == eNetObjType::NET_OBJ_TYPE_PLAYER)
		{
			return;
		}

		g_hooking->get_original<hooks::send_clone_create>()(_this, object, player, buffer);
	}
}