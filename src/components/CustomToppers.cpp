#include "pch.h"
#include "CustomToppers.hpp"
#include "components/CustomAssetTypes.hpp"
#include "Events.hpp"
#include "ModUtils/gui/GuiTools.hpp"
#include "ModUtils/util/Utils.hpp"
#include "util/Instances.hpp"
#include "util/Macros.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>

// ##############################################################################################################
// ###############################################    INIT    ###################################################
// ##############################################################################################################

void CustomToppersComponent::init(const std::shared_ptr<GameWrapper> &gw) {
	gameWrapper = gw;
	initFolders();
	readInCustomAssetData();
	loadPersistantData();
	initHooks();

	LOG("CustomToppers component initialized");
}

void CustomToppersComponent::onUnload() {
	for (auto &[prodId, prodAsset] : m_customTopperProdAssets)
		Instances.markForDestroy(prodAsset);
	LOG("Cleaned up custom UProductAsset_Attachment_TA instances");
}

void CustomToppersComponent::initFolders() {
	auto pluginFolder     = gameWrapper->GetDataFolder() / "CustomCar";
	m_persistanceJsonFile = pluginFolder / "CustomToppers_save.json";
	m_customToppersFolder = pluginFolder / "CustomToppers";

	if (!fs::exists(m_persistanceJsonFile)) {
		std::ofstream file(m_persistanceJsonFile);
		file << "{}" << std::endl;
		file.close();
		LOG("Created empty settings save file: \"{}\"", m_persistanceJsonFile.string());
	}

	if (!fs::exists(m_customToppersFolder)) {
		fs::create_directories(m_customToppersFolder);
		LOG("Created custom toppers folder: \"{}\"", m_customToppersFolder.string());
	}
}

void CustomToppersComponent::readInCustomAssetData() {
	m_topperCustomizations.clear();

	try {
		for (const auto &entry : fs::recursive_directory_iterator(m_customToppersFolder)) {
			if (!entry.is_regular_file() || entry.path().extension() != ".json")
				continue;

			json j = Files::get_json(entry.path());
			if (j.empty())
				continue;

			for (const auto &[key, val] : j.items()) {
				if (!val.is_object())
					continue;
				auto assetData = createTopperDataFromJson(val);
				if (!assetData)
					continue;
				assetData->name = key;
				m_topperCustomizations[assetData->productId].toppers.emplace_back(*assetData);
			}
		}
	} catch (const fs::filesystem_error &e) {
		LOGERROR("Exception occured while reading data from folder \"{}\": {}", m_customToppersFolder.string(), e.what());
	}

	// findDefaultProductData(); // always call this to update the state of m_bodyProducts as well, to keep everything in sync
	Instances.spawnNotification("Custom Car", std::format("Found {} custom toppers", m_topperCustomizations.size()), 3, true);
}

/*
 * Example JSON:
 *
 *  {
 *      "203": "lesbian",
 *      "23": "opium"
 *  }
 */
// NOTE: must be called after readInCustomAssetData() bc it relies on m_topperCustomizations
void CustomToppersComponent::loadPersistantData() {
	json j = Files::get_json(m_persistanceJsonFile);
	if (j.empty())
		return;

	for (const auto &[key, val] : j.items()) {
		if (!val.is_string())
			continue;
		std::string valStr = val.get<std::string>();
		int32_t     bodyId = std::stoi(key);

		auto it = m_topperCustomizations.find(bodyId);
		if (it == m_topperCustomizations.end())
			continue;
		TopperCustomization &custo = it->second;

		for (size_t i = 0; i < custo.toppers.size(); ++i) {
			const auto &customTopper = custo.toppers[i];
			if (customTopper.name == valStr) {
				custo.selectedTopperIdx = i;
				break;
			}
		}
	}
}

