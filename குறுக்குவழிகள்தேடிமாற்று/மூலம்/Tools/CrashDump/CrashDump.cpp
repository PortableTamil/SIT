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

#include "CrashDump.h"

BOOL CCrashDump::GenerateCrashDump(MINIDUMP_TYPE Flags, EXCEPTION_POINTERS* pSEH)
{
	// get the process information
	HANDLE hProcess = ::GetCurrentProcess();
	DWORD ProcessID = ::GetProcessId(hProcess);
	DWORD ThreadId = ::GetCurrentThreadId();
	return CCrashDump::GenerateCrashDumpEx(hProcess, ProcessID,Flags, pSEH, ThreadId);
}

BOOL CCrashDump::GenerateCrashDumpEx(HANDLE hProcess, DWORD ProcessID,MINIDUMP_TYPE Flags, EXCEPTION_POINTERS* pSEH, DWORD ThreadId)
{
    // generate dump file name (ApplicationName.dmp)
    TCHAR DumpFileName[MAX_PATH];
    ::GetModuleFileName(::GetModuleHandle(NULL),DumpFileName,_countof(DumpFileName));
    TCHAR* psz=_tcsrchr(DumpFileName,'.');
    if (psz)
    {
        psz++;
        *psz=0;
    }
    _tcscat(DumpFileName,_T("dmp"));
    
    // dynamically load GetSaveFileNameW function (avoid static linking)
    HMODULE hModule = ::GetModuleHandle(_T("comdlg32.dll"));
    if (!hModule)
    {
        hModule = ::LoadLibrary(_T("comdlg32.dll"));
        if (!hModule)
            return FALSE;
    }
    pfGetSaveFileName pGetSaveFileName = (pfGetSaveFileName)::GetProcAddress(hModule,
#if (defined(UNICODE)||defined(_UNICODE))
                                                                            "GetSaveFileNameW"
#else
                                                                            "GetSaveFileNameA"
#endif
                                                                            );
    if (!pGetSaveFileName)
        return FALSE;

    // ask user for file name
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.lpstrFilter=_T("CrashDump (.dmp)\0*.dmp\0All(*.*)\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt=_T("dmp");
    ofn.lpstrFile=DumpFileName;
    ofn.nMaxFile=_countof(DumpFileName);

    if (!pGetSaveFileName(&ofn))
        return FALSE;

    // save dump
    return CCrashDump::GenerateCrashDumpEx(hProcess,ProcessID,DumpFileName,Flags,pSEH,ThreadId);
}

BOOL CCrashDump::GenerateCrashDump(TCHAR* Path,MINIDUMP_TYPE Flags, EXCEPTION_POINTERS* pSEH)
{
	// get the process information
	HANDLE hProcess = ::GetCurrentProcess();
	DWORD ProcessID = ::GetProcessId(hProcess);
	DWORD ThreadId = ::GetCurrentThreadId();
	return CCrashDump::GenerateCrashDumpEx(hProcess, ProcessID, Path,Flags, pSEH, ThreadId);
}

// ThreadId : optional only for ExceptionInformation if pSEH is set
BOOL CCrashDump::GenerateCrashDumpEx(HANDLE hProcess, DWORD ProcessID, TCHAR* Path,MINIDUMP_TYPE Flags, EXCEPTION_POINTERS* pSEH, DWORD ThreadId)
{
    // dynamically load MiniDumpWriteDump function (avoid static linking)
    HMODULE hModule = ::GetModuleHandle(_T("Dbghelp.dll"));
    if (!hModule)
    {
        hModule = ::LoadLibrary(_T("Dbghelp.dll"));
        if (!hModule)
            return FALSE;
    }
    pfMiniDumpWriteDump pMiniDumpWriteDump = (pfMiniDumpWriteDump)::GetProcAddress(hModule,"MiniDumpWriteDump");
    if (!pMiniDumpWriteDump)
        return FALSE;

    // open the file
    HANDLE hFile = ::CreateFile(Path,
                                GENERIC_READ|GENERIC_WRITE,
                                FILE_SHARE_DELETE|FILE_SHARE_READ,
                                NULL, 
                                CREATE_ALWAYS, 
                                FILE_ATTRIBUTE_NORMAL, 
                                NULL
                                );

    if(hFile == INVALID_HANDLE_VALUE)
        return FALSE;



    // store SEH info, into MINIDUMP_EXCEPTION_INFORMATION
    MINIDUMP_EXCEPTION_INFORMATION ExceptionInformation = {0};
    MINIDUMP_EXCEPTION_INFORMATION* pExceptionInformation = NULL;
    if(pSEH)
    {
        ExceptionInformation.ThreadId = ThreadId;
        ExceptionInformation.ExceptionPointers = pSEH;
        ExceptionInformation.ClientPointers = FALSE; // must be FALSE in our case
        pExceptionInformation = &ExceptionInformation;
    }

    // generate the crash dump
	/*
		hProcess : This handle must have PROCESS_QUERY_INFORMATION and PROCESS_VM_READ access to the process. 
					If handle information is to be collected then PROCESS_DUP_HANDLE access is also required. 
					The caller must also be able to get THREAD_ALL_ACCESS access to the threads in the process. 
	*/
    BOOL RetValue = pMiniDumpWriteDump(hProcess, ProcessID, hFile, Flags, pExceptionInformation, NULL, NULL);

    // close the file
    ::CloseHandle(hFile);

    return RetValue;
}