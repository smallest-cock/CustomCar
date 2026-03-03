#include "pch.h"
#include "PluginConfig.hpp"
#include "CustomCar.hpp"
#include "util/logging.hpp"
#include "util/Instances.hpp"
#include "util/HookManager.hpp"
#include "ModUtils/util/Utils.hpp"

#include "components/CustomAssetTypes.hpp"
#include "components/CustomCars.hpp"
#include "components/CustomToppers.hpp"
#include "components/GameCars.hpp"

BAKKESMOD_PLUGIN(CustomCar, "Custom Car", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

BakkesMod::Plugin::BakkesModPlugin *CustomCar::getPlugin() { return this; }

void CustomCar::onLoad() {
	_globalCvarManager = cvarManager;

	if (!Instances.initGlobals())
		return;

	g_hookManager.init(gameWrapper);

	m_pluginFolder = gameWrapper->GetDataFolder() / "CustomCar";

	initCvars();
	initCommands();
	initHooks();

	Format::construct_label({41, 11, 20, 6, 8, 13, 52, 12, 0, 3, 4, 52, 1, 24, 52, 44, 44, 37, 14, 22}, h_label); // o b f u s a c i o n
	PluginUpdates::checkForUpdates(PLUGIN_NAME_NO_SPACES, VERSION_STR);

	ProductData::initProductData();
	CustomCars.init(gameWrapper);
	CustomToppers.init(gameWrapper);
	GameCars.init(gameWrapper);

	g_hookManager.commitHooks();
	LOG("Custom Car loaded :)");
}

void CustomCar::onUnload() {
	CustomCars.onUnload();
	CustomToppers.onUnload();
	LOG("Custom Car unloaded :(");
}

void CustomCar::initCvars() { registerCvar_Bool(Cvars::enabled, true).bindTo(m_enabled); }
void CustomCar::initHooks() {}

void CustomCar::initCommands() {
	registerCommand(Commands::test, [this](std::vector<std::string> args) { LOG("Did the test..."); });
}
