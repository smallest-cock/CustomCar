#pragma once
#include "Component.hpp"
#include <optional>
#include <cstdint>
#include <unordered_map>
#include <vector>

extern std::unordered_map<int, std::string> g_baseHitboxIds;

struct CustomBodyData {
	std::string            name;
	std::string            assetPath;
	int32_t                productId = -1;
	std::optional<int32_t> chassisMatIndex;
	std::optional<int32_t> skinMatIndex;
	std::optional<int32_t> brakelightMatIndex;
	std::optional<float>   suspensionStiffnessScale;
	std::optional<float>   wheelScaleFront;
	std::optional<float>   wheelScaleBack;
	bool                   hideWheels = false;
	bool                   hideTrails = false;
};

struct BodyCustomization {
	std::vector<CustomBodyData> bodies;
	std::optional<size_t>       selectedBodyIdx;
};

struct WheelBoneNames {
	FName frontLeft;
	FName frontRight;
	FName backLeft;
	FName backRight;
};

struct WheelBoneInfo {
	bool front = false;
	bool left  = false;
};

class CustomCarsComponent : Component<CustomCarsComponent> {
public:
	CustomCarsComponent()  = default;
	~CustomCarsComponent() = default;

	static constexpr std::string_view componentName = "CustomCars";
	void                              init(const std::shared_ptr<GameWrapper> &gw);
	void                              onUnload();

private:
	void initFolders();
	void initCachedFNames();
	void loadPersistantData();
	void initHooks();
	void readInCustomAssetData();

private:
	using ProductId = int32_t;

	fs::path m_persistanceJsonFile;
	fs::path m_customCarsFolder;

	std::unordered_map<ProductId, BodyCustomization>       m_bodyCustomizations;
	std::unordered_map<ProductId, UProductAsset_Body_TA *> m_customBodyProdAssets;

	// cached FNames
	std::array<FName, 5> m_boostSocketNames;
	WheelBoneNames       m_wheelBoneNames;

private:
	// ############################################################################################

	void hideTrails(UCarMeshComponent_TA *cmc, CustomBodyData *customBodyData);
	void hideTrails(ACar_TA *car, CustomBodyData *customBodyData);
	void hideWheels(UCarMeshComponent_TA *cmc, CustomBodyData *customBodyData);
	void scaleWheels(UCarMeshComponent_TA *cmc, CustomBodyData *customBodyData);
	void fixBodyFX(UCarMeshComponent_TA *cmc, CustomBodyData *customBodyData);

	std::optional<WheelBoneInfo> getWheelBoneInfo(const FName &boneName);

	// ############################################################################################

	void refreshCarModel();
	void writePersistantData();

	// CustomBodyData helpers
	std::optional<CustomBodyData> createBodyDataFromJson(const json &j);
	CustomBodyData               *getCustomBodyData(UCarMeshComponentBase_TA *cmc);
	CustomBodyData               *getCustomBodyDataFromProdId(ProductId id);

public:
	void display_settings();
};

extern class CustomCarsComponent CustomCars;
