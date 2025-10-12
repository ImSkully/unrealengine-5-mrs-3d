using UnrealBuildTool;
using System.Collections.Generic;

public class MRS3DEditorTarget : TargetRules
{
	public MRS3DEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange(new string[] { "MRS3D" });
	}
}
