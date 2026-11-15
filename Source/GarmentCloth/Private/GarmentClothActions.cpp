// Copyright (c) 2026. MIT License.

#include "GarmentClothActions.h"

#include "Animation/Skeleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "CoreGlobals.h"
#include "EditorAssetLibrary.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

// Vertex color check is off by default: the FStaticMeshVertexBuffers member names changed in 5.8,
// so using them directly is a compile risk. Set to 1 and adjust one line if you want it.
#define GARMENTCLOTH_CHECK_VERTEXCOLOR 0

#if GARMENTCLOTH_CHECK_VERTEXCOLOR
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogGarmentCloth, Log, All);

// ---------------------------------------------------------------------------
// Facts verified against the local UE 5.8 source (D:\UE\UE_5.8):
//   * Cloth asset class : /Script/ChaosClothAsset.ChaosClothAsset
//                         properties: Skeleton (USkeleton*), PhysicsAsset
//                         public API: SetDataflow(UDataflow*), GetDataflowInstance()
//   * No UFactory for cloth assets -> AssetTools::CreateAsset(..., nullptr)
//   * Legacy clothing (ClothingAssetCommon) removed -> Chaos Cloth Asset only
//   * Dataflow node headers live in Internal/ -> third-party plugins cannot build the graph
// So "one click" = create asset + set skeleton + attach template Dataflow (optional) + open + guidance.
// ---------------------------------------------------------------------------

namespace
{
	const TCHAR* ConfigSection = TEXT("GarmentCloth");
	const TCHAR* ConfigKeyTemplate = TEXT("TemplateDataflow");

	UClass* FindClassByCandidates(const TArray<FString>& Candidates)
	{
		for (const FString& Path : Candidates)
		{
			if (UClass* Found = FindObject<UClass>(nullptr, *Path))
			{
				return Found;
			}
		}
		return nullptr;
	}

	bool SetObjectProperty(UObject* Obj, const TCHAR* PropName, UObject* Value)
	{
		if (!Obj || !Value)
		{
			return false;
		}
		if (FObjectPropertyBase* Prop = FindFProperty<FObjectPropertyBase>(Obj->GetClass(), PropName))
		{
			if (!Prop->PropertyClass || Value->IsA(Prop->PropertyClass))
			{
				Prop->SetObjectPropertyValue_InContainer(Obj, Value);
				return true;
			}
		}
		return false;
	}

	bool CallObjectFunction(UObject* Obj, const TCHAR* FuncName, UObject* Arg)
	{
		if (!Obj)
		{
			return false;
		}
		UFunction* Func = Obj->FindFunction(FName(FuncName));
		if (!Func)
		{
			return false;
		}
		if (Func->NumParms == 0)
		{
			Obj->ProcessEvent(Func, nullptr);
			return true;
		}
		uint8 Buffer[64] = { 0 };
		FObjectPropertyBase* Param = nullptr;
		for (TFieldIterator<FProperty> It(Func); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			Param = CastField<FObjectPropertyBase>(*It);
			break;
		}
		if (Param && Arg && Arg->IsA(Param->PropertyClass))
		{
			Param->SetObjectPropertyValue(Buffer, Arg);
			Obj->ProcessEvent(Func, Buffer);
			return true;
		}
		return false;
	}

	bool SaveAsset(UObject* Asset)
	{
		if (!Asset)
		{
			return false;
		}
		return UEditorAssetLibrary::SaveAsset(Asset->GetOutermost()->GetName(), false);
	}
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

USkeletalMesh* FGarmentClothActions::FindSimMeshFor(const USkeletalMesh* GarmentMesh)
{
	if (!GarmentMesh)
	{
		return nullptr;
	}

	const FString MeshName = GarmentMesh->GetName();
	const FString PackagePath = FPackageName::GetLongPackagePath(GarmentMesh->GetOutermost()->GetName());

	const TArray<FString> Candidates =
	{
		MeshName + TEXT("_Sim"),
		MeshName + TEXT("_sim"),
		MeshName + TEXT("_SimMesh"),
	};

	// 1) same folder, exact name
	for (const FString& CandidateName : Candidates)
	{
		const FString ObjectPath = PackagePath / CandidateName + TEXT(".") + CandidateName;
		if (UObject* Obj = UEditorAssetLibrary::LoadAsset(ObjectPath))
		{
			if (USkeletalMesh* SM = Cast<USkeletalMesh>(Obj))
			{
				return SM;
			}
		}
	}

	// 2) any skeletal mesh in the same folder ending with _Sim
	FAssetRegistryModule& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FARFilter Filter;
	Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("SkeletalMesh")));
	Filter.PackagePaths.Add(FName(*PackagePath));
	Filter.bRecursivePaths = false;
	TArray<FAssetData> Found;
	AR.Get().GetAssets(Filter, Found);
	for (const FAssetData& D : Found)
	{
		if (D.AssetName.ToString().EndsWith(TEXT("_Sim")))
		{
			if (USkeletalMesh* SM = Cast<USkeletalMesh>(D.GetAsset()))
			{
				return SM;
			}
		}
	}
	return nullptr;
}

