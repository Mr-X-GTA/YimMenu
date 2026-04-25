#include "gui.hpp"

#include "lua/lua_manager.hpp"
#include "natives.hpp"
#include "renderer/renderer.hpp"
#include "script.hpp"
#include "views/view.hpp"

#include <imgui.h>

namespace big
{
	/**
	 * @brief The later an entry comes in this enum to higher up it comes in the z-index.
	 */
	enum eRenderPriority
	{
		// low priority
		ESP,
		CONTEXT_MENU,

		// medium priority
		MENU = 0x1000,
		VEHICLE_CONTROL,
		LUA,

		// high priority
		INFO_OVERLAY = 0x2000,
		CMD_EXECUTOR,

		GTA_DATA_CACHE = 0x3000,
		ONBOARDING,

		// should remain in a league of its own
		NOTIFICATIONS = 0x4000,
	};

	gui::gui() :
	    m_is_open(false),
	    m_override_mouse(false)
	{
		g_renderer.add_dx_callback(view::notifications, eRenderPriority::NOTIFICATIONS);
		g_renderer.add_dx_callback(view::onboarding, eRenderPriority::ONBOARDING);
		g_renderer.add_dx_callback(view::gta_data, eRenderPriority::GTA_DATA_CACHE);
		g_renderer.add_dx_callback(view::cmd_executor, eRenderPriority::CMD_EXECUTOR);
		g_renderer.add_dx_callback(view::overlay, eRenderPriority::INFO_OVERLAY);

		g_renderer.add_dx_callback(view::vehicle_control, eRenderPriority::VEHICLE_CONTROL);
		g_renderer.add_dx_callback(esp::draw, eRenderPriority::ESP); // TODO: move to ESP service
		g_renderer.add_dx_callback(view::context_menu, eRenderPriority::CONTEXT_MENU);

		g_renderer.add_dx_callback(
		    [this] {
			    dx_on_tick();
		    },
		    eRenderPriority::MENU);

		g_renderer.add_dx_callback(
		    [] {
			    g_lua_manager->draw_always_draw_gui();
		    },
		    eRenderPriority::LUA);

		g_renderer.add_wndproc_callback([](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
			g_lua_manager->trigger_event<menu_event::Wndproc>(hwnd, msg, wparam, lparam);
		});
		g_renderer.add_wndproc_callback([this](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
			wndproc(hwnd, msg, wparam, lparam);
		});
		g_renderer.add_wndproc_callback([](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
			if (g.cmd_executor.enabled && msg == WM_KEYUP && wparam == VK_ESCAPE)
			{
				g.cmd_executor.enabled = false;
			}
		});


		dx_init();

		g_gui = this;
		g_renderer.rescale(g.window.gui_scale);
	}

	gui::~gui()
	{
		g_gui = nullptr;
	}

	bool gui::is_open()
	{
		return m_is_open;
	}

	void gui::toggle(bool toggle)
	{
		m_is_open = toggle;

		toggle_mouse();
	}

	void gui::override_mouse(bool override)
	{
		m_override_mouse = override;

		toggle_mouse();
	}

