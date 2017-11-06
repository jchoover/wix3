// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
//#include <gdiplus.h>
//#include <msiquery.h>
#include <objbase.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <strsafe.h>

#include "dutil.h"
#include "strutil.h"
#include "dictutil.h"
#include "memutil.h"

#include "BalBaseBootstrapperApplication.h"
#include "balutil.h"
#include "IBootstrapperEngine.h"
#include "IBootstrapperApplication.h"
#include "IBootstrapperBAFunction.h"
