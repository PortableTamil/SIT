/*
Copyright (C) 2004-2013 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004-2023 Jacquelin POTIER <jacquelin.potier@free.fr>

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
// Object: make an optimized output buffer (strcat like) function with reallocation if needed
//-----------------------------------------------------------------------------

#pragma once

#ifndef _DefineStringLen
    // _DefineStringLen : remove string \0 from array size
    #define _DefineStringLen(Array) (_countof(Array)-1)
#endif

#ifndef _DefineStringAndDefineStringSize
    // allow quicker Append call
    #define _DefineStringAndDefineStringSize(x) x,_DefineStringLen(x)
#endif


#include <windows.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)

#if ( defined(_DEBUG) && defined(CHECK_MEMORY_LEAKS) )
#include "../MemoryCheck/MemoryLeaksChecker.h"
#endif

class COutputString
#if ( defined(_DEBUG) && defined(CHECK_MEMORY_LEAKS) )
    :public CMemoryLeaksChecker
#endif
{
protected:
    TCHAR* String;  // pointer to string
    SIZE_T Size;    // _tcslen like : size without \0
    SIZE_T MaxSize; // max size including \0
public:
    // bDeleteStringOnObjectDestruction : when FALSE, user can continue to use pointer retreived through GetString() after object destruction
    // User is responsible of calling delete[] on the pointer returned by GetString()
    // /!\ as pointer can be internally changed by object operations, no functions must be called between the GetString() and object destruction call
    // example
    //          COutputString* pMyStr;
    //          pMyStr->Append(_T("Hello"));
    //          pMyStr->bDeleteStringOnObjectDestruction = FALSE;
    //          TCHAR* pString = pMyStr->GetString();
    //          // no pMyStr->XXX should occur
    //          delete pMyStr;
    //          // user can use pString as a standard TCHAR* allocated with new[]
    //          delete[] pString;
    BOOL bDeleteStringOnObjectDestruction;

    COutputString(SIZE_T InitialSize=4096)
#if ( defined(_DEBUG) && defined(CHECK_MEMORY_LEAKS) )
        :CMemoryLeaksChecker(TEXT("COutputString"))
#endif
    {
        this->bDeleteStringOnObjectDestruction = TRUE;
        this->MaxSize=InitialSize;
        this->String=new TCHAR[this->MaxSize];
        this->Clear();
    }
    ~COutputString(void)
    {
        if (bDeleteStringOnObjectDestruction)
            delete[] this->String;
    }
    
    void IncreaseCapacity(SIZE_T NewSize);

    void Append(TCHAR CharToAdd);
    // faster than AppendSafe, but StringToAddSize MUST be <= _tcslen(StringToAdd)
    void Append(TCHAR* StringToAdd,SIZE_T StringToAddSizeInTCHAR);
    // AppendSafe: slower than Append, but StringToAddSize can be >= _tcslen(StringToAdd)
    void AppendSafe(TCHAR* StringToAdd,SIZE_T StringToAddSizeInTCHAR); 
    FORCEINLINE void Append(TCHAR* StringToAdd){return this->Append(StringToAdd,_tcslen(StringToAdd));}
    FORCEINLINE void Append(COutputString* pOutputString){return this->Append(pOutputString->GetString(),pOutputString->GetLength());}
    void AppendW(WCHAR CharToAdd);
    void AppendW(WCHAR* UnicodeStringToAdd,SIZE_T StringToAddSizeInWCHAR);
    FORCEINLINE void AppendW(WCHAR* StringToAdd){return this->AppendW(StringToAdd,wcslen(StringToAdd));}
    void AppendA(CHAR CharToAdd);
    void AppendA(CHAR* AsciiStringToAdd,SIZE_T StringToAddSizeInCHAR);
    FORCEINLINE void AppendA(CHAR* StringToAdd){return this->AppendA(StringToAdd,strlen(StringToAdd));}

    FORCEINLINE void Prepend(TCHAR* StringToAdd){return this->Prepend(StringToAdd,_tcslen(StringToAdd));}
    void Prepend(TCHAR* StringToAdd,SIZE_T StringToAddSize);

    FORCEINLINE void Copy(TCHAR CharToCopy){this->Clear();this->Append(CharToCopy);}
    FORCEINLINE void Copy(TCHAR* StringToCopy,SIZE_T StringToCopySize){this->Clear();this->Append(StringToCopy,StringToCopySize);}
    FORCEINLINE void Copy(TCHAR* StringToCopy){this->Clear();this->Append(StringToCopy);}
    FORCEINLINE void Copy(COutputString* pOutputStringToCopy)
    {
        // we MUST check point. 
        // if (pOutputStringToCopy == this)
        // {
        //      in this case we will empty pOutputStringToCopy and this
        //      the reason is the following
        //      this->Clear(); remove this aka pOutputStringToCopy content
        //      this->Append(pOutputStringToCopy); according to previous line pOutputStringToCopy is now empty, so this content will be empty
        // }
        if (pOutputStringToCopy!=this)
        {
            this->Clear();
            this->Append(pOutputStringToCopy);
        }
    }

    // if after GetString, user change string content manually (like end string after _tcspbrk checking)
    // he HAS TO call UpdateAfterExternalContentChange function to update class members, else next Append will fail
    FORCEINLINE TCHAR* GetString(){return this->String;}
    // GetLength: return length in TCHAR without \0
    FORCEINLINE SIZE_T GetLength(){return this->Size;}
    FORCEINLINE BOOL IsEmpty(){return this->Size==0;}
    FORCEINLINE SIZE_T GetMaxSize(){return this->MaxSize;}
    FORCEINLINE void Clear(){*this->String=0;this->Size=0;}
    int Snprintf(SIZE_T MaxCount,const TCHAR* Format,...);
    int AppendSnprintf(SIZE_T MaxCount,const TCHAR* Format,...);// MaxCount : must include '\0'
    int AppendVsnprintf(SIZE_T MaxCount,const TCHAR* Format,va_list args);
    // Ellipse : return TRUE is Ellipse has been done, FALSE is current size is less than MaxWantedNbChars
    BOOL Ellipse(SIZE_T MaxWantedNbChars);
    // Ellipse : return TRUE is Ellipse has been done, FALSE is current size is less than MaxWantedNbChars
    BOOL EllipseWithout3Points(SIZE_T MaxWantedNbChars);

    BOOL RemoveStart(SIZE_T NbCharsAtStart);
    BOOL RemoveEnd(SIZE_T NbCharsAtEnd);
    BOOL RemovePart(SIZE_T RemoveStartIndex,SIZE_T RemovedSize);

    BOOL IsStartingWith(TCHAR c,BOOL bSensitive=TRUE);
    BOOL IsStartingWith(TCHAR* Str,BOOL bSensitive=TRUE);
    BOOL IsEndingWith(TCHAR c,BOOL bSensitive=TRUE);
    BOOL IsEndingWith(TCHAR* Str,BOOL bSensitive=TRUE);

    TCHAR GetLastChar();

    SIZE_T FindPosition(TCHAR c);
    SIZE_T ReverseFindPosition(TCHAR c);

    SIZE_T Replace(TCHAR OldCharToReplace,TCHAR NewReplacementChar);       // return the number of replacement(s) done
    SIZE_T Replace(TCHAR* OldStringToReplace,TCHAR* NewReplacementString); // return the number of replacement(s) done

    // if after GetString, user change string content manually (like end string after _tcspbrk checking)
    // he HAS TO call this function to update class members, else next Append will fail
    FORCEINLINE void UpdateAfterExternalContentChange(){this->Size=_tcslen(this->String);}
};

