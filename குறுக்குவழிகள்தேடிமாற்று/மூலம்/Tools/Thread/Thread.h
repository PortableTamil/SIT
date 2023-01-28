/*
Copyright (C) 2011-2016 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2011-2016 Jacquelin POTIER <jacquelin.potier@free.fr>

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
// Object: manages threads, catching software and hardware exceptions
//-----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
//              Example of use
//////////////////////////////////////////////////////////////////////////
//
//#include "Thread.h"
//int GCnt = 0;
//DWORD WINAPI ThreadProc(
//                        LPVOID lpParameter
//                        )
//{
//    int a=5;
//    int b=0;
//    GCnt++;
//    switch (GCnt)
//    {
//    case 1:
//        return a/b;
//    case 2:
//        throw 1;
//    case 3:
//        break;
//    }
//    return 4;
//}
//
//void main()
//{
//    CThread Thread;
//    Thread.Start(ThreadProc,(LPVOID)1,0,0,TRUE);
//    Thread.WaitForSuccessFulEnd();
//}




#pragma once

#ifdef _DEBUG
    #ifndef _WIN32_WINNT 
        #define _WIN32_WINNT 0x0400 // for IsDebuggerPresent
    #endif
#endif

#include <windows.h>

// default stack size 1Mb in case object used inside a dll
#define CThread_ThreadsStackSize 0x100000
class CThread
{
public:
    typedef enum EXCEPTION_TYPE
    {
        ExceptionType_Hardware,
        ExceptionType_Software
    };
    typedef struct _EXCEPTION_INFOS
    {
        EXCEPTION_TYPE ExceptionType;
        EXCEPTION_POINTERS* pHardwareExceptionSEH;// valid only if ExceptionType == ExceptionType_Hardware
    }EXCEPTION_INFOS;

    // called on uncatched exception
    // use it to re-initialize some data signals in case of bAutoRestartOnException flag set
    typedef void (__stdcall *pfOnException)(EXCEPTION_INFOS* pExceptionInfos,PVOID UserParam);

    CThread(BOOL bAutoDeleteThreadObjectOnThreadEnd=FALSE);

    HANDLE Start(LPTHREAD_START_ROUTINE lpStartAddress);

    HANDLE Start(
                    LPTHREAD_START_ROUTINE lpStartAddress,
                    LPVOID lpParameter
                );

    HANDLE Start(
                    LPTHREAD_START_ROUTINE lpStartAddress,
                    LPVOID lpParameter,
                    BOOL bAutoRestartOnException,
                    BOOL bAskForCrashDumpOnHardwareException
                );

    HANDLE Start(
                    LPTHREAD_START_ROUTINE lpStartAddress,
                    LPVOID lpParameter,
                    pfOnException pOnException,PVOID OnExceptionUserParam,
                    BOOL bAutoRestartOnException,
                    BOOL bAskForCrashDumpOnHardwareException
                );

    HANDLE Start(
                    LPSECURITY_ATTRIBUTES lpThreadAttributes,
                    SIZE_T dwStackSize,
                    LPTHREAD_START_ROUTINE lpStartAddress,
                    LPVOID lpParameter,
                    DWORD dwCreationFlags,
                    LPDWORD lpThreadId,
                    pfOnException pOnException,PVOID OnExceptionUserParam,
                    BOOL bAutoRestartOnException,
                    BOOL bAskForCrashDumpOnHardwareException
                 );
    FORCEINLINE BOOL Stop(OPTIONAL DWORD ExitCode = 0)
	{
		// if thread is already stopped
		if (::WaitForSingleObject(this->hThread,0) == WAIT_OBJECT_0)
        {
            this->ResetThreadAttributes();
			return TRUE;
        }
        // else

		// thread is not stop, terminate it
		BOOL bRet = (::TerminateThread(this->hThread,ExitCode) !=0 );
        this->ResetThreadAttributes();
        return bRet;
	}
    BOOL WaitForSuccessFullEnd(OPTIONAL HANDLE hWaitCancelEvent = 0,OPTIONAL SIZE_T TimeOutInMs = INFINITE);
    FORCEINLINE HANDLE GetHandle(){return this->hThread;}
    // TlsAlloc, TlsFree, TlsSetValue, TlsGetValue
    FORCEINLINE SIZE_T Suspend(){return ::SuspendThread(this->hThread);}
    FORCEINLINE SIZE_T Resume(){return ::ResumeThread(this->hThread);}
    FORCEINLINE BOOL IsAlive()
    {
        return (!this->bEnded);
/*
return valid information until this->hThread is not closed
#if (_WIN32_WINNT>=0x0502)
        return (::GetThreadId(this->hThread) !=0) ;
#else
        FILETIME CreationTime,ExitTime,KernelTime,UserTime; 
        return (::GetThreadTimes(this->hThread,&CreationTime,&ExitTime,&KernelTime,&UserTime) !=0) ;
#endif
*/
    }
    
    FORCEINLINE SIZE_T GetThreadId(){return this->ThreadId;}
    FORCEINLINE void SetDefaultStackSize(IN DWORD DefaultStackSize){this->DefaultStackSize=DefaultStackSize;}
    virtual ~CThread(void);

protected:
    HANDLE hThread;
	DWORD ThreadId;
    DWORD DefaultStackSize;
    HANDLE hEventThreadRestarted;
    BOOL bEnded;
    BOOL bSuccessfulEnd;
    BOOL bAutoDeleteThreadObjectOnThreadEnd;
    pfOnException pOnException;
    PVOID OnExceptionUserParam;
    BOOL bAutoRestartOnException;
    BOOL bAskForCrashDumpOnHardwareException;
    LPTHREAD_START_ROUTINE lpStartAddress;
    LPVOID lpParameter;
    LPSECURITY_ATTRIBUTES lpThreadAttributes;
    SIZE_T dwStackSize;
    DWORD dwCreationFlags;

    void ResetThreadAttributes();
    void FreeMemory();
    FORCEINLINE void OnException(EXCEPTION_INFOS* pExceptionInfos);
    static DWORD WINAPI ThreadProcProxy(LPVOID lpParameter);
};
