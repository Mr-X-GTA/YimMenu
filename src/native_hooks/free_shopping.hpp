#pragma once
#include "invoker/invoker.hpp"
#include "native_hooks.hpp"
#include "pointers.hpp"
#include "types/netshop/netCatalog.hpp"

#include "memory/module.hpp"

#include <climits>
#include <cstdio>
#include <cstring>

namespace big::free_shopping
{
	inline bool g_enabled = false;

	struct basket_item_data
	{
		alignas(8) Hash key;
		alignas(8) Hash item;
		alignas(8) int price;
		alignas(8) int stat_value;
	};
	static_assert(sizeof(basket_item_data) == 32);

	constexpr rage::joaat_t free_purchase_coupon        = "PO_COUPON_CAR_XMAS2017"_J;
	constexpr rage::joaat_t category_casino_chips       = "CATEGORY_CASINO_CHIPS"_J;
	constexpr rage::joaat_t category_casino_chip_reason = "CATEGORY_CASINO_CHIP_REASON"_J;
	constexpr rage::joaat_t casino_chips_item           = "CASINO_CHIPS_v0"_J;
	constexpr rage::joaat_t action_casino_chips_spend   = 0xFEAE09E5;
	constexpr rage::joaat_t category_property           = "CATEGORY_PROPERTIE"_J;
	constexpr rage::joaat_t category_warehouse          = "CATEGORY_WAREHOUSE"_J;

	inline bool is_property_action(rage::joaat_t action)
	{
		return action == "NET_SHOP_ACTION_BUY_PROPERTY"_J || action == "NET_SHOP_ACTION_BUY_WAREHOUSE"_J;
	}

	inline bool is_chip_category(rage::joaat_t category)
	{
		return category == category_casino_chips || category == category_casino_chip_reason;
	}

	inline rage::joaat_t joaat_string(const char* string)
	{
		rage::joaat_t result = 0;
		for (const char* c = string; *c; ++c)
		{
			result += *c >= 'A' && *c <= 'Z' ? (*c | 1 << 5) : *c;
			result += (result << 10);
			result ^= (result >> 6);
		}
		result += (result << 3);
		result ^= (result >> 11);
		result += (result << 15);
		return result;
	}

	inline void property_price_check(const char* name, rage::joaat_t category, rage::joaat_t& best_hash, int& best_price)
	{
		const auto hash  = joaat_string(name);
		const int  price = NETSHOPPING::NET_GAMESERVER_GET_PRICE(hash, category, TRUE);
		if (price > 0 && (best_hash == 0 || price < best_price))
		{
			best_hash  = hash;
			best_price = price;
		}
	}

	struct cheapest_cache
	{
		bool done         = false;
		rage::joaat_t hash = 0;
		int price          = 0;
	};

	inline bool cheapest_for_category(rage::joaat_t category, rage::joaat_t& out_hash, int& out_price)
	{
		static cheapest_cache caches[2];
		cheapest_cache* cache = category == category_warehouse ? &caches[1] : &caches[0];

		if (cache->done)
		{
			out_hash  = cache->hash;
			out_price = cache->price;
			return cache->hash != 0;
		}

		rage::joaat_t best_hash = 0;
		int best_price          = 0;
		char name[64];

		const auto check = [&](const char* pattern, int n) {
			std::snprintf(name, sizeof(name), pattern, n);
			property_price_check(name, category, best_hash, best_price);
		};

		if (category == category_warehouse)
		{
			for (int n = 0; n <= 32; ++n)
				check("FACTORY_INDEX_%d_t0_v0", n);
			for (int n = 0; n <= 22; ++n)
				check("WH_INDEX_%d_t0_v0", n);
			for (int n = 0; n <= 10; ++n)
				check("BHUB_INDEX_%d_t0_v0", n);
			for (int n = 0; n <= 10; ++n)
				check("IE_WH_INDEX_%d_t0_v0", n);
			for (int n = 0; n <= 10; ++n)
				check("NCLU_INDEX_%d_t0_v0", n);
			for (int n = 0; n <= 9; ++n)
				check("DBASE_INDEX_%d_t0_v0", n);
			for (int n = 0; n <= 6; ++n)
				check("ARC_INDEX_%d_t0_v0", n);
			for (int n = 0; n <= 5; ++n)
				check("HANGAR_INDEX_%d_t0_v0", n);
			for (int n = 1; n <= 5; ++n)
				check("BAOF_INDEX_%d_t0_v0", n);
			for (int n = 1; n <= 5; ++n)
				check("AUTO_SHOP_INDEX_%d_t0_v0", n);
			for (int n = 1; n <= 5; ++n)
				check("SALVAGE_YARD_INDEX_%d_t0_v0", n);
			for (int n = 1; n <= 4; ++n)
				check("FIXER_HQ_INDEX_%d_t0_v0", n);

			for (const char* special : {"CAR_CLUB_MEMBER_v0", "FIELD_HANGAR_INDEX_1_v0", "HACKER_DEN_INDEX_1_v0", "LAB_EQUIPMENT_INSTALLED_v0", "MANSION_INDEX1_t0_v0", "MANSION_INDEX2_t0_v0", "MANSION_INDEX3_t0_v0", "MP_ARENAG_1_t0_v0", "MP_CASINO_1_t0_v0", "MULTISTOREY_GARAGE_INDEX_1_v0", "SMALL_BUSINESS_INDEX_1_v0", "SMALL_BUSINESS_INDEX_2_v0", "SMALL_BUSINESS_INDEX_3_v0"})
				property_price_check(special, category, best_hash, best_price);
		}
		else
		{
			for (int n = 1; n <= 95; ++n)
				check("PR_MP_PROP_%d_t0_v0", n);
			for (int n : {5, 31, 34, 38, 40, 42})
				check("PR_MP_PROP_%d_t0_v1", n);
			for (int n = 1; n <= 12; ++n)
				check("PR_MP_PROP_CLUBH%d_t0_v0", n);
			for (int n = 1; n <= 4; ++n)
				check("PR_MP_PROP_OFF%d_t0_v0", n);
			for (int n = 1; n <= 3; ++n)
				check("PR_MP_PROP_OFFG%d_t0_v0", n);
			for (const char* special : {"PR_MP_PROP_18_t0_v0_CESP", "PR_MP_PROP_62_t0_v0_CESP", "PR_MP_PROP_CLUBH4_t0_v0_CESP", "PR_MP_PROP_OFF2_t0_v0_CESP"})
				property_price_check(special, category, best_hash, best_price);
		}

		cache->done  = true;
		cache->hash  = best_hash;
		cache->price = best_price;
		out_hash     = best_hash;
		out_price    = best_price;
		return best_hash != 0;
	}

