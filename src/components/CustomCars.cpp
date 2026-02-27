#include "pch.h"
#include "CustomCars.hpp"
#include "components/CustomAssetTypes.hpp"
#include "Events.hpp"
#include "ModUtils/gui/GuiTools.hpp"
#include "ModUtils/util/Utils.hpp"
#include "util/Instances.hpp"
#include "util/Macros.hpp"
#include "util/logging.hpp"
#include "util/FNameCache.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string_view>
#include <thread>

std::unordered_map<int, std::string> g_baseHitboxIds = {{22, "Breakout"},
    {23, "Octane"},
    {24, "Plank"},  // Paladin ID, plank hitbox
    {28, "Hybrid"}, // X-Devil ID, hybrid hitbox
    {30, "Merc"},
    {31, "Hybrid"}, // Venom ID, hybrid hitbox
    {403, "Dominus"}};

// ##############################################################################################################
// ###############################################    INIT    ###################################################
// ##############################################################################################################

void CustomCarsComponent::init(const std::shared_ptr<GameWrapper> &gw) {
	gameWrapper = gw;

	std::jthread fnamesThread{[this]() { initCachedFNames(); }};

	initFolders();
	readInCustomAssetData();
	loadPersistantData();
	initHooks();

	LOG("CustomCars component initialized");
}

void CustomCarsComponent::onUnload() {
	for (auto &[prodId, prodAsset] : m_customBodyProdAssets) {
		Instances.markForDestroy(prodAsset);
		LOG("Marked to destroy the custom duplicated UProductAsset_Body_TA instance ({}) for body ID {}", prodAsset, prodId);
	}
	LOG("Cleaned up cached custom UProductAsset_Body_TA instances");
}

void CustomCarsComponent::initFolders() {
	auto pluginFolder     = gameWrapper->GetDataFolder() / "CustomCar";
	m_persistanceJsonFile = pluginFolder / "CustomCars_save.json";
	m_customCarsFolder    = pluginFolder / "CustomCars";

	if (!fs::exists(m_persistanceJsonFile)) {
		std::ofstream file(m_persistanceJsonFile);
		file << "{}" << std::endl;
		file.close();
		LOG("Created empty settings save file: \"{}\"", m_persistanceJsonFile.string());
	}

	if (!fs::exists(m_customCarsFolder)) {
		fs::create_directories(m_customCarsFolder);
		LOG("Created custom cars folder: \"{}\"", m_customCarsFolder.string());
	}
}

void CustomCarsComponent::readInCustomAssetData() {
	m_bodyCustomizations.clear();

	try {
		for (const auto &entry : fs::recursive_directory_iterator(m_customCarsFolder)) {
			if (!entry.is_regular_file() || entry.path().extension() != ".json")
				continue;

			json j = Files::get_json(entry.path());
			if (j.empty())
				continue;

			for (const auto &[key, val] : j.items()) {
				if (!val.is_object())
					continue;
				auto assetData = createBodyDataFromJson(val);
				if (!assetData)
					continue;
				assetData->name = key;
				m_bodyCustomizations[assetData->productId].bodies.emplace_back(*assetData);
			}
		}
	} catch (const fs::filesystem_error &e) {
		LOGERROR("Exception occured while reading data from folder \"{}\": {}", m_customCarsFolder.string(), e.what());
	}

	Instances.spawnNotification("Custom Car", std::format("Found {} custom cars", m_bodyCustomizations.size()), 3, true);
}

/*
 * Example JSON:
 *
 *  {
 *      "203": "RS",
 *      "23": "Fenmobile"
 *  }
 */
// NOTE: must be called after readInCustomAssetData() bc it relies on m_bodyCustomizations
void CustomCarsComponent::loadPersistantData() {
	json j = Files::get_json(m_persistanceJsonFile);
	if (j.empty())
		return;

	for (const auto &[key, val] : j.items()) {
		if (!val.is_string())
			continue;
		std::string valStr = val.get<std::string>();
		int32_t     bodyId = std::stoi(key);

		auto it = m_bodyCustomizations.find(bodyId);
		if (it == m_bodyCustomizations.end())
			continue;
		BodyCustomization &custo = it->second;

		for (size_t i = 0; i < custo.bodies.size(); ++i) {
			const auto &customBody = custo.bodies[i];
			if (customBody.name == valStr) {
				custo.selectedBodyIdx = i;
				break;
			}
		}
	}
}

