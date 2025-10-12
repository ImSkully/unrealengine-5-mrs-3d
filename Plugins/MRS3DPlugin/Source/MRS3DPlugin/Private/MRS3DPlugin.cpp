#include "MRS3DPlugin.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FMRS3DPluginModule"

void FMRS3DPluginModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("MRS3D Plugin Module Started"));
}

void FMRS3DPluginModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("MRS3D Plugin Module Shutdown"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMRS3DPluginModule, MRS3DPlugin)
