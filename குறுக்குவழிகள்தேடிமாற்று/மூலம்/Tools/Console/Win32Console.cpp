#include "Win32Console.h"
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

CWin32Console::CWin32Console(IN BOOL bAttachToParentProcess,IN BOOL bAllocConsoleIfAttachConsoleFail)
{
    if (bAttachToParentProcess)
    {
        this->bConsoleOpened = ::AttachConsole(ATTACH_PARENT_PROCESS);
        if (bAllocConsoleIfAttachConsoleFail && ( this->bConsoleOpened == FALSE) )
            this->bConsoleOpened = ::AllocConsole();
    }
    else
        this->bConsoleOpened = ::AllocConsole();
}

CWin32Console::~CWin32Console()
{
    if (this->bConsoleOpened)
        ::FreeConsole();
}

BOOL CWin32Console::Write(IN TCHAR* String)
{
    if (this->bConsoleOpened==FALSE)
        return FALSE;

    DWORD NbWrittenChar;
    return this->Write(String,(DWORD)_tcslen(String),&NbWrittenChar);
}

BOOL CWin32Console::Write(IN TCHAR* String, IN DWORD NbTcharToWrite,OUT DWORD* pNbWrittenTchar,IN LPVOID Control)
{
    if (this->bConsoleOpened==FALSE)
        return FALSE;

    return ::WriteConsole(::GetStdHandle(STD_OUTPUT_HANDLE),(LPVOID)String,NbTcharToWrite,pNbWrittenTchar,Control);
}

BOOL CWin32Console::Read(IN OUT TCHAR* String, IN DWORD NbTcharToRead,OUT DWORD* pNbReadTchar,IN LPVOID Control)
{
    if (this->bConsoleOpened==FALSE)
        return FALSE;

    DWORD TotalNbReadChar=0;
    DWORD CurrentNbReadChar;
    BOOL bRet = TRUE;
    while (NbTcharToRead && (bRet==TRUE) )
    {
        bRet = ::ReadConsole(::GetStdHandle(STD_INPUT_HANDLE),(LPVOID)String,NbTcharToRead,&CurrentNbReadChar,(PCONSOLE_READCONSOLE_CONTROL)Control);
        TotalNbReadChar+=CurrentNbReadChar;
        NbTcharToRead-=CurrentNbReadChar;
    }

    *pNbReadTchar = TotalNbReadChar;
    return bRet;
}

TCHAR CWin32Console::GetChar()
{
    if (this->bConsoleOpened==FALSE)
        return 0;

    DWORD NbRead = 0;
    TCHAR t=0;
    // read would block until \r\n or return directly depending of console configuration
    this->Read(&t,1,&NbRead);
    return t;
}