void CustomCarsComponent::writePersistantData() {
	json j;

	for (const auto &[prodId, custo] : m_bodyCustomizations) {
		if (!custo.selectedBodyIdx)
			continue;
		j[std::to_string(prodId)] = custo.bodies[*custo.selectedBodyIdx].name;
	}

	Files::write_json(m_persistanceJsonFile, j);
	LOG("Saved selected bodies to \"{}\"", m_persistanceJsonFile.string());
}

void CustomCarsComponent::initCachedFNames() {
	g_fnameCache.registerName(L"BoostConeMesh", &m_boostSocketNames[0]);
	g_fnameCache.registerName(L"BoostConeMesh02", &m_boostSocketNames[1]);
	g_fnameCache.registerName(L"BoostEmitterLeft", &m_boostSocketNames[2]);
	g_fnameCache.registerName(L"BoostEmitterRight", &m_boostSocketNames[3]);
	g_fnameCache.registerName(L"AmbientBoostMesh", &m_boostSocketNames[4]);
	g_fnameCache.registerName(L"FL_Disc_jnt", &m_wheelBoneNames.frontLeft);
	g_fnameCache.registerName(L"FR_Disc_jnt", &m_wheelBoneNames.frontRight);
	g_fnameCache.registerName(L"BL_Disc_jnt", &m_wheelBoneNames.backLeft);
	g_fnameCache.registerName(L"BR_Disc_jnt", &m_wheelBoneNames.backRight);
	g_fnameCache.cacheNames();
}

