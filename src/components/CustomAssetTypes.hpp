#pragma once

struct ProductData {
	int32_t     id;
	std::string name;

public:
	static void                                            initProductData();
	static inline std::unordered_map<int32_t, ProductData> s_bodyProducts;
	static inline std::unordered_map<int32_t, ProductData> s_topperProducts;
};

struct ProductAssetData : public ProductData {
	std::string assetPath; // the OG asset path, can prolly be used to revert back to no customization (aka "None" is selected)
};

template <typename T>
void readOptionalJsonVal(const json &j, std::string_view key, std::optional<T> &out) {
	if (auto it = j.find(key); it != j.end())
		out = it->get<T>();
}
