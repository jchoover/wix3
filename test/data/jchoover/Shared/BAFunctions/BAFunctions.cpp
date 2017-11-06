// Caps.BAFunction.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#pragma comment(lib, "balutil")
#pragma comment(lib, "dutil")
#pragma comment(lib, "shlwapi")

static const LPCWSTR BUNDLE_INI_FILE_PATH_FORMAT = L"%ls\\Setup.ini";
static const LPCWSTR BUNDLE_INI_FILE_SECTION_CONFIG = L"Config";
static const LPCWSTR BUNDLE_INI_FILE_KEY_AUTOMATIC_UPDATE = L"AutomaticUpdates";

static const LPCWSTR BUNDLE_VARIABLE_AUTOMATIC_UPDATE = L"AUTOMATICUPDATE";
static const LPCWSTR BUNDLE_VARIABLE_PRIOR_INSTALL_DIR = L"PRIOR_INSTALL_DIR";
static const LPCWSTR BUNDLE_VARIABLE_REMOVE_OLDER_VERSIONS = L"REMOVE_OLDER_VERSIONS";

static const LPCWSTR WIXSTDBA_VARIABLE_BUNDLE_FILE_VERSION = L"WixBundleFileVersion";

typedef struct _RELATED_BUNDLE
{
    LPWSTR wzBundleId;
    BOOTSTRAPPER_RELATION_TYPE relationType;
    LPWSTR wzBundleTag;
    BOOL fPerMachine;
    DWORD64 dw64Version;
    BOOTSTRAPPER_RELATED_OPERATION operation;
} RELATED_BUNDLE;

typedef struct _RELATED_BUNDLES
{
    RELATED_BUNDLE* rgRelatedBundles;
    DWORD cRelatedBundles;
} RELATED_BUNDLES;

class CWixBootstrapperBAFunction : IBootstrapperBAFunction
{
public:
    STDMETHODIMP OnDetect() { return S_OK; }
    //STDMETHODIMP OnDetectComplete() { return S_OK; }
    //STDMETHODIMP OnPlan() { return S_OK; }
    //STDMETHODIMP OnPlanComplete() { return S_OK; }


    STDMETHODIMP OnDetectComplete()
    {
        HRESULT hr = S_OK;
        LPWSTR wzPriorAppDataDir = NULL;
        LPWSTR wzValue = NULL;
        LONGLONG llValue = 0;

        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Running detect complete BA function");
        //-------------------------------------------------------------------------------------------------
        // Probing for install ini in order: Bundle Dir, Install Dir.
        hr = this->ReadIniFromInstallLocation();
        if (FAILED(hr) && (hr != E_FILENOTFOUND))
        {
            BalExitOnFailure(hr, "Failed to read install ini.");
        }

        if ((E_FILENOTFOUND == hr) && BalStringVariableExists(BUNDLE_VARIABLE_PRIOR_INSTALL_DIR))
        {
            hr = BalGetStringVariable(BUNDLE_VARIABLE_PRIOR_INSTALL_DIR, &wzPriorAppDataDir);
            BalExitOnFailure(hr, "Failed to read the value of %ls.", BUNDLE_VARIABLE_PRIOR_INSTALL_DIR);

            hr = this->ReadIniFromPriorInstall(wzPriorAppDataDir);
            if (FAILED(hr) && (hr != E_FILENOTFOUND))
            {
                BalExitOnFailure(hr, "Failed to read install ini.");
            }
            hr = S_OK;
        }


        // -
        if (m_wzBundleId)
        {
            BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Related bundle %ls found, asking for shared variable.", m_wzBundleId);

            hr = BalGetRelatedBundleStringVariable(m_wzBundleId, L"InstallOptionA", &wzValue);
            BalExitOnFailure(hr, "Unable to read related bundle %ls shared variable 'InstallOptionA'.", m_wzBundleId);

            BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Shared variable has a value of %ls.", wzValue);

            hr = BalGetRelatedBundleNumericVariable(m_wzBundleId, L"InstallOptionB", &llValue);
            BalExitOnFailure(hr, "Unable to read related bundle %ls shared variable 'InstallOptionB'.", m_wzBundleId);

            BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Shared variable has a value of %I64d.", llValue);
        }
        //-------------------------------------------------------------------------------------------------
    LExit:
        ReleaseStr(wzValue);
        ReleaseStr(wzPriorAppDataDir);
        return hr;
    }
    
