#pragma once

namespace Events {
	constexpr auto EngineShare_X_EventPreLoadMap      = "Function ProjectX.EngineShare_X.EventPreLoadMap";
	constexpr auto LoadingScreen_TA_HandlePostLoadMap = "Function TAGame.LoadingScreen_TA.HandlePostLoadMap";

	constexpr auto GFxData_Chat_TA_SendChatPresetMessage = "Function TAGame.GFxData_Chat_TA.SendChatPresetMessage";
	constexpr auto PlayerControllerBase_TA_GetSaveObject = "Function TAGame.PlayerControllerBase_TA.GetSaveObject";

	constexpr auto GFxData_StartMenu_TA_ProgressToMainMenu = "Function TAGame.GFxData_StartMenu_TA.ProgressToMainMenu";

	constexpr auto Loadout_TA_ValidateForcedProducts             = "Function TAGame.Loadout_TA.ValidateForcedProducts";
	constexpr auto GFxData_Garage_TA_SetPreviewLoadout           = "Function TAGame.GFxData_Garage_TA.SetPreviewLoadout";
	constexpr auto SkeletalMeshComponent_AttachComponentToSocket = "Function Engine.SkeletalMeshComponent.AttachComponentToSocket";

	constexpr auto CarMeshComponentBase_TA_SetBodyAsset = "Function TAGame.CarMeshComponentBase_TA.SetBodyAsset";
	constexpr auto CarMeshComponent_TA_SetBodyFXActor   = "Function TAGame.CarMeshComponent_TA.SetBodyFXActor";
	constexpr auto CarMeshComponent_TA_SetLoadout       = "Function TAGame.CarMeshComponent_TA.SetLoadout";
	constexpr auto CarMeshComponent_TA_InitBodyVisuals  = "Function TAGame.CarMeshComponent_TA.InitBodyVisuals";
	constexpr auto CarMeshComponent_TA_OnAttached       = "Function TAGame.CarMeshComponent_TA.OnAttached";
	constexpr auto CarMeshComponent_TA_InitAttachments  = "Function TAGame.CarMeshComponent_TA.InitAttachments";

	constexpr auto ProductLoader_TA_HandleAssetLoaded = "Function TAGame.ProductLoader_TA.HandleAssetLoaded";
	constexpr auto ProductDatabase_TA__TLoadAsset__ProductAsset_Body_TA =
	    "Function TAGame.FunctionTemplates.ProductDatabase_TA__TLoadAsset__ProductAsset_Body_TA";

	constexpr auto PRI_TA_ReplicatedEvent = "Function TAGame.PRI_TA.ReplicatedEvent";

	// the ACPlugin.dll hooked functions for CustomCars module (in the order which they're hooked)
	constexpr auto CarMeshComponentBase_TA_InitBodyAsset  = "Function TAGame.CarMeshComponentBase_TA.InitBodyAsset";
	constexpr auto CarMeshComponent_TA_ApplyPaintSettings = "Function TAGame.CarMeshComponent_TA.ApplyPaintSettings";
	constexpr auto Car_TA_HandleAllAssetsLoaded           = "Function TAGame.Car_TA.HandleAllAssetsLoaded";

	constexpr auto PRI_TA_EventCarSet           = "Function TAGame.PRI_TA.EventCarSet";
	constexpr auto PRI_TA_OnLoadoutsSetInternal = "Function TAGame.PRI_TA.OnLoadoutsSetInternal";
	constexpr auto PRI_TA_OnLoadoutsSet         = "Function TAGame.PRI_TA.OnLoadoutsSet";
	constexpr auto PRI_TA_HandleLoadoutLoaded   = "Function TAGame.PRI_TA.HandleLoadoutLoaded";

	constexpr auto CarPreviewActor_TA_BuildOnlineLoadout = "Function TAGame.CarPreviewActor_TA.BuildOnlineLoadout";
}