void CustomCarsComponent::initHooks() {
	hookEventPost(Events::GFxData_StartMenu_TA_ProgressToMainMenu, [this](std::string e) {
		refreshCarModel();
		LOG("Refreshed car model after progressing to main menu");
	});

	hookWithCallerPost(Events::CarMeshComponentBase_TA_InitBodyAsset, [this](ActorWrapper Caller, void *Params, ...) {
		auto *cmc = UnrealCast<UCarMeshComponent_TA>(reinterpret_cast<UCarMeshComponentBase_TA *>(Caller.memory_address));
		if (!cmc) {
			LOGERROR("UCarMeshComponent_TA* from caller is null in InitBodyAsset post hook");
			return;
		}

		if (!cmc->bLocalPlayer)
			return;

		// get CustomBodyData
		if (!validUObject(cmc->BodyAsset)) {
			LOGERROR("cmc->BodyAsset is null");
			return;
		}
		UProduct_TA *prod = cmc->BodyAsset->GetProduct();
		if (!prod) {
			LOGERROR("cmc->BodyAsset->GetProduct() is null");
			return;
		}
		int32_t prodId = prod->GetID();

		CustomBodyData *customBodyData = getCustomBodyDataFromProdId(prodId);
		if (!customBodyData) {
			DLOG("Couldn't find custom body data for ID {}", prodId);
			return;
		}

		// get custom USkeletalMesh* (from CustomBodyData)
		auto *customMesh = Instances.loadObject<USkeletalMesh>(customBodyData->assetPath);
		if (!customMesh) {
			LOGERROR("USkeletalMesh* is null from Instances.loadObject");
			return;
		}
		if (!customMesh->IsA<USkeletalMesh>()) {
			LOGERROR("UObject loaded from \"{}\" isn't a USkeletalMesh", customBodyData->assetPath);
			return;
		}

		// apply custom mesh
		cmc->SetSkeletalMesh(customMesh, false);
		DLOG("Called cmc->SetSkeletalMesh in InitBodyAsset post hook");

		// apply suspension stiffness scale
		if (customBodyData->suspensionStiffnessScale && validUObject(cmc->ChassisSpringComponent)) {
			FVector &springStrength = cmc->ChassisSpringComponent->Spring.Strength;
			float    scaleFactor    = *customBodyData->suspensionStiffnessScale;
			springStrength.X *= scaleFactor;
			springStrength.Y *= scaleFactor;
			springStrength.Z *= scaleFactor;

			LOG("Scaled ChassisSpringComponent->Spring.Strength by factor of {} ... Now it's: X:{}-Y:{}-Z:{}",
			    *customBodyData->suspensionStiffnessScale,
			    springStrength.X,
			    springStrength.Y,
			    springStrength.X);
		}

		UProductAsset_Body_TA *customBodyAsset = nullptr;
		auto                   it              = m_customBodyProdAssets.find(prodId);
		if (it != m_customBodyProdAssets.end()) {
			customBodyAsset = it->second;
		} else if (validUObject(cmc->BodyAsset)) {
			UObject *bodyAssetDupResult = cmc->BodyAsset->DuplicateObject(cmc->BodyAsset, nullptr, nullptr);
			customBodyAsset             = UnrealCast<UProductAsset_Body_TA>(bodyAssetDupResult);
			if (customBodyAsset) {
				Instances.markInvincible(customBodyAsset);
				m_customBodyProdAssets[prodId] = customBodyAsset;
				DLOG("Marked duped UProductAsset_Body_TA instance as invincible and cached it: {}", customBodyAsset);
			}
		}
		if (!validUObject(customBodyAsset)) {
			LOGERROR("Custom body asset is null");
			return;
		}

		// set custom material indices
		if (customBodyData->chassisMatIndex) {
			customBodyAsset->ChassisMaterialIndex = *customBodyData->chassisMatIndex;
			LOG("Set ChassisMaterialIndex to {}", customBodyAsset->ChassisMaterialIndex);
		}
		if (customBodyData->skinMatIndex) {
			customBodyAsset->SkinMaterialIndex = *customBodyData->skinMatIndex;
			LOG("Set SkinMaterialIndex to {}", customBodyAsset->SkinMaterialIndex);
		}
		if (customBodyData->brakelightMatIndex) {
			customBodyAsset->BrakelightMaterialIndex = *customBodyData->brakelightMatIndex;
			LOG("Set BrakelightMaterialIndex to {}", customBodyAsset->BrakelightMaterialIndex);
		}

		// set custom UProductAsset_Body_TA on cmc
		cmc->BodyAsset = customBodyAsset;
		DLOG("Set cmc->BodyAsset to be customBodyAsset ({})", customBodyAsset);
	});

	hookWithCallerPost(Events::CarMeshComponent_TA_ApplyPaintSettings, [this](ActorWrapper Caller, void *Params, ...) {
		DLOG("Inside the delay callback for CarMeshComponent_TA_ApplyPaintSettings hook...");

		auto *cmc = reinterpret_cast<UCarMeshComponent_TA *>(Caller.memory_address);
		if (!validUObject(cmc)) {
			LOGERROR("Caller is null in CarMeshComponent_TA_ApplyPaintSettings post hook");
			return;
		}

		if (!cmc->bLocalPlayer)
			return;

		CustomBodyData *customBodyData = getCustomBodyData(cmc);
		if (!customBodyData) {
			DLOGERROR("Unable to find customBodyData (CarMeshComponent_TA_ApplyPaintSettings posst hook)");
			return;
		}

		if (customBodyData->hideTrails)
			hideTrails(cmc, customBodyData);

		if (customBodyData->hideWheels)
			hideWheels(cmc, customBodyData);
		else if (customBodyData->wheelScaleFront || customBodyData->wheelScaleBack)
			scaleWheels(cmc, customBodyData);

		fixBodyFX(cmc, customBodyData);
	});

	hookWithCallerPost(Events::Car_TA_HandleAllAssetsLoaded, [this](ActorWrapper Caller, void *Params, ...) {
		DLOG("We in the Car_TA_HandleAllAssetsLoaded post hook...");

		auto *car = reinterpret_cast<ACar_TA *>(Caller.memory_address);
		if (!validUObject(car)) {
			LOGERROR("Caller is null in Car_TA_HandleAllAssetsLoaded post hook");
			return;
		}
		auto *cmc = car->CarMesh;
		if (!validUObject(cmc)) {
			LOGERROR("car->CarMesh is null in Car_TA_HandleAllAssetsLoaded post hook");
			return;
		}

		if (!cmc->bLocalPlayer)
			return;

		CustomBodyData *customBodyData = getCustomBodyData(cmc);
		if (!customBodyData) {
			DLOGERROR("Unable to find customBodyData (CarMeshComponent_TA_ApplyPaintSettings posst hook)");
			return;
		}

		if (customBodyData->hideTrails)
			hideTrails(cmc, customBodyData);

		if (customBodyData->hideWheels)
			hideWheels(cmc, customBodyData);
		else if (customBodyData->wheelScaleFront || customBodyData->wheelScaleBack)
			scaleWheels(cmc, customBodyData);

		fixBodyFX(cmc, customBodyData);
	});
}

