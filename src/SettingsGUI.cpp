#include "BakkesmodPluginTemplate/IMGUI/imgui.h"
#include "pch.h"
#include "CustomCar.hpp"
#include "Cvars.hpp"
#include "ModUtils/gui/GuiTools.hpp"
#include "util/Macros.hpp"

#include "components/CustomCars.hpp"
#include "components/CustomToppers.hpp"
#include "components/GameCars.hpp"

void CustomCar::RenderSettings() {
	const float contentHeight = ImGui::GetContentRegionAvail().y - FOOTER_HEIGHT;
	{
		GUI::ScopedChild c{"MainSettingsSection", ImVec2(0, contentHeight)};

		GUI::alt_settings_header(h_label.c_str(), plugin_version_display, gameWrapper);

		auto enabled_cvar = getCvar(Cvars::enabled);

		GUI::Spacing(4);

		bool enabled = enabled_cvar.getBoolValue();
		if (ImGui::Checkbox("Enable", &enabled))
			enabled_cvar.setValue(enabled);

		GUI::Spacing(4);

		std::string openMenuCommand = "togglemenu " + GetMenuName();
		if (ImGui::Button("Open Menu")) {
			GAME_THREAD_EXECUTE({ cvarManager->executeCommand(openMenuCommand); }, openMenuCommand);
		}

		GUI::Spacing(4);

		ImGui::TextUnformatted("Or bind this command:");

		GUI::SameLineSpacing_relative(20.0f);

		ImGui::SetNextItemWidth(180.0f);
		ImGui::InputText("", &openMenuCommand, ImGuiInputTextFlags_ReadOnly);

		GUI::CopyButton("Copy", openMenuCommand.c_str());
	}

	GUI::alt_settings_footer("Need help? Join the Discord", "https://discord.gg/d5ahhQmJbJ");
}

void CustomCar::RenderWindow() {
	ImGui::BeginTabBar("##Tabs");

	if (ImGui::BeginTabItem("Custom Cars")) {
		CustomCars.display_settings();
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Custom Toppers")) {
		CustomToppers.display_settings();
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("RL Cars")) {
		GameCars.display_settings();
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Info")) {
		info_tab();
		ImGui::EndTabItem();
	}
	ImGui::EndTabBar();
}

void CustomCar::info_tab() {
	static constexpr auto COOKEDPCCONSOLE_PATH = "C:\\Program Files\\Epic Games\\rocketleague\\TAGame\\CookedPCConsole";
	static constexpr auto CUSTOMCARS_PATH =
	    "C:\\Program Files\\Epic Games\\rocketleague\\TAGame\\CookedPCConsole\\mods\\CustomCars\\MyCustomCar.upk";

	GUI::Spacing(2);

	GUI::ColoredTextFormat("For each custom car (or topper), there will be a {} file and a matching {} file.",
	    GUI::WordColor{".json", GUI::Colors::LighterBlue},
	    GUI::WordColor{".upk", GUI::Colors::LighterBlue});

	ImGui::TextColored(
	    GUI::Colors::Yellow, "\nThe .upk file MUST be placed inside the CookedPCConsole folder of your Rocket League installation!\n\n");

	GUI::ColoredTextFormat("On Epic, it will be somewhere like this: {}", GUI::WordColor{COOKEDPCCONSOLE_PATH, GUI::Colors::LightGreen});
	ImGui::TextUnformatted("On Steam, it will be somewhere else (you must figure it out... I don't have Steam)\n\n");

	ImGui::TextUnformatted(
	    "It's also recommended to put your .upk files in a subfolder so they don't get lost among the other files in CookedPCConsole.");

	GUI::ColoredTextFormat("e.g. {}", GUI::WordColor{CUSTOMCARS_PATH, GUI::Colors::LightGreen});
}
