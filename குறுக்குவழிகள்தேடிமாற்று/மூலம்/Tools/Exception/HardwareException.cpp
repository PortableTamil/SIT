/*
Copyright (C) 2004-2015 Jacquelin POTIER <jacquelin.potier@free.fr>
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

#include "HardwareException.h"

#ifndef _countof
	#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

// to catch ALL exceptions (even memory access) assume to have the /EHa option 
// VS2003 "project" / "properties" / "C/C++" / "Command Line" / "Additional options" / "/EHa"
// VS2005 "project" / "properties" / "C/C++" / "Code Generation" / "Enable C++ exceptions"  --> "Yes With SEH Exceptions (/EHa)"
# pragma message (__FILE__ " Information : to catch ALL exceptions (even memory access) assume to have the /EHa option enable for project\r\n")


/* Sample of use

try
{
    CExceptionHardware::RegisterTry();

    // your code goes here
}
catch( CExceptionHardware e ) // catch hardware exceptions
{
    MessageBox(0,e.ExceptionText,_T("Error"),MB_OK|MB_ICONERROR | MB_SYSTEMMODAL);
}

[catch (...) // optional only to catch software exception [exception launched by user code by "throw 1" or throw "My Error Text"]
{
    MessageBox(0,_T("Software Exception occurs"),_T("Error"),MB_OK|MB_ICONERROR | MB_SYSTEMMODAL);
    [throw;] // optional only to rethrow the current exception for an upper try/catch block
}]

*/

CExceptionHardware::CExceptionHardware(unsigned int u, EXCEPTION_POINTERS* pException)
{
    this->ExceptionCode=u;
    this->pExceptionInformation=pException;
    this->GetExceptionText();
}

CExceptionHardware::~CExceptionHardware(void)
{
}
// se_translator_function: exception callback
void __cdecl CExceptionHardware::se_translator_function(unsigned int u, EXCEPTION_POINTERS* pException)
{
    throw CExceptionHardware(u,pException);
}
// set CExceptionHardware for current try block
_se_translator_function CExceptionHardware::RegisterTry()
{
    return _set_se_translator(CExceptionHardware::se_translator_function);
}
void CExceptionHardware::UnregisterTry(_se_translator_function Previous_se_translator_function)
{
    _set_se_translator(Previous_se_translator_function);
}

void CExceptionHardware::GetExceptionText(TCHAR* ExceptionString)
{
    _sntprintf(this->ExceptionText, MAX_PATH, 
        _T("%s at address 0x%p."),
        ExceptionString,
        (PBYTE)this->pExceptionInformation->ExceptionRecord->ExceptionAddress);
}

void CExceptionHardware::GetExceptionText()
{
    switch(this->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION: 
        {
            TCHAR sz[MAX_PATH];
            _tcscpy(sz,_T("EXCEPTION_ACCESS_VIOLATION"));
            if (this->pExceptionInformation->ExceptionRecord->NumberParameters>=2)
            {
                if (this->pExceptionInformation->ExceptionRecord->ExceptionInformation[0]==0)
                    _sntprintf(sz,_countof(sz),_T("EXCEPTION_ACCESS_VIOLATION reading location 0x%p"),(PBYTE)(this->pExceptionInformation->ExceptionRecord->ExceptionInformation[1]));
                else
                    _sntprintf(sz,_countof(sz),_T("EXCEPTION_ACCESS_VIOLATION writing location 0x%p"),(PBYTE)(this->pExceptionInformation->ExceptionRecord->ExceptionInformation[1]));
            }
            this->GetExceptionText(sz);
        }
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        this->GetExceptionText(_T("EXCEPTION_DATATYPE_MISALIGNMENT"));
        break;
    case EXCEPTION_BREAKPOINT:
        this->GetExceptionText(_T("EXCEPTION_BREAKPOINT"));
        break;
    case EXCEPTION_SINGLE_STEP:
        this->GetExceptionText(_T("EXCEPTION_SINGLE_STEP"));
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        this->GetExceptionText(_T("EXCEPTION_ARRAY_BOUNDS_EXCEEDED"));
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        this->GetExceptionText(_T("EXCEPTION_FLT_DENORMAL_OPERAND"));
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        this->GetExceptionText(_T("EXCEPTION_FLT_DIVIDE_BY_ZERO"));
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        this->GetExceptionText(_T("EXCEPTION_FLT_INEXACT_RESULT"));
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        this->GetExceptionText(_T("EXCEPTION_FLT_INVALID_OPERATION"));
        break;
    case EXCEPTION_FLT_OVERFLOW:
        this->GetExceptionText(_T("EXCEPTION_FLT_OVERFLOW"));
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        this->GetExceptionText(_T("EXCEPTION_FLT_STACK_CHECK"));
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        this->GetExceptionText(_T("EXCEPTION_FLT_UNDERFLOW"));
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        this->GetExceptionText(_T("EXCEPTION_INT_DIVIDE_BY_ZERO"));
        break;
    case EXCEPTION_INT_OVERFLOW:
        this->GetExceptionText(_T("EXCEPTION_INT_OVERFLOW"));
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        this->GetExceptionText(_T("EXCEPTION_PRIV_INSTRUCTION"));
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        this->GetExceptionText(_T("EXCEPTION_IN_PAGE_ERROR"));
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        this->GetExceptionText(_T("EXCEPTION_ILLEGAL_INSTRUCTION"));
        break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        this->GetExceptionText(_T("EXCEPTION_NONCONTINUABLE_EXCEPTION"));
        break;
    case EXCEPTION_STACK_OVERFLOW:
        this->GetExceptionText(_T("EXCEPTION_STACK_OVERFLOW"));
        break;
    case EXCEPTION_INVALID_DISPOSITION:
        this->GetExceptionText(_T("EXCEPTION_INVALID_DISPOSITION"));
        break;
    case EXCEPTION_GUARD_PAGE:
        this->GetExceptionText(_T("EXCEPTION_GUARD_PAGE"));
        break;
    case EXCEPTION_INVALID_HANDLE:
        this->GetExceptionText(_T("EXCEPTION_INVALID_HANDLE"));
        break;
    default:
        _sntprintf(this->ExceptionText, MAX_PATH,
                  _T("Unknown exception at address 0x%p."),
                  (PBYTE)this->pExceptionInformation->ExceptionRecord->ExceptionAddress);
        break;
    }
}