#include "native_hooks.hpp"

#include "all_scripts.hpp"
#include "am_launcher.hpp"
#include "am_pi_menu.hpp"
#include "creator.hpp"
#include "freemode.hpp"
#include "network_session_host.hpp"
#include "shop_controller.hpp"
#include "tunables.hpp"

#include <script/scrProgram.hpp>

namespace big
{
	native_hook::native_hook(rage::scrProgram* program, const std::unordered_map<NativeIndex, rage::scrNativeHandler>& native_replacements)
	{
		hook_instance(program, native_replacements);
	}

	native_hook::~native_hook()
	{
		if (m_handler_hook)
		{
			m_handler_hook->disable();
			m_handler_hook.reset();
		}

		if (m_vmt_hook)
		{
			m_vmt_hook->disable();
			m_vmt_hook.reset();
		}
	}

	void native_hook::hook_instance(rage::scrProgram* program, const std::unordered_map<NativeIndex, rage::scrNativeHandler>& native_replacements)
	{
		m_program  = program;
		m_vmt_hook = std::make_unique<vmt_hook>(m_program, 9);
		m_vmt_hook->hook(6, &scrprogram_dtor);
		m_vmt_hook->enable();

		m_handler_hook = std::make_unique<vmt_hook>(&m_program->m_native_entrypoints, m_program->m_native_count);
		m_handler_hook->enable();

		std::unordered_map<rage::scrNativeHandler, rage::scrNativeHandler> handler_replacements;

		for (auto& [replacement_index, replacement_handler] : native_replacements)
		{
			auto og_handler                  = native_invoker::get_handlers()[static_cast<int>(replacement_index)];
			handler_replacements[og_handler] = replacement_handler;
		}

		for (int i = 0; i < m_program->m_native_count; i++)
		{
			if (auto it = handler_replacements.find((rage::scrNativeHandler)program->m_native_entrypoints[i]);
			    it != handler_replacements.end())
			{
				m_handler_hook->hook(i, it->second);
			}
		}
	}

	void native_hook::scrprogram_dtor(rage::scrProgram* this_, char free_memory)
	{
		if (auto it = g_native_hooks->m_native_hooks.find(this_); it != g_native_hooks->m_native_hooks.end())
		{
			auto og_func = it->second->m_vmt_hook->get_original<decltype(&native_hook::scrprogram_dtor)>(6);
			it->second->m_vmt_hook->disable();
			it->second->m_vmt_hook.reset();
			it->second->m_handler_hook->disable();
			it->second->m_handler_hook.reset();
			g_native_hooks->m_native_hooks.erase(it);
			og_func(this_, free_memory);
		}
		else
		{
			LOG(FATAL) << "Cannot find hook for program";
		}
	}

	constexpr auto ALL_SCRIPT_HASH = "ALL_SCRIPTS"_J;

