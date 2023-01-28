/*
Copyright (C) 2011-2020 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2011 Jacquelin POTIER <jacquelin.potier@free.fr>

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
// Object: manages crash dump
//-----------------------------------------------------------------------------

/* usage 
//////////////////////////////////////////////////////////////////////////////////////////////
//    Step 1) in your code 
//////////////////////////////////////////////////////////////////////////////////////////////

try
{
    CExceptionHardware::RegisterTry();

    // your code goes here
}
catch( CExceptionHardware e ) // catch hardware exceptions
{
    // MessageBox(0,e.ExceptionText,_T("Error"),MB_OK|MB_ICONERROR | MB_SYSTEMMODAL);
    GenerateCrashDump( _T("c:\\mydump.dmp"), MiniDumpNormal, e.pExceptionInformation );
}

[catch (...) // optional only to catch software exception [exception launched by user code by "throw 1" or throw "My Error Text"]
{
    MessageBox(0,_T("Software Exception occurs"),_T("Error"),MB_OK|MB_ICONERROR | MB_SYSTEMMODAL);
    [throw;] // optional only to rethrow the current exception for an upper try/catch block
}]

//////////////////////////////////////////////////////////////////////////////////////////////
//    Step 2) To analyze crash dump
//////////////////////////////////////////////////////////////////////////////////////////////
Solution A) Visual Studio
    Open the dump file with visual studio and choose the "Debug" action
    Exception will occur, and next you can access stack, memory and if symbols are correct, code is displayed

Solution B) WinDbg
Open the dump file in WinDbg and use the following commands : 
.ecxr    to display the exception
k, kb, kc, kd, kp, kP, kv        to display stack backtrace


*/

#pragma once

#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <Dbghelp.h>
// #pragma comment (lib, "Dbghelp.lib") use dynamic loading to avoid Dbghelp to be loaded if not needed
class CCrashDump
{
protected :
    typedef BOOL (WINAPI *pfGetSaveFileName)(LPOPENFILENAME lpofn);

    typedef BOOL (WINAPI *pfMiniDumpWriteDump)(
                                                __in  HANDLE hProcess,
                                                __in  DWORD ProcessId,
                                                __in  HANDLE hFile,
                                                __in  MINIDUMP_TYPE DumpType,
                                                __in  PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                                __in  PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
                                                __in  PMINIDUMP_CALLBACK_INFORMATION CallbackParam
                                                );
public:
    static BOOL GenerateCrashDump(MINIDUMP_TYPE Flags = MiniDumpNormal /*MiniDumpValidTypeFlags*/, EXCEPTION_POINTERS* pSEH=NULL);
	static BOOL GenerateCrashDump(TCHAR* Path,MINIDUMP_TYPE Flags = MiniDumpNormal /*MiniDumpValidTypeFlags*/, EXCEPTION_POINTERS* pSEH=NULL);
    static BOOL GenerateCrashDumpEx(HANDLE hProcess, DWORD ProcessID,MINIDUMP_TYPE Flags = MiniDumpNormal /*MiniDumpValidTypeFlags*/, EXCEPTION_POINTERS* pSEH=NULL, DWORD ThreadId=0);
	static BOOL GenerateCrashDumpEx(HANDLE hProcess, DWORD ProcessID, TCHAR* Path,MINIDUMP_TYPE Flags = MiniDumpNormal /*MiniDumpValidTypeFlags*/, EXCEPTION_POINTERS* pSEH=NULL, DWORD ThreadId=0);
};