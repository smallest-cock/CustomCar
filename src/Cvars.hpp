#pragma once

#define CVAR(name, desc) CvarData("customcar_" name, desc) // automatically apply the prefix to cvar names

struct CvarData {
	const char *name;
	const char *description;

	constexpr CvarData(const char *name, const char *description) : name(name), description(description) {}
};

namespace Cvars {
	// bools
	constexpr CvarData enabled        = CVAR("enabled", "Enable/disable custom car");
	constexpr CvarData useSpawnedCars = CVAR("use_spawned_cars", "Use spawned RL cars in matches");

	// numbers

	// strings

	// colors
}

namespace Commands {
	constexpr CvarData test = CVAR("test", "test");
}
