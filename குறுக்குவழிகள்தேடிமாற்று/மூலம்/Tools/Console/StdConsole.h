#pragma once

#include <windows.h>
#include "Win32Console.h"

class CStdConsole:public CWin32Console
{
protected:
    BOOL bStdAttachedToConsole;

    int fdIn  ;
    int fdOut ;
    int fdErr ;

    FILE *fpIn  ;
    FILE *fpOut ;
    FILE *fpErr ;

    FILE OrgStdIn  ;
    FILE OrgStdOut ;
    FILE OrgStdErr ;

    void ResetAttachStdToConsoleVars();
    static int Win32Handle_To_C_Handle(HANDLE Win32Handle);
public:
    CStdConsole(IN BOOL bAttachToParentProcess = TRUE);
    virtual ~CStdConsole();

    BOOL AttachStdToConsole();
    BOOL DetachStdFromConsole();
};