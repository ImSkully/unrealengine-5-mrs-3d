using UnrealBuildTool;
using System.Collections.Generic;

public class BaseTarget : TargetRules
{
	public BaseTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "Base" } );
	}
}
