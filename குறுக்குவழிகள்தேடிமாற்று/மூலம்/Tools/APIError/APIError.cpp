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

//-----------------------------------------------------------------------------
// Object: class helper for displaying a messagebox containing the win32 error message 
//         after an API failure
//         As this class is used in many other tools class, and if you don't want to 
//         report error to the user, just define the keyword "TOOLS_NO_MESSAGEBOX"
//         in preprocessor options
//-----------------------------------------------------------------------------


#include "APIError.h"
#include <stdio.h>

#ifndef _countof
	#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

HWND CAPIError::hParentWindow = NULL;
CAPIError::pfRtlNtStatusToDosError CAPIError::pRtlNtStatusToDosError = NULL;

#ifndef TOOLS_NO_MESSAGEBOX
int WINAPI CAPIError::MessageBoxModelessProtection(HWND hWnd,LPCTSTR lpText,LPCTSTR lpCaption,UINT uType)
{
    int iRet;
    if (CAPIError::hParentWindow)
    {
        BOOL bRootDisabled = FALSE;
        BOOL bRootOwnerDisabled = FALSE;
        HWND hWndRoot = ::GetAncestor(CAPIError::hParentWindow, GA_ROOT);
        HWND hWndRootOwner = ::GetAncestor(CAPIError::hParentWindow, GA_ROOTOWNER);
        if (::IsWindowEnabled(hWndRoot))
        {
            bRootDisabled = TRUE;
            ::EnableWindow(hWndRoot,FALSE);
        }
        if (::IsWindowEnabled(hWndRootOwner))
        {
            bRootOwnerDisabled = TRUE;
            ::EnableWindow(hWndRootOwner,FALSE);
        }

        iRet = ::MessageBox(hWnd,lpText,lpCaption,uType);

        if (bRootDisabled)
            ::EnableWindow(hWndRoot,TRUE);
        if (bRootOwnerDisabled)
            ::EnableWindow(hWndRootOwner,TRUE);
    }
    else
        iRet = ::MessageBox(hWnd,lpText,lpCaption,uType);

    return iRet;
}
#endif

void CAPIError::SetParentWindowHandle(HWND ParentWindow)
{
    CAPIError::hParentWindow = ParentWindow;
}
ULONG CAPIError::NtStatusToDosError(NTSTATUS Status)
{
    if (!CAPIError::pRtlNtStatusToDosError)
    {
        HMODULE hModule = ::GetModuleHandle(_T("ntdll.dll"));
        CAPIError::pRtlNtStatusToDosError = NULL;
        if (hModule)
            CAPIError::pRtlNtStatusToDosError=(pfRtlNtStatusToDosError)::GetProcAddress(hModule,"RtlNtStatusToDosError");
        if (CAPIError::pRtlNtStatusToDosError==NULL)
            return ERROR_INVALID_FUNCTION;
    }
    return CAPIError::pRtlNtStatusToDosError(Status);
}


BOOL CAPIError::GetLastErrorMessage(OUT TCHAR* Error,IN SIZE_T ErrorMaxSize)
{
    return CAPIError::GetErrorMessage(::GetLastError(),Error,ErrorMaxSize);
}

BOOL CAPIError::GetLastErrorMessage(OUT TCHAR* Error,IN SIZE_T ErrorMaxSize,IN TCHAR* Prefix)
{
    return CAPIError::GetErrorMessageEx(::GetLastError(),NULL,Error,ErrorMaxSize,Prefix,FALSE);
}

BOOL CAPIError::GetErrorMessage(IN DWORD dwErrorNum,OUT TCHAR* Error,IN SIZE_T ErrorMaxSize)
{
    return CAPIError::GetErrorMessageEx(dwErrorNum,NULL,Error,ErrorMaxSize,TEXT(""),FALSE);
}

