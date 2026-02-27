#pragma once
#include "Component.hpp"
#include <optional>
#include <unordered_map>

struct CustomTopperData {
	std::string          name;
	std::string          assetPath;
	std::string          materialPath;
	int32_t              productId = -1;
	std::optional<float> scale;
};

struct TopperCustomization {
	std::vector<CustomTopperData> toppers;
	std::optional<size_t>         selectedTopperIdx;
};

class CustomToppersComponent : Component<CustomToppersComponent> {
public:
	CustomToppersComponent()  = default;
	~CustomToppersComponent() = default;

	static constexpr std::string_view componentName = "CustomToppers";
	void                              init(const std::shared_ptr<GameWrapper> &gw);
	void                              onUnload();

private:
	void initFolders();
	void initHooks();
	void readInCustomAssetData();
	void loadPersistantData();

private:
	using ProductId = int32_t;

	fs::path m_persistanceJsonFile;
	fs::path m_customToppersFolder;

	std::unordered_map<ProductId, TopperCustomization>           m_topperCustomizations;
	std::unordered_map<ProductId, UProductAsset_Attachment_TA *> m_customTopperProdAssets;

private:
	void refreshCarModel();
	void writePersistantData();

	// CustomTopperData helpers
	std::optional<CustomTopperData> createTopperDataFromJson(const json &j);
	CustomTopperData               *getCustomTopperDataFromProdId(ProductId id);

public:
	void display_settings();
};

extern class CustomToppersComponent CustomToppers;
