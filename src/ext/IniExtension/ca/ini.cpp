//-------------------------------------------------------------------------------------------------
// <copyright file="ini.cpp" company="Outercurve Foundation">
//   Copyright (c) 2004, Outercurve Foundation.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the root directory of the distribution.
// </copyright>
// 
// <summary>
//    Ini custom action code.
// </summary>
//-------------------------------------------------------------------------------------------------

#include "precomp.h"

LPCWSTR vcsIniSearchQuery =
    L"SELECT `WixIniSearch`.`WixIniSearch`,   `WixIniSearch`.`Name`,   `WixIniSearch`.`Section`, "
    L"`WixIniSearch`.`Key`, `WixIniSearch`.`Value` "
    L"FROM `WixIniSearch`";

enum eIniSearchQuery { eiqInstanceId = 1, eiqFile, eiqSection, eiqKey, eiqValue };

extern "C" HRESULT ReadIni(
    __in LPCWSTR wzFile,
    __in LPCWSTR wzSection,
    __in LPCWSTR wzKey,
    __in LPWSTR* ppwzValue)
{
    HRESULT hr = S_OK;
    LPWSTR pwzValue = NULL;
    DWORD dwLength = 0;
    DWORD dwAllocatedLength = 10;

    do
    {
        dwAllocatedLength*=2;

        StrAlloc(&pwzValue, dwAllocatedLength);
        ExitOnFailure(hr, "Failed to allocate value buffer");

        dwLength = ::GetPrivateProfileStringW(wzSection, wzKey, NULL, pwzValue, dwAllocatedLength, wzFile );

        if (0 == dwLength)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ExitOnFailure(hr, "failed to read ini");
        }
    } while(dwLength == dwAllocatedLength - 1);

    hr =  StrAllocConcat(ppwzValue, pwzValue, dwLength);
    ExitOnFailure(hr, "Failed to copy value buffer");

LExit:
    ReleaseStr(pwzValue);
    
    return hr;
}

/******************************************************************
 ExecIniSearch - entry point for Ini Search Custom Action

*******************************************************************/
extern "C" UINT __stdcall ExecIniSearch(
    __in MSIHANDLE hInstall
    )
{
    HRESULT hr = S_OK;
    UINT er = ERROR_SUCCESS;
    int cSearches = 0;

    PMSIHANDLE hView = NULL;
    PMSIHANDLE hRec = NULL;

    LPWSTR pwzIniSearch = NULL;
    LPWSTR pwzFile = NULL;
    LPWSTR pwzFormattedFile = NULL;
    LPWSTR pwzSection = NULL;
    LPWSTR pwzKey = NULL;
    LPWSTR pwzValue = NULL;
    LPWSTR pwzValueProperty = NULL;
    
    // AssertSz(FALSE, "debug ExecIniSearch here.");

    // initialize
    hr = WcaInitialize(hInstall, "ExecIniSearch");
    ExitOnFailure(hr, "failed to initialize");

    // anything to do?
    if (S_OK != WcaTableExists(L"WixIniSearch"))
    {
        WcaLog(LOGMSG_STANDARD, "WixIniSearch table doesn't exist, so there are no ini files to read");
        goto LExit;
    }

    // query and loop through all the ini searches
    hr = WcaOpenExecuteView(vcsIniSearchQuery, &hView);
    ExitOnFailure(hr, "failed to open view on WixIniSearch table");

    while (S_OK == (hr = WcaFetchRecord(hView, &hRec)))
    {
        ++cSearches;

        // start with the instance guid
        hr = WcaGetRecordString(hRec, eiqInstanceId, &pwzIniSearch);
        ExitOnFailure(hr, "failed to get ini instance id");

        // get file 
        hr = WcaGetRecordString(hRec, eiqFile, &pwzFile);
        ExitOnFailure(hr, "failed to get ini file");

        // turn that into the path to the target file
        hr = WcaGetFormattedString(pwzFile, &pwzFormattedFile);
        ExitOnFailure1(hr, "failed to get formatted string for file: %ls", pwzFile);

        // get section
        hr = WcaGetRecordString(hRec, eiqSection, &pwzSection);
        ExitOnFailure(hr, "failed to get section");

        // get key
        hr = WcaGetRecordString(hRec, eiqKey, &pwzKey);
        ExitOnFailure(hr, "failed to get key");

        // get value
        hr = WcaGetRecordString(hRec, eiqValue, &pwzValueProperty);
        ExitOnFailure(hr, "failed to get value");

        hr = ReadIni(pwzFormattedFile, pwzSection, pwzKey, &pwzValue);
        ExitOnFailure(hr, "failed to read ini");

        hr = WcaSetProperty(pwzValueProperty, pwzValue);
        ExitOnFailure(hr, "failed to set property value");
    }

    // reaching the end of the list is actually a good thing, not an error
    if (E_NOMOREITEMS == hr)
    {
        hr = S_OK;
    }
    ExitOnFailure(hr, "Failure occured while processing WixIniSearch table");

LExit:
    ReleaseStr(pwzIniSearch);
    ReleaseStr(pwzFile);
    ReleaseStr(pwzFormattedFile);
    ReleaseStr(pwzSection);
    ReleaseStr(pwzKey);
    ReleaseStr(pwzValueProperty);
    ReleaseStr(pwzValue);

    return WcaFinalize(er = FAILED(hr) ? ERROR_INSTALL_FAILURE : er);
}