// ##############################################################################################################
// #############################################    FUNCTIONS    ################################################
// ##############################################################################################################

// ##############################################################################################################
//  WARNING: These functions assume the param pointers aren't null!

void CustomCarsComponent::hideTrails(UCarMeshComponent_TA *cmc, CustomBodyData *customBodyData) {
	auto *car = UnrealCast<ACar_TA>(cmc->Owner);
	if (car)
		hideTrails(car, customBodyData);
}

void CustomCarsComponent::hideTrails(ACar_TA *car, CustomBodyData *customBodyData) {
	auto *carFxActor = UnrealCast<AFXActor_Car_TA>(car->FXActor);
	if (carFxActor) {
		for (auto &effect : carFxActor->WheelEffects) {
			if (effect.SupersonicFXActor)
				effect.SupersonicFXActor->DrawScale = 0.0f;
		}
		LOG("Hid SupersonicFXActor trails");
	}
}

void CustomCarsComponent::hideWheels(UCarMeshComponent_TA *cmc, CustomBodyData *customBodyData) {
	for (auto &meshCmp : cmc->WheelMeshes) {
		if (auto *smc = UnrealCast<UStaticMeshComponent>(meshCmp)) {
			smc->SetStaticMesh(nullptr, false);
			smc->SetHidden(true);
		}
	}
	LOG("Hid wheels");
}

std::optional<WheelBoneInfo> CustomCarsComponent::getWheelBoneInfo(const FName &boneName) {
	if (boneName == m_wheelBoneNames.frontLeft)
		return WheelBoneInfo{true, true};
	if (boneName == m_wheelBoneNames.frontRight)
		return WheelBoneInfo{true, false};
	if (boneName == m_wheelBoneNames.backLeft)
		return WheelBoneInfo{false, true};
	if (boneName == m_wheelBoneNames.backRight)
		return WheelBoneInfo{false, false};
	return std::nullopt;
}

void CustomCarsComponent::scaleWheels(UCarMeshComponent_TA *cmc, CustomBodyData *customBodyData) {
	auto &atts = cmc->Attachments;
	for (auto &att : atts) {
		auto wheelboneInfo = getWheelBoneInfo(att.BoneName);
		if (!wheelboneInfo)
			continue;
		bool shouldScaleFrontWheel = wheelboneInfo->front && customBodyData->wheelScaleFront;
		bool shouldScaleBackWheel  = !wheelboneInfo->front && customBodyData->wheelScaleBack;
		bool shouldScaleWheel      = shouldScaleFrontWheel || shouldScaleBackWheel;
		if (!shouldScaleWheel)
			continue;

		auto *wheelMesh = UnrealCast<UStaticMeshComponent>(att.Component);
		if (!wheelMesh) {
			DLOGERROR("Invalid mesh for wheel attachment {}", att.BoneName);
			continue;
		}

		float wheelScale     = wheelboneInfo->front ? *customBodyData->wheelScaleFront : *customBodyData->wheelScaleBack;
		wheelMesh->Scale3D.X = wheelScale;
		wheelMesh->Scale3D.Y = wheelboneInfo->left ? -wheelScale : wheelScale;
		wheelMesh->Scale3D.Z = wheelScale;
		LOG("Scaled {} wheel mesh to {}", att.BoneName, wheelScale);
	}
}

