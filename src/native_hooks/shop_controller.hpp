#pragma once
#include "native_hooks.hpp"
#include "core/scr_globals.hpp"

namespace big
{
	namespace shop_controller
	{
		inline std::string format_chips(int value)
		{
			std::string formatted = std::to_string(value);
			for (int i = formatted.size() - 3; i > 0; i -= 3)
			{
				formatted.insert(i, ",");
			}
			return formatted;
		}

		void SET_WARNING_MESSAGE_WITH_HEADER(rage::scrNativeCallContext* src)
		{
			if (auto entry_line = src->get_arg<const char*>(1); !strcmp(entry_line, "CTALERT_F_2"))
			{
				if (g.notifications.transaction_rate_limit.log)
					LOG(WARNING) << "Received transaction rate limit";
				if (g.notifications.transaction_rate_limit.notify)
					g_notification_service.push_warning("TRANSACTION_RATE_LIMIT"_T.data(), "TRANSACTION_RATE_LIMIT_MESSAGE"_T.data());

				*scr_globals::transaction_overlimit.as<PBOOL>() = FALSE;

				return;
			}

			HUD::SET_WARNING_MESSAGE_WITH_HEADER(src->get_arg<const char*>(0), src->get_arg<const char*>(1), src->get_arg<int>(2), src->get_arg<const char*>(3), src->get_arg<BOOL>(4), src->get_arg<Any>(5), src->get_arg<Any*>(6), src->get_arg<Any*>(7), src->get_arg<BOOL>(8), src->get_arg<Any>(9));
		}

		void SCALEFORM_MOVIE_METHOD_ADD_PARAM_INT(rage::scrNativeCallContext* src)
		{
			auto arg0 = src->get_arg<int>(0);
			if (g.window.gui.format_money)
			{
				Hash casino_chips = self::char_index ? "MP1_CASINO_CHIPS"_J : "MP0_CASINO_CHIPS"_J;
				int player_chips;

				STATS::STAT_GET_INT(casino_chips, &player_chips, -1);
				if (arg0 == player_chips && player_chips >= 1000)
				{
					auto chips_format = format_chips(player_chips);
					return GRAPHICS::SCALEFORM_MOVIE_METHOD_ADD_PARAM_PLAYER_NAME_STRING(chips_format.c_str());
				}
			}

			GRAPHICS::SCALEFORM_MOVIE_METHOD_ADD_PARAM_INT(arg0);
		}

		void NET_GAMESERVER_GET_PRICE(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				src->set_return_value<int>(0);
				return;
			}
			
			// We can call the native directly since we are replacing the handler, 
			// checking native_hooks.cpp details, it appears standard pattern.
			// However, since we don't have the original handler passed in easily here 
			// (native_hooks uses a map), and calling the native via namespace calls the invoke logic
			// which uses the handler table?
			// If we replace the handler in the script table, calling it from C++ invoker 
			// (which usually uses the global table or a separate mechanism) should be fine 
			// IF the C++ invoker doesn't use the script table we just hooked.
			// BigBase / YimMenu invoker typically finds the handler from the global table.
			// The `add_native_detour` replaces it in the *script program's* import table.
			// So calling it globally is safe.
			
			auto itemHash = src->get_arg<Hash>(0);
			auto categoryHash = src->get_arg<Hash>(1);
			auto p2 = src->get_arg<BOOL>(2);
			
			// Native: 0xC27009422FCCA88D -> NET_GAMESERVER_GET_PRICE
			// We need to define it or find it. The user provided the definition.
			// Let's assume it's available via NETSHOPPING namespace or similar if defined in natives.hpp
			// or we can use invoke<int>(0xC27009422FCCA88D, ...)
			
			src->set_return_value<int>(NETSHOPPING::NET_GAMESERVER_GET_PRICE(itemHash, categoryHash, p2));
		}

