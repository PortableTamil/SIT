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
// Object: text file operation
//         use no C library, 
//         support ascii or little endian unicode files
//-----------------------------------------------------------------------------

#include "textfile.h"
#include <stdio.h>

//-----------------------------------------------------------------------------
// Name: CreateTextFile
// Object: create text file with read write attributes
// Parameters :
//     in : TCHAR* FullPath : path of file to be created
//     out : HANDLE* phFile : if successful return, handle to the created file (must be closed by caller)
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTextFile::CreateTextFile(TCHAR* FullPath,OUT HANDLE* phFile)
{
    *phFile = ::CreateFile(FullPath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (*phFile==INVALID_HANDLE_VALUE)
        return FALSE;

#if (defined(UNICODE)||defined(_UNICODE))
    DWORD dwWrittenBytes;
    // in unicode mode, write the unicode little endian header (FFFE)
    BYTE pbUnicodeHeader[2]={0xFF,0xFE};
    ::WriteFile(*phFile,pbUnicodeHeader,2,&dwWrittenBytes,NULL);
#endif

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: CreateTextFile
// Object: create text file with read write attributes
// Parameters :
//     in : TCHAR* FullPath : path of file to be created
//     out : HANDLE* phFile : if successful return, handle to the created file
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CTextFile::CreateOrOpenForAppending(TCHAR* FullPath,OUT HANDLE* phFile)
{
    *phFile = ::CreateFile(FullPath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (*phFile==INVALID_HANDLE_VALUE)
    {
        if (CTextFile::CreateTextFile(FullPath,phFile))
            return FALSE;
    }

    ::SetFilePointer(*phFile,NULL,NULL,FILE_END);

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: WriteText
// Object: write text at current position
// Parameters :
//     in : HANDLE hFile : File handle
//          TCHAR* Text : text to write
//     out  
//     return : result of WriteFile
//-----------------------------------------------------------------------------
BOOL CTextFile::WriteText(HANDLE hFile,TCHAR* Text)
{
    return CTextFile::WriteText(hFile,Text,_tcslen(Text));
}

//-----------------------------------------------------------------------------
// Name: WriteText
// Object: write text at current position
// Parameters :
//     in : HANDLE hFile : File handle
//          TCHAR* Text : text to write
//          SIZE_T LenInTCHAR : number of TCHAR to be written 
//     out  
//     return : result of WriteFile
//-----------------------------------------------------------------------------
BOOL CTextFile::WriteText(HANDLE hFile,TCHAR* Text,SIZE_T LenInTCHAR)
{
    DWORD dwWrittenBytes;
    return WriteFile(hFile,Text,(DWORD)(LenInTCHAR*sizeof(TCHAR)),&dwWrittenBytes,NULL);
}


//-----------------------------------------------------------------------------
// Name: ReportError
// Object: show an error message
// Parameters :
//     in : TCHAR* pszMsg
//     out  
//     return 
//-----------------------------------------------------------------------------
void CTextFile::ReportError(TCHAR* pszMsg)
{
#ifdef TOOLS_NO_MESSAGEBOX
    UNREFERENCED_PARAMETER(pszMsg);
#else
    // else use static linking
    ::MessageBox(NULL,pszMsg,_T("Error"),MB_OK|MB_ICONERROR | MB_SYSTEMMODAL);
#endif
}

//-----------------------------------------------------------------------------
// Name: Read
// Object: 
// Parameters :
//     in : TCHAR* FileName file name
//     out : TCHAR** ppszContent : content of file converted to current TCHAR definition
//           must be free with delete *ppszContent
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CTextFile::Read(TCHAR* FileName,TCHAR** ppszContent)
{
    BOOL b;
    return CTextFile::Read(FileName,ppszContent,&b);
}


//-----------------------------------------------------------------------------
// Name: Read
// Object: 
// Parameters :
//     in : TCHAR* FileName file name
//     out : TCHAR** ppszContent : content of file converted to current TCHAR definition
//           must be free with delete *ppszContent
//          BOOL* pbUnicode16File : gives information about file encoding
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CTextFile::Read(TCHAR* FileName,TCHAR** ppszContent,BOOL* pbUnicode16File)
{
    ULONG64 ContentSize;
    BOOL bRet = CTextFile::ReadEx(FileName,ppszContent,&ContentSize,pbUnicode16File);

    return bRet;
}

// pContentSize : content in TCHAR count
BOOL CTextFile::ReadEx(TCHAR* FileName,TCHAR** ppszContent,ULONG64* pContentSize,BOOL* pbUnicode16File)
{
    *pbUnicode16File=FALSE;
    HANDLE hFile= INVALID_HANDLE_VALUE;
    TCHAR pszFileName[MAX_PATH];
    TCHAR szMsg[2*MAX_PATH];
    LARGE_INTEGER LiFileSize;
    ULONG64 FileSize;
    ULONG64 Remaining;
    DWORD dwNbData;
    ULONG64 ContentSize=0;
    BOOL bSuccess;
    BOOL bRetValue = FALSE;
    BYTE* pbFileContent=NULL;
    *pContentSize = 0;
    *ppszContent=0;

    // check empty file name
    if (*FileName==0)
    {
        CTextFile::ReportError(_T("Empty file name"));
        goto CleanUp;
    }

    // assume we get full path file name, else add the current exe path to pszFileName
    if (_tcschr(FileName,'\\')==0)
    {
        TCHAR* pszLastSep;
        // we don't get full path, so add current exe path
        ::GetModuleFileName(::GetModuleHandle(NULL),pszFileName,MAX_PATH);
        pszLastSep=_tcsrchr(pszFileName,'\\');
        if (pszLastSep)
            *(pszLastSep+1)=0;// keep \ char

        _tcscat(pszFileName,FileName);
    }
    else
        // we get full path only copy file name
        _tcscpy(pszFileName,FileName);

    // open file
    hFile = ::CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        _sntprintf(szMsg,_countof(szMsg),_T("File %s not found"),pszFileName);
        CTextFile::ReportError(szMsg);
        goto CleanUp;
    }

    // get file size
    bSuccess = ::GetFileSizeEx(hFile, &LiFileSize);
    if (!bSuccess)
    {
        _sntprintf(szMsg,_countof(szMsg),_T("Can't get file size for %s"),pszFileName);
        CTextFile::ReportError(szMsg);
        goto CleanUp;
    }
    FileSize = LiFileSize.QuadPart;

    if (FileSize==0)
    {
        // as we return true, assume to put an empty string
        *ppszContent=new TCHAR[1];
        (*ppszContent)[0]=0;
        bRetValue = TRUE;
        goto CleanUp;
    }

    DWORD dwRealyRead;

    BOOL bUtf8 = FALSE;
    #define FILE_MAX_HEADER_SIZE 3
    BYTE Header[FILE_MAX_HEADER_SIZE];
    DWORD HeaderSize = 0;
    memset(Header,0,FILE_MAX_HEADER_SIZE);
    if (!::ReadFile(hFile,Header,__min((DWORD)FileSize,FILE_MAX_HEADER_SIZE),&dwRealyRead,NULL))
        goto CleanUp;

    // check if file is unicode or ansi file
    // unicode files begin with FFFE in little endian
    if (FileSize>=2)
    {
        // check FFFE (we have to cast pcFileOrig in PBYTE as we don't know if it's 8 or 16 byte pointer)
        if ((Header[0]==0xFF)&&(Header[1]==0xFE))
        {
            *pbUnicode16File=TRUE;
            HeaderSize=2;
        }
    }

    // check for utf8
    if ( (*pbUnicode16File==FALSE)&&(FileSize>=3) )
    {
        if ((Header[0]==0xEF)&&(Header[1]==0xBB)&&(Header[2]==0xBF))
        {
            bUtf8 = TRUE;
            HeaderSize=3;
        }
    }

    if (HeaderSize==FileSize)
    {
        // as we return true, assume to put an empty string
        *ppszContent=new TCHAR[1];
        (*ppszContent)[0]=0;
        bRetValue = TRUE;
        goto CleanUp;
    }

    ::SetFilePointer(hFile,HeaderSize-dwRealyRead,NULL,FILE_CURRENT);

    ContentSize = FileSize-HeaderSize;

    try
    {
    pbFileContent=new BYTE[(SIZE_T)(ContentSize+__max( sizeof(CHAR),sizeof(WCHAR) ))];// __max( sizeof(CHAR),sizeof(WCHAR) ) : use to force string ending by adding a \0 to file
    
    BYTE* Tmp_pb;
    Tmp_pb = pbFileContent;
    for(Remaining=ContentSize;
        Remaining>0;
        Remaining=Remaining-dwNbData,Tmp_pb+=dwNbData
        )
    {
        dwNbData = (DWORD)__min(0xFFFFFFFF,Remaining);

        if (!::ReadFile(hFile,Tmp_pb,dwNbData,&dwRealyRead,NULL))
        {
            delete[] pbFileContent;
            goto CleanUp;
        }
    }

#if (defined(UNICODE)||defined(_UNICODE))
    if (*pbUnicode16File)
    {
        *ppszContent=(TCHAR*)pbFileContent;

        // adjust file size : file size will now be in TCHAR count no more in bytes
        ContentSize/=2;

        // ends string
        (*ppszContent)[ContentSize]=0;
    }
    // if we work in unicode and the file is in ansi, convert it in unicode
    else
    {
        UINT CodePage = CP_ACP;
        if (bUtf8)
        {
            CodePage = CP_UTF8;
        }

        // convert into unicode
        *ppszContent=new TCHAR[(SIZE_T)(ContentSize+1)];
        TCHAR* Tmp;
        BYTE* Tmp_pb2;
        int iNbData;

        for(Tmp = *ppszContent,Tmp_pb2 = pbFileContent,Remaining=ContentSize;
            Remaining>0;
            Remaining-=iNbData,Tmp+=iNbData,Tmp_pb2+=iNbData
            )
        {
            iNbData = (DWORD)__min(0x7FFFFFFF,Remaining);

            if (!::MultiByteToWideChar(CodePage, 0, (LPCSTR)Tmp_pb2, iNbData,Tmp,(int)iNbData))
            {
                _sntprintf(szMsg,_countof(szMsg),_T("Error converting file %s in unicode"),pszFileName);
                CTextFile::ReportError(szMsg);
                delete[] pbFileContent;
                delete[] *ppszContent;
                *ppszContent=0;
                goto CleanUp;
            }
        }

        delete[] pbFileContent;
        (*ppszContent)[ContentSize]=0;
    }
#else
    #pragma warning(__FILE__ " currently no utf8 support for ansi")

    // if we work in ansi and the file is in unicode, convert it in ansi
    if (*pbUnicode16File)
    {
        // remove signature
        ContentSize=FileSize-2;
        // adjust file size : file size will now be in TCHAR count no more in bytes
        ContentSize/=2;

        // convert into ansi
        *ppszContent=new TCHAR[ContentSize+1];

        TCHAR* Tmp;
        BYTE* Tmp_pb2;
        int iNbData;

        for(Tmp = *ppszContent,Tmp_pb2 = pbFileContent,Remaining=ContentSize;
            Remaining>0;
            Remaining-=iNbData,Tmp+=iNbData,Tmp_pb2+=(2*iNbData)
            )
        {
            iNbData = (DWORD)__min(0x7FFFFFFF/2,Remaining);// /2 : assume 2*iNbData is a int

            if (!::WideCharToMultiByte(CP_ACP, 0,(LPCWSTR)Tmp_pb2, iNbData, *ppszContent, iNbData, NULL, NULL))
            {
                DWORD dwLastError = ::GetLastError();
                _sntprintf(szMsg,_countof(szMsg),_T("Error 0x%x converting file %s in ansi"),dwLastError,pszFileName);
                CTextFile::ReportError(szMsg);
                delete[] pbFileContent;
                delete[] *ppszContent;
                *ppszContent=0;
                goto CleanUp;
            }

        }
        delete[] pbFileContent;
        (*ppszContent)[ContentSize]=0;
    }
    else
    {
        *ppszContent=(TCHAR*)pbFileContent;

        // ends string
        (*ppszContent)[ContentSize]=0;
    }
#endif
    bRetValue = TRUE;

    }
    catch (...)
    {
        CTextFile::ReportError(TEXT("Error allocating memory"));
        if (pbFileContent)
            delete[] pbFileContent;
        *ppszContent=0;
    }


CleanUp:
    if (hFile != INVALID_HANDLE_VALUE)
        ::CloseHandle(hFile);

    *pContentSize = ContentSize;
    return bRetValue;
}

//-----------------------------------------------------------------------------
// Name: ParseLines
// Object: for each line of the file pszFileName, call the LineCallBack Callback
//          giving it 
//              - line content
//              - the current line number (1 based number)
//              - the provide user parameter
// Parameters :
//     in : TCHAR* pszFileName : file to parse
//          tagLineCallBack LineCallBack : callback called on each new line, 
//                                         must return TRUE to continue parsing, FALSE to stop it
//          LPVOID CallBackUserParam : user parameter translated as callback arg
//     out  
//     return : TRUE on success, False on error
//-----------------------------------------------------------------------------
BOOL CTextFile::ParseLines(TCHAR* pszFileName,tagLineCallBack LineCallBack,LPVOID CallBackUserParam)
{
    return CTextFile::ParseLines(pszFileName,NULL,LineCallBack,CallBackUserParam);
}



//-----------------------------------------------------------------------------
// Name: ParseLines
// Object: for each line of the file pszFileName, call the LineCallBack Callback
//          giving it 
//              - line content
//              - the current line number (1 based number)
//              - the provide user parameter
// Parameters :
//     in : TCHAR* pszFileName : file to parse
//          HANDLE hCancelEvent : a event that is set to stop parsing (can be null)
//          tagLineCallBack LineCallBack : callback called on each new line
//          LPVOID CallBackUserParam : user parameter translated as callback arg
//     out  
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CTextFile::ParseLines(TCHAR* pszFileName,HANDLE hCancelEvent,tagLineCallBack LineCallBack,LPVOID CallBackUserParam, OUT BOOL* pbCanceled)
{
    DWORD dwCurrentLine=1;
    TCHAR* PreviousPos;
    TCHAR* NextPos;
    TCHAR* pszLine;
#ifdef CTextFile_SECURE_PARSELINE
    DWORD dwLineSize;
    DWORD MaxLineSize;
#endif

    TCHAR* pszContent;
    BOOL bCanceled;

    if (pbCanceled)
        *pbCanceled = FALSE;

    if (::IsBadCodePtr((FARPROC)LineCallBack))
        return FALSE;

    if (!CTextFile::Read(pszFileName,&pszContent))
        return FALSE;

    bCanceled=FALSE;
#ifdef CTextFile_SECURE_PARSELINE
    MaxLineSize=512;
    pszLine=new TCHAR[MaxLineSize];
#endif

    PreviousPos=pszContent;
    NextPos=_tcschr(PreviousPos,_T('\n'));
    while (NextPos)
    {
#ifdef CTextFile_SECURE_PARSELINE
        // make a local buffer to avoid callback modifying content
        dwLineSize=(DWORD)(NextPos-PreviousPos);
        if (dwLineSize+1>MaxLineSize)
        {
            MaxLineSize=dwLineSize+1;
            delete[] pszLine;
            pszLine=new TCHAR[MaxLineSize];
        }
        if (!pszLine)// buffer allocation error
        {
            delete[] pszContent;
            return FALSE;
        }

        if (dwLineSize>0)
        {
            if (*(NextPos-1) == '\r')
                dwLineSize--;
        }
        memcpy(pszLine,PreviousPos,dwLineSize*sizeof(TCHAR));
        pszLine[dwLineSize]=0;
#else
        *NextPos = 0;
        if (NextPos>pszContent)
        {
            if (*(NextPos-1) == '\r')
                *(NextPos-1) = 0;
        }

        pszLine = PreviousPos;
#endif
        if (hCancelEvent)
        {
            // if event is signaled
            if (::WaitForSingleObject(hCancelEvent,0)==WAIT_OBJECT_0)
            {
                bCanceled=TRUE;
                // stop parsing
                break;
            }
        }

        // call callback
        if (!LineCallBack(pszFileName,pszLine,dwCurrentLine,FALSE,CallBackUserParam))
        {
            bCanceled=TRUE;
            // stop parsing
            break;
        }

        dwCurrentLine++;
        PreviousPos=NextPos+1;// PreviousPos and NextPos are already in TCHAR so no *sizeof(TCHAR) required
        NextPos=_tcschr(PreviousPos,_T('\n'));
    }

    // last line
    if (!bCanceled)
    {
#ifdef CTextFile_SECURE_PARSELINE
        // make local copy
        dwLineSize=(DWORD)_tcslen(PreviousPos);
        if (dwLineSize+1>MaxLineSize)
        {
            MaxLineSize=dwLineSize+1;
            delete[] pszLine;
            pszLine=new TCHAR[MaxLineSize];
        }
        if (!pszLine)// buffer allocation error
        {
            delete[] pszContent;
            return FALSE;
        }

        memcpy(pszLine,PreviousPos,dwLineSize*sizeof(TCHAR));
        pszLine[dwLineSize]=0;
#else
        pszLine = PreviousPos;
#endif

        LineCallBack(pszFileName,pszLine,dwCurrentLine,TRUE,CallBackUserParam);
    }

#ifdef CTextFile_SECURE_PARSELINE
    delete[] pszLine;
#endif
    delete[] pszContent;

    if (pbCanceled)
        *pbCanceled = bCanceled;

    return TRUE;
}