void CustomCarsComponent::fixBodyFX(UCarMeshComponent_TA *cmc, CustomBodyData *customBodyData) {
	if (!validUObject(cmc->BodyFXActor)) {
		LOGERROR("carMesh->BodyFXActor is null in CarMeshComponent_TA_ApplyPaintSettings post hook");
		return;
	}

	for (int i = 0; i < m_boostSocketNames.size(); ++i) {
		std::string socketName = m_boostSocketNames[i].ToString();
		auto       *meshCmp    = UnrealCast<UStaticMeshComponent>(
            cmc->BodyFXActor->GetComponentByName(UStaticMeshComponent::StaticClass(), m_boostSocketNames[i]));
		if (!meshCmp) {
			DLOGWARNING("UStaticMeshComponent* from cmc->BodyFXActor->GetComponentByName(...) for {} is null. Apparenly the original "
			            "Body FXActor doesn't have a mesh component for that socket.",
			    socketName);
			continue;
		}
		USkeletalMeshSocket *cmcSocket = cmc->GetSocketByName(m_boostSocketNames[i]);
		if (!validUObject(cmcSocket)) {

			// NOTE: this works for removing FX meshes that the custom SK doesn't have a socket for (like for Ship and Fenmobile)
			// ... but that means if a custom car is created wrong (i.e. missing a socket on accident) we are unforgiving

			meshCmp->StaticMesh = nullptr;
			meshCmp->Scale      = 0.0f;
			LOG("Cleared mesh component for {} (custom SK doesn't have that socket)", socketName);
			continue;
		}

		meshCmp->Translation = cmcSocket->RelativeLocation;
		meshCmp->Rotation    = cmcSocket->RelativeRotation;

		if (i < 4) {
			// indices 0-3: BoostConeMesh, BoostConeMesh02, BoostEmitterLeft, BoostEmitterRight
			// swizzle scale: (X, Y, Z) → (Y, Z, X)
			meshCmp->Scale3D.X = cmcSocket->RelativeScale.Y;
			meshCmp->Scale3D.Y = cmcSocket->RelativeScale.Z;
			meshCmp->Scale3D.Z = cmcSocket->RelativeScale.X;
			DLOG("Applied swizzled scale for {}: {} --> {}", socketName, cmcSocket->RelativeScale, meshCmp->Scale3D);

			meshCmp->Rotation.Pitch -= 0x4000; // 0x4000 is -90 degrees in unreal units
			DLOG("Applied -0x4000 correction to Rotation.Pitch for {}", socketName);
		} else if (i == 4) {
			// index 4: AmbientBoostMesh
			// directly copy scale
			meshCmp->Scale3D = cmcSocket->RelativeScale;

			meshCmp->Translation.X += 40.0f;
			meshCmp->Translation.Z -= 25.0f;
			DLOG("Applied +40.0f correction to Translation.X for {}", socketName);
			DLOG("Applied -25.0f correction to Translation.Z for {}", socketName);
		}
	}
}

// ##############################################################################################################

std::optional<CustomBodyData> CustomCarsComponent::createBodyDataFromJson(const json &j) {
	auto idIt       = j.find("BodyId");
	auto meshPathIt = j.find("MeshPath");
	if (idIt == j.end() || meshPathIt == j.end()) // required values
		return std::nullopt;

	CustomBodyData body{};

	try {
		body.productId = idIt->get<int32_t>();
		body.assetPath = meshPathIt->get<std::string>();

		readOptionalJsonVal<int32_t>(j, "ChassisMaterialIndex", body.chassisMatIndex);
		readOptionalJsonVal<int32_t>(j, "SkinMaterialIndex", body.skinMatIndex);
		readOptionalJsonVal<int32_t>(j, "BrakelightMaterialIndex", body.brakelightMatIndex);
		readOptionalJsonVal<float>(j, "SuspensionStiffnessScale", body.suspensionStiffnessScale);

		if (auto it = j.find("WheelScale"); it != j.end()) {
			const auto &wheelScaleVal = *it;
			if (wheelScaleVal.is_object()) {
				readOptionalJsonVal<float>(wheelScaleVal, "Front", body.wheelScaleFront);
				readOptionalJsonVal<float>(wheelScaleVal, "Back", body.wheelScaleBack);
			} else if (wheelScaleVal.is_number_float()) {
				const float scale    = wheelScaleVal.get<float>();
				body.wheelScaleFront = scale;
				body.wheelScaleBack  = scale;
			}
		}

		if (auto it = j.find("HideWheels"); it != j.end())
			body.hideWheels = it->get<bool>();
		if (auto it = j.find("HideTrails"); it != j.end())
			body.hideTrails = it->get<bool>();
	} catch (const json::exception &e) {
		LOGERROR("Unable to read JSON data: \"{}\"", e.what());
		return std::nullopt;
	}

	return body;
}

