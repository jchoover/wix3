#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#include <windows.h>

#include "IBootstrapperEngine.h"

interface IBootstrapperBAFunction
{
    STDMETHOD(OnDetect)() = 0;
    STDMETHOD(OnDetectComplete)() = 0;
    STDMETHOD(OnPlan)() = 0;
    STDMETHOD(OnPlanComplete)() = 0;
	STDMETHOD(OnDetectRelatedBundle)(
        __in LPCWSTR wzBundleId,
        __in BOOTSTRAPPER_RELATION_TYPE relationType,
        __in LPCWSTR /*wzBundleTag*/,
        __in BOOL fPerMachine,
        __in DWORD64 /*dw64Version*/,
        __in BOOTSTRAPPER_RELATED_OPERATION operation
        ) = 0;
	STDMETHOD(OnPlanRelatedBundle)(__in_z LPCWSTR /*wzBundleId*/, __inout_z BOOTSTRAPPER_REQUEST_STATE* pRequestedState) = 0;
};

#ifdef __cplusplus
extern "C" {
#endif

typedef HRESULT (WINAPI *PFN_BOOTSTRAPPER_BA_FUNCTION_CREATE)(
    __in IBootstrapperEngine* pEngine,
    __in HMODULE hModule,
    __out IBootstrapperBAFunction** ppBAFunction
    );

#ifdef __cplusplus
}
#endif