    STDMETHODIMP OnPlan()
    {
        HRESULT hr = S_OK;
        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Running plan BA function");
        //-------------------------------------------------------------------------------------------------
        hr = BalGetVersionVariable(WIXSTDBA_VARIABLE_BUNDLE_FILE_VERSION, &m_qwBundleVersion);
        BalExitOnFailure(hr, "Failed to read bundle version.");
        //-------------------------------------------------------------------------------------------------
    LExit:

        return hr;
    }
    
    STDMETHODIMP OnPlanComplete()
    {
        HRESULT hr = S_OK;
        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "Running plan complete BA function");
        //-------------------------------------------------------------------------------------------------
        UninitializeRelatedBundles(&m_RelatedBundles);
        BalExitOnFailure(hr, "Change this message to represent real error handling.");
        //-------------------------------------------------------------------------------------------------
    LExit:
        return hr;
    }
    

    STDMETHODIMP OnDetectRelatedBundle(
        __in LPCWSTR wzBundleId,
        __in BOOTSTRAPPER_RELATION_TYPE relationType,
        __in LPCWSTR wzBundleTag,
        __in BOOL fPerMachine,
        __in DWORD64 dw64Version,
        __in BOOTSTRAPPER_RELATED_OPERATION operation
    )
    {
        HRESULT hr = S_OK;
        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "OnDetectRelatedBundle BA function: %ls", wzBundleId);

        if (m_qwRelatedBundleVersion < dw64Version)
        {
            hr = StrAllocString(&m_wzBundleId, wzBundleId, 0);
            BalExitOnFailure(hr, "Failed to alloc memory for releated bundle id.");
            m_qwRelatedBundleVersion = dw64Version;
        }

        hr = CacheRelatedBundle(wzBundleId, relationType, wzBundleTag, fPerMachine, dw64Version, operation);
    LExit:
        return hr;
    }

    STDMETHODIMP OnPlanRelatedBundle(__in_z LPCWSTR wzBundleId, __inout_z BOOTSTRAPPER_REQUEST_STATE* pRequestedState)
    {
        HRESULT hr = S_OK;
        RELATED_BUNDLE* pRelatedBundle = NULL;

        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "OnPlanRelatedBundle BA function: %ls", wzBundleId);

        hr = GetRelatedBundle(wzBundleId, &pRelatedBundle);
        ExitOnFailure(hr, "Planning bundle that wasn't detected.");

        if (BOOTSTRAPPER_REQUEST_STATE_ABSENT == *pRequestedState)
        {
            LONGLONG llRemoveOld = 0;
            hr = BalGetNumericVariable(BUNDLE_VARIABLE_REMOVE_OLDER_VERSIONS, &llRemoveOld);
            ExitOnFailure(hr, "Failed to read bundle variable %ls.", BUNDLE_VARIABLE_REMOVE_OLDER_VERSIONS);

            if (0 == llRemoveOld)
            {
                // If we are planning on removing the related bundle, look to see if it is a different Major.Minor version of ourselves
                // If so, leave it be.
                WORD wMajor = 0;
                WORD wMinor = 0;
                // WORD wBuild = 0;
                // WORD wRevision = 0;

                // Mask and shift each WORD for each field.
                wMajor = (WORD)(m_qwBundleVersion >> 48 & 0xffff);
                wMinor = (WORD)(m_qwBundleVersion >> 32 & 0xffff);
                // wBuild = (WORD)(pRelatedBundle->dw64Version >> 16 & 0xffff);
                // wRevision = (WORD)(pRelatedBundle->dw64Version & 0xffff);

                WORD wRelatedMajor = 0;
                WORD wRelatedMinor = 0;
                // WORD wRelatedBuild = 0;
                // WORD wRelatedRevision = 0;

                // Mask and shift each WORD for each field.
                wRelatedMajor = (WORD)(pRelatedBundle->dw64Version >> 48 & 0xffff);
                wRelatedMinor = (WORD)(pRelatedBundle->dw64Version >> 32 & 0xffff);
                // wBuild = (WORD)(pRelatedBundle->dw64Version >> 16 & 0xffff);
                // wRevision = (WORD)(pRelatedBundle->dw64Version & 0xffff);

                if ((wMajor != wRelatedMajor) && (wMinor != wRelatedMinor))
                {
                    *pRequestedState = BOOTSTRAPPER_REQUEST_STATE_PRESENT;
                }
            }
        }
    LExit:
        return hr;
    }

    STDMETHODIMP OnProcessCommandLine(__in_z LPCWSTR wzCommandLine, __in STRINGDICT_HANDLE sdOverridden)
    {
        HRESULT hr = S_OK;
        BalLog(BOOTSTRAPPER_LOG_LEVEL_STANDARD, "OnProcessCommandLine BA function: %ls", wzCommandLine);

        if (sdOverridden)
        {
            hr = DictDuplicateStringList(&m_sdOverridden, sdOverridden);
            BalExitOnFailure(hr, "Failed to duplicate Overridden variables dict.");
        }

    LExit:
        return hr;
    }
