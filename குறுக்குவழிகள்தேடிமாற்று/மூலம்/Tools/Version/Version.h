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

#pragma once

#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

class CVersion
{
protected:
    typedef struct tagLANGANDCODEPAGE 
    {
        WORD wLanguage;
        WORD wCodePage;
    }LANGANDCODEPAGE,*PLANGANDCODEPAGE;
    
    LANGANDCODEPAGE LangAndCodePage;
    BYTE* pVersionInfo;
    
    HMODULE hModule;
    BOOL bFunctionsLoaded;
    typedef BOOL (__stdcall *pfVerQueryValue)(LPCVOID pBlock,LPCTSTR lpSubBlock,LPVOID* lplpBuffer,PUINT puLen);
    typedef BOOL (__stdcall *pfGetFileVersionInfo)(LPCTSTR lptstrFilename,DWORD dwHandle,DWORD dwLen,LPVOID lpData);
    typedef DWORD (__stdcall *pfGetFileVersionInfoSize)(LPCTSTR lptstrFilename,LPDWORD lpdwHandle);


    pfGetFileVersionInfoSize pGetFileVersionInfoSize;
    pfGetFileVersionInfo pGetFileVersionInfo;
    pfVerQueryValue pVerQueryValue;
    BOOL LoadFunctions();
public:
    VS_FIXEDFILEINFO FixedFileInfo;
    DWORD MajorVersion;
    DWORD MinorVersion;

    TCHAR CompanyName[MAX_PATH];
    TCHAR FileDescription[MAX_PATH];
    TCHAR FileVersion[MAX_PATH];
    TCHAR InternalName[MAX_PATH];
    TCHAR LegalCopyright[MAX_PATH];
    TCHAR OriginalFilename[MAX_PATH];
    TCHAR ProductName[MAX_PATH];
    TCHAR ProductVersion[MAX_PATH];

    CVersion(void);
    ~CVersion(void);
    BOOL Read(LPCTSTR FileName);
    BOOL GetValue(LPCTSTR lpKeyName,LPTSTR szValue);
    static BOOL GetPrettyVersion(LPCTSTR lpVersion,IN const DWORD NbDigits,OUT LPTSTR PrettyVersion,IN const SIZE_T PrettyVersionMaxSize);
};
