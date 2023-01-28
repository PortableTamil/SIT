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

#include "Thread.h"
#include "..\Exception\HardwareException.h"
#include "..\CrashDump\CrashDump.h"

CThread::CThread(BOOL bAutoDeleteThreadObjectOnThreadEnd)
{
    this->hThread = NULL;
	this->ThreadId = NULL;
    this->DefaultStackSize = CThread_ThreadsStackSize;
    this->pOnException = NULL;
    this->OnExceptionUserParam = NULL;
    this->bAutoRestartOnException = FALSE;
    this->lpStartAddress = NULL;
    this->lpParameter = NULL;
    this->lpThreadAttributes = NULL;
    this->dwStackSize = 0;
    this->dwCreationFlags = 0;
    this->bEnded = TRUE;// so IsAlive return FALSE when Start has never been called
    this->bSuccessfulEnd = TRUE;
    this->bAskForCrashDumpOnHardwareException = FALSE;
    this->bAutoDeleteThreadObjectOnThreadEnd = bAutoDeleteThreadObjectOnThreadEnd;
    this->hEventThreadRestarted = ::CreateEvent(NULL,FALSE,FALSE,NULL);

}

CThread::~CThread(void)
{
    this->FreeMemory();
    if (this->hEventThreadRestarted)
    {
        ::CloseHandle(this->hEventThreadRestarted);
        this->hEventThreadRestarted = NULL;
    }
}
void CThread::FreeMemory()
{
    if (this->hThread)
    {
		// if thread calling destructor is not the launched thread, terminate the launched thread
		if (::GetCurrentThreadId()!=this->ThreadId)
            this->Stop();
    }
    ::ResetEvent(this->hEventThreadRestarted);
}

void CThread::ResetThreadAttributes()
{
    ::CloseHandle(this->hThread);
    this->hThread = NULL;
    this->ThreadId = NULL;
    // mark thread as ended
    this->bEnded = TRUE;
}

HANDLE CThread::Start(LPTHREAD_START_ROUTINE lpStartAddress)
{
    return this->Start(NULL,this->DefaultStackSize,lpStartAddress,NULL,0,NULL,NULL,NULL,FALSE,TRUE);
}

HANDLE CThread::Start(
                     LPTHREAD_START_ROUTINE lpStartAddress,
                     LPVOID lpParameter
                     )
{
    return this->Start(NULL,this->DefaultStackSize,lpStartAddress,lpParameter,0,NULL,NULL,NULL,FALSE,TRUE);
}
HANDLE CThread::Start(
                     LPTHREAD_START_ROUTINE lpStartAddress,
                     LPVOID lpParameter,
                     BOOL bAutoRestartOnException,
                     BOOL bAskForCrashDumpOnHardwareException
                     )
{
    return this->Start(NULL,this->DefaultStackSize,lpStartAddress,lpParameter,0,NULL,NULL,NULL,bAutoRestartOnException,bAskForCrashDumpOnHardwareException);
}

HANDLE CThread::Start(
                     LPTHREAD_START_ROUTINE lpStartAddress,
                     LPVOID lpParameter,
                     pfOnException pOnException,PVOID OnExceptionUserParam,
                     BOOL bAutoRestartOnException,
                     BOOL bAskForCrashDumpOnHardwareException
                     )
{
    return this->Start(NULL,this->DefaultStackSize,lpStartAddress,lpParameter,0,NULL,pOnException,OnExceptionUserParam,bAutoRestartOnException,bAskForCrashDumpOnHardwareException);
}

HANDLE CThread::Start(
                     LPSECURITY_ATTRIBUTES lpThreadAttributes,
                     SIZE_T dwStackSize,
                     LPTHREAD_START_ROUTINE lpStartAddress,
                     LPVOID lpParameter,
                     DWORD dwCreationFlags,
                     LPDWORD lpThreadId,
                     pfOnException pOnException,PVOID OnExceptionUserParam,
                     BOOL bAutoRestartOnException,
                     BOOL bAskForCrashDumpOnHardwareException
                     )
{
    // free previous memory, stop previous launched thread if any
    this->FreeMemory();

    // store exception callback informations
    this->OnExceptionUserParam = OnExceptionUserParam;
    if (::IsBadCodePtr((FARPROC)pOnException))
        this->pOnException = NULL;
    else
        this->pOnException = pOnException;
    this->bAskForCrashDumpOnHardwareException = bAskForCrashDumpOnHardwareException;

    // store thread required var
    this->bAutoRestartOnException = bAutoRestartOnException;
    this->lpStartAddress = lpStartAddress;
    this->lpParameter = lpParameter;
    if (bAutoRestartOnException)
    {
        this->lpThreadAttributes = lpThreadAttributes;
        this->dwStackSize = dwStackSize;
        this->lpStartAddress = lpStartAddress;
        this->dwCreationFlags = dwCreationFlags;
    }

    // reset success flag
    this->bEnded = FALSE;
    this->bSuccessfulEnd = FALSE;

    // create thread with a proxy function
	this->ThreadId = NULL;
    this->hThread = ::CreateThread(lpThreadAttributes,dwStackSize,CThread::ThreadProcProxy,this,dwCreationFlags,&this->ThreadId);
	if (lpThreadId)
		*lpThreadId = this->ThreadId;

    // return thread value
    return this->hThread;

}