	constexpr rage::joaat_t discount_modifiers[] = {
	    "PM_CARMOD_BUYNOW"_J,
	    "PM_CARMOD_TUNER_OWNER_DISCOUNT"_J,
	    "PM_CARMOD_VINEWOOD_GARAGE_DISCOUNT"_J,
	    "PM_CLOTHING_BIN"_J,
	    "PM_CLOTHING_DESIGNER_FEE"_J,
	    "PM_COUPON_ADD_VEH_MOD_P"_J,
	    "PM_COUPON_CAR_MEET_VEH_P"_J,
	    "PM_COUPON_CAR_SITE"_J,
	    "PM_COUPON_CASINO_BIKE_SITE"_J,
	    "PM_COUPON_CASINO_BOAT_SITE"_J,
	    "PM_COUPON_CASINO_CAR_SITE"_J,
	    "PM_COUPON_CASINO_CAR_SITE2"_J,
	    "PM_COUPON_CASINO_MIL_SITE"_J,
	    "PM_COUPON_CASINO_PLANE_SITE"_J,
	    "PM_COUPON_MIL_SITE"_J,
	    "PM_COUPON_PLANE_SITE"_J,
	    "PM_TATTOO_DISCOUNT_MANSION"_J,
	    "PM_WEAPON_DISCOUNT_BRONZE_DRIVEBY"_J,
	    "PM_WEAPON_DISCOUNT_BRONZE_HEADSHOT"_J,
	    "PM_WEAPON_DISCOUNT_BRONZE_KILLS"_J,
	    "PM_WEAPON_DISCOUNT_BRONZE_MEDAL"_J,
	    "PM_WEAPON_DISCOUNT_FIXER_ARMORY"_J,
	    "PM_WEAPON_DISCOUNT_GOLD_DRIVEBY"_J,
	    "PM_WEAPON_DISCOUNT_GOLD_HEADSHOT"_J,
	    "PM_WEAPON_DISCOUNT_GOLD_KILLS"_J,
	    "PM_WEAPON_DISCOUNT_GOLD_MEDAL"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_0_0"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_0_1"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_0_2"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_0_3"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_0_4"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_0_5"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_0_6"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_0_7"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_0_8"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_0_9"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_1_0"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_1_1"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_1_2"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_1_3"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_1_4"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_2_0"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_2_1"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_2_2"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_2_3"_J,
	    "PM_WEAPON_DISCOUNT_GUN_VAN_2_4"_J,
	    "PM_WEAPON_DISCOUNT_MANSION_ARMORY"_J,
	    "PM_WEAPON_DISCOUNT_PLAT_DRIVEBY"_J,
	    "PM_WEAPON_DISCOUNT_PLAT_HEADSHOT"_J,
	    "PM_WEAPON_DISCOUNT_PLAT_KILLS"_J,
	    "PM_WEAPON_DISCOUNT_SILVER_DRIVEBY"_J,
	    "PM_WEAPON_DISCOUNT_SILVER_HEADSHOT"_J,
	    "PM_WEAPON_DISCOUNT_SILVER_KILLS"_J,
	    "PM_WEAPON_DISCOUNT_SILVER_MEDAL"_J,
	    "PM_WEAPON_PIM_AMMO_INCREASE"_J};