	native_hooks::native_hooks()
	{
		add_native_detour(NativeIndex::IS_DLC_PRESENT, all_scripts::IS_DLC_PRESENT);
		add_native_detour(NativeIndex::NETWORK_SET_THIS_SCRIPT_IS_NETWORK_SCRIPT, all_scripts::NETWORK_SET_THIS_SCRIPT_IS_NETWORK_SCRIPT);
		add_native_detour(NativeIndex::NETWORK_TRY_TO_SET_THIS_SCRIPT_IS_NETWORK_SCRIPT, all_scripts::NETWORK_TRY_TO_SET_THIS_SCRIPT_IS_NETWORK_SCRIPT);
		add_native_detour(NativeIndex::SET_CURRENT_PED_WEAPON, all_scripts::SET_CURRENT_PED_WEAPON);
		add_native_detour(NativeIndex::DISABLE_CONTROL_ACTION, all_scripts::DISABLE_CONTROL_ACTION);
		add_native_detour(NativeIndex::HUD_FORCE_WEAPON_WHEEL, all_scripts::HUD_FORCE_WEAPON_WHEEL);
		add_native_detour(NativeIndex::NETWORK_CASINO_CAN_BET, all_scripts::RETURN_TRUE); // bypass casino country restrictions
		add_native_detour(NativeIndex::SC_PROFANITY_GET_STRING_STATUS, all_scripts::RETURN_FALSE); // bypass SC profanity checks
		add_native_detour(NativeIndex::NETWORK_OVERRIDE_CLOCK_TIME, all_scripts::NETWORK_OVERRIDE_CLOCK_TIME);
		add_native_detour(NativeIndex::SET_ENTITY_HEALTH, all_scripts::SET_ENTITY_HEALTH);
		add_native_detour(NativeIndex::APPLY_DAMAGE_TO_PED, all_scripts::APPLY_DAMAGE_TO_PED);
		add_native_detour(NativeIndex::REGISTER_SCRIPT_VARIABLE, all_scripts::DO_NOTHING);
		add_native_detour(NativeIndex::UNREGISTER_SCRIPT_VARIABLE, all_scripts::DO_NOTHING);
		add_native_detour(NativeIndex::FORCE_CHECK_SCRIPT_VARIABLES, all_scripts::DO_NOTHING);
		add_native_detour(NativeIndex::NETWORK_CONCEAL_PLAYER, all_scripts::NETWORK_CONCEAL_PLAYER);
		add_native_detour(NativeIndex::_GET_BATTLEYE_INIT_STATE, all_scripts::RETURN_FALSE); 

		add_native_detour("shop_controller"_J, NativeIndex::IS_PED_SHOOTING, all_scripts::RETURN_FALSE); // prevent exploit reports
		add_native_detour("shop_controller"_J, NativeIndex::SET_WARNING_MESSAGE_WITH_HEADER, shop_controller::SET_WARNING_MESSAGE_WITH_HEADER);
		add_native_detour("shop_controller"_J, NativeIndex::SCALEFORM_MOVIE_METHOD_ADD_PARAM_INT, shop_controller::SCALEFORM_MOVIE_METHOD_ADD_PARAM_INT);
		add_native_detour(NativeIndex::NET_GAMESERVER_GET_PRICE, shop_controller::NET_GAMESERVER_GET_PRICE);
		add_native_detour(NativeIndex::NET_GAMESERVER_CATALOG_ITEM_IS_VALID, shop_controller::NET_GAMESERVER_CATALOG_ITEM_IS_VALID);
		add_native_detour(NativeIndex::NET_GAMESERVER_CATALOG_ITEM_KEY_IS_VALID, shop_controller::NET_GAMESERVER_CATALOG_ITEM_KEY_IS_VALID);
		add_native_detour(NativeIndex::NET_GAMESERVER_CATALOG_IS_VALID, shop_controller::NET_GAMESERVER_CATALOG_IS_VALID);
		add_native_detour(NativeIndex::NET_GAMESERVER_BEGIN_SERVICE, shop_controller::NET_GAMESERVER_BEGIN_SERVICE);
		add_native_detour(NativeIndex::NET_GAMESERVER_USE_SERVER_TRANSACTIONS, shop_controller::NET_GAMESERVER_USE_SERVER_TRANSACTIONS);
		add_native_detour(NativeIndex::NET_GAMESERVER_GET_CATALOG_CLOUD_CRC, shop_controller::NET_GAMESERVER_GET_CATALOG_CLOUD_CRC);
		add_native_detour(NativeIndex::NET_GAMESERVER_RETRIEVE_INIT_SESSION_STATUS, shop_controller::NET_GAMESERVER_RETRIEVE_INIT_SESSION_STATUS);
		add_native_detour(NativeIndex::NET_GAMESERVER_START_SESSION, shop_controller::NET_GAMESERVER_START_SESSION);
		add_native_detour(NativeIndex::NET_GAMESERVER_START_SESSION_PENDING, shop_controller::NET_GAMESERVER_START_SESSION_PENDING);
		add_native_detour(NativeIndex::NET_GAMESERVER_RETRIEVE_START_SESSION_STATUS, shop_controller::NET_GAMESERVER_RETRIEVE_START_SESSION_STATUS);
		add_native_detour(NativeIndex::NET_GAMESERVER_IS_SESSION_REFRESH_PENDING, shop_controller::NET_GAMESERVER_IS_SESSION_REFRESH_PENDING);
		add_native_detour(NativeIndex::NET_GAMESERVER_TRANSACTION_IN_PROGRESS, shop_controller::NET_GAMESERVER_TRANSACTION_IN_PROGRESS);
		add_native_detour(NativeIndex::NET_GAMESERVER_GET_SESSION_STATE_AND_STATUS, shop_controller::NET_GAMESERVER_GET_SESSION_STATE_AND_STATUS);
		add_native_detour(NativeIndex::NET_GAMESERVER_IS_SESSION_VALID, shop_controller::NET_GAMESERVER_IS_SESSION_VALID);
		add_native_detour(NativeIndex::NET_GAMESERVER_RETRIEVE_SESSION_ERROR_CODE, shop_controller::NET_GAMESERVER_RETRIEVE_SESSION_ERROR_CODE);
		add_native_detour(NativeIndex::NET_GAMESERVER_IS_CATALOG_CURRENT, shop_controller::NET_GAMESERVER_IS_CATALOG_CURRENT);
		add_native_detour(NativeIndex::NET_GAMESERVER_RETRIEVE_CATALOG_REFRESH_STATUS, shop_controller::NET_GAMESERVER_RETRIEVE_CATALOG_REFRESH_STATUS);
		add_native_detour(NativeIndex::NET_GAMESERVER_CHECKOUT_START, shop_controller::NET_GAMESERVER_CHECKOUT_START);
		add_native_detour(NativeIndex::NET_GAMESERVER_BASKET_START, shop_controller::NET_GAMESERVER_BASKET_START);
		add_native_detour(NativeIndex::NET_GAMESERVER_BASKET_ADD_ITEM, shop_controller::NET_GAMESERVER_BASKET_ADD_ITEM);
		add_native_detour(NativeIndex::NET_GAMESERVER_BASKET_END, shop_controller::NET_GAMESERVER_BASKET_END);
		add_native_detour(NativeIndex::NETWORK_BUY_ITEM, shop_controller::NETWORK_BUY_ITEM);
		add_native_detour(NativeIndex::NETWORK_BUY_PROPERTY, shop_controller::NETWORK_BUY_PROPERTY);
		add_native_detour(NativeIndex::NETWORK_DEDUCT_CASH, shop_controller::NETWORK_DEDUCT_CASH);
		add_native_detour(NativeIndex::NETWORK_BUY_HEALTHCARE, shop_controller::NETWORK_BUY_HEALTHCARE);
		add_native_detour(NativeIndex::NETWORK_BUY_AIRSTRIKE, shop_controller::NETWORK_BUY_AIRSTRIKE);
		add_native_detour(NativeIndex::NETWORK_BUY_HELI_STRIKE, shop_controller::NETWORK_BUY_HELI_STRIKE);
		add_native_detour(NativeIndex::NETWORK_BUY_BOUNTY, shop_controller::NETWORK_BUY_BOUNTY);
		add_native_detour(NativeIndex::NETWORK_BUY_FAIRGROUND_RIDE, shop_controller::NETWORK_BUY_FAIRGROUND_RIDE);
		add_native_detour(NativeIndex::NETWORK_CAN_SPEND_MONEY, shop_controller::NETWORK_CAN_SPEND_MONEY);
		add_native_detour(NativeIndex::NETWORK_CAN_SPEND_MONEY2, shop_controller::NETWORK_CAN_SPEND_MONEY2);
		add_native_detour(NativeIndex::NETWORK_SPENT_MOVE_YACHT, shop_controller::NETWORK_SPENT_MOVE_YACHT);
		add_native_detour(NativeIndex::NETWORK_SPENT_HANGAR_UTILITY_CHARGES, shop_controller::NETWORK_SPENT_HANGAR_UTILITY_CHARGES);
		add_native_detour(NativeIndex::NETWORK_SPENT_HANGAR_STAFF_CHARGES, shop_controller::NETWORK_SPENT_HANGAR_STAFF_CHARGES);
		add_native_detour(NativeIndex::NETWORK_GET_VC_BANK_BALANCE, shop_controller::NETWORK_GET_VC_BANK_BALANCE);
		add_native_detour(NativeIndex::NETWORK_GET_VC_BALANCE, shop_controller::NETWORK_GET_VC_BALANCE);
		add_native_detour(NativeIndex::NETWORK_GET_VC_WALLET_BALANCE, shop_controller::NETWORK_GET_VC_WALLET_BALANCE);
		add_native_detour(NativeIndex::NETWORK_GET_EVC_BALANCE, shop_controller::NETWORK_GET_EVC_BALANCE);
		add_native_detour(NativeIndex::NETWORK_GET_PVC_BALANCE, shop_controller::NETWORK_GET_PVC_BALANCE);
		add_native_detour(NativeIndex::NETWORK_GET_STRING_WALLET_BALANCE, shop_controller::NETWORK_GET_STRING_WALLET_BALANCE);
		add_native_detour(NativeIndex::NETWORK_GET_STRING_BANK_BALANCE, shop_controller::NETWORK_GET_STRING_BANK_BALANCE);
		add_native_detour(NativeIndex::NETWORK_GET_STRING_BANK_WALLET_BALANCE, shop_controller::NETWORK_GET_STRING_BANK_WALLET_BALANCE);
		add_native_detour(NativeIndex::NETWORK_CASINO_BUY_CHIPS, shop_controller::NETWORK_CASINO_BUY_CHIPS);
		add_native_detour(NativeIndex::NETWORK_SPENT_UPGRADE_OFFICE_PROPERTY, shop_controller::NETWORK_SPENT_UPGRADE_OFFICE_PROPERTY);
		add_native_detour(NativeIndex::NETWORK_SPENT_PURCHASE_OFFICE_PROPERTY, shop_controller::NETWORK_SPENT_PURCHASE_OFFICE_PROPERTY);
		add_native_detour(NativeIndex::NETWORK_SPENT_UPGRADE_WAREHOUSE_PROPERTY, shop_controller::NETWORK_SPENT_UPGRADE_WAREHOUSE_PROPERTY);
		add_native_detour(NativeIndex::NETWORK_SPENT_PURCHASE_WAREHOUSE_PROPERTY, shop_controller::NETWORK_SPENT_PURCHASE_WAREHOUSE_PROPERTY);
		add_native_detour(NativeIndex::NETWORK_SPENT_UPGRADE_IMPEXP_WAREHOUSE_PROPERTY, shop_controller::NETWORK_SPENT_UPGRADE_IMPEXP_WAREHOUSE_PROPERTY);
		add_native_detour(NativeIndex::NETWORK_SPENT_PURCHASE_IMPEXP_WAREHOUSE_PROPERTY, shop_controller::NETWORK_SPENT_PURCHASE_IMPEXP_WAREHOUSE_PROPERTY);
		add_native_detour(NativeIndex::NETWORK_SPENT_PURCHASE_CLUB_HOUSE, shop_controller::NETWORK_SPENT_PURCHASE_CLUB_HOUSE);
		add_native_detour(NativeIndex::NETWORK_SPENT_UPGRADE_CLUB_HOUSE, shop_controller::NETWORK_SPENT_UPGRADE_CLUB_HOUSE);
		add_native_detour(NativeIndex::NETWORK_SPENT_PURCHASE_BUSINESS_PROPERTY, shop_controller::NETWORK_SPENT_PURCHASE_BUSINESS_PROPERTY);
		add_native_detour(NativeIndex::NETWORK_SPENT_UPGRADE_BUSINESS_PROPERTY, shop_controller::NETWORK_SPENT_UPGRADE_BUSINESS_PROPERTY);
		add_native_detour(NativeIndex::NETWORK_SPENT_UPGRADE_OFFICE_GARAGE, shop_controller::NETWORK_SPENT_UPGRADE_OFFICE_GARAGE);
		add_native_detour(NativeIndex::NETWORK_SPENT_PURCHASE_OFFICE_GARAGE, shop_controller::NETWORK_SPENT_PURCHASE_OFFICE_GARAGE);
		add_native_detour(NativeIndex::NETWORK_SPENT_UPGRADE_HANGAR, shop_controller::NETWORK_SPENT_UPGRADE_HANGAR);
		add_native_detour(NativeIndex::NETWORK_SPENT_PURCHASE_HANGAR, shop_controller::NETWORK_SPENT_PURCHASE_HANGAR);
		add_native_detour(NativeIndex::NETWORK_SPENT_UPGRADE_TRUCK, shop_controller::NETWORK_SPENT_UPGRADE_TRUCK);
		add_native_detour(NativeIndex::NETWORK_SPENT_BUY_TRUCK, shop_controller::NETWORK_SPENT_BUY_TRUCK);
		add_native_detour(NativeIndex::NETWORK_SPENT_UPRADE_BUNKER, shop_controller::NETWORK_SPENT_UPRADE_BUNKER);
		add_native_detour(NativeIndex::NETWORK_SPENT_BUY_BUNKER, shop_controller::NETWORK_SPENT_BUY_BUNKER);
		add_native_detour(NativeIndex::NETWORK_SPENT_UPGRADE_BASE, shop_controller::NETWORK_SPENT_UPGRADE_BASE);
		add_native_detour(NativeIndex::NETWORK_SPENT_BUY_BASE, shop_controller::NETWORK_SPENT_BUY_BASE);
		add_native_detour(NativeIndex::NETWORK_SPENT_BUY_TILTROTOR, shop_controller::NETWORK_SPENT_BUY_TILTROTOR);
		add_native_detour(NativeIndex::NETWORK_SPENT_UPGRADE_TILTROTOR, shop_controller::NETWORK_SPENT_UPGRADE_TILTROTOR);
		add_native_detour(NativeIndex::NETWORK_SPENT_UPGRADE_HACKER_TRUCK, shop_controller::NETWORK_SPENT_UPGRADE_HACKER_TRUCK);
		add_native_detour(NativeIndex::NETWORK_SPENT_PURCHASE_HACKER_TRUCK, shop_controller::NETWORK_SPENT_PURCHASE_HACKER_TRUCK);
		add_native_detour(NativeIndex::NETWORK_SPENT_UPGRADE_NIGHTCLUB_AND_WAREHOUSE, shop_controller::NETWORK_SPENT_UPGRADE_NIGHTCLUB_AND_WAREHOUSE);
		add_native_detour(NativeIndex::NETWORK_SPENT_PURCHASE_NIGHTCLUB_AND_WAREHOUSE, shop_controller::NETWORK_SPENT_PURCHASE_NIGHTCLUB_AND_WAREHOUSE);
		add_native_detour(NativeIndex::NETWORK_SPEND_UPGRADE_ARENA, shop_controller::NETWORK_SPEND_UPGRADE_ARENA);
		add_native_detour(NativeIndex::NETWORK_SPEND_BUY_ARENA, shop_controller::NETWORK_SPEND_BUY_ARENA);
		add_native_detour(NativeIndex::NETWORK_SPEND_UPGRADE_CASINO, shop_controller::NETWORK_SPEND_UPGRADE_CASINO);
		add_native_detour(NativeIndex::NETWORK_SPEND_BUY_CASINO, shop_controller::NETWORK_SPEND_BUY_CASINO);
		add_native_detour(NativeIndex::NETWORK_SPEND_UPGRADE_ARCADE, shop_controller::NETWORK_SPEND_UPGRADE_ARCADE);
		add_native_detour(NativeIndex::NETWORK_SPEND_BUY_ARCADE, shop_controller::NETWORK_SPEND_BUY_ARCADE);
		add_native_detour(NativeIndex::NETWORK_SPEND_UPGRADE_SUB, shop_controller::NETWORK_SPEND_UPGRADE_SUB);
		add_native_detour(NativeIndex::NETWORK_SPEND_BUY_SUB, shop_controller::NETWORK_SPEND_BUY_SUB);
		add_native_detour(NativeIndex::NETWORK_SPEND_UPGRADE_AUTOSHOP, shop_controller::NETWORK_SPEND_UPGRADE_AUTOSHOP);
		add_native_detour(NativeIndex::NETWORK_SPEND_BUY_AUTOSHOP, shop_controller::NETWORK_SPEND_BUY_AUTOSHOP);
		add_native_detour(NativeIndex::NETWORK_SPEND_UPGRADE_AGENCY, shop_controller::NETWORK_SPEND_UPGRADE_AGENCY);
		add_native_detour(NativeIndex::NETWORK_SPEND_BUY_AGENCY, shop_controller::NETWORK_SPEND_BUY_AGENCY);
		add_native_detour(NativeIndex::_NETWORK_SPEND_BUY_MFGARAGE, shop_controller::_NETWORK_SPEND_BUY_MFGARAGE);
		add_native_detour(NativeIndex::_NETWORK_SPEND_UPGRADE_MFGARAGE, shop_controller::_NETWORK_SPEND_UPGRADE_MFGARAGE);
		add_native_detour(NativeIndex::_NETWORK_SPEND_BUY_ACID_LAB, shop_controller::_NETWORK_SPEND_BUY_ACID_LAB);
		add_native_detour(NativeIndex::_NETWORK_SPEND_BUY_SUPPLIES, shop_controller::_NETWORK_SPEND_BUY_SUPPLIES);
		add_native_detour(NativeIndex::_NETWORK_SPEND_UPGRADE_ACID_LAB_EQUIPMENT, shop_controller::_NETWORK_SPEND_UPGRADE_ACID_LAB_EQUIPMENT);
		add_native_detour(NativeIndex::_NETWORK_SPEND_UPGRADE_ACID_LAB_ARMOR, shop_controller::_NETWORK_SPEND_UPGRADE_ACID_LAB_ARMOR);
		add_native_detour(NativeIndex::_NETWORK_SPEND_UPGRADE_ACID_LAB_SCOOP, shop_controller::_NETWORK_SPEND_UPGRADE_ACID_LAB_SCOOP);
		add_native_detour(NativeIndex::_NETWORK_SPEND_UPGRADE_ACID_LAB_MINES, shop_controller::_NETWORK_SPEND_UPGRADE_ACID_LAB_MINES);
		add_native_detour(NativeIndex::_NETWORK_SPENT_AIR_FREIGHT, shop_controller::_NETWORK_SPENT_AIR_FREIGHT);
		add_native_detour(NativeIndex::_NETWORK_SPENT_STEALTH_MODULE, shop_controller::_NETWORK_SPENT_STEALTH_MODULE);
		add_native_detour(NativeIndex::_NETWORK_SPENT_MISSILE_JAMMER, shop_controller::_NETWORK_SPENT_MISSILE_JAMMER);
		add_native_detour(NativeIndex::_NETWORK_SPENT_GENERIC, shop_controller::_NETWORK_SPENT_GENERIC);

		add_native_detour("freemode"_J, NativeIndex::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH, freemode::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH);
		add_native_detour("freemode"_J, NativeIndex::STAT_GET_INT, freemode::STAT_GET_INT);
		add_native_detour("freemode"_J, NativeIndex::IS_PLAYER_PLAYING, freemode::IS_PLAYER_PLAYING);
		add_native_detour("freemode"_J, NativeIndex::SET_ENTITY_VISIBLE, freemode::SET_ENTITY_VISIBLE);
		add_native_detour("freemode"_J, NativeIndex::SET_BIGMAP_ACTIVE, freemode::SET_BIGMAP_ACTIVE);
		add_native_detour("freemode"_J, NativeIndex::SET_BLIP_DISPLAY, freemode::SET_BLIP_DISPLAY);
		add_native_detour("freemode"_J, NativeIndex::NETWORK_HAS_RECEIVED_HOST_BROADCAST_DATA, freemode::NETWORK_HAS_RECEIVED_HOST_BROADCAST_DATA);
		add_native_detour("freemode"_J, NativeIndex::REMOVE_WEAPON_FROM_PED, freemode::REMOVE_WEAPON_FROM_PED);

		add_native_detour("fmmc_launcher"_J, NativeIndex::NETWORK_HAS_RECEIVED_HOST_BROADCAST_DATA, freemode::NETWORK_HAS_RECEIVED_HOST_BROADCAST_DATA);
		add_native_detour("maintransition"_J, NativeIndex::NETWORK_SESSION_HOST, network::NETWORK_SESSION_HOST);

		add_native_detour("am_launcher"_J, NativeIndex::START_NEW_SCRIPT_WITH_ARGS, am_launcher::START_NEW_SCRIPT_WITH_ARGS);
		add_native_detour("am_launcher"_J, NativeIndex::NETWORK_HAS_RECEIVED_HOST_BROADCAST_DATA, freemode::NETWORK_HAS_RECEIVED_HOST_BROADCAST_DATA);

		add_native_detour("am_pi_menu"_J, NativeIndex::DISPLAY_ONSCREEN_KEYBOARD, am_pi_menu::DISPLAY_ONSCREEN_KEYBOARD);

		add_native_detour("fm_race_creator"_J, NativeIndex::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH, creator::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH);
		add_native_detour("fm_capture_creator"_J, NativeIndex::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH, creator::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH);
		add_native_detour("fm_deathmatch_creator"_J, NativeIndex::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH, creator::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH);
		add_native_detour("fm_lts_creator"_J, NativeIndex::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH, creator::GET_NUMBER_OF_THREADS_RUNNING_THE_SCRIPT_WITH_THIS_HASH);

		add_native_detour("fm_race_creator"_J, NativeIndex::GET_ENTITY_MODEL, creator::GET_ENTITY_MODEL);
		add_native_detour("fm_capture_creator"_J, NativeIndex::GET_ENTITY_MODEL, creator::GET_ENTITY_MODEL);
		add_native_detour("fm_deathmatch_creator"_J, NativeIndex::GET_ENTITY_MODEL, creator::GET_ENTITY_MODEL);
		add_native_detour("fm_lts_creator"_J, NativeIndex::GET_ENTITY_MODEL, creator::GET_ENTITY_MODEL);

		// Infinite Model Memory
		add_native_detour("fm_race_creator"_J, NativeIndex::GET_USED_CREATOR_BUDGET, creator::GET_USED_CREATOR_BUDGET);
		add_native_detour("fm_capture_creator"_J, NativeIndex::GET_USED_CREATOR_BUDGET, creator::GET_USED_CREATOR_BUDGET);
		add_native_detour("fm_deathmatch_creator"_J, NativeIndex::GET_USED_CREATOR_BUDGET, creator::GET_USED_CREATOR_BUDGET);
		add_native_detour("fm_lts_creator"_J, NativeIndex::GET_USED_CREATOR_BUDGET, creator::GET_USED_CREATOR_BUDGET);
		add_native_detour("fm_survival_creator"_J, NativeIndex::GET_USED_CREATOR_BUDGET, creator::GET_USED_CREATOR_BUDGET);

		add_native_detour("tuneables_processing"_J, NativeIndex::WAIT, tunables::WAIT);
		add_native_detour("tuneables_processing"_J, NativeIndex::_NETWORK_GET_TUNABLES_REGISTRATION_INT, tunables::_NETWORK_GET_TUNABLES_REGISTRATION_INT);
		add_native_detour("tuneables_processing"_J, NativeIndex::_NETWORK_GET_TUNABLES_REGISTRATION_BOOL, tunables::_NETWORK_GET_TUNABLES_REGISTRATION_BOOL);
		add_native_detour("tuneables_processing"_J, NativeIndex::_NETWORK_GET_TUNABLES_REGISTRATION_FLOAT, tunables::_NETWORK_GET_TUNABLES_REGISTRATION_FLOAT);

		// TODO: is this safe?
		add_native_detour("am_mp_hacker_den"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("am_mp_yacht"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("arena_carmod"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("armory_aircraft_carmod"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("base_carmod"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("business_hub_carmod"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("carmod_shop"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("car_meet_carmod"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("clothes_shop_mp"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("clothes_shop_sp"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("fixer_hq_carmod"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("gunclub_shop"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("hacker_den_carmod"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("hacker_truck_carmod"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("hairdo_shop_mp"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("hairdo_shop_sp"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("hangar_carmod"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("juggalo_hideout_carmod"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("personal_carmod_shop"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("tattoo_shop"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("tuner_property_carmod"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.
		add_native_detour("vinewood_premium_garage_carmod"_J, NativeIndex::FORCE_PED_AI_AND_ANIMATION_UPDATE, all_scripts::DO_NOTHING); //Fix jittering weapons.

		for (auto& entry : *g_pointers->m_gta.m_script_program_table)
			if (entry.m_program)
				hook_program(entry.m_program);

		g_native_hooks = this;
	}

	native_hooks::~native_hooks()
	{
		m_native_hooks.clear();
		g_native_hooks = nullptr;
	}

	void native_hooks::add_native_detour(NativeIndex index, rage::scrNativeHandler detour)
	{
		add_native_detour(ALL_SCRIPT_HASH, index, detour);
	}

	void native_hooks::add_native_detour(rage::joaat_t script_hash, NativeIndex index, rage::scrNativeHandler detour)
	{
		if (const auto& it = m_native_registrations.find(script_hash); it != m_native_registrations.end())
		{
			it->second.emplace_back(index, detour);
			return;
		}

		m_native_registrations.emplace(script_hash, std::vector<native_detour>({{index, detour}}));
	}

	void native_hooks::hook_program(rage::scrProgram* program)
	{
		std::unordered_map<NativeIndex, rage::scrNativeHandler> native_replacements;
		const auto script_hash = program->m_name_hash;

		// Functions that need to be detoured for all scripts
		if (const auto& pair = m_native_registrations.find(ALL_SCRIPT_HASH); pair != m_native_registrations.end())
			for (const auto& native_hook_reg : pair->second)
				native_replacements.insert(native_hook_reg);

		// Functions that only need to be detoured for a specific script
		if (const auto& pair = m_native_registrations.find(script_hash); pair != m_native_registrations.end())
			for (const auto& native_hook_reg : pair->second)
				native_replacements.insert(native_hook_reg);

		if (!native_replacements.empty())
		{
			m_native_hooks.emplace(program, std::make_unique<native_hook>(program, native_replacements));
		}
	}
}
