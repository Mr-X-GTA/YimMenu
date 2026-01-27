#include "hooking/hooking.hpp"

#include <rage/gameSkeleton.hpp>

namespace big
{
	void hooks::game_skeleton_update(rage::game_skeleton* skeleton, int type)
	{
		for (auto mode = skeleton->m_update_modes; mode; mode = mode->m_next)
		{
			if (mode->m_type != type)
				continue;

			for (auto group = mode->m_head; group; group = group->m_next)
			{
				if (group->m_hash != "Common Main"_J)
				{
					group->run();
					continue;
				}

				for (auto item = static_cast<rage::game_skeleton_update_group*>(group)->m_head; item; item = item->m_next)
				{
					if (item->m_hash != 0xA0F39FB6 && item->m_hash != "TamperActions"_J)
					{
						item->run();
					}
				}
			}

			break;
		}
	}
}