	inline bool is_discount_modifier(rage::joaat_t item_id)
	{
		return std::ranges::contains(discount_modifiers, item_id);
	}

	inline rage::joaat_t current_basket_action   = 0;
	inline rage::joaat_t current_basket_category = 0;

	inline void NET_GAMESERVER_BASKET_START(rage::scrNativeCallContext* src)
	{
		current_basket_category = src->get_arg<rage::joaat_t>(1);
		current_basket_action   = src->get_arg<rage::joaat_t>(2);

		if (g_enabled)
			LOG(INFO) << "free_shopping: basket category " << HEX_TO_UPPER(static_cast<std::uint32_t>(current_basket_category)) << " action " << HEX_TO_UPPER(static_cast<std::uint32_t>(current_basket_action));

		src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_BASKET_START(src->get_arg<int*>(0), current_basket_category, current_basket_action, src->get_arg<int>(3)));
	}

	inline void NET_GAMESERVER_BASKET_ADD_ITEM(rage::scrNativeCallContext* src)
	{
		auto item_data     = src->get_arg<basket_item_data*>(0);
		const int quantity = src->get_arg<int>(1);

		if (!g_enabled)
		{
			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_BASKET_ADD_ITEM(reinterpret_cast<Any*>(item_data), quantity));
			return;
		}

		const auto key_hash  = static_cast<rage::joaat_t>(item_data->key);
		const auto item_hash = static_cast<rage::joaat_t>(item_data->item);
		const int price      = item_data->price;

		if (is_discount_modifier(key_hash))
		{
			src->set_return_value<BOOL>(TRUE);
			return;
		}

		if (current_basket_category == category_casino_chips)
		{
			LOG(INFO) << "free_shopping: chip basket line " << HEX_TO_UPPER(static_cast<std::uint32_t>(key_hash)) << '/' << HEX_TO_UPPER(static_cast<std::uint32_t>(item_hash)) << " price " << price << " quantity " << quantity;

			if (price == 1 && quantity > 1 && (key_hash == casino_chips_item || item_hash == casino_chips_item))
			{
				LOG(INFO) << "free_shopping: casino chip spend " << quantity << " -> 1";
				src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_BASKET_ADD_ITEM(reinterpret_cast<Any*>(item_data), 1));
				return;
			}

			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_BASKET_ADD_ITEM(reinterpret_cast<Any*>(item_data), quantity));
			return;
		}

		if (is_property_action(current_basket_action))
		{
			if (price > 0)
			{
				const auto category = current_basket_action == "NET_SHOP_ACTION_BUY_WAREHOUSE"_J ? category_warehouse : category_property;

				rage::joaat_t best_hash = 0;
				int best_price          = 0;
				if (cheapest_for_category(category, best_hash, best_price) && best_price < price)
				{
					LOG(INFO) << "free_shopping: property item " << HEX_TO_UPPER(static_cast<std::uint32_t>(key_hash)) << '/' << HEX_TO_UPPER(static_cast<std::uint32_t>(item_hash)) << " price " << price << " -> " << HEX_TO_UPPER(static_cast<std::uint32_t>(best_hash)) << " price " << best_price;
					item_data->item  = best_hash;
					item_data->price = best_price;
				}
				else
				{
					LOG(INFO) << "free_shopping: property item " << HEX_TO_UPPER(static_cast<std::uint32_t>(key_hash)) << '/' << HEX_TO_UPPER(static_cast<std::uint32_t>(item_hash)) << " no cheaper candidate";
				}
			}

			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_BASKET_ADD_ITEM(reinterpret_cast<Any*>(item_data), quantity));
			return;
		}

		const bool apply_coupon = price > 0;

		if (apply_coupon)
			item_data->price = 0;

		BOOL item_added = NETSHOPPING::NET_GAMESERVER_BASKET_ADD_ITEM(reinterpret_cast<Any*>(item_data), quantity);

		if (apply_coupon && item_added)
		{
			basket_item_data coupon_data{};
			coupon_data.key        = free_purchase_coupon;
			coupon_data.item       = key_hash ? key_hash : item_hash;
			coupon_data.price      = 0;
			coupon_data.stat_value = item_data->stat_value;

			src->set_return_value<BOOL>(NETSHOPPING::NET_GAMESERVER_BASKET_ADD_ITEM(reinterpret_cast<Any*>(&coupon_data), quantity));
			return;
		}

		src->set_return_value<BOOL>(std::move(item_added));
	}

	inline void USE_FAKE_MP_CASH(rage::scrNativeCallContext* src)
	{
		if (g_enabled)
			return;

		HUD::USE_FAKE_MP_CASH(src->get_arg<BOOL>(0));
	}

	inline void CHANGE_FAKE_MP_CASH(rage::scrNativeCallContext* src)
	{
		if (g_enabled)
			return;

		HUD::CHANGE_FAKE_MP_CASH(src->get_arg<int>(0), src->get_arg<int>(1));
	}
}
