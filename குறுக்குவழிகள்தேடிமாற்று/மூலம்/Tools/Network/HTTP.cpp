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

#include "http.h"

#include <malloc.h>
// use wininet function to avoid to manually grep proxy and other internet settings from registry (let the wininet do all by itself)
#include <Wininet.h>

// required lib : Wininet.lib
#pragma comment (lib,"Wininet")
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <stdio.h>

#define CHTTP_AGENT_NAME                        TEXT("Mozilla/5.0 (Windows NT 6.1; Win64; x64; rv:106.0) Gecko/20100101 Firefox/106.0")
#define CHTTP_EXTRA_DATA_LEN_FOR_STRING_ENDING  2   // assume to be ready for wchar :)
#define CHTTP_HTTP_PROTOCOL_PREFIX              TEXT("http://")
#define CHTTP_HTTPS_PROTOCOL_PREFIX             TEXT("https://")
#define CHTTP_MAX_URL_SIZE                      1024 // according to rfc max is 256

#ifndef _countof
    #define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

#ifndef DefineTcharStringLength
    #define DefineTcharStringLengthWithEndingZero(x) (sizeof(x)/sizeof(TCHAR))
    #define DefineTcharStringLength(x) (sizeof(x)/sizeof(TCHAR)-1)
#endif

#ifndef BreakInDebugMode
    #ifdef _DEBUG
        #define BreakInDebugMode if ( ::IsDebuggerPresent() ) { ::DebugBreak();}
    #else 
        #define BreakInDebugMode 
    #endif
#endif

void CHTTP::ShowLastError(IN HWND hWndParent)
{
    DWORD dwLastError=::GetLastError();
    CHTTP::ShowError(hWndParent,dwLastError);
}

void CHTTP::ShowError(IN HWND hWndParent,IN DWORD dwError)
{
    TCHAR pcError[512];
    CHTTP::GetErrorText(dwError,pcError,_countof(pcError));
    if (*pcError == 0) // avoid to display empty popup
    {
        _tcscpy(pcError,TEXT("Unknown HTTP error"));
    }
    ::MessageBox(hWndParent,pcError,_T("Error"),MB_OK|MB_ICONERROR);
}

void CHTTP::GetErrorText(IN DWORD dwError,OUT TCHAR* StrError,IN DWORD StrErrorMaxSize)
{
    *StrError=0;
    if (this->bHttpStatusError)
    {
        _sntprintf(StrError,StrErrorMaxSize,TEXT("HTTP protocol error %u"),this->LastHttpReplyCode);
        StrError[StrErrorMaxSize-1]=0;
    }
    else
    {
        if (dwError==0)
            return;
        DWORD dw=::FormatMessage(FORMAT_MESSAGE_FROM_HMODULE,
                               GetModuleHandle( _T("wininet.dll") ),
                               dwError,
                               NULL,//GetUserDefaultLangID(), wininet errors seems to be only in English
                               StrError,
                               StrErrorMaxSize,
                               NULL);
        StrError[StrErrorMaxSize-1]=0;
        //If the function succeeds, the return value is the number of TCHARs stored in the output buffer,
        //  excluding the terminating null character.
        //If the function fails, the return value is zero
        if(dw==0)
        {
            // FormatMessage failed
            _sntprintf(StrError,StrErrorMaxSize,_T("Error 0x%08X\r\n"),dwError);
            StrError[StrErrorMaxSize-1]=0;
        }
    }
}


CHTTP::CHTTP(void)
{
    this->bHttps = FALSE;
    this->bHttpStatusError = FALSE;
    this->LastWin32Error = 0;
    this->LastHttpReplyCode = 0;
}

CHTTP::~CHTTP(void)
{
}
BOOL CHTTP::Get(CONST TCHAR* Url,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize)
{
    return this->Get(Url,INTERNET_DEFAULT_HTTP_PORT,pReceivedData,pReceivedDataSize);
}
BOOL CHTTP::Get(CONST TCHAR* Url,CONST USHORT Port,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize)
{
    return this->DoHttpRequest(Url,Port,0,_T("GET"),0,0,pReceivedData,pReceivedDataSize);
}

