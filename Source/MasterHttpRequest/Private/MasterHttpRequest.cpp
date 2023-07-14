// Copyright Epic Games, Inc. All Rights Reserved.
/*
==========================================================================================
File: MasterHttpRequest.cpp
Author: Mario Tarosso
Year: 2023
Publisher: MJGT Studio
==========================================================================================
*/
#include "MasterHttpRequest.h"

#define LOCTEXT_NAMESPACE "FMasterHttpRequestModule"

void FMasterHttpRequestModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

}

void FMasterHttpRequestModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMasterHttpRequestModule, MasterHttpRequest)
