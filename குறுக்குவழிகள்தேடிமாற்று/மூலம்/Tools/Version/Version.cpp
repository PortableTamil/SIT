/*
Copyright (C) 2004-2016 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004-2016 Jacquelin POTIER <jacquelin.potier@free.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "version.h"

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

CVersion::CVersion(void)
{
    memset(&this->LangAndCodePage, 0, sizeof(LANGANDCODEPAGE));
    this->pVersionInfo = NULL;

    this->hModule = NULL;
    this->bFunctionsLoaded = FALSE;
    this->pGetFileVersionInfoSize   = NULL;
    this->pGetFileVersionInfo       = NULL;
    this->pVerQueryValue            = NULL;

    memset(&this->FixedFileInfo, 0, sizeof(VS_FIXEDFILEINFO));
    this->MajorVersion=0;
    this->MinorVersion=0;

    *this->CompanyName=0;
    *this->FileDescription=0;
    *this->FileVersion=0;
    *this->InternalName=0;
    *this->LegalCopyright=0;
    *this->OriginalFilename=0;
    *this->ProductName=0;
    *this->ProductVersion=0;
}

CVersion::~CVersion(void)
{
    if (this->pVersionInfo)
        delete [] this->pVersionInfo;

    if (this->hModule)
    {
        this->bFunctionsLoaded = FALSE;
        this->pGetFileVersionInfoSize   = NULL;
        this->pGetFileVersionInfo       = NULL;
        this->pVerQueryValue            = NULL;

        ::FreeLibrary(this->hModule);
        this->hModule = NULL;
    }
}

BOOL CVersion::LoadFunctions()
{
    if (this->hModule == NULL)
    {
        this->hModule = ::LoadLibrary(TEXT("Version.dll")); // use the old dll name to be compatible to XP, Seven, 8, 10 (as 7,8,10 have stub dll it's still safe to use old dll name)
        if (this->hModule == NULL)
            return FALSE;
    }

#if (defined(UNICODE)||defined(_UNICODE))
    #define CVersion_GetFileVersionInfoSize_NAME      "GetFileVersionInfoSizeW"
    #define CVersion_GetFileVersionInfo_NAME          "GetFileVersionInfoW"
    #define CVersion_VerQueryValue_NAME               "VerQueryValueW"
#else
    #define CVersion_GetFileVersionInfoSize_NAME      "GetFileVersionInfoSizeA"
    #define CVersion_GetFileVersionInfo_NAME          "GetFileVersionInfoA"
    #define CVersion_VerQueryValue_NAME               "VerQueryValueA"
#endif

    this->pGetFileVersionInfoSize   = (pfGetFileVersionInfoSize)::GetProcAddress(this->hModule,CVersion_GetFileVersionInfoSize_NAME);
    this->pGetFileVersionInfo       = (pfGetFileVersionInfo)::GetProcAddress(this->hModule,CVersion_GetFileVersionInfo_NAME);
    this->pVerQueryValue            = (pfVerQueryValue)::GetProcAddress(this->hModule,CVersion_VerQueryValue_NAME);
    this->bFunctionsLoaded = (this->pGetFileVersionInfoSize && this->pGetFileVersionInfo && this->pVerQueryValue);
    return this->bFunctionsLoaded;
}

BOOL CVersion::Read(LPCTSTR FileName)
{
    DWORD dwRet;
    UINT iLen;
    DWORD dwHandle;
    LPVOID lpvi;

    if (!this->bFunctionsLoaded)
    {
        if (!this->LoadFunctions())
            return FALSE;
    }

    // read file version info
    dwRet = this->pGetFileVersionInfoSize(FileName, &dwHandle);
    if (dwRet <= 0)
        return FALSE;

    // allocate version info
    if (this->pVersionInfo)
        delete[] this->pVersionInfo;
    this->pVersionInfo = new BYTE[dwRet]; 

    if (!this->pGetFileVersionInfo(FileName, 0, dwRet, this->pVersionInfo))
        return FALSE;
    memset(&this->FixedFileInfo, 0, sizeof(VS_FIXEDFILEINFO));



	if (!this->pVerQueryValue(this->pVersionInfo, _TEXT("\\"), &lpvi, &iLen))
        return FALSE;
    memcpy(&this->FixedFileInfo,lpvi,sizeof(VS_FIXEDFILEINFO));


    
#if (defined(UNICODE)||defined(_UNICODE))
    this->LangAndCodePage.wCodePage = 1200;// code page unicode
#else
    this->LangAndCodePage.wCodePage = 1252;// code page us ASCII
#endif
    // Get translation info
    if (!this->pVerQueryValue(this->pVersionInfo,_TEXT("\\VarFileInfo\\Translation"),&lpvi, &iLen))
        return FALSE;

    // memcpy(&this->LangAndCodePage,lpvi,iLen);
    memcpy(&this->LangAndCodePage,lpvi,sizeof(LANGANDCODEPAGE));

    this->GetValue(_TEXT("CompanyName"),this->CompanyName);
    this->GetValue(_TEXT("FileDescription"),this->FileDescription);
    this->GetValue(_TEXT("FileVersion"),this->FileVersion);
    this->GetValue(_TEXT("InternalName"),this->InternalName);
    this->GetValue(_TEXT("LegalCopyright"),this->LegalCopyright);
    this->GetValue(_TEXT("OriginalFilename"),this->OriginalFilename);
    this->GetValue(_TEXT("ProductName"),this->ProductName);
    this->GetValue(_TEXT("ProductVersion"),this->ProductVersion);

    // the more portable stuff as VS_FIXEDFILEINFO masks depends on 64 or 32 bit processes
    this->MajorVersion=0;
    this->MinorVersion=0;
#pragma warning (push)
#pragma warning(disable : 6031)
    _stscanf(this->FileVersion,TEXT("%u.%u"),&this->MajorVersion,&this->MinorVersion);
#pragma warning (pop)

    // check signature (must be VS_FFI_SIGNATURE)
    return (this->FixedFileInfo.dwSignature == VS_FFI_SIGNATURE);
}


// Get string file info.
// Key name is something like "CompanyName".
BOOL CVersion::GetValue(LPCTSTR lpKeyName,LPTSTR szValue)
{
    TCHAR sz[MAX_PATH];
    UINT iLenVal;
    LPVOID lpvi;

    if (!this->bFunctionsLoaded)
    {
        if (!this->LoadFunctions())
            return FALSE;
    }

    if (!this->pVersionInfo)
        return FALSE;

    if (IsBadWritePtr(szValue,1))
        return FALSE;

    if (IsBadReadPtr(lpKeyName,1))
        return FALSE;

    *szValue=0;

    // To get a string, value must pass query in the form
    //
    //    "\StringFileInfo\<langID><codepage>\keyname"
    //
    // where <lang-codepage> is the languageID concatenated with the
    // code page, in hex. Wow.
    //

    _sntprintf(sz,_countof(sz),_TEXT("\\StringFileInfo\\%04x%04x\\%s"),
        this->LangAndCodePage.wLanguage,
        this->LangAndCodePage.wCodePage,
        lpKeyName);

    if(!this->pVerQueryValue(this->pVersionInfo,sz,&lpvi,&iLenVal))
        return FALSE;
    if (iLenVal>MAX_PATH)
        iLenVal=MAX_PATH;
    memcpy(szValue,lpvi,iLenVal*sizeof(TCHAR));

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetPrettyVersion
// Object: translate something like "1 ,2 ,3 ,4" to "1.2.3.4" notice :  according to windows version, version delimiter can be " ," or "."
// Parameters :
//     in  : LPCTSTR lpVersion : ProductVersion or FileVersion
//           DWORD NbDigits : number of wanted digits
//           SIZE_T PrettyVersionMaxSize : max size of translated string version
//     out : LPTSTR PrettyVersion : translated string version
//     return : 
//-----------------------------------------------------------------------------
BOOL CVersion::GetPrettyVersion(IN LPCTSTR lpVersion,IN const DWORD NbDigits,OUT LPTSTR PrettyVersion,IN const SIZE_T PrettyVersionMaxSize)
{
    LPTSTR pszBegin;
    LPTSTR pszEnd;
    DWORD NbDigitsAdded;
    SIZE_T RemainingMaxSize;

    
    RemainingMaxSize=PrettyVersionMaxSize-1;// -1 for \0
    *PrettyVersion=0;
    pszBegin=(LPTSTR)lpVersion;

    pszEnd=_tcschr(pszBegin,',');
    if (pszEnd == NULL)// according to windows version, version delimiter can be " ," or "."
    {
        _tcscpy(PrettyVersion,pszBegin);
        pszEnd = PrettyVersion;
        for (NbDigitsAdded=0;
            (NbDigitsAdded<NbDigits) && pszEnd;
            NbDigitsAdded++)
        {
            pszEnd=_tcschr(pszEnd+1,'.');
        }
        if (pszEnd == NULL)
            return FALSE;
        // else
        *pszEnd = 0;
        return TRUE;
    }
    // else

    for (NbDigitsAdded=0;
         NbDigitsAdded<NbDigits;
         NbDigitsAdded++)
    {
        // find next ','
        pszEnd=_tcschr(pszBegin,',');

        // if not found
        if (pszEnd == NULL)
        {
            // if remaining content can't be inserted in buffer
            if (_tcslen(pszBegin)+1>RemainingMaxSize)// +1 for '.' splitter
                return FALSE;

            // if not first digit (main version number)
            if (NbDigitsAdded)
            {
                // add digit delimiter
                _tcscat(PrettyVersion,_T("."));
            }

            // add remaining content to buffer
            _tcscat(PrettyVersion,pszBegin);

            // remaining content is supposed to contains a digit
            // so increase NbDigitsAdded
            NbDigitsAdded++;

            // if NbDigitsAdded match the wanted digit number
            if (NbDigitsAdded==NbDigits)
                return TRUE;
            else
                return FALSE;
        }

        // if next digit content can't be inserted in buffer
        if ((SIZE_T)(pszEnd-pszBegin)+1>RemainingMaxSize) // +1 for '.' splitter
            return FALSE;

        // if not first digit (main version number)
        if (NbDigitsAdded)
        {
            // add digit delimiter
            _tcscat(PrettyVersion,_T("."));
        }

        // add next digit to buffer
        _tcsncat(PrettyVersion,pszBegin,pszEnd-pszBegin);

        // update remaining size
        RemainingMaxSize-=(SIZE_T)(pszEnd-pszBegin);

        // remove ',' and space
        pszBegin=pszEnd+2;
    }
    return TRUE;
}