private:
    HMODULE m_hModule;
    IBootstrapperEngine* m_pEngine;

    DWORD64 m_qwBundleVersion;
    DWORD64 m_qwRelatedBundleVersion;
    LPWSTR m_wzBundleId;
    STRINGDICT_HANDLE m_sdOverridden;
    RELATED_BUNDLES m_RelatedBundles;

    void UninitializeRelatedBundles(RELATED_BUNDLES* pRelatedBundles)
    {
        for (DWORD i = 0; i < pRelatedBundles->cRelatedBundles; ++i)
        {
            
            ReleaseStr(pRelatedBundles->rgRelatedBundles[i].wzBundleId);
            ReleaseStr(pRelatedBundles->rgRelatedBundles[i].wzBundleTag);
        }

        ReleaseMem(pRelatedBundles->rgRelatedBundles);

        memset(pRelatedBundles, 0, sizeof(RELATED_BUNDLES));
    }

    STDMETHODIMP CacheRelatedBundle(
        __in LPCWSTR wzBundleId,
        __in BOOTSTRAPPER_RELATION_TYPE relationType,
        __in LPCWSTR wzBundleTag,
        __in BOOL fPerMachine,
        __in DWORD64 dw64Version,
        __in BOOTSTRAPPER_RELATED_OPERATION operation
    )
    {
        RELATED_BUNDLE* pBundle = NULL;
        HRESULT hr = S_OK;

        // Check to see if the bundle is already in the list.
        for (DWORD i = 0; i < m_RelatedBundles.cRelatedBundles; ++i)
        {
            if (CSTR_EQUAL == ::CompareStringW(LOCALE_NEUTRAL, 0, wzBundleId, -1, m_RelatedBundles.rgRelatedBundles[i].wzBundleId, -1))
            {
                ExitFunction1(hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));
            }
        }

        // Add the related bundle as a package.
        hr = MemEnsureArraySize(reinterpret_cast<LPVOID*>(&m_RelatedBundles.rgRelatedBundles), m_RelatedBundles.cRelatedBundles + 1, sizeof(RELATED_BUNDLE), 2);
        ExitOnFailure(hr, "Failed to allocate memory for related bundle package information.");

        pBundle = m_RelatedBundles.rgRelatedBundles + m_RelatedBundles.cRelatedBundles;
        ++m_RelatedBundles.cRelatedBundles;

        hr = StrAllocString(&pBundle->wzBundleId, wzBundleId, 0);
        ExitOnFailure(hr, "Failed to copy related bundle package id.");

        pBundle->relationType = relationType;

        hr = StrAllocString(&pBundle->wzBundleTag, wzBundleTag, 0);
        ExitOnFailure(hr, "Failed to copy related bundle tag.");

        pBundle->fPerMachine = fPerMachine;
        pBundle->dw64Version = dw64Version;
        pBundle->operation = operation;
    LExit:

        return hr;
    }

    STDMETHODIMP GetRelatedBundle(__in LPCWSTR wzBundleId, __out RELATED_BUNDLE** ppBundle)
    {
        HRESULT hr = S_OK;
        ExitOnNull(ppBundle, hr, E_INVALIDARG, "Handle not specified when requesting related bundle info.");

        for (DWORD i = 0; i < m_RelatedBundles.cRelatedBundles; ++i)
        {
            if (CSTR_EQUAL == ::CompareStringW(LOCALE_NEUTRAL, 0, wzBundleId, -1, m_RelatedBundles.rgRelatedBundles[i].wzBundleId, -1))
            {
                *ppBundle = &m_RelatedBundles.rgRelatedBundles[i];
                ExitFunction1(hr = S_OK);
            }
        }

        hr = E_NOTFOUND;
    LExit:
        return hr;
    }

    STDMETHODIMP ReadIniFromInstallLocation()
    {
        HRESULT hr = S_OK;
        LPWSTR wzOrigionalSource = NULL;
        LPWSTR wzIniFile = NULL;

        hr = BalGetStringVariable(L"WixBundleOriginalSource", &wzOrigionalSource);
        BalExitOnFailure(hr, "Unable to resolve WixBundleOriginalSource.");

        hr = StrAllocFormatted(&wzIniFile, L"%ls\\Setup.ini", wzOrigionalSource);
        BalExitOnFailure(hr, "Unable to allocate ini file path.");

        hr = ReadIni(wzIniFile);
    LExit:
        ReleaseStr(wzOrigionalSource);

        return hr;
    }

    STDMETHODIMP ReadIniFromPriorInstall(__in_z LPCWSTR wzInstallRoot)
    {
        HRESULT hr = S_OK;
        LPWSTR wzIniFile = NULL;

        hr = StrAllocFormatted(&wzIniFile, BUNDLE_INI_FILE_PATH_FORMAT, wzInstallRoot);
        BalExitOnFailure(hr, "Unable to allocate ini file path.");

        hr = ReadIni(wzIniFile);
    LExit:
        ReleaseStr(wzIniFile);

        return hr;

    }
    STDMETHODIMP ReadIni(__in_z LPCWSTR wzIniFile)
    {
        HRESULT hr = S_OK;
        bool fAutomaticUpdate = true;
        bool fUseNetworkData = false;
        LPWSTR wzNetworkFolder = NULL;

        if (PathFileExists(wzIniFile))
        {
            hr = HasOverride(BUNDLE_VARIABLE_AUTOMATIC_UPDATE);
            if (E_NOTFOUND == hr)
            {
                int iAutomaticUpdate = ::GetPrivateProfileInt(BUNDLE_INI_FILE_SECTION_CONFIG, BUNDLE_INI_FILE_KEY_AUTOMATIC_UPDATE, 1, wzIniFile);

                hr = BalSetNumericVariable(BUNDLE_VARIABLE_AUTOMATIC_UPDATE, iAutomaticUpdate);
                BalExitOnFailure(hr, "Unable to set variable %ls.", BUNDLE_VARIABLE_AUTOMATIC_UPDATE);
            }
            else if (FAILED(hr))
            {
                ExitFunction();
            }
        }
        else
        {
            hr = E_FILENOTFOUND;
        }

    LExit:
        ReleaseStr(wzNetworkFolder);
        return hr;
    }

    STDMETHODIMP HasOverride(LPCWSTR wzVariable)
    {
        HRESULT hr = E_NOTFOUND;

        if (m_sdOverridden)
        {
            hr = DictKeyExists(m_sdOverridden, wzVariable);
            if (E_NOTFOUND == hr)
            {
                ExitFunction();
            }
            BalExitOnFailure(hr, "Unable to query override dict for value.");
        }
    LExit:
        return hr;
    }
