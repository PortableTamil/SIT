#pragma once

//@rem use either 
//@rem .bat 
//@rem cmd /c ShortcutsSearchAndReplace64.exe args
//@rem START /WAIT ShortcutsSearchAndReplace64.exe args


#include <windows.h>

class CWin32Console
{
protected:
//    static FORCEINLINE BOOL AllocConsole(){return ::AllocConsole();}
//    static FORCEINLINE BOOL AttachConsole(){return ::AttachConsole(ATTACH_PARENT_PROCESS);}
//    static FORCEINLINE BOOL FreeConsole(){return ::FreeConsole();}
    BOOL bConsoleOpened;
public:
    CWin32Console(IN BOOL bAttachToParentProcess = TRUE,IN BOOL bAllocConsoleIfAttachConsoleFail = FALSE); // if (bAttachToParentProcess) AttachConsole is called else AllocConsole is called
    virtual ~CWin32Console();

    FORCEINLINE BOOL DoesConsoleOpenSuccessfully(){return this->bConsoleOpened;}

    BOOL Write(IN TCHAR* String);
    BOOL Write(IN TCHAR* String, IN DWORD NbTcharToWrite,OUT DWORD* pNbWrittenTchar,IN LPVOID Control=NULL);

    BOOL Read(IN OUT TCHAR* String, IN DWORD NbTcharToRead,OUT DWORD* pNbReadTchar,IN LPVOID Control=NULL);

    // GetChar : sucks if binary is called directly from command line, works if call from batch
    TCHAR GetChar();
};
