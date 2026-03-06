#pragma once
#include "Component.hpp"
#include <memory>

class GameCarsComponent : Component<GameCarsComponent> {
public:
	GameCarsComponent()  = default;
	~GameCarsComponent() = default;

	static constexpr std::string_view componentName = "GameCars";
	void                              init(const std::shared_ptr<GameWrapper> &gw);

private:
	void initHooks();
	void initCvars();

private:
	std::shared_ptr<bool> m_useSpawnedCars = std::make_shared<bool>(true);

private:
	fs::path m_persistanceJsonFile;

private:
	FProductInstanceID generatePIID(int64_t prod = -1);
	FProductInstanceID intToProductInstanceID(int64_t val);
	uint64_t           getTimestampLong();

public:
	UOnlineProduct_TA *spawnProduct(int item,
	    TArray<FOnlineProductAttribute> attributes   = {},
	    int                             seriesid     = 0,
	    int                             tradehold    = 0,
	    bool                            log          = false,
	    const std::string              &spawnMessage = "");

	bool spawnProductData(FOnlineProductData productData, const std::string &spawnMessage);

private:
	void applyLoadoutToPRI(APRI_TA *pri);
	void resyncInventory();

public:
	void display_settings();
};

extern class GameCarsComponent GameCars;
