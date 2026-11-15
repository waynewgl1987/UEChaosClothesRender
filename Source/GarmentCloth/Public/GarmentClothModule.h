// Copyright (c) 2026. MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FGarmentClothModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Content Browser 右键菜单扩展 */
	TSharedRef<FExtender> OnExtendContentBrowserMenu(const TArray<FAssetData>& SelectedAssets);
	void AddMenuEntry(FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<UObject>> SelectedObjects);

	/** 执行: 选中资产 -> 生成 Chaos Cloth Asset */
	void ExecuteCreateCloth(TArray<TWeakObjectPtr<UObject>> SelectedObjects);

	FDelegateHandle ContentBrowserExtenderHandle;
};