void CustomToppersComponent::writePersistantData() {
	json j;

	for (const auto &[prodId, custo] : m_topperCustomizations) {
		if (!custo.selectedTopperIdx)
			continue;
		j[std::to_string(prodId)] = custo.toppers[*custo.selectedTopperIdx].name;
	}

	Files::write_json(m_persistanceJsonFile, j);
	LOG("Saved selected toppers to \"{}\"", m_persistanceJsonFile.string());
}

void CustomToppersComponent::initHooks() {
	hookEventPost(Events::GFxData_StartMenu_TA_ProgressToMainMenu, [this](std::string e) {
		refreshCarModel();
		LOG("Refreshed car model after progressing to main menu");
	});

	// NOTE: this hook works... but we had to manually set materials. Idk if possible to get it to auto-load materials?
	hookWithCaller(Events::ProductLoader_TA_HandleAssetLoaded, [this](ActorWrapper Caller, void *Params, ...) {
		auto *caller = reinterpret_cast<UProductLoader_TA *>(Caller.memory_address);
		if (!caller)
			return;
		if (!caller->Outer) {
			LOGERROR("caller->Outer is null in ProductLoader_TA_HandleAssetLoaded hook");
			return;
		}

		// UProductLoader_TA* as car actor member:
		// - ACarPreviewActor_TA::ProductLoader
		// - ACar_TA::Loadout
		bool isUser = false;
		if (caller->Outer->IsA<ACarPreviewActor_TA>()) {
			auto *car = reinterpret_cast<ACarPreviewActor_TA *>(caller->Outer);
			isUser    = car->ControllerId == 0;

		} else if (caller->Outer->IsA<ACar_TA>()) {
			auto *car = reinterpret_cast<ACar_TA *>(caller->Outer);
			if (validUObject(car->PRI)) {
				isUser = car->PRI->IsLocalPlayerPRI();
			}
		}
		if (!isUser)
			return;

		auto *params = reinterpret_cast<UProductLoader_TA_execHandleAssetLoaded_Params *>(Params);
		if (!params)
			return;

		FAssetLoadResult &loadResult       = params->Result;
		CustomTopperData *customTopperData = getCustomTopperDataFromProdId(loadResult.ProductID);
		if (!customTopperData) {
			DLOG("No custom topper data for product ID {}", loadResult.ProductID);
			return;
		}

		auto *attachmentAsset = UnrealCast<UProductAsset_Attachment_TA>(loadResult.Asset);
		if (!attachmentAsset) {
			DLOGWARNING("UProductAsset_Attachment_TA* casted from loadResult.Asset is null. It prolly aint a UProductAsset_Attachment_TA");
			return;
		}

		UProductAsset_Attachment_TA *customTopperAsset = nullptr;
		auto                         it                = m_customTopperProdAssets.find(loadResult.ProductID);
		if (it != m_customTopperProdAssets.end()) {
			customTopperAsset = it->second;
		} else if (validUObject(attachmentAsset)) {
			UObject *topperAssetDupResult = attachmentAsset->DuplicateObject(attachmentAsset, nullptr, nullptr);
			customTopperAsset             = UnrealCast<UProductAsset_Attachment_TA>(topperAssetDupResult);
			if (customTopperAsset) {
				Instances.markInvincible(customTopperAsset);
				m_customTopperProdAssets[loadResult.ProductID] = customTopperAsset;
				DLOG("Marked duped UProductAsset_Attachment_TA instance as invincible and cached it: {}", customTopperAsset);
			}
		}
		if (!validUObject(customTopperAsset)) {
			LOGERROR("Custom topper asset is null... UObject duplication might have failed");
			return;
		}

		// TODO: maybe add a bool and check for whether the custom asset is a SK or SM
		auto *customMesh = Instances.loadObject<UStaticMesh>(customTopperData->assetPath);
		if (!customMesh) {
			LOGERROR("UStaticMesh* is null after attempting to load asset: \"{}\"", customTopperData->assetPath);
			return;
		}
		FProductAttachment &att = customTopperAsset->Attachments[0];
		att.StaticMesh          = customMesh;
		LOG("Set custom static mesh for {}", loadResult.AssetName.ToString());

		auto *customMat = Instances.loadObject<UMaterialInstanceConstant>(customTopperData->materialPath);
		if (!customMat) {
			LOGERROR("UMaterialInstanceConstant* is null from Instances.loadObject");
			return;
		}
		att.Material = customMat;
		LOG("Set custom material for {}", loadResult.AssetName.ToString());

		if (customTopperData->scale) {
			att.Scale = *customTopperData->scale;
			LOG("Set att.Scale to {}", *customTopperData->scale);
		}

		loadResult.Asset = customTopperAsset;
		LOG("Set loadResult.Asset to be our custom (duplicated) UProductAsset_Attachment_TA");
	});
}

