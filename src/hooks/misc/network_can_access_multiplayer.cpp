#include "hooking/hooking.hpp"

namespace big
{
	uint32_t hooks::network_can_access_multiplayer(uint32_t a1, uint64_t* a2)
	{
		if (a2)
			*a2 = 0;

		return 0;
	}
}
