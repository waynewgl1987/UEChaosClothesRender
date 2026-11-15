// Copyright (c) 2026. MIT License.

#pragma once

#include "CoreMinimal.h"

class USkeletalMesh;
class UObject;

/**
 * Creates a Chaos Cloth Asset from a garment SkeletalMesh.
 *
 * No ChaosCloth headers are included: everything goes through reflection,
 * so a 5.x point release renaming a class or property degrades gracefully (log) instead of failing the build.
 *
 * Facts verified against UE 5.8 source:
 *   - class path: /Script/ChaosClothAsset.ChaosClothAsset
 *   - has Skeleton / PhysicsAsset properties and a public SetDataflow(UDataflow*)
 *   - no UFactory for cloth assets -> CreateAsset(..., nullptr)
 *   - legacy clothing (ClothingAssetCommon) removed
 *   - Dataflow node headers are Internal/ -> a third-party plugin cannot build the graph reliably
 */
class FGarmentClothActions
{
public:
	struct FResult
	{
		bool bSuccess = false;
		FString Message;
		TArray<FString> Steps;
		UObject* CreatedAsset = nullptr;
	};

	static FResult CreateClothAssetFromMesh(USkeletalMesh* GarmentMesh);

	/** Finds <Name>_Sim (or any *_Sim skeletal mesh) in the same folder. */
	static USkeletalMesh* FindSimMeshFor(const USkeletalMesh* GarmentMesh);

	/** Checks whether the mesh carries vertex colors (ClothMask exported from Blender). */
	static bool HasClothMaskVertexColor(const USkeletalMesh* Mesh);

	/** Console command: GarmentCloth.CreateFrom <SkeletalMeshPath|Name> */
	static void ConsoleCreateFrom(const TArray<FString>& Args);
};
