#include "MRS3D.h"
#include "Modules/ModuleManager.h"

void FMRS3DModule::StartupModule()
{
	// This code will execute after your module is loaded into memory
	UE_LOG(LogTemp, Log, TEXT("MRS3D Module Started"));
}

void FMRS3DModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module
	UE_LOG(LogTemp, Log, TEXT("MRS3D Module Shutdown"));
}

IMPLEMENT_PRIMARY_GAME_MODULE(FMRS3DModule, MRS3D, "MRS3D");