		void NET_GAMESERVER_CATALOG_ITEM_IS_VALID(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				src->set_return_value<BOOL>(TRUE);
				return;
			}
			auto name = src->get_arg<const char*>(0);
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_CATALOG_ITEM_IS_VALID(name));
		}

		void NET_GAMESERVER_CATALOG_ITEM_KEY_IS_VALID(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				src->set_return_value<BOOL>(TRUE);
				return;
			}
			auto hash = src->get_arg<Hash>(0);
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_CATALOG_ITEM_KEY_IS_VALID(hash));
		}

		void NET_GAMESERVER_CATALOG_IS_VALID(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				src->set_return_value<BOOL>(TRUE);
				return;
			}
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_CATALOG_IS_VALID());
		}

		void NET_GAMESERVER_BEGIN_SERVICE(rage::scrNativeCallContext* src)
		{
			auto transactionId = src->get_arg<int*>(0);
			auto categoryHash = src->get_arg<Hash>(1);
			auto itemHash = src->get_arg<Hash>(2);
			auto actionTypeHash = src->get_arg<Hash>(3);
			auto value = src->get_arg<int>(4);
			auto flags = src->get_arg<int>(5);

			if (g.self.free_shopping)
				value = 0;

			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_BEGIN_SERVICE(transactionId, categoryHash, itemHash, actionTypeHash, value, flags));
		}

		void NET_GAMESERVER_USE_SERVER_TRANSACTIONS(rage::scrNativeCallContext *src)
		{
			if (g.self.free_shopping)
			{
				src->set_return_value<BOOL>(TRUE);
				return;
			}
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_USE_SERVER_TRANSACTIONS());
		}

		void NET_GAMESERVER_RETRIEVE_INIT_SESSION_STATUS(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				if (auto p0 = src->get_arg<int*>(0))
					*p0 = 1; // Success
				src->set_return_value<BOOL>(TRUE);
				return;
			}
			auto p0 = src->get_arg<int*>(0);
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_RETRIEVE_INIT_SESSION_STATUS(p0));
		}

		void NET_GAMESERVER_START_SESSION(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				src->set_return_value<BOOL>(TRUE);
				return;
			}
			auto charSlot = src->get_arg<int>(0);
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_START_SESSION(charSlot));
		}

		void NET_GAMESERVER_START_SESSION_PENDING(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				src->set_return_value<BOOL>(FALSE);
				return;
			}
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_START_SESSION_PENDING());
		}

		void NET_GAMESERVER_RETRIEVE_START_SESSION_STATUS(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				if (auto p0 = src->get_arg<int*>(0))
					*p0 = 1; // Success
				src->set_return_value<BOOL>(TRUE);
				return;
			}
			auto p0 = src->get_arg<int*>(0);
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_RETRIEVE_START_SESSION_STATUS(p0));
		}

		void NET_GAMESERVER_IS_SESSION_REFRESH_PENDING(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				src->set_return_value<BOOL>(FALSE);
				return;
			}
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_IS_SESSION_REFRESH_PENDING());
		}

		void NET_GAMESERVER_TRANSACTION_IN_PROGRESS(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				src->set_return_value<BOOL>(FALSE);
				return;
			}
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_TRANSACTION_IN_PROGRESS());
		}

		void NET_GAMESERVER_IS_SESSION_VALID(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				src->set_return_value<BOOL>(TRUE);
				return;
			}
			auto charSlot = src->get_arg<int>(0);
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_IS_SESSION_VALID(charSlot));
		}

		void NET_GAMESERVER_RETRIEVE_SESSION_ERROR_CODE(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				if (auto p0 = src->get_arg<int*>(0))
					*p0 = 0;
				src->set_return_value<BOOL>(TRUE);
				return;
			}
			auto p0 = src->get_arg<int*>(0);
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_RETRIEVE_SESSION_ERROR_CODE(p0));
		}

		void NET_GAMESERVER_IS_CATALOG_CURRENT(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				src->set_return_value<BOOL>(TRUE);
				return;
			}
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_IS_CATALOG_CURRENT());
		}

		void NET_GAMESERVER_RETRIEVE_CATALOG_REFRESH_STATUS(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				auto state = src->get_arg<int*>(0);
				if (state)
					*state = 1; // Finished
				src->set_return_value<BOOL>(TRUE);
				return;
			}

			auto state = src->get_arg<int*>(0);
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_RETRIEVE_CATALOG_REFRESH_STATUS(state));
		}

		void NETWORK_BUY_ITEM(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
				src->set_arg<int>(0, 0);

			auto amount = src->get_arg<int>(0);
			auto item = src->get_arg<Hash>(1);
			auto p2 = src->get_arg<Any>(2);
			auto p3 = src->get_arg<Any>(3);
			auto p4 = src->get_arg<BOOL>(4);
			auto item_name = src->get_arg<const char*>(5);
			auto p6 = src->get_arg<Any>(6);
			auto p7 = src->get_arg<Any>(7);
			auto p8 = src->get_arg<Any>(8);
			auto p9 = src->get_arg<BOOL>(9);

			MONEY::NETWORK_BUY_ITEM(amount, item, p2, p3, p4, item_name, p6, p7, p8, p9);
		}

		void NETWORK_BUY_PROPERTY(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
				src->set_arg<int>(0, 0);

			auto cost = src->get_arg<int>(0);
			auto propertyName = src->get_arg<Hash>(1);
			auto p2 = src->get_arg<BOOL>(2);
			auto p3 = src->get_arg<BOOL>(3);

			MONEY::NETWORK_BUY_PROPERTY(cost, propertyName, p2, p3);
		}

		void NETWORK_DEDUCT_CASH(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
				src->set_arg<int>(0, 0);

			auto amount = src->get_arg<int>(0);
			auto p1 = src->get_arg<const char*>(1);
			auto p2 = src->get_arg<const char*>(2);
			auto p3 = src->get_arg<BOOL>(3);
			auto p4 = src->get_arg<BOOL>(4);
			auto p5 = src->get_arg<BOOL>(5);

			MONEY::NETWORK_DEDUCT_CASH(amount, p1, p2, p3, p4, p5);
		}

		void NETWORK_BUY_HEALTHCARE(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
				src->set_arg<int>(0, 0);

			auto cost = src->get_arg<int>(0);
			auto p1 = src->get_arg<BOOL>(1);
			auto p2 = src->get_arg<BOOL>(2);

			MONEY::NETWORK_BUY_HEALTHCARE(cost, p1, p2);
		}

		void NETWORK_BUY_AIRSTRIKE(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
				src->set_arg<int>(0, 0);

			auto cost = src->get_arg<int>(0);
			auto p1 = src->get_arg<BOOL>(1);
			auto p2 = src->get_arg<BOOL>(2);
			auto p3 = src->get_arg<Any>(3);

			MONEY::NETWORK_BUY_AIRSTRIKE(cost, p1, p2, p3);
		}

		void NETWORK_BUY_HELI_STRIKE(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
				src->set_arg<int>(0, 0);

			auto cost = src->get_arg<int>(0);
			auto p1 = src->get_arg<BOOL>(1);
			auto p2 = src->get_arg<BOOL>(2);
			auto p3 = src->get_arg<Any>(3);

			MONEY::NETWORK_BUY_HELI_STRIKE(cost, p1, p2, p3);
		}

		void NETWORK_BUY_BOUNTY(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
				src->set_arg<int>(0, 0);

			auto amount = src->get_arg<int>(0);
			auto victim = src->get_arg<Player>(1);
			auto p2 = src->get_arg<BOOL>(2);
			auto p3 = src->get_arg<BOOL>(3);
			auto p4 = src->get_arg<Any>(4);

			MONEY::NETWORK_BUY_BOUNTY(amount, victim, p2, p3, p4);
		}

		void NETWORK_BUY_FAIRGROUND_RIDE(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
				src->set_arg<int>(0, 0);

			auto amount = src->get_arg<int>(0);
			auto p1 = src->get_arg<Any>(1);
			auto p2 = src->get_arg<BOOL>(2);
			auto p3 = src->get_arg<BOOL>(3);
			auto p4 = src->get_arg<Any>(4);

			MONEY::NETWORK_BUY_FAIRGROUND_RIDE(amount, p1, p2, p3, p4);
		}

		void NETWORK_SPENT_MOVE_YACHT(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
				src->set_arg<int>(0, 0);

			auto amount = src->get_arg<int>(0);
			auto p1 = src->get_arg<BOOL>(1);
			auto p2 = src->get_arg<BOOL>(2);

			MONEY::NETWORK_SPENT_MOVE_YACHT(amount, p1, p2);
		}

		void NETWORK_SPENT_HANGAR_UTILITY_CHARGES(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
				src->set_arg<int>(0, 0);

			auto amount = src->get_arg<int>(0);
			auto p1 = src->get_arg<BOOL>(1);
			auto p2 = src->get_arg<BOOL>(2);

			MONEY::NETWORK_SPENT_HANGAR_UTILITY_CHARGES(amount, p1, p2);
		}

		void NETWORK_SPENT_HANGAR_STAFF_CHARGES(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
				src->set_arg<int>(0, 0);

			auto amount = src->get_arg<int>(0);
			auto p1 = src->get_arg<BOOL>(1);
			auto p2 = src->get_arg<BOOL>(2);

			MONEY::NETWORK_SPENT_HANGAR_STAFF_CHARGES(amount, p1, p2);
		}

		void NETWORK_CASINO_BUY_CHIPS(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
				src->set_arg<int>(0, 0);

			auto amount = src->get_arg<int>(0);
			auto p1 = src->get_arg<int>(1);

			MONEY::NETWORK_CASINO_BUY_CHIPS(amount, p1);
		}

		void NETWORK_CAN_SPEND_MONEY(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				src->set_return_value<BOOL>(TRUE);
				return;
			}
			auto p0 = src->get_arg<Any>(0);
			auto p1 = src->get_arg<BOOL>(1);
			auto p2 = src->get_arg<BOOL>(2);
			auto p3 = src->get_arg<BOOL>(3);
			auto p4 = src->get_arg<Any>(4);
			auto p5 = src->get_arg<Any>(5);
			src->set_return_value<BOOL>(MONEY::NETWORK_CAN_SPEND_MONEY(p0, p1, p2, p3, p4, p5));
		}

		void NETWORK_CAN_SPEND_MONEY2(rage::scrNativeCallContext* src)
		{
			if (g.self.free_shopping)
			{
				src->set_return_value<BOOL>(TRUE);
				return;
			}
			auto p0 = src->get_arg<Any>(0);
			auto p1 = src->get_arg<BOOL>(1);
			auto p2 = src->get_arg<BOOL>(2);
			auto p3 = src->get_arg<BOOL>(3);
			auto p4 = src->get_arg<Any*>(4);
			auto p5 = src->get_arg<Any>(5);
			auto p6 = src->get_arg<Any>(6);
			src->set_return_value<BOOL>(MONEY::NETWORK_CAN_SPEND_MONEY2(p0, p1, p2, p3, p4, p5, p6));
		}
	}
}