CustomBodyData *CustomCarsComponent::getCustomBodyData(UCarMeshComponentBase_TA *cmc) {
	if (!cmc)
		return nullptr;
	if (!validUObject(cmc->BodyAsset)) {
		LOGERROR("Invalid cmc->BodyAsset");
		return nullptr;
	}
	UProduct_TA *prod = cmc->BodyAsset->GetProduct();
	if (!prod) {
		DLOGWARNING("UProduct_TA* from cmc->BodyAsset->GetProduct() is null. Finna try other method to get Body ID...");
		if (!validUObject(cmc->Owner)) {
			LOGERROR("cmc->Owner is invalid");
			return nullptr;
		}

		UProductLoader_TA *prodLoader = nullptr;
		if (cmc->Owner->IsA<ACarPreviewActor_TA>()) {
			auto *car  = reinterpret_cast<ACarPreviewActor_TA *>(cmc->Owner);
			prodLoader = car->ProductLoader;
		} else if (cmc->Owner->IsA<ACar_TA>()) {
			auto *car  = reinterpret_cast<ACar_TA *>(cmc->Owner);
			prodLoader = car->Loadout;
		}

		if (!validUObject(prodLoader)) {
			LOGERROR("car->ProductLoader is invalid");
			return nullptr;
		}
		auto *bodyAsset = UnrealCast<UProductAsset_Body_TA>(prodLoader->GetAsset(UProductAsset_Body_TA::StaticClass()));
		if (!validUObject(bodyAsset)) {
			LOGERROR("UProductAsset_Body_TA* from prodLoader->GetAsset(...) is invalid");
			return nullptr;
		}
		prod = bodyAsset->GetProduct();
		if (!prod) {
			LOGERROR("UProduct_TA* from bodyAsset->GetProduct() is null");
			return nullptr;
		}
	}
	return getCustomBodyDataFromProdId(prod->GetID());
}

CustomBodyData *CustomCarsComponent::getCustomBodyDataFromProdId(ProductId id) {
	auto it = m_bodyCustomizations.find(id);
	if (it == m_bodyCustomizations.end())
		return nullptr;
	auto &customization = it->second;
	if (!customization.selectedBodyIdx)
		return nullptr;
	return &customization.bodies[*customization.selectedBodyIdx];
}

void CustomCarsComponent::refreshCarModel() {
	auto *carActor = Instances.getCarActor();
	if (!carActor)
		return;
	if (carActor->IsA<ACarPreviewActor_TA>()) {
		auto *car = reinterpret_cast<ACarPreviewActor_TA *>(carActor);
		car->HandleAllProductsLoaded(car->ProductLoader);
		LOG("Called ACarPreviewActor_TA::HandleAllProductsLoaded as a way to refresh car model (triggers hooks)");
	} else if (carActor->IsA<ACar_TA>()) {
		auto *car = reinterpret_cast<ACar_TA *>(carActor);
		car->RespawnInPlace(); // works, but does a whole ass respawn in freeplay (prolly doesnt work in-game)

		// TODO: find better way of refreshing the body on ACar_TA
	}
}

// ##############################################################################################################
// #############################################    DISPLAY    ##################################################
// ##############################################################################################################

