#include "hooking/hooking.hpp"
#include "pointers.hpp"

namespace big
{
	void* hooks::add_skeleton_extension(rage::fwEntity* entity)
	{
		if (*g_pointers->m_gta.m_skeleton_extension_count >= 32) [[unlikely]]
		{
			return nullptr;
		}

		return g_hooking->get_original<hooks::add_skeleton_extension>()(entity);
	}
}