// ##############################################################################################################
// #############################################    FUNCTIONS    ################################################
// ##############################################################################################################

void CustomToppersComponent::refreshCarModel() {
	auto *carActor = Instances.getCarActor();
	if (!carActor)
		return;
	if (carActor->IsA<ACarPreviewActor_TA>()) {
		auto *car = reinterpret_cast<ACarPreviewActor_TA *>(carActor);
		car->eventOnOwnerChanged();
		DLOG("Called eventOnOwnerChanged");
	} else if (carActor->IsA<ACar_TA>()) {
		auto *car = reinterpret_cast<ACar_TA *>(carActor);
		car->RespawnInPlace(); // works, but does a whole respawn at spawn point
	}
	LOG("Refreshed car model");
}

/*
 *
 * Example JSON stucture:
 *
 * {
 *      "TopperId": 3954,
 *      "MeshPath": "MyCustomTopper.CustomTopper_SM"
 *      "MaterialPath": "MyCustomTopper.CustomTopper_MAT"
 *      "Scale": 0.75
 * }
 *
 */
// TODO: maybe add a bool for whether the custom asset is a SK or SM.. since some toppers are animated (and i assume SK)
std::optional<CustomTopperData> CustomToppersComponent::createTopperDataFromJson(const json &j) {
	auto idIt       = j.find("TopperId");
	auto meshPathIt = j.find("MeshPath");
	auto matPathIt  = j.find("MaterialPath"); // NOTE: might be unnecessary? bc maybe lesbian topper was just made wrong in UDK?
	if (idIt == j.end() || meshPathIt == j.end() || matPathIt == j.end()) // required values
		return std::nullopt;

	CustomTopperData topper{};

	try {
		topper.productId    = idIt->get<int32_t>();
		topper.assetPath    = meshPathIt->get<std::string>();
		topper.materialPath = matPathIt->get<std::string>();

		readOptionalJsonVal<float>(j, "Scale", topper.scale);
	} catch (const json::exception &e) {
		LOGERROR("Unable to read JSON data: \"{}\"", e.what());
		return std::nullopt;
	}

	return topper;
}

CustomTopperData *CustomToppersComponent::getCustomTopperDataFromProdId(ProductId id) {
	auto it = m_topperCustomizations.find(id);
	if (it == m_topperCustomizations.end())
		return nullptr;
	auto &customization = it->second;
	if (!customization.selectedTopperIdx)
		return nullptr;
	return &customization.toppers[*customization.selectedTopperIdx];
}

// ##############################################################################################################
// #############################################    DISPLAY    ##################################################
// ##############################################################################################################