void CustomCarsComponent::display_settings() {
	{
		GUI::ScopedChild c{"Content", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.9f)};

		GUI::Spacing(2);

		if (ImGui::Button("Open CustomCars folder"))
			Files::OpenFolder(m_customCarsFolder);
		GUI::ToolTip("Put your custom car JSON files here");

		GUI::SameLineSpacing_relative(20.0f);

		if (ImGui::Button("Refresh"))
			GAME_THREAD_EXECUTE({ readInCustomAssetData(); });
		GUI::ToolTip("Search for cars in the CustomCars folder");

		GUI::Spacing(2);

		if (ImGui::Button("Download cars")) {
			std::jthread openLink{[]() { GUI::open_link("https://alphaconsole.io/browse?category=model"); }};
		}
		GUI::ToolTip("One of the places to download car models");

		GUI::Spacing(6);

		bool shouldApplyBody = false;
		for (auto &[prodId, customization] : m_bodyCustomizations) {
			GUI::ScopedID id{prodId};
			auto          it = ProductData::s_bodyProducts.find(prodId);
			if (it == ProductData::s_bodyProducts.end())
				continue;
			const auto       &prodData = it->second;
			const std::string label    = std::format("{} (ID: {})", prodData.name, prodId);
			const char *preview = customization.selectedBodyIdx ? customization.bodies[*customization.selectedBodyIdx].name.c_str() : "";
			if (ImGui::BeginCombo(label.c_str(), preview)) {
				if (ImGui::Selectable("None")) {
					customization.selectedBodyIdx = std::nullopt;
					shouldApplyBody               = true;
				}

				for (size_t i = 0; i < customization.bodies.size(); ++i) {
					GUI::ScopedID id{static_cast<int>(i)};
					const auto   &body = customization.bodies[i];
					if (ImGui::Selectable(body.name.c_str())) {
						customization.selectedBodyIdx = i;
						shouldApplyBody               = true;
					}
				}

				ImGui::EndCombo();
			}
		}

		if (shouldApplyBody) {
			GAME_THREAD_EXECUTE({
				writePersistantData();
				refreshCarModel();
			});
		}

		if (!m_customBodyProdAssets.empty()) {
			GUI::Spacing(4);

			if constexpr (DEBUG_LOG) {
				std::string cachedAssetsLabel = std::format("Cached assets (size: {})", m_customBodyProdAssets.size());
				if (ImGui::CollapsingHeader(cachedAssetsLabel.c_str())) {
					for (auto &[prodId, asset] : m_customBodyProdAssets) {
						GUI::ScopedID id{prodId};
						ImGui::BulletText("ID: %d --> UProductAsset_Body_TA instance: %p", prodId, asset);
					}
				}
			}
		}
	}

	{
		GUI::ScopedChild c{"RefreshCarButtonSection"};

		auto  availSpace  = ImGui::GetContentRegionAvail();
		float availWidth  = availSpace.x;
		float buttonWidth = availWidth * 0.5f;
		ImGui::SetCursorPosX((availWidth - buttonWidth) * 0.5f);
		if (ImGui::Button("Refresh car model", ImVec2(buttonWidth, availSpace.y))) {
			GAME_THREAD_EXECUTE({ refreshCarModel(); });
		}
	}
}

class CustomCarsComponent CustomCars;

// ##############################################################################################################
// ##########################################    ProductData    #################################################
// ##############################################################################################################

void ProductData::initProductData() {
	auto *productsDatabase = Instances.getInstanceOf<UProductDatabase_TA>();
	if (!productsDatabase) {
		LOGERROR("UProductDatabase_TA* is null... unable to init product data");
		return;
	}

	for (UProduct_TA *prod : productsDatabase->Products_Pristine) {
		if (!prod || !prod->Slot)
			continue;

		int32_t id = prod->GetID();
		if (prod->Slot->Label == L"Body") {
			ProductData data{};
			data.id   = id;
			data.name = prod->LongLabel.ToString();

			// TODO: Maybe call a func to parse the hitbox from a given body UProduct_TA*?? Might need a BodyProductData child class...

			s_bodyProducts[id] = std::move(data);
		} else if (prod->Slot->Label == L"Topper") {
			ProductData data{};
			data.id              = id;
			data.name            = prod->LongLabel.ToString();
			s_topperProducts[id] = std::move(data);
		}
	}

	LOG("Initialized product data. {} body products, {} topper products", s_bodyProducts.size(), s_topperProducts.size());
}
