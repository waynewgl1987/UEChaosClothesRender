// Copyright (c) 2026. MIT License.

#include "GarmentClothModule.h"
#include "GarmentClothActions.h"

#include "ContentBrowserModule.h"
#include "ContentBrowserDelegates.h"
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Subsystems/AssetEditorSubsystem.h"

#define LOCTEXT_NAMESPACE "GarmentCloth"

void FGarmentClothModule::StartupModule()
{
	FContentBrowserModule& ContentBrowser =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	TArray<FContentBrowserMenuExtender_SelectedAssets>& Extenders =
		ContentBrowser.GetAllAssetViewContextMenuExtenders();

	ContentBrowserExtenderHandle = Extenders.Add_GetRef(
		FContentBrowserMenuExtender_SelectedAssets::CreateRaw(
			this, &FGarmentClothModule::OnExtendContentBrowserMenu)).GetHandle();

	UE_LOG(LogTemp, Log, TEXT("[GarmentCloth] Module started."));
}

void FGarmentClothModule::ShutdownModule()
{
	if (FContentBrowserModule* ContentBrowser =
		FModuleManager::GetModulePtr<FContentBrowserModule>("ContentBrowser"))
	{
		ContentBrowser->GetAllAssetViewContextMenuExtenders().RemoveAll(
			[this](const FContentBrowserMenuExtender_SelectedAssets& Delegate)
			{
				return Delegate.GetHandle() == ContentBrowserExtenderHandle;
			});
	}
	UE_LOG(LogTemp, Log, TEXT("[GarmentCloth] Module shutdown."));
}

TSharedRef<FExtender> FGarmentClothModule::OnExtendContentBrowserMenu(
	const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();

	TArray<TWeakObjectPtr<UObject>> Objects;
	bool bHasSkeletalMesh = false;

	for (const FAssetData& AssetData : SelectedAssets)
	{
		UObject* Obj = AssetData.GetAsset();
		if (!Obj)
		{
			continue;
		}
		Objects.Add(Obj);
		if (Obj->IsA<USkeletalMesh>())
		{
			bHasSkeletalMesh = true;
		}
	}

	if (!bHasSkeletalMesh)
	{
		return Extender;
	}

	Extender->AddMenuExtension(
		"GetAssetActions",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateRaw(
			this, &FGarmentClothModule::AddMenuEntry, Objects));

	return Extender;
}

void FGarmentClothModule::AddMenuEntry(
	FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<UObject>> SelectedObjects)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("CreateCloth", "Garment: Create Chaos Cloth Asset"),
		LOCTEXT("CreateClothTip",
			"Create a UChaosClothAsset from this garment (auto-detects the _Sim mesh)."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(
			this, &FGarmentClothModule::ExecuteCreateCloth, SelectedObjects)));
}

void FGarmentClothModule::ExecuteCreateCloth(TArray<TWeakObjectPtr<UObject>> SelectedObjects)
{
	USkeletalMesh* Garment = nullptr;
	for (const TWeakObjectPtr<UObject>& Weak : SelectedObjects)
	{
		if (UObject* Obj = Weak.Get())
		{
			if (USkeletalMesh* SM = Cast<USkeletalMesh>(Obj))
			{
				Garment = SM;
				break;
			}
		}
	}

	if (!Garment)
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoMesh", "Select a garment SkeletalMesh asset first."));
		return;
	}

	const FGarmentClothActions::FResult Result =
		FGarmentClothActions::CreateClothAssetFromMesh(Garment);

	UE_LOG(LogTemp, Log, TEXT("[GarmentCloth] ===== Create cloth asset: %s ====="),
		*Garment->GetName());
	for (const FString& Step : Result.Steps)
	{
		UE_LOG(LogTemp, Log, TEXT("[GarmentCloth]   %s"), *Step);
	}

	if (Result.bSuccess && Result.CreatedAsset)
	{
		if (UAssetEditorSubsystem* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AssetEditor->OpenEditorForAsset(Result.CreatedAsset);
		}
	}

	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Result.Message));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGarmentClothModule, GarmentCloth)
