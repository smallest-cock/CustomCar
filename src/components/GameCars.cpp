#include "pch.h"
#include "GameCars.hpp"
#include "components/CustomAssetTypes.hpp"
#include "Events.hpp"
#include "Cvars.hpp"
#include "util/Macros.hpp"
#include "ModUtils/gui/GuiTools.hpp"
#include "util/Instances.hpp"
#include <chrono>
#include <vector>

// ##############################################################################################################
// ###############################################    INIT    ###################################################
// ##############################################################################################################

void GameCarsComponent::init(const std::shared_ptr<GameWrapper> &gw) {
	gameWrapper = gw;
	initCvars();
	initHooks();

	LOG("GameCars component initialized");
}

void GameCarsComponent::initHooks() {
	hookWithCallerPost(Events::PRI_TA_ReplicatedEvent, [this](ActorWrapper Caller, void *Params, ...) {
		if (!*m_useSpawnedCars)
			return;

		auto *pri = reinterpret_cast<APRI_TA *>(Caller.memory_address);
		if (!pri)
			return;
		auto *params = reinterpret_cast<APRI_TA_eventReplicatedEvent_Params *>(Params);
		if (!params)
			return;

		if (params->VarName != L"ClientLoadouts")
			return;
		if (!sameId(pri->UniqueId, Instances.GetUniqueID()))
			return;
		applyLoadoutToPRI(pri);
	});
}

void GameCarsComponent::initCvars() {
	registerCvar_bool(Cvars::useSpawnedCars, false).bindTo(m_useSpawnedCars);
	// ...
}

// ##############################################################################################################
// #############################################    FUNCTIONS    ################################################
// ##############################################################################################################

UOnlineProduct_TA *GameCarsComponent::spawnProduct(
    int item, TArray<FOnlineProductAttribute> attributes, int seriesid, int tradehold, bool log, const std::string &spawnMessage) {
	FOnlineProductData productData;
	productData.ProductID      = item;
	productData.SeriesID       = seriesid;
	productData.InstanceID     = generatePIID(item);
	productData.TradeHold      = tradehold;
	productData.AddedTimestamp = getTimestampLong();
	productData.Attributes     = attributes;

	UOnlineProduct_TA *product = UProductUtil_TA::CreateOnlineProduct(productData);
	if (!product)
		return nullptr;
	if (spawnProductData(product->InstanceOnlineProductData(), spawnMessage))
		return product;
	else
		return nullptr;
}

bool GameCarsComponent::spawnProductData(FOnlineProductData productData, const std::string &spawnMessage) {
	auto *drops = Instances.getInstanceOf<UGFxData_ContainerDrops_TA>();
	if (!drops) {
		LOGERROR("UGFxData_ContainerDrops_TA* is null!");
		return false;
	}

	UOnlineProduct_TA *onlineProd = drops->CreateTempOnlineProduct(productData);
	if (!onlineProd) {
		LOGERROR("UOnlineProduct_TA* is null!");
		return false;
	}

	USaveData_TA *saveData = Instances.getSaveData();
	if (!saveData) {
		LOGERROR("[ERROR] USaveData_TA* is null!");
		return false;
	}

	FString msg = (spawnMessage.empty()) ? L"" : FString::create(spawnMessage);

	saveData->eventGiveOnlineProduct(onlineProd, msg, 0.0f);
	saveData->GiveOnlineProductHelper(onlineProd);
	saveData->OnNewOnlineProduct(onlineProd, msg);
	saveData->EventNewOnlineProduct(saveData, onlineProd, msg);
	if (saveData->OnlineProductSet) {
		saveData->OnlineProductSet->Add(onlineProd);

		/*
		    auto ProductData = online_product->InstanceOnlineProductData();
		    Events.spawnedProducts.push_back(ProductData);
		*/
	}

	LOG("Successfully spawned product: {}", onlineProd->ToJson().ToString());
	return true;
}

FProductInstanceID GameCarsComponent::generatePIID(int64_t prod) {
	static int s_generatedPIIDs = 0;
	return intToProductInstanceID(getTimestampLong() * prod + ++s_generatedPIIDs);
}

FProductInstanceID GameCarsComponent::intToProductInstanceID(int64_t val) {
	FProductInstanceID id;
	id.UpperBits = static_cast<uint64_t>(val >> 32);
	id.LowerBits = static_cast<uint64_t>(val & 0xFFFFFFFF);
	return id;
}

