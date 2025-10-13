using UnrealBuildTool;
using System.Collections.Generic;

public class BaseEditorTarget : TargetRules
{
	public BaseEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "BaseEditor" } );
	}
}