void CustomToppersComponent::display_settings() {
	{
		static constexpr auto CUSTOMTOPPERS_PATH =
		    "C:\\Program Files\\Epic Games\\rocketleague\\TAGame\\CookedPCConsole\\mods\\CustomToppers\\MyCustomTopper.upk";

		GUI::ScopedChild c{"Content", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.9f)};

		GUI::Spacing(2);

		if (ImGui::Button("Open CustomToppers folder")) {
			Files::OpenFolder(m_customToppersFolder);
		}
		if (ImGui::IsItemHovered()) {
			GUI::ScopedTooltip t{};

			ImGui::TextUnformatted("Put your custom topper JSON files here\n\nAnd put the corresponding .upk file in the CookedPCConsole "
			                       "folder of your Rocket League installation.\n");
			GUI::ColoredTextFormat("e.g. {}", GUI::WordColor{CUSTOMTOPPERS_PATH, GUI::Colors::LightGreen});

			GUI::Spacing(2);
			GUI::ColoredTextFormat("See the {} tab for more information", GUI::WordColor{"Info", GUI::Colors::LighterBlue});
		}

		GUI::SameLineSpacing_relative(20.0f);

		if (ImGui::Button("Refresh"))
			GAME_THREAD_EXECUTE({ readInCustomAssetData(); });
		GUI::ToolTip("Search for toppers in the CustomToppers folder");

		GUI::Spacing(6);

		bool shouldApplyTopper = false;
		for (auto &[prodId, customization] : m_topperCustomizations) {
			GUI::ScopedID id{prodId};
			auto          it = ProductData::s_topperProducts.find(prodId);
			if (it == ProductData::s_topperProducts.end())
				continue;
			const auto       &prodData = it->second;
			const std::string label    = std::format("{} (ID: {})", prodData.name, prodId);
			const char *preview = customization.selectedTopperIdx ? customization.toppers[*customization.selectedTopperIdx].name.c_str()
			                                                      : "";
			if (ImGui::BeginCombo(label.c_str(), preview)) {
				if (ImGui::Selectable("None")) {
					customization.selectedTopperIdx = std::nullopt;
					shouldApplyTopper               = true;
				}

				for (size_t i = 0; i < customization.toppers.size(); ++i) {
					GUI::ScopedID id{static_cast<int>(i)};
					const auto   &body = customization.toppers[i];
					if (ImGui::Selectable(body.name.c_str())) {
						customization.selectedTopperIdx = i;
						shouldApplyTopper               = true;
					}
				}

				ImGui::EndCombo();
			}
		}

		if (shouldApplyTopper) {
			GAME_THREAD_EXECUTE({
				writePersistantData();
				refreshCarModel();
			});
		}

		if (!m_customTopperProdAssets.empty()) {
			GUI::Spacing(4);

			if constexpr (DEBUG_LOG) {
				std::string cachedAssetsLabel = std::format("Cached assets (size: {})", m_customTopperProdAssets.size());
				if (ImGui::CollapsingHeader(cachedAssetsLabel.c_str())) {
					for (auto &[prodId, asset] : m_customTopperProdAssets) {
						GUI::ScopedID     id{prodId};
						auto              it       = ProductData::s_topperProducts.find(prodId);
						const auto       &prodData = it->second;
						const std::string label    = std::format(
                            "{} (ID: {}) custom UProductAsset_Attachment_TA instance:", prodData.name, prodId);
						if (it == ProductData::s_topperProducts.end())
							continue;
						ImGui::BulletText("%s", label.c_str());
						GUI::SameLineSpacing_relative(20.0f);
						std::string addr = std::format("0x{:X}", reinterpret_cast<uintptr_t>(asset));
						ImGui::InputText("##ObjAddress", &addr, ImGuiInputTextFlags_ReadOnly);
					}
				}
			}
		}
	}

	{
		GUI::ScopedChild c{"ApplyTopperButton"};

		auto  availSpace  = ImGui::GetContentRegionAvail();
		float availWidth  = availSpace.x;
		float buttonWidth = availWidth * 0.5f;
		ImGui::SetCursorPosX((availWidth - buttonWidth) * 0.5f);
		if (ImGui::Button("Apply topper", ImVec2(buttonWidth, availSpace.y))) {
			GAME_THREAD_EXECUTE({ refreshCarModel(); });
		}
	}
}

class CustomToppersComponent CustomToppers;
