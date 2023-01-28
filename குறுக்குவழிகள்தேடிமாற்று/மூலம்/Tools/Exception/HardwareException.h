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

#pragma once

/* Sample of use

try
{
    CExceptionHardware::RegisterTry();

    // your code goes here
}
catch( CExceptionHardware e ) // catch hardware exceptions
{

MessageBox(0,e.ExceptionText,_T("Error"),MB_OK|MB_ICONERROR | MB_SYSTEMMODAL);

// optional to translate the current exception to an upper try/catch block
[RaiseException(e.pExceptionInformation->ExceptionRecord->ExceptionCode,
                e.pExceptionInformation->ExceptionRecord->ExceptionFlags,
                e.pExceptionInformation->ExceptionRecord->NumberParameters,
                e.pExceptionInformation->ExceptionRecord->ExceptionInformation);]
}

[catch (...) // optional only to catch software exception [exception launched by user code by "throw 1" or throw "My Error Text"]
{

MessageBox(0,_T("Software Exception occurs"),_T("Error"),MB_OK|MB_ICONERROR | MB_SYSTEMMODAL);

// optional to translate the current exception to an upper try/catch block
[throw;] 
}]
*/


#include <Windows.h>
#include <eh.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

// WARNING Break points are not catched by _set_se_translator !!!
// that means if you call function like ::DebugBreak() excpetion is not catched and make your software crash !!!
class CExceptionHardware
{
private:
    void GetExceptionText();
    void GetExceptionText(TCHAR* ExceptionString);
    static void __cdecl CExceptionHardware::se_translator_function(unsigned int u, EXCEPTION_POINTERS* pException);
public:
    TCHAR ExceptionText[MAX_PATH];
    unsigned int ExceptionCode; // GetExceptionCode() value
    EXCEPTION_POINTERS* pExceptionInformation; // GetExceptionInformation() value
    
    CExceptionHardware(unsigned int u, EXCEPTION_POINTERS* pException);
    ~CExceptionHardware(void);
    _se_translator_function static RegisterTry();
    void static UnregisterTry(_se_translator_function Previous_se_translator_function);
};