	void gui::dx_init()
	{
		static auto bgColor     = ImVec4(0.09f, 0.094f, 0.129f, .9f);
		static auto primary     = ImVec4(0.172f, 0.380f, 0.909f, 1.f);
		static auto secondary   = ImVec4(0.443f, 0.654f, 0.819f, 1.f);
		static auto whiteBroken = ImVec4(0.792f, 0.784f, 0.827f, 1.f);

		auto& style             = ImGui::GetStyle();
		style.WindowPadding     = ImVec2(15, 15);
		style.WindowRounding    = 10.f;
		style.WindowBorderSize  = 0.f;
		style.FramePadding      = ImVec2(5, 5);
		style.FrameRounding     = 4.0f;
		style.ItemSpacing       = ImVec2(12, 8);
		style.ItemInnerSpacing  = ImVec2(8, 6);
		style.IndentSpacing     = 25.0f;
		style.ScrollbarSize     = 15.0f;
		style.ScrollbarRounding = 9.0f;
		style.GrabMinSize       = 5.0f;
		style.GrabRounding      = 3.0f;
		style.ChildRounding     = 4.0f;

		auto& colors                          = style.Colors;
		colors[ImGuiCol_Text]                 = ImGui::ColorConvertU32ToFloat4(g.window.text_color);
		colors[ImGuiCol_TextDisabled]         = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_WindowBg]             = ImGui::ColorConvertU32ToFloat4(g.window.background_color);
		colors[ImGuiCol_ChildBg]              = ImGui::ColorConvertU32ToFloat4(g.window.background_color);
		colors[ImGuiCol_PopupBg]              = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		colors[ImGuiCol_Border]               = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
		colors[ImGuiCol_BorderShadow]         = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		colors[ImGuiCol_FrameBg]              = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_FrameBgActive]        = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_TitleBg]              = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
		colors[ImGuiCol_TitleBgActive]        = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		colors[ImGuiCol_MenuBarBg]            = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_CheckMark]            = ImVec4(1.00f, 0.98f, 0.95f, 0.61f);
		colors[ImGuiCol_SliderGrab]           = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_Button]               = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_ButtonHovered]        = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		colors[ImGuiCol_ButtonActive]         = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_Header]               = ImVec4(0.30f, 0.29f, 0.32f, 1.00f);
		colors[ImGuiCol_HeaderHovered]        = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_HeaderActive]         = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_ResizeGrip]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_ResizeGripHovered]    = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		colors[ImGuiCol_ResizeGripActive]     = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		colors[ImGuiCol_PlotLines]            = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		colors[ImGuiCol_PlotLinesHovered]     = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogram]        = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);

		save_default_style();
	}

	void gui::dx_on_tick()
	{
		if (m_is_open)
		{
			push_theme_colors();
			view::root(); // frame bg
			pop_theme_colors();
		}
	}

	void gui::save_default_style()
	{
		memcpy(&m_default_config, &ImGui::GetStyle(), sizeof(ImGuiStyle));
	}

	void gui::restore_default_style()
	{
		memcpy(&ImGui::GetStyle(), &m_default_config, sizeof(ImGuiStyle));
	}

	void gui::push_theme_colors()
	{
		auto button_color = ImGui::ColorConvertU32ToFloat4(g.window.button_color);
		auto button_active_color =
		    ImVec4(button_color.x + 0.33f, button_color.y + 0.33f, button_color.z + 0.33f, button_color.w);
		auto button_hovered_color =
		    ImVec4(button_color.x + 0.15f, button_color.y + 0.15f, button_color.z + 0.15f, button_color.w);
		auto frame_color = ImGui::ColorConvertU32ToFloat4(g.window.frame_color);
		auto frame_hovered_color =
		    ImVec4(frame_color.x + 0.14f, frame_color.y + 0.14f, frame_color.z + 0.14f, button_color.w);
		auto frame_active_color =
		    ImVec4(frame_color.x + 0.30f, frame_color.y + 0.30f, frame_color.z + 0.30f, button_color.w);

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::ColorConvertU32ToFloat4(g.window.background_color));
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(g.window.text_color));
		ImGui::PushStyleColor(ImGuiCol_Button, button_color);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_hovered_color);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_active_color);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_color);
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, frame_hovered_color);
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, frame_active_color);
	}

	void gui::pop_theme_colors()
	{
		ImGui::PopStyleColor(8);
	}

	void gui::script_on_tick()
	{
		if (g_gui->m_is_open || g_gui->m_override_mouse)
		{
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_NEXT_CAMERA), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_LOOK_LR), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_LOOK_UD), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_LOOK_UP_ONLY), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_LOOK_DOWN_ONLY), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_LOOK_LEFT_ONLY), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_LOOK_RIGHT_ONLY), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_WEAPON_WHEEL_NEXT), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_WEAPON_WHEEL_PREV), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_SELECT_NEXT_WEAPON), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_SELECT_PREV_WEAPON), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_ATTACK), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_VEH_ATTACK), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_VEH_ATTACK2), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_VEH_PREV_RADIO_TRACK), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_VEH_RADIO_WHEEL), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_VEH_PASSENGER_ATTACK), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_VEH_SELECT_NEXT_WEAPON), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_VEH_SELECT_PREV_WEAPON), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_VEH_MOUSE_CONTROL_OVERRIDE), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_VEH_FLY_ATTACK), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_VEH_FLY_SELECT_NEXT_WEAPON), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_VEH_FLY_ATTACK_CAMERA), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_MELEE_ATTACK_ALTERNATE), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_CURSOR_SCROLL_UP), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_ATTACK2), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_PREV_WEAPON), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_NEXT_WEAPON), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_VEH_DRIVE_LOOK), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_VEH_DRIVE_LOOK2), true);
			PAD::DISABLE_CONTROL_ACTION(2, static_cast<int>(ControllerInputs::INPUT_VEH_FLY_ATTACK2), true);
		}
	}

	void gui::script_func()
	{
		while (true)
		{
			g_gui->script_on_tick();
			script::get_current()->yield();
		}
	}

	void gui::wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (msg == WM_KEYUP && wparam == g.settings.hotkeys.menu_toggle)
		{
			//Persist and restore the cursor position between menu instances.
			static POINT cursor_coords{};
			if (g_gui->m_is_open)
			{
				GetCursorPos(&cursor_coords);
			}
			else if (cursor_coords.x + cursor_coords.y != 0)
			{
				SetCursorPos(cursor_coords.x, cursor_coords.y);
			}

			toggle(g.settings.hotkeys.editing_menu_toggle || !m_is_open);
			if (g.settings.hotkeys.editing_menu_toggle)
				g.settings.hotkeys.editing_menu_toggle = false;
		}
	}

	void gui::toggle_mouse()
	{
		if (m_is_open || g_gui->m_override_mouse)
		{
			ImGui::GetIO().MouseDrawCursor = true;
			ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
		}
		else
		{
			ImGui::GetIO().MouseDrawCursor = false;
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
		}
	}
}