bool FGarmentClothActions::HasClothMaskVertexColor(const USkeletalMesh* Mesh)
{
#if GARMENTCLOTH_CHECK_VERTEXCOLOR
	if (!Mesh)
	{
		return false;
	}
	if (FSkeletalMeshRenderData* RenderData = Mesh->GetResourceForRendering())
	{
		for (const FSkeletalMeshLODRenderData& LOD : RenderData->LODRenderData)
		{
			if (LOD.StaticVertexBuffers.ColorVertexBuffer.GetNumVertices() > 0)
			{
				return true;
			}
		}
	}
#endif
	return false;
}

// ---------------------------------------------------------------------------
// Console command (for automation / headless verification)
// ---------------------------------------------------------------------------

static FAutoConsoleCommand GCreateClothCmd(
	TEXT("GarmentCloth.CreateFrom"),
	TEXT("GarmentCloth.CreateFrom <SkeletalMeshPath|Name>  : create a Chaos Cloth Asset from a garment mesh"),
	FConsoleCommandWithArgsDelegate::CreateStatic(&FGarmentClothActions::ConsoleCreateFrom));

void FGarmentClothActions::ConsoleCreateFrom(const TArray<FString>& Args)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogGarmentCloth, Warning, TEXT("Usage: GarmentCloth.CreateFrom <SkeletalMeshPath|Name>"));
		return;
	}

	const FString Spec = Args[0];
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *Spec);
	if (!Mesh && !Spec.StartsWith(TEXT("/")))
	{
		FAssetRegistryModule& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		FARFilter Filter;
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("SkeletalMesh")));
		Filter.bRecursivePaths = true;
		TArray<FAssetData> Found;
		AR.Get().GetAssets(Filter, Found);
		for (const FAssetData& D : Found)
		{
			if (D.AssetName.ToString().Contains(Spec))
			{
				Mesh = Cast<USkeletalMesh>(D.GetAsset());
				if (Mesh)
				{
					break;
				}
			}
		}
	}

	if (!Mesh)
	{
		UE_LOG(LogGarmentCloth, Error, TEXT("SkeletalMesh not found: %s"), *Spec);
		return;
	}

	const FResult Result = CreateClothAssetFromMesh(Mesh);
	UE_LOG(LogGarmentCloth, Display, TEXT("=== RESULT: %s ==="), Result.bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
	for (const FString& Step : Result.Steps)
	{
		UE_LOG(LogGarmentCloth, Display, TEXT("  %s"), *Step);
	}
	UE_LOG(LogGarmentCloth, Display, TEXT("%s"), *Result.Message);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

FGarmentClothActions::FResult FGarmentClothActions::CreateClothAssetFromMesh(USkeletalMesh* GarmentMesh)
{
	FResult Result;
	auto Log = [&Result](const FString& Step)
	{
		Result.Steps.Add(Step);
		UE_LOG(LogGarmentCloth, Log, TEXT("%s"), *Step);
	};

	if (!GarmentMesh)
	{
		Result.Message = TEXT("No mesh selected.");
		return Result;
	}
	Log(FString::Printf(TEXT("Source mesh: %s"), *GarmentMesh->GetPathName()));

	// 1) cloth asset class
	UClass* ClothClass = FindClassByCandidates({
		TEXT("/Script/ChaosClothAssetEngine.ChaosClothAsset"),   // UE 5.8: module is ChaosClothAssetEngine
		TEXT("/Script/ChaosClothAsset.ChaosClothAsset"),
	});
	if (!ClothClass)
	{
		// last resort: scan for any class named ChaosClothAsset
		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* C = *It;
			if (C && C->GetName() == TEXT("ChaosClothAsset") && !C->HasAnyClassFlags(CLASS_Abstract))
			{
				ClothClass = C;
				break;
			}
		}
	}
	if (!ClothClass)
	{
		Result.Message = TEXT(
			"UChaosClothAsset not found.\n"
			"Enable the 'Chaos Cloth Asset' plugin (Edit > Plugins), restart the editor, then retry.");
		Log(TEXT("FAILED: /Script/ChaosClothAsset.ChaosClothAsset not found"));
		return Result;
	}
	Log(FString::Printf(TEXT("Cloth asset class: %s"), *ClothClass->GetPathName()));

	// 2) create asset (5.8 has no cloth-asset UFactory -> nullptr)
	const FString PackagePath = FPackageName::GetLongPackagePath(GarmentMesh->GetOutermost()->GetName());
	const FString AssetName = GarmentMesh->GetName() + TEXT("_Cloth");

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UObject* NewAsset = AssetTools.CreateAsset(AssetName, PackagePath, ClothClass, nullptr);
	if (!NewAsset)
	{
		Result.Message = TEXT("CreateAsset failed (asset may already exist, or folder is read-only).");
		Log(TEXT("FAILED: CreateAsset returned null"));
		return Result;
	}
	Log(FString::Printf(TEXT("Created: %s"), *NewAsset->GetPathName()));

	// 3) skeleton
	USkeleton* Skel = GarmentMesh->GetSkeleton();
	if (Skel)
	{
		const bool bByProp = SetObjectProperty(NewAsset, TEXT("Skeleton"), Skel);
		const bool bByFunc = bByProp ? false : CallObjectFunction(NewAsset, TEXT("SetSkeleton"), Skel);
		Log(FString::Printf(TEXT("Skeleton %s -> %s"), *Skel->GetName(),
			(bByProp || bByFunc) ? TEXT("set") : TEXT("FAILED (set it manually)")));
	}
	else
	{
		Log(TEXT("WARNING: source mesh has no Skeleton; specify the character skeleton on import"));
	}

	// 4) template dataflow (optional, makes future runs fully automatic)
	FString TemplatePath;
	GConfig->GetString(ConfigSection, ConfigKeyTemplate, TemplatePath, *GEditorPerProjectIni);
	if (!TemplatePath.IsEmpty())
	{
		if (UObject* Template = UEditorAssetLibrary::LoadAsset(TemplatePath))
		{
			const bool bSet = CallObjectFunction(NewAsset, TEXT("SetDataflow"), Template);
			Log(FString::Printf(TEXT("Template Dataflow %s -> %s"), *TemplatePath,
				bSet ? TEXT("attached") : TEXT("FAILED")));
		}
		else
		{
			Log(FString::Printf(TEXT("WARNING: template Dataflow not found: %s"), *TemplatePath));
		}
	}
	else
	{
		Log(TEXT("No template Dataflow configured ([GarmentCloth] TemplateDataflow in Config/DefaultEditor.ini)"));
	}

	// 5) sim mesh / vertex color hints
	if (USkeletalMesh* SimMesh = FindSimMeshFor(GarmentMesh))
	{
		Log(FString::Printf(TEXT("Sim mesh found: %s (pick it in the Dataflow Render/Sim Mesh node)"),
			*SimMesh->GetName()));
	}
	else
	{
		Log(TEXT("No <name>_Sim mesh found; high-poly only also works, just slower"));
	}

#if GARMENTCLOTH_CHECK_VERTEXCOLOR
	if (HasClothMaskVertexColor(GarmentMesh))
	{
		Log(TEXT("Vertex colors found: use the ClothMask R channel to paint Max Distance"));
	}
	else
	{
		Log(TEXT("No vertex colors found (enable Vertex Colors on FBX import)"));
	}
#else
	Log(TEXT("Vertex color check disabled; verify ClothMask imported in the Cloth Asset"));
#endif

	// 6) save
	NewAsset->MarkPackageDirty();
	SaveAsset(NewAsset);

	Result.bSuccess = true;
	Result.CreatedAsset = NewAsset;
	Result.Message = FString::Printf(
		TEXT("Chaos Cloth Asset created: %s\n\n")
		TEXT("UE 5.8 builds the cloth geometry from a Dataflow graph. One-time setup (then save as template):\n")
		TEXT("1. In the opened asset, add 'Import Skeletal Mesh' and pick this mesh\n")
		TEXT("2. Add Render Mesh / Sim Mesh nodes (Sim = *_Sim), connect to Make Cloth Asset\n")
		TEXT("3. Click Rebuild, then paint Max Distance using the ClothMask vertex color (R=1 simulate, G=1 pin)\n\n")
		TEXT("To make it truly one-click later, save that graph as a template Dataflow and add\n")
		TEXT("[GarmentCloth]\nTemplateDataflow=/Game/ClothTemplates/DF_GarmentTemplate\n")
		TEXT("to Config/DefaultEditor.ini"),
		*NewAsset->GetPathName());

	return Result;
}
