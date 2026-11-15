// Copyright (c) 2026. MIT License.

using UnrealBuildTool;

public class GarmentCloth : ModuleRules
{
	public GarmentCloth(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"AssetTools",
			"AssetRegistry",
			"ContentBrowser",
			"ContentBrowserData",
			"EditorSubsystem",
			"ToolMenus",
			"Slate",
			"SlateCore",
			"EditorFramework",
			"Projects",
			"InputCore",
			"ApplicationCore",
			"EditorScriptingUtilities",
		});

		// 说明: 这里**故意不**硬依赖 ChaosClothAsset / ChaosClothAssetEditor。
		// 插件用反射在运行时找 UChaosClothAsset 和它的 Factory,
		// 这样就算 5.8 里模块名/类名有变化, 也只是功能降级, 不会导致工程编不过。
	}
}
