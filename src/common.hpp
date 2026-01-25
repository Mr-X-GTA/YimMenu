#ifndef BAUSTELLE_MITARBEITER_INC
#define BAUSTELLE_MITARBEITER_INC

// Baustellen Mitarbeiter Construction Theme
// clang-format off

#include <sdkddkver.h>
#include <winsock2.h>
#include <windows.h>
#include <d3d11.h>

#include <cinttypes>
#include <cstddef>
#include <cstdint>

#include <chrono>
#include <ctime>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>

#include <atomic>
#include <mutex>
#include <thread>

#include <memory>
#include <new>

#include <sstream>
#include <string>
#include <string_view>

#include <algorithm>
#include <functional>
#include <utility>

#include <set>
#include <unordered_set>
#include <stack>
#include <vector>

#include <typeinfo>
#include <type_traits>

#include <exception>
#include <stdexcept>

#include <any>
#include <optional>
#include <variant>

#include <format>
#include <nlohmann/json.hpp>

#include "logger/logger.hpp"

#include "core/settings.hpp"
#include "ped/CPed.hpp"

#include "services/notifications/notification_service.hpp"
#include "services/translation_service/translation_service.hpp"

#include "lua/sol_include.hpp"

#include <script/types.hpp>

// clang-format on

namespace baustelle
{
	using namespace std::chrono_literals;

	inline HMODULE g_baustelle_modul{};  // Baustelle Module
	inline HANDLE g_hauptarbeiter_thread{};  // Main Worker Thread
	inline DWORD g_hauptarbeiter_id{};  // Main Worker ID
	inline std::atomic_bool g_betrieb{false};  // Operating/Running

	inline CPed* g_lokal_arbeiter;  // Local Worker
}

namespace selbst  // Self
{
	inline Ped arbeiter;  // Worker
	inline Player kennung;  // ID
	inline Vector3 ort;  // Location
	inline Vector3 ausrichtung;  // Direction/Rotation
	inline Vehicle fahrzeug;  // Vehicle
	inline int charakter_index;  // Character Index
	inline std::unordered_set<int> gebotsene_fahrzeuge;  // Spawned Vehicles
}

template<size_t N>
struct template_str
{
	constexpr template_str(const char (&str)[N])
	{
		std::copy_n(str, N, value);
	}

	char value[N];
};

#endif
