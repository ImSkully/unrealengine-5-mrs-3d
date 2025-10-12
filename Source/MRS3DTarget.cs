using UnrealBuildTool;
using System.Collections.Generic;

public class MRS3DTarget : TargetRules
{
	public MRS3DTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange(new string[] { "MRS3D" });
	}
}
