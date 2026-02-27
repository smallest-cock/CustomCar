#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "boilerplate/GuiBase.hpp"
#include "boilerplate/PluginHelperBase.hpp"
#include "version.hpp"
#include <memory>
#include <minwindef.h>

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(
    VERSION_BUILD);

constexpr auto plugin_version_display = "v" VERSION_STR;

class CustomCar : public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase, public PluginWindowBase, public PluginHelperBase {
	void                                onLoad() override;
	void                                onUnload() override;
	BakkesMod::Plugin::BakkesModPlugin *getPlugin() override;

private:
	// init
	void initCvars();
	void initCommands();
	void initHooks();

private:
	std::string h_label;
	fs::path    m_pluginFolder;

	std::shared_ptr<bool> m_enabled = std::make_shared<bool>(true);

	// gui
private:
	static constexpr float FOOTER_HEIGHT = 40.0f;

public:
	void RenderSettings() override;
	void RenderWindow() override;

	void info_tab();
};
