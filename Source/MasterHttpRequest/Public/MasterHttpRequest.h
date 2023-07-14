// Copyright Epic Games, Inc. All Rights Reserved.
/*
==========================================================================================
File: MasterHttpRequest.h
Author: Mario Tarosso
Year: 2023
Publisher: MJGT Studio
==========================================================================================
*/
#pragma once

#include "Modules/ModuleManager.h"

class FMasterHttpRequestModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
