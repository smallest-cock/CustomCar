#include "pch.h"
#include "CustomCar.hpp"
#include "Cvars.hpp"
#include "ModUtils/gui/GuiTools.hpp"
#include "util/Macros.hpp"

#include "components/CustomCars.hpp"
#include "components/CustomToppers.hpp"
#include "components/GameCars.hpp"

void CustomCar::RenderSettings() {
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
	// if (ImGui::BeginTabItem("Info")) {
	// 	info_tab();
	// 	ImGui::EndTabItem();
	// }
	ImGui::EndTabBar();
}

void CustomCar::info_tab() {
	GUI::Spacing(2);

	ImGui::TextUnformatted("Some plugin info ...");
}
