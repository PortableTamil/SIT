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

class CHTTP
{
public:
    CHTTP(void);
    ~CHTTP(void);

    void ShowLastError(IN HWND hWndParent = NULL);
    void ShowError(IN HWND hWndParent,IN DWORD dwError);
    void GetErrorText(IN DWORD dwError,OUT TCHAR* StrError,IN DWORD StrErrorMaxSize);

    BOOL bHttps;
    BOOL bHttpStatusError;
    DWORD LastWin32Error;
    DWORD LastHttpReplyCode;

    BOOL Get(CONST TCHAR* Url,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize);
    BOOL Get(CONST TCHAR* Url,CONST USHORT Port,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize);
    BOOL Post(CONST TCHAR* Url,CONST PBYTE Data, CONST SIZE_T DataSize,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize);
    BOOL Post(CONST TCHAR* Url,CONST USHORT Port,CONST PBYTE Data, CONST SIZE_T DataSize,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize);
    BOOL DoHttpRequest(CONST TCHAR* Url,CONST USHORT Port,CONST TCHAR* Headers,CONST TCHAR* Method,CONST PBYTE Data,CONST SIZE_T DataSize,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize);

};
