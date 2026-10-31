using UnrealBuildTool;

public class SimCore : ModuleRules
{
    public SimCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Public: our headers (Public/) include these,
        // so any module that uses SimCore needs them too
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine"
        });
    }
}