BOOL CHTTP::Post(CONST TCHAR* Url,CONST PBYTE Data, CONST SIZE_T DataSize,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize)
{
    return this->Post(Url,INTERNET_DEFAULT_HTTP_PORT,Data,DataSize,pReceivedData,pReceivedDataSize);
}

//sample static TCHAR Data[] = _T("name=John&userid=123&other=P%26Q");
BOOL CHTTP::Post(CONST TCHAR* URL,CONST USHORT Port,CONST PBYTE Data, CONST SIZE_T DataSize,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize)
{
    static TCHAR Headers[] = _T("Content-Type: application/x-www-form-urlencoded");
    /*
    Notice: POST different ways: 
    1) application/x-www-form-urlencoded
    ________________________
    Content-Type: application/x-www-form-urlencoded
    Content-Length: 21

    time=16h35&date=21/01
    ________________________


    2) multipart/form-data
    ________________________
    Content-Type: multipart/form-data; boundary=---------------------------2116772612019
    Content-Length: 550

    -----------------------------2116772612019
    Content-Disposition: form-data; name="time"

    16h35
    -----------------------------2116772612019
    Content-Disposition: form-data; name="date"

    21/01
    -----------------------------2116772612019--
    ________________________
    */
    return this->DoHttpRequest(URL,Port,Headers,_T("POST"),Data,DataSize,pReceivedData,pReceivedDataSize);
}
// Method : GET / POST /PUT
BOOL CHTTP::DoHttpRequest(CONST TCHAR* Url,CONST USHORT Port,CONST TCHAR* Headers,CONST TCHAR* Method,CONST PBYTE Data,CONST SIZE_T DataSize,OUT PBYTE* pReceivedData,OUT ULONG* pReceivedDataSize)
{
    this->LastHttpReplyCode = 0;
    static LPCTSTR Accept[]={_T("*/*"), NULL};
    BOOL bSuccess=FALSE;

    USHORT LocalServerPort=Port;

    //////////////////////////////
    // convert url into HostName + UrlPath
    //////////////////////////////
    TCHAR* LocalURL;
    TCHAR UrlPath[CHTTP_MAX_URL_SIZE];
    TCHAR HostName[CHTTP_MAX_URL_SIZE];
    TCHAR UserName[CHTTP_MAX_URL_SIZE];
    TCHAR PassWord[CHTTP_MAX_URL_SIZE];

    *pReceivedData = NULL;
    *pReceivedDataSize = NULL;

    if (this->bHttps)
    {
        // assume "https://" prefix is present
        if (_tcsnicmp(Url,CHTTP_HTTPS_PROTOCOL_PREFIX,DefineTcharStringLength(CHTTP_HTTPS_PROTOCOL_PREFIX))==0)
        {
            LocalURL=(TCHAR*)Url;
        }
        else
        {
            LocalURL=(TCHAR*)_alloca((DefineTcharStringLength(CHTTP_HTTPS_PROTOCOL_PREFIX)+_tcslen(Url)+1)*sizeof(TCHAR));
            _tcscpy(LocalURL,CHTTP_HTTPS_PROTOCOL_PREFIX);
            _tcscat(LocalURL,Url);
        }
    }
    else
    {
        // assume "http://" prefix is present
        if (_tcsnicmp(Url,CHTTP_HTTP_PROTOCOL_PREFIX,DefineTcharStringLength(CHTTP_HTTP_PROTOCOL_PREFIX))==0)
        {
            LocalURL=(TCHAR*)Url;
        }
        else
        {
            LocalURL=(TCHAR*)_alloca((DefineTcharStringLength(CHTTP_HTTP_PROTOCOL_PREFIX)+_tcslen(Url)+1)*sizeof(TCHAR));
            _tcscpy(LocalURL,CHTTP_HTTP_PROTOCOL_PREFIX);
            _tcscat(LocalURL,Url);
        }
    }

    URL_COMPONENTS uc;
    memset(&uc,0,sizeof(uc));

    *UrlPath=0;
    *HostName=0;
    *UserName=0;
    *PassWord=0;

    uc.dwStructSize = sizeof(uc);
    uc.dwUrlPathLength = CHTTP_MAX_URL_SIZE;
    uc.lpszUrlPath=UrlPath;
    uc.dwHostNameLength = CHTTP_MAX_URL_SIZE;
    uc.lpszHostName = HostName;
    uc.dwPasswordLength = CHTTP_MAX_URL_SIZE;
    uc.lpszPassword = PassWord;
    uc.dwUserNameLength = CHTTP_MAX_URL_SIZE;
    uc.lpszUserName = UserName;

    HINTERNET hSession=NULL;
    HINTERNET hConnect=NULL;
    HINTERNET hHttpRequest=NULL;

    if (!::InternetCrackUrl(LocalURL,(DWORD)_tcslen(LocalURL),ICU_DECODE,&uc))
        return FALSE;

    // if a port is specified (if no port specified InternetCrackUrl set uc.nPort to INTERNET_DEFAULT_HTTP_PORT)
    if (this->bHttps)
    {
        if (uc.nPort!=INTERNET_DEFAULT_HTTPS_PORT)
            LocalServerPort=uc.nPort;
    }
    else
    {
        if (uc.nPort!=INTERNET_DEFAULT_HTTP_PORT)
            LocalServerPort=uc.nPort;
    }

    //////////////////////////////
    // end of convert url into HostName + UrlPath
    //////////////////////////////

    // Open Internet session.
    hSession = ::InternetOpen(  CHTTP_AGENT_NAME,
                                INTERNET_OPEN_TYPE_PRECONFIG,
                                NULL, 
                                NULL,
                                0) ;
    if (!hSession)
        goto CleanUp;

    // Connect to server (like www.microsoft.com)
    hConnect = ::InternetConnect(hSession,
                                HostName,
                                LocalServerPort,
                                UserName,
                                PassWord,
                                INTERNET_SERVICE_HTTP,
                                0,
                                0) ;
    if (!hConnect)
        goto CleanUp;

    // Request the file (like /MSDN/MSDNINFO/) from the server.
    // Prepare the request
    DWORD Flags = INTERNET_FLAG_DONT_CACHE;
    if (this->bHttps)
        Flags |= INTERNET_FLAG_SECURE;
    hHttpRequest = ::HttpOpenRequest(  hConnect,
                                    Method,
                                    UrlPath,
                                    NULL,// version
                                    NULL,// referrer
                                    Accept,
                                    Flags,
                                    NULL // Pointer to a null-terminated array of strings that indicates media types accepted by the client. 
                                         // If this parameter is NULL, no types are accepted by the client. 
                                         // Servers generally interpret a lack of accept types to indicate that the client accepts 
                                         // only documents of type "text/*" (that is, only text documents—no pictures or other binary files). 
                                         // For a list of valid media types, see https://www.iana.org/assignments/media-types/media-types.xhtml
                                  );
    if (!hHttpRequest)
    {
#ifdef _DEBUG
        DWORD dwLastError = ::GetLastError();
        BreakInDebugMode;
        dwLastError = dwLastError;
#endif
        goto CleanUp;
    }

    // Send the request.
    BOOL bSendRequest;
    SIZE_T HeadersSize;
    if (Headers)
        HeadersSize=_tcslen(Headers);
    else
        HeadersSize=0;

    // send the request
    bSendRequest = ::HttpSendRequest(hHttpRequest, Headers, (DWORD)HeadersSize, Data, (DWORD)DataSize);
    if (!bSendRequest)
    {
#ifdef _DEBUG
        DWORD dwLastError = ::GetLastError();
        BreakInDebugMode;
        dwLastError = dwLastError;
#endif
        goto CleanUp;
    }


    // get status code
    DWORD Status;
    DWORD StatusSize=sizeof(Status);
    if (!::HttpQueryInfo(hHttpRequest,
                        HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, 
                        (LPVOID)&Status, 
                        &StatusSize,
                        NULL)
        )
    {
#ifdef _DEBUG
        DWORD dwLastError = ::GetLastError();
        BreakInDebugMode;
        dwLastError = dwLastError;
#endif
        goto CleanUp;
    }
    
    this->LastHttpReplyCode = Status;
    // assume status is ok
    if (Status!=HTTP_STATUS_OK)
    {
        this->bHttpStatusError = TRUE;
        goto CleanUp;
    }

    // Get the length of the file.            
    DWORD dwContentLen;
    DWORD dwLengthBufQuery = sizeof(dwContentLen);
    PBYTE pEnd;
    if (::HttpQueryInfo(hHttpRequest,
                        HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, 
                        (LPVOID)&dwContentLen, &dwLengthBufQuery,
                        NULL
                        )
        )
    {
        *pReceivedDataSize = (ULONG)dwContentLen ;

        // Allocate a buffer for the file.   
        *pReceivedData = new BYTE[*pReceivedDataSize+CHTTP_EXTRA_DATA_LEN_FOR_STRING_ENDING] ;

        // Read the file into the buffer. 
        DWORD dwBytesRead ;
        BOOL bRead = ::InternetReadFile(hHttpRequest,
                                        *pReceivedData,
                                        *pReceivedDataSize,
                                        &dwBytesRead);
        // Put a zero on the end of the buffer to allow string parsing without re allocating memory
        pEnd = &(*pReceivedData)[dwBytesRead];
        memset(pEnd,0,CHTTP_EXTRA_DATA_LEN_FOR_STRING_ENDING);

        if (!bRead)
            goto CleanUp;
    }
    else
    {
        DWORD BufferSize=4096;
        DWORD RemainingBufferSize=BufferSize;
        DWORD UsedBufferSize=0;
        DWORD dwBytesRead=0;
        PBYTE TmpBuffer;
        *pReceivedData = new BYTE[BufferSize+CHTTP_EXTRA_DATA_LEN_FOR_STRING_ENDING] ;
        do
        {
            
            RemainingBufferSize-=dwBytesRead;

            // get used buffer size
            UsedBufferSize=BufferSize-RemainingBufferSize;

            if (RemainingBufferSize<=1024)
            {
                // save current buffer
                TmpBuffer=*pReceivedData;

                // increase buffer size
                BufferSize*=2;
                *pReceivedData = new BYTE[BufferSize+CHTTP_EXTRA_DATA_LEN_FOR_STRING_ENDING] ;

                // compute new remaining data size
                RemainingBufferSize=BufferSize-UsedBufferSize;

                // copy old data to new buffer
                memcpy(*pReceivedData,TmpBuffer,UsedBufferSize);

                // destroy old memory
                delete TmpBuffer;

                // update used buffer size
                UsedBufferSize=BufferSize-RemainingBufferSize;
            }
            dwBytesRead = 0;
            ::InternetReadFile(hHttpRequest, &((*pReceivedData)[UsedBufferSize]), RemainingBufferSize, &dwBytesRead);
            
        }while ( dwBytesRead );

        // compute used data size
        *pReceivedDataSize=BufferSize-RemainingBufferSize;

        // Put a zero on the end of the buffer.
        pEnd = &(*pReceivedData)[*pReceivedDataSize];
        memset(pEnd,0,CHTTP_EXTRA_DATA_LEN_FOR_STRING_ENDING);
    }

    bSuccess=TRUE;
    this->LastWin32Error = 0;
CleanUp:
    if (!bSuccess)
        this->LastWin32Error = ::GetLastError();
    // Close all of the Internet handles.
    if (hHttpRequest)
        ::InternetCloseHandle(hHttpRequest); 
    if (hConnect)
        ::InternetCloseHandle(hConnect) ;
    if (hSession)
        ::InternetCloseHandle(hSession) ;

    if (!bSuccess)
        ::SetLastError(this->LastWin32Error);

    return bSuccess;
}