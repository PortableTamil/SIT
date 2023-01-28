#include <io.h>
#include <stdio.h>
#include <fcntl.h>

#include "StdConsole.h"

CStdConsole::CStdConsole(IN BOOL bAttachToParentProcess):CWin32Console(bAttachToParentProcess)
{
    this->ResetAttachStdToConsoleVars();
}

CStdConsole::~CStdConsole()
{
    if (this->bStdAttachedToConsole)
        this->DetachStdFromConsole();
}

void CStdConsole::ResetAttachStdToConsoleVars()
{
    this->bStdAttachedToConsole = FALSE;

    this->fdIn  = 0;
    this->fdOut = 0;
    this->fdErr = 0;

    this->fpIn  = NULL;
    this->fpOut = NULL;
    this->fpErr = NULL;

    memset(&this->OrgStdIn ,0,sizeof(this->OrgStdIn ));
    memset(&this->OrgStdOut,0,sizeof(this->OrgStdOut));
    memset(&this->OrgStdErr,0,sizeof(this->OrgStdErr));
}

//BOOL CStdConsole::AllocConsole()
//{
//    // allocate a console for this app
//    ::AllocConsole();
//
//    HANDLE consoleHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
//
//    // give the console window a nicer title
//    ::SetConsoleTitle(L"Debug Output");
//
//    // give the console window a bigger buffer size
//    CONSOLE_SCREEN_BUFFER_INFO csbi;
//    if ( ::GetConsoleScreenBufferInfo(consoleHandle, &csbi) )
//    {
//        COORD bufferSize;
//        bufferSize.X = csbi.dwSize.X;
//        bufferSize.Y = 9999;
//        ::SetConsoleScreenBufferSize(consoleHandle, bufferSize);
//    }
//}

int CStdConsole::Win32Handle_To_C_Handle(HANDLE Win32Handle)
{
    // _tprintf does the job, we don't need the _O_WTEXT
    return ::_open_osfhandle((intptr_t)Win32Handle,
//#if (defined(UNICODE)||defined(_UNICODE))
//                            _O_WTEXT
//#else
                            _O_TEXT
//#endif
                            );
}

BOOL CStdConsole::AttachStdToConsole()
{
    if (this->bStdAttachedToConsole)
    {
#ifdef _DEBUG
        if (::IsDebuggerPresent())
            ::DebugBreak();
#endif
        this->DetachStdFromConsole();
    }

    HANDLE ConsoleHandleIn  = ::GetStdHandle(STD_INPUT_HANDLE);
    HANDLE ConsoleHandleOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE ConsoleHandleErr = ::GetStdHandle(STD_ERROR_HANDLE);

    if ( (ConsoleHandleIn==0) || (ConsoleHandleOut==0) || (ConsoleHandleErr==0) )
        return FALSE;

    this->bStdAttachedToConsole = TRUE;

    this->fdIn  = CStdConsole::Win32Handle_To_C_Handle(ConsoleHandleIn);
    this->fdOut = CStdConsole::Win32Handle_To_C_Handle(ConsoleHandleOut);
    this->fdErr = CStdConsole::Win32Handle_To_C_Handle(ConsoleHandleErr);


    this->fpIn  = ::_fdopen( fdIn , "r" );
    this->fpOut = ::_fdopen( fdOut, "a"/*"w"*/ );
    this->fpErr = ::_fdopen( fdErr, "a"/*"w"*/ );

    //#define stdin  (&__iob_func()[0])
    //#define stdout (&__iob_func()[1])
    //#define stderr (&__iob_func()[2])

    this->OrgStdIn  = *stdin ;
    this->OrgStdOut = *stdout;
    this->OrgStdErr = *stderr;

    *stdin  = *this->fpIn ;
    *stdout = *this->fpOut;
    *stderr = *this->fpErr;

    // disable buffering
    ::setvbuf( stdin, NULL, _IONBF, 0 );
    ::setvbuf( stdout, NULL, _IONBF, 0 );
    ::setvbuf( stderr, NULL, _IONBF, 0 );

    return TRUE;
}

BOOL CStdConsole::DetachStdFromConsole()
{
    if (!this->bStdAttachedToConsole)
        return TRUE;

    *stdin  = this->OrgStdIn ;
    *stdout = this->OrgStdOut;
    *stderr = this->OrgStdErr;

    // as this->fpIn and this->fdIn are pointing the same file, only one close should be done
    //fclose(this->fpIn );
    //fclose(this->fpOut);
    //fclose(this->fpErr);

    _close(this->fdIn );
    _close(this->fdOut);
    _close(this->fdErr);

    this->ResetAttachStdToConsoleVars();

    return TRUE;
}