public:
    //
    // Constructor - initialize member variables.
    //
    CWixBootstrapperBAFunction(
        __in IBootstrapperEngine* pEngine,
        __in HMODULE hModule
    )
    {
        m_hModule = hModule;
        m_pEngine = pEngine;
        m_wzBundleId = NULL;
        m_qwBundleVersion = 0;
        m_qwRelatedBundleVersion = 0;
        m_sdOverridden = NULL;
        memset(&m_RelatedBundles, 0, sizeof(m_RelatedBundles));

    }

    //
    // Destructor - release member variables.
    //
    ~CWixBootstrapperBAFunction()
    {
        ReleaseNullStr(m_wzBundleId);
        ReleaseDict(m_sdOverridden);
        UninitializeRelatedBundles(&m_RelatedBundles);
    }
};


extern "C" HRESULT WINAPI CreateBootstrapperBAFunction(
    __in IBootstrapperEngine* pEngine,
    __in HMODULE hModule,
    __out CWixBootstrapperBAFunction** ppBAFunction
)
{
    HRESULT hr = S_OK;
    CWixBootstrapperBAFunction* pBAFunction = NULL;

    // This is required to enable logging functions
    BalInitialize(pEngine);

    pBAFunction = new CWixBootstrapperBAFunction(pEngine, hModule);
    ExitOnNull(pBAFunction, hr, E_OUTOFMEMORY, "Failed to create new bootstrapper BA function object.");

    *ppBAFunction = pBAFunction;
    pBAFunction = NULL;

LExit:
    delete pBAFunction;
    return hr;
}