uint64_t GameCarsComponent::getTimestampLong() {
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void GameCarsComponent::applyLoadoutToPRI(APRI_TA *pri) {
	if (!validUObject(pri))
		return;

	// get instances of stuff we need
	ULocalPlayer *lp = Instances.IULocalPlayer();
	if (!lp || !validUObject(lp->Actor) || !lp->Actor->IsA<APlayerControllerBase_TA>())
		return;
	auto *pc = static_cast<APlayerControllerBase_TA *>(lp->Actor);

	UProfile_TA *profile = pc->GetProfile();
	if (!validUObject(profile))
		return;
	UProfileLoadoutSave_TA *loadoutSave = profile->LoadoutSave;
	if (!validUObject(loadoutSave))
		return;
	USaveData_TA *saveData = Instances.getSaveData();
	if (!saveData)
		return;
	ULoadoutSet_TA *loadoutSet = loadoutSave->EquippedLoadoutSet;
	if (!validUObject(loadoutSet))
		return;

	// apply to blue (0) and orange (1) loadouts
	for (int i = 0; i < 2; ++i) {
		ULoadout_TA *loadout = loadoutSet->Loadouts[i];
		if (!validUObject(loadout))
			continue;

		for (int j = 0; j < loadout->OnlineProducts128.size(); ++j) {
			// set products
			FProductInstanceID &onlineId                = loadout->OnlineProducts128[j];
			int32_t             prodId                  = saveData->GetProductIDFromOnlineID(onlineId);
			pri->ClientLoadouts.Loadouts[i].Products[j] = prodId;

			// set product attributes
			UOnlineProduct_TA *onlineProduct = saveData->GetOnlineProduct(onlineId);
			if (!onlineProduct)
				continue;
			pri->ClientLoadoutsOnline.Loadouts[i].Products[j].Attributes = onlineProduct->GetAttributes();
		}
	}
	LOG("Manually applied loadouts to PRI");
}

void GameCarsComponent::resyncInventory() {
	USaveData_TA *saveData = Instances.getSaveData();
	if (!saveData)
		return;
	saveData->ForceResyncOnlineProducts();
	Instances.spawnNotification("Custom Car", "Forced inventory resync", 3, true);
}

// ##############################################################################################################
// #############################################    DISPLAY    ##################################################
// ##############################################################################################################

void GameCarsComponent::display_settings() {
	static int                        currentCarNameIndex = -1;
	static std::vector<std::string>   carNames;
	static std::map<std::string, int> carIDs;
	static bool                       carGuiDataInitialized = false;

	auto useSpawnedCars_cvar = getCvar(Cvars::useSpawnedCars);

	if (!carGuiDataInitialized) {
		carNames.clear();

		for (const auto &[key, val] : ProductData::s_bodyProducts) {
			carIDs[val.name] = key;
			carNames.push_back(val.name);
		}

		std::sort(carNames.begin(), carNames.end()); // sort alphabetically
		carGuiDataInitialized = true;
	}

	{
		GUI::ScopedChild c{"Content", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.8f)};

		GUI::Spacing(2);

		bool useSpawnedCars = useSpawnedCars_cvar.getBoolValue();
		if (ImGui::Checkbox("Use spawned cars in game", &useSpawnedCars)) {
			useSpawnedCars_cvar.setValue(useSpawnedCars);
		}
		GUI::ToolTip("If you join a match with a car you don't own, RL will force it to be a stocktane (server-side)."
		             "\nSo your in-game hitbox will always be octane.\n\nAKA, you'll "
		             "get the best "
		             "results using octane hitbox cars");

		GUI::verticalSpacing_relative(100.0f);

		ImGui::SearchableCombo("RL cars", &currentCarNameIndex, carNames, "Select a car...", "Search...");

		GUI::SameLineSpacing_relative(20.0f);

		if (ImGui::Button("Refresh")) {
			GAME_THREAD_EXECUTE({
				ProductData::initProductData();
				Instances.spawnNotification("Custom Car", "Refreshed RL cars list", 3, true);
			});
		}

		ImGui::Text("%zu cars found", ProductData::s_bodyProducts.size());
	}

	{
		GUI::ScopedChild c{"Buttons"};

		{
			GUI::ScopedChild c{"SpawnButton", ImVec2(ImGui::GetContentRegionAvailWidth() * 0.75f, 0)};

			if (ImGui::Button("Spawn", ImGui::GetContentRegionAvail())) {
				if (currentCarNameIndex != -1 && carNames.size() > currentCarNameIndex) {
					const std::string &name = carNames[currentCarNameIndex];
					auto               it   = carIDs.find(name);
					if (it != carIDs.end()) {
						int32_t carProdId = it->second;
						GAME_THREAD_EXECUTE({ spawnProduct(carProdId); }, carProdId);
					}
				}
			}
		}

		ImGui::SameLine();

		{
			GUI::ScopedChild c{"ResyncButton", ImGui::GetContentRegionAvail()};

			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.0f, 0.6f, 0.6f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.0f, 0.7f, 0.7f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.0f, 0.8f, 0.8f));

			if (ImGui::Button("Resync Inventory", ImGui::GetContentRegionAvail())) {
				GAME_THREAD_EXECUTE({ resyncInventory(); });
			}
			GUI::ToolTip("Will yeet all spawned items");

			ImGui::PopStyleColor(3);
		}
	}
}

GameCarsComponent GameCars{};
