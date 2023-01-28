/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

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
// Object: manage windows clipboard
//-----------------------------------------------------------------------------

#include "clipboard.h"

#pragma intrinsic(memcpy)

//-----------------------------------------------------------------------------
// Name: SetClipboardString
// Object: This function paste string inside clipboard
// Parameters :
//     in  : HWND hWindow : window handle
//           TCHAR* szData : data to copy
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CClipboard::SetClipboardString(HWND hWindow,TCHAR* szData)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return CClipboard::SetClipboardStringW(hWindow,szData);
#else
    return CClipboard::SetClipboardStringA(hWindow,szData);
#endif
}

BOOL CClipboard::SetClipboardStringA(HWND hWindow,CHAR* szData)
{
    return CClipboard::SetClipboardData( hWindow, CF_TEXT, (PBYTE)szData, strlen(szData)+1 );
}
BOOL CClipboard::SetClipboardStringW(HWND hWindow,WCHAR* szData)
{
    return CClipboard::SetClipboardData( hWindow, CF_UNICODETEXT, (PBYTE)szData, (wcslen(szData)+1)*sizeof(WCHAR) );
}
BOOL CClipboard::SetClipboardData(HWND hWindow,UINT uFormat,PBYTE Data,SIZE_T DataSize)
{
    LPVOID  glbData; 
    HGLOBAL hglb; 

    if (::IsBadReadPtr(Data, DataSize))
        return FALSE;

    if (!::OpenClipboard(hWindow))
    {
        CAPIError::ShowLastError(CClipboard_ERROR_COPYING_DATA);
        return FALSE;
    }
    ::EmptyClipboard();

    // Allocate a global memory object for the text. 
    hglb = ::GlobalAlloc(GMEM_MOVEABLE,DataSize); 
    if (!hglb) 
    {
        ::CloseClipboard();
        CAPIError::ShowLastError(CClipboard_ERROR_COPYING_DATA);
        return FALSE;
    }

    // Lock the handle and copy the text to the buffer. 
    glbData = ::GlobalLock(hglb); 
    if (!glbData)
    {
        ::CloseClipboard();
        ::GlobalFree(hglb);
        CAPIError::ShowLastError(CClipboard_ERROR_COPYING_DATA);
        return FALSE;
    }
    // copy data and ending null char
    memcpy(glbData, Data, DataSize);
    // unlock handle
    ::GlobalUnlock(hglb); 

    // Place the handle on the clipboard.
    // Note we let the system do the CF_UNICODETEXT<->CF_TEXT<->CF_OEMTEXT conversion for us
    // MSDN : "
    // The system implicitly converts data between certain clipboard formats: 
    // if a window requests data in a format that is not on the clipboard, 
    // the system converts an available format to the requested format. 
    // The system can convert data as indicated in the following table. 
    //  Clipboard   Format      Conversion Format Platform Support 
    //  CF_BITMAP   CF_DIB      Microsoft Windows NT®/Windows 2000, Windows 95/Windows 98/Windows Millennium Edition (Windows Me) 
    //  CF_BITMAP   CF_DIBV5    Windows 2000/XP 
    //  CF_DIB      CF_BITMAP   Windows NT/Windows 2000, Windows 95/Windows 98/Windows Me 
    //  CF_DIB      CF_PALETTE  Windows NT/Windows 2000, Windows 95/Windows 98/Windows Me 
    //  CF_DIB      CF_DIBV5    Windows 2000 
    //  CF_DIBV5    CF_BITMAP   Windows 2000 
    //  CF_DIBV5    CF_DIB      Windows 2000 
    //  CF_DIBV5    CF_PALETTE  Windows 2000 
    //  CF_ENHMETAFILE  CF_METAFILEPICT Windows NT/Windows 2000, Windows 95/Windows 98/Windows Me 
    //  CF_METAFILEPICT CF_ENHMETAFILE  Windows NT/Windows 2000, Windows 95/Windows 98/Windows Me 
    //  CF_OEMTEXT  CF_TEXT     Windows NT/Windows 2000, Windows 95/Windows 98/Windows Me 
    //  CF_OEMTEXT  CF_UNICODETEXT      Windows NT/Windows 2000 
    //  CF_TEXT     CF_OEMTEXT  Windows NT/Windows 2000, Windows 95/Windows 98/Windows Me 
    //  CF_TEXT     CF_UNICODETEXT      Windows NT/Windows 2000 
    //  CF_UNICODETEXT  CF_OEMTEXT      Windows NT/Windows 2000 
    //  CF_UNICODETEXT  CF_TEXT         Windows NT/Windows 2000 
    // "
    if ( !::SetClipboardData(uFormat,hglb) )
    {
        CAPIError::ShowLastError(CClipboard_ERROR_COPYING_DATA);
        ::CloseClipboard();
        ::GlobalFree(hglb);
        return FALSE;
    }
    ::CloseClipboard();

    // Don't free memory with GlobalFree(hglb); in case of SetClipboardData success
    // Windows do it itself at the next EmptyClipboard(); call

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: GetClipboardString
// Object: This function get string from clipboard
// Parameters :
//     in  : HWND hWindow : window handle
//     out : 
//     return : NULL on failure, string on success.
//              returned string must be free with a "free()" call
//-----------------------------------------------------------------------------
TCHAR* CClipboard::GetClipboardString(HWND hWindow)
{
#if (defined(UNICODE)||defined(_UNICODE))
    return CClipboard::GetClipboardStringW(hWindow);
#else
    return CClipboard::GetClipboardStringA(hWindow);
#endif
}

// return NULL on error, Data buffer on success
//      /!\ Data buffer must be free with delete[] !!!
CHAR* CClipboard::GetClipboardStringA(HWND hWindow)
{
    SIZE_T Dummy;
    return (CHAR*) CClipboard::GetClipboardData(hWindow,CF_TEXT,&Dummy);
}
// return NULL on error, Data buffer on success
//      /!\ Data buffer must be free with delete[] !!!
WCHAR* CClipboard::GetClipboardStringW(HWND hWindow)
{
    SIZE_T Dummy;
    return (WCHAR*) CClipboard::GetClipboardData(hWindow,CF_UNICODETEXT,&Dummy);
}

// return NULL on error, Data buffer on success
//      /!\ Data buffer must be free with delete[] !!!
PBYTE CClipboard::GetClipboardData(HWND hWindow,UINT uFormat,OUT SIZE_T* pDataSize)
{
    *pDataSize = 0;
    if (!::IsClipboardFormatAvailable(uFormat)) 
        return NULL; 
    if (!::OpenClipboard(hWindow)) 
        return NULL;

    HGLOBAL   hglb; 
    SIZE_T    DataSize;
    LPVOID    glbData; 
    PBYTE     LocalData=NULL; 
    hglb = ::GetClipboardData(uFormat); 
    if (hglb != NULL) 
    { 
        glbData = ::GlobalLock(hglb); 
        if (glbData != NULL) 
        { 
            // do a local copy
            DataSize = ::GlobalSize(glbData);
            *pDataSize = DataSize;
            LocalData = new BYTE[DataSize];
            memcpy(LocalData,glbData,DataSize);
            ::GlobalUnlock(hglb); 
        } 
    } 
    ::CloseClipboard();

    return LocalData;
}
