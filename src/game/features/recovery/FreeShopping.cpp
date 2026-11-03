#include "backend/bool_command.hpp"
#include "native_hooks/free_shopping.hpp"

namespace big
{
	bool_command g_free_shopping("freeshopping", "Free Shopping", "Allows you to buy everything for free. Some properties are not free, they are discounted to the cheapest possible price instead.", free_shopping::g_enabled);
}