BOOL CAPIError::GetErrorMessageEx(IN DWORD dwErrorNum,IN HMODULE hModule,OUT TCHAR* Error,IN SIZE_T ErrorMaxSize,IN TCHAR* Prefix,IN BOOL bLineBreakAfterPrefix)
{
    TCHAR* Format = TEXT("%s ");
    if (bLineBreakAfterPrefix)
        Format = TEXT("%s\r\n");
    int NbChars = _sntprintf(Error,ErrorMaxSize,Format,Prefix);

    DWORD dw=::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                           hModule,
                           dwErrorNum,
                            //If you pass a specific LANGID in this parameter, FormatMessage will return a message for that LANGID only. 
                            //If the function cannot find a message for that LANGID, it sets Last-Error to ERROR_RESOURCE_LANG_NOT_FOUND. 
                            //If you pass in zero, FormatMessage looks for a message for LANGIDs in the following order:
                            //    Language neutral
                            //    Thread LANGID, based on the thread's locale value
                            //    User default LANGID, based on the user's default locale value
                            //    System default LANGID, based on the system default locale value
                            //    US English
                           0, // GetUserDefaultLangID(),//GetSystemDefaultLangID()
                           &Error[NbChars],
                           (DWORD)(ErrorMaxSize-1-NbChars),
                           NULL);
    //If the function succeeds, the return value is the number of TCHARs stored in the output buffer,
    //  excluding the terminating null character.
    //If the function fails, the return value is zero
    if(dw==0)
    {
        // FormatMessage failed       
        _sntprintf(Error,ErrorMaxSize,TEXT("%s Error 0x%08X"),Prefix,dwErrorNum);
        return FALSE;
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: ShowLastError
// Object: show last windows api error
// Parameters :
//     in  :
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CAPIError::ShowLastError(TCHAR* szPrefix,HWND ParentWindow)
{
#if (defined(TOOLS_NO_MESSAGEBOX))
    UNREFERENCED_PARAMETER(szPrefix);
    UNREFERENCED_PARAMETER(ParentWindow);
    return TRUE;
#else
    TCHAR ErrorMsg[512];
    DWORD dwLastError=GetLastError();
    CAPIError::GetErrorMessageEx(dwLastError,NULL,ErrorMsg,_countof(ErrorMsg),szPrefix,TRUE);

    HWND LocalParentWindow;

    // if a specific parent window is defined, use it
    if (ParentWindow)
        LocalParentWindow = ParentWindow;
    else // else, use the default one
        LocalParentWindow = CAPIError::hParentWindow;

    return CAPIError::MessageBoxModelessProtection(LocalParentWindow,ErrorMsg,_T("Error"),MB_OK|MB_ICONERROR);
#endif
}

//-----------------------------------------------------------------------------
// Name: ShowLastError
// Object: show last windows api error
// Parameters :
//     in  :
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CAPIError::ShowLastError(HWND ParentWindow)
{
#if (defined(TOOLS_NO_MESSAGEBOX))
    UNREFERENCED_PARAMETER(ParentWindow);
    return TRUE;
#else
    return CAPIError::ShowLastError(TEXT(""),ParentWindow);
#endif
}
//-----------------------------------------------------------------------------
// Name: ShowError
// Object: show windows api error (avoid to remove last error value)
// Parameters :
//     in  : DWORD dwError
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CAPIError::ShowError(DWORD dwError,HWND ParentWindow)
{
#if (defined(TOOLS_NO_MESSAGEBOX))
    UNREFERENCED_PARAMETER(dwError);
    UNREFERENCED_PARAMETER(ParentWindow);
    return TRUE;
#else
    return CAPIError::ShowError(TEXT(""),dwError,ParentWindow);
#endif
}

//-----------------------------------------------------------------------------
// Name: ShowError
// Object: show windows api error
// Parameters :
//     in  :
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CAPIError::ShowError(TCHAR* szPrefix,DWORD dwError,HWND ParentWindow)
{
#if (defined(TOOLS_NO_MESSAGEBOX))
    UNREFERENCED_PARAMETER(szPrefix);
    UNREFERENCED_PARAMETER(dwError);
    UNREFERENCED_PARAMETER(ParentWindow);
    return TRUE;
#else
    TCHAR ErrorMsg[512];
    CAPIError::GetErrorMessageEx(dwError,NULL,ErrorMsg,_countof(ErrorMsg),szPrefix,TRUE);

    HWND LocalParentWindow;

    // if a specific parent window is defined, use it
    if (ParentWindow)
        LocalParentWindow = ParentWindow;
    else // else, use the default one
        LocalParentWindow = CAPIError::hParentWindow;

    return CAPIError::MessageBoxModelessProtection(LocalParentWindow,ErrorMsg,_T("Error"),MB_OK|MB_ICONERROR);

#endif
}


//-----------------------------------------------------------------------------
// Name: ShowError
// Object: show windows api error
// Parameters :
//     in  :
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CAPIError::ShowError(TCHAR* szPrefix,DWORD dwError,TCHAR* ModuleName,HWND ParentWindow)
{
#if (defined(TOOLS_NO_MESSAGEBOX))
    UNREFERENCED_PARAMETER(szPrefix);
    UNREFERENCED_PARAMETER(dwError);
    UNREFERENCED_PARAMETER(ModuleName);
    UNREFERENCED_PARAMETER(ParentWindow);
    return TRUE;
#else
    HMODULE hModule = ::LoadLibrary(ModuleName);
    if (!hModule)
        return CAPIError::ShowError(szPrefix,dwError,hParentWindow);

    TCHAR ErrorMsg[512];
    CAPIError::GetErrorMessageEx(dwError,hModule,ErrorMsg,_countof(ErrorMsg),szPrefix,TRUE);

    HWND LocalParentWindow;

    // if a specific parent window is defined, use it
    if (ParentWindow)
        LocalParentWindow = ParentWindow;
    else // else, use the default one
        LocalParentWindow = CAPIError::hParentWindow;

    ::FreeLibrary(hModule);
    return CAPIError::MessageBoxModelessProtection(LocalParentWindow,ErrorMsg,_T("Error"),MB_OK|MB_ICONERROR);

#endif
}

//-----------------------------------------------------------------------------
// Name: ShowError
// Object: show windows api error
// Parameters :
//     in  :
//     out :
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CAPIError::ShowError(TCHAR* szPrefix,DWORD dwError,HMODULE hModule,HWND ParentWindow)
{
#if (defined(TOOLS_NO_MESSAGEBOX))
    UNREFERENCED_PARAMETER(szPrefix);
    UNREFERENCED_PARAMETER(dwError);
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(ParentWindow);
    return TRUE;
#else
    TCHAR ErrorMsg[512];
    CAPIError::GetErrorMessageEx(dwError,hModule,ErrorMsg,_countof(ErrorMsg),szPrefix,TRUE);

    HWND LocalParentWindow;

    // if a specific parent window is defined, use it
    if (ParentWindow)
        LocalParentWindow = ParentWindow;
    else // else, use the default one
        LocalParentWindow = CAPIError::hParentWindow;

    return CAPIError::MessageBoxModelessProtection(LocalParentWindow,ErrorMsg,_T("Error"),MB_OK|MB_ICONERROR);
#endif
}