BOOL CThread::WaitForSuccessFullEnd(HANDLE hWaitCancelEvent,SIZE_T TimeOutInMs)
{
    HANDLE pHandle[2];
    SIZE_T HandleCount;
    SIZE_T TimeOut;
    SIZE_T Result;
    SIZE_T Ticks;

    // WaitForSuccessFullEnd is incompatible with bAutoDeleteThreadObjectOnThreadEnd
    if (this->bAutoDeleteThreadObjectOnThreadEnd)
    {
#ifdef _DEBUG
        if (::IsDebuggerPresent())
            ::DebugBreak();
#endif
        return FALSE;
    }

    if (!this->IsAlive())
        return TRUE;

    Ticks = 0;
    TimeOut = TimeOutInMs;

    if (hWaitCancelEvent)
    {
        HandleCount = 2;
        pHandle[1]=hWaitCancelEvent;
    }
    else 
    {
        HandleCount = 1;
    }

    for(;;)
    {
        // update pHandle[0] at each iteration, as this->hThread changes at each iteration
        pHandle[0] = this->hThread;

        // if timeout is not INFINITE
        if (TimeOut != INFINITE)
        {
            // get number of ticks in ms
            Ticks = ::GetTickCount();
        }

        // wait for events and timeout
        Result = ::WaitForMultipleObjects((DWORD)HandleCount,pHandle,FALSE,(DWORD)TimeOut);
        switch (Result)
        {
        case WAIT_OBJECT_0: // thread end
            // if end of thread was successful
            if (this->bSuccessfulEnd)
                return TRUE;
            // else

            if (!this->bAutoRestartOnException)
                return FALSE;
            // else

            // wait thread restart
            ::WaitForSingleObject(this->hEventThreadRestarted,500);

            // if timeout is not INFINITE
            if (TimeOut != INFINITE)
            {
                // get number of elapsed ms
                Ticks = ::GetTickCount() - Ticks;

                // compute remaining timeout
                TimeOut -= Ticks;
            }
            break;
        case WAIT_OBJECT_0+1:// cancel event
        case WAIT_TIMEOUT:   // timeout
        default:
            return FALSE;
        }
    }

}
DWORD CThread::ThreadProcProxy(LPVOID lpParameter)
{
    CThread* pThread = (CThread*)lpParameter;
    DWORD RetValue = (DWORD)(-1);
    BOOL bExceptionOccurred = FALSE;
#ifndef _DEBUG
    EXCEPTION_INFOS ExceptionInfos;

    try
    {
        CExceptionHardware::RegisterTry();
#endif
        // call thread routine
        RetValue = pThread->lpStartAddress(pThread->lpParameter);
#ifndef _DEBUG
    }
    catch( CExceptionHardware e ) // catch hardware exceptions
    {
#ifdef _DEBUG
        OutputDebugString(e.ExceptionText);
        if (::IsDebuggerPresent())
            ::DebugBreak();// let user debug it's software using call stack debug window
#endif
        // do report before relaunching thread to avoid looping message box before first saving
        if (pThread->bAskForCrashDumpOnHardwareException)
        {
            // dynamically load MessageBox function (avoid static linking)
            HMODULE hModule = ::GetModuleHandle(_T("user32.dll"));
            if (!hModule)
            {
                hModule = ::LoadLibrary(_T("user32.dll"));
                if (!hModule)
                    return FALSE;
            }

            typedef int (WINAPI *pfMessageBoxW)(HWND hWnd,LPCWSTR lpText,LPCWSTR lpCaption,UINT uType);
            pfMessageBoxW pMessageBoxW = (pfMessageBoxW)::GetProcAddress(hModule,"MessageBoxW");
            if (pMessageBoxW)
            {
                if (
                    pMessageBoxW(NULL,
                                L"An exception has occurred.\r\nDo you want to save a crash dump file to report error to author ?",
                                L"Exception",
                                MB_ICONQUESTION | MB_YESNO | MB_SYSTEMMODAL
                                )
                                == IDYES
                    )
                {
                    CCrashDump::GenerateCrashDump(MiniDumpNormal,e.pExceptionInformation);
                }
            }
        }

        ExceptionInfos.ExceptionType = ExceptionType_Hardware;
        ExceptionInfos.pHardwareExceptionSEH = e.pExceptionInformation;
        pThread->OnException(&ExceptionInfos);
        bExceptionOccurred = TRUE;
    }
    catch (...)
    {
#ifdef _DEBUG
        OutputDebugString(_T("Software Exception"));
        if (::IsDebuggerPresent())
            ::DebugBreak();// let user debug it's software using call stack debug window
#endif
        ExceptionInfos.ExceptionType = ExceptionType_Software;
        pThread->OnException(&ExceptionInfos);
        bExceptionOccurred = TRUE;
    }
#endif

    if (!bExceptionOccurred)
        pThread->bSuccessfulEnd = TRUE;

    pThread->bEnded = TRUE;

    // delete thread if no auto restart on exception or if auto restart on exception and no exception
    if ( pThread->bAutoDeleteThreadObjectOnThreadEnd 
         && ( (!pThread->bAutoRestartOnException) || (pThread->bAutoRestartOnException && !bExceptionOccurred) )
         )
        delete pThread;

    return RetValue;
}

void CThread::OnException(EXCEPTION_INFOS* pExceptionInfos)
{
    // call exception callback if any
    if (this->pOnException)
        this->pOnException(pExceptionInfos,this->OnExceptionUserParam);

    // close handle
    if (this->hThread)
    {
        this->ResetThreadAttributes();
    }

	this->ThreadId = NULL;
    // if auto-restart is query
    if (this->bAutoRestartOnException)
    {
        // remove suspended state if any
        this->dwCreationFlags = this->dwCreationFlags & (~CREATE_SUSPENDED);
        // Use all same parameter as first thread creation
        this->hThread = ::CreateThread(this->lpThreadAttributes,this->dwStackSize,CThread::ThreadProcProxy,this,this->dwCreationFlags,&this->ThreadId);

        ::SetEvent(this->hEventThreadRestarted);
    }
}
