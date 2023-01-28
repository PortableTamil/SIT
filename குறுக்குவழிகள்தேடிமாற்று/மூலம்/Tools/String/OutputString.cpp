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
#include "OutputString.h"
#include <stdio.h>

#pragma intrinsic (memcpy,memset,memcmp)

void COutputString::Prepend(TCHAR* StringToAdd,SIZE_T StringToAddSize)
{
    SIZE_T MinRequiredLen = this->Size + StringToAddSize+1;
    if ( MinRequiredLen > this->MaxSize)
    {
        // warning : != that Increase size

        // store current string buffer to TmpString
        TCHAR* TmpString = this->String;
        // double string size at least and assume that size is enough to store StringToAdd
        this->MaxSize = __max(this->MaxSize*2,MinRequiredLen*2);// *2 to reduce as possible the number of allocation / delete
        // allocate a new buffer
        this->String = new TCHAR[this->MaxSize];
        if (this->Size)
        {
            // copy new content to allocated string
            memcpy(this->String,StringToAdd,StringToAddSize*sizeof(TCHAR));
            // copy previous content to new allocated string
            memcpy(&this->String[StringToAddSize],StringToAdd,(this->Size+1)*sizeof(TCHAR));
        }

        // delete previously allocated buffer
        delete[] TmpString;

        this->Size+=StringToAddSize;

        return;
    }

    // change old string position first
    memmove(&this->String[StringToAddSize],this->String,(this->Size+1)*sizeof(TCHAR));

    // copy new content
    memcpy(this->String,StringToAdd,StringToAddSize*sizeof(TCHAR));

    this->Size+=StringToAddSize;
}

size_t COutputString_tcsncpy_counted(TCHAR* Dst,TCHAR* Src,size_t Max)
{
    TCHAR* SrcEnd = Src+Max;
    TCHAR* pDst = Dst;
    TCHAR c;
    for( ;(Src<SrcEnd);pDst++,Src++)
    {
        c = *Src;
        *pDst = c;
        if (c==0)
            break; // avoid end of loop increment
    };

    return (pDst-Dst);
};

// StringToAddSize size in TCHAR
// faster than AppendSafe, but StringToAddSize MUST be <= _tcslen(StringToAdd)
void COutputString::Append(TCHAR* StringToAdd,SIZE_T StringToAddSize)
{
#ifdef _DEBUG
    if ( StringToAddSize > _tcslen(StringToAdd) )
        ::DebugBreak();
#endif

    if ( ( this->Size + StringToAddSize + 1) > this->MaxSize)
    {
        this->IncreaseCapacity(this->Size + StringToAddSize+1);// +1 for \0
    }
    // do the _tcscat
    // optimize as we know current string size
    // _tcscat(this->String,StringToAdd);
    // _tcs,cpy(&this->String[this->Size],StringToAdd,StringToAddSize);
    // as we know size optimize even more using memcpy 
    // trouble : no respect of '\0' if inside StringToAdd
    memcpy(&this->String[this->Size],StringToAdd,StringToAddSize*sizeof(TCHAR));
    this->Size+=StringToAddSize;

    // assume string end
    this->String[this->Size]=0;
}

// slower than Append, but StringToAddSize can be >= _tcslen(StringToAdd)
void COutputString::AppendSafe(TCHAR* StringToAdd,SIZE_T StringToAddSize)
{
    if ( ( this->Size + StringToAddSize + 1) > this->MaxSize)
    {
        this->IncreaseCapacity(this->Size + StringToAddSize+1);// +1 for \0
    }
    // do the _tcscat
    // optimize as we know current string size
    // _tcscat(this->String,StringToAdd);
    // _tcsncpy(&this->String[this->Size],StringToAdd,StringToAddSize);
    // as we know size optimize even more using memcpy 
    // trouble : no respect of '\0'
    // memcpy(&this->String[this->Size],StringToAdd,StringToAddSize*sizeof(TCHAR));

    this->Size+=COutputString_tcsncpy_counted(&this->String[this->Size],StringToAdd,StringToAddSize);
    // assume string end
    this->String[this->Size]=0;
}

void COutputString::Append(TCHAR CharToAdd)
{  
    if (CharToAdd=='\0')
        return;

    if ( ( this->Size + 2) > this->MaxSize)// +2: +1 for \0 +1 for current char
    {
        this->IncreaseCapacity(this->Size + 2);
    }
    // do the _tcscat
    // // optimize as we know current string size
    // // _tcscat(this->String,StringToAdd);
    // _tcscpy(&this->String[this->Size],StringToAdd);
    // optimize : avoid _tcscpy call
    TCHAR* pAdd;
    pAdd = &this->String[this->Size];
    *pAdd = CharToAdd;
    pAdd++;
    *pAdd = 0;
    this->Size++;
}

void COutputString::AppendW(WCHAR CharToAdd)
{
#if (defined(UNICODE)||defined(_UNICODE))
    this->Append(CharToAdd);
#else
    CHAR ch;
    ::WideCharToMultiByte(CP_ACP, 0, &CharToAdd, 1, &ch, 1, NULL, NULL);
    this->Append(ch);
#endif
}

void COutputString::AppendW(WCHAR* UnicodeStringToAdd,SIZE_T StringToAddSize)
{
#if (defined(UNICODE)||defined(_UNICODE))
    this->Append(UnicodeStringToAdd,StringToAddSize);
#else
    // increase internal buffer and do direct output into internal buffer
    if ( ( this->Size + StringToAddSize + 1) > this->MaxSize)
    {
        this->IncreaseCapacity(this->Size + StringToAddSize+1);// +1 for \0
    }
    int NbBytesWithoutZero = ::WideCharToMultiByte(CP_ACP, 0, UnicodeStringToAdd, (int)StringToAddSize, &this->String[this->Size], (int)StringToAddSize, NULL, NULL);
    if (NbBytesWithoutZero>0)
    {
#ifdef _DEBUG
        if ( StringToAddSize != NbBytesWithoutZero )
            ::DebugBreak();
#endif

        // this->Size+=StringToAddSize;
        this->Size+=NbBytesWithoutZero; 

        // assume string end
        this->String[this->Size]=0;
    }
#endif
}
void COutputString::AppendA(CHAR CharToAdd)
{
#if (defined(UNICODE)||defined(_UNICODE))
    WCHAR wch;
    ::MultiByteToWideChar(CP_ACP, 0, &CharToAdd, 1, &wch, 1);
    this->Append(wch);
#else
    this->Append(CharToAdd);
#endif
}
void COutputString::AppendA(CHAR* AsciiStringToAdd,SIZE_T StringToAddSize)
{
#if (defined(UNICODE)||defined(_UNICODE))
    // increase internal buffer and do direct output into internal buffer
    if ( ( this->Size + StringToAddSize + 1) > this->MaxSize)
    {
        this->IncreaseCapacity(this->Size + StringToAddSize+1);// +1 for \0
    }

    int NbWcharWithoutZero = ::MultiByteToWideChar(CP_ACP, 0, AsciiStringToAdd, (int)StringToAddSize, &this->String[this->Size], (int)StringToAddSize);
    if (NbWcharWithoutZero>0)
    {
#ifdef _DEBUG
        if ( StringToAddSize != NbWcharWithoutZero )
            ::DebugBreak();
#endif
        // this->Size+=StringToAddSize;
        this->Size+=NbWcharWithoutZero;
        
        // assume string end
        this->String[this->Size]=0;
    }
#else
    this->Append(AsciiStringToAdd,StringToAddSize);
#endif
}


void COutputString::IncreaseCapacity(SIZE_T MinRequiredLen)
{
    if (MinRequiredLen<this->MaxSize)
        return;

    // store current string buffer to TmpString
    TCHAR* TmpString = this->String;
    // double string size at least and assume that size is enough to store StringToAdd
    this->MaxSize = __max(this->MaxSize*2,MinRequiredLen*2);// *2 to reduce as possible the number of allocation / delete
    // allocate a new buffer
    this->String = new TCHAR[this->MaxSize];
    // copy previous content to new allocated string
    // optimize : avoid checking of \0
    // _tcscpy(this->String,TmpString);
    if (this->Size)
        memcpy(this->String,TmpString,(this->Size+1)*sizeof(TCHAR));
    else
        *this->String=0;

    // delete previously allocated buffer
    delete[] TmpString;
}

// MaxCount : must include '\0'
int COutputString::Snprintf(SIZE_T MaxCount,const TCHAR* Format,...)
{
    va_list args;
    va_start(args, Format);
    SIZE_T MinSize = MaxCount+1;
    if (this->MaxSize<MinSize)
        this->IncreaseCapacity(MinSize);
    int RetValue = _vsntprintf(this->String,MaxCount,Format,args);
    this->String[MaxCount-1]=0;
    // _vsnprintf and _vsnwprintf return the number of characters written, not including the terminating null character, or a negative value if an output error occurs
    // If the number of characters to write exceeds count, then count characters are written and –1 is returned
    if (RetValue>0)
        this->Size = (SIZE_T)RetValue;
    else
        this->Size = _tcslen(this->String);

    va_end(args);
    return RetValue;
}

// MaxCount : must include '\0'
int COutputString::AppendSnprintf(SIZE_T MaxCount,const TCHAR* Format,...)
{
    va_list args;
    va_start(args, Format);
    int RetValue = this->AppendVsnprintf(MaxCount,Format,args);
    va_end(args);
    return RetValue;
}

// MaxCount : must include '\0'
int COutputString::AppendVsnprintf(SIZE_T MaxCount,const TCHAR* Format,va_list args)
{
    SIZE_T MinSize = this->Size+MaxCount+1;
    if (this->MaxSize<MinSize)
        this->IncreaseCapacity(MinSize);
    // take pointer after IncreaseCapacity call as IncreaseCapacity can affect this->String
    TCHAR* Ptr = this->String+this->Size;
    int RetValue = _vsntprintf(Ptr,MaxCount,Format,args);
    // _vsnprintf and _vsnwprintf return the number of characters written, not including the terminating null character, or a negative value if an output error occurs
    // If the number of characters to write exceeds count, then count characters are written and –1 is returned
    if (RetValue>0)
        this->Size+= (SIZE_T)RetValue;
    else
    {
        Ptr[MaxCount-1]=0;
        this->Size += _tcslen(Ptr);
    }
    return RetValue;
}

//-----------------------------------------------------------------------------
// Name: EllipseWithout3Points
// Object: Ellipse current string representation without changing end to "..."
// Parameters :
//      SIZE_T MaxWantedNbChars : max number of char required including \0
// Return : return TRUE is Ellipse has been done, FALSE is current size is less than MaxWantedNbChars
//-----------------------------------------------------------------------------
BOOL COutputString::EllipseWithout3Points(SIZE_T MaxWantedNbChars)
{
    // MaxWantedNbChars : max number of char required including \0
    // --> must be at least 1
    if (MaxWantedNbChars<=1)
    {
        *this->String=0;
        this->Size=0;
    }
    // if string size is smaller than required char numbers
    if ( this->Size <= MaxWantedNbChars)
        return FALSE;
    // else: (this->Size > MaxWantedNbChars)

    this->Size = MaxWantedNbChars-1;// -1 \0 is not taken into account for length
    this->String[this->Size]=0;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Ellipse
// Object: Ellipse current string representation
// Parameters :
//      SIZE_T MaxWantedNbChars : max number of char required including \0
// Return : return TRUE is Ellipse has been done, FALSE is current size is less than MaxWantedNbChars
//-----------------------------------------------------------------------------
BOOL COutputString::Ellipse(SIZE_T MaxWantedNbChars)
{
    // MaxWantedNbChars : max number of char required including \0
    // --> must be at least 1
    if (MaxWantedNbChars<=1)
    {
        *this->String=0;
        this->Size=0;
    }
    // if string size is smaller than required char numbers
    if ( this->Size <= MaxWantedNbChars)
        return FALSE;
    // else: (this->Size > MaxWantedNbChars)

    // if possible, add "..." at the end of string to tell user string representation is not fully completed
    if (MaxWantedNbChars>3)
    {
        TCHAR* pc = &this->String[MaxWantedNbChars-4];
        _tcscpy(pc,_T("..."));
    }
    else
    {
        this->String[MaxWantedNbChars-1]=0;
    }
    this->Size = MaxWantedNbChars-1;// -1 \0 is not taken into account for length

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RemoveStart
// Object: remove first n char of the string
// Parameters :
//      SIZE_T NbCharsAtStart : number of char to remove
// Return : return TRUE
//-----------------------------------------------------------------------------
BOOL COutputString::RemoveStart(SIZE_T NbCharsAtStart)
{
    // if string size is smaller than required char numbers
    if ( this->Size <= NbCharsAtStart)
        this->Clear();
    else // (this->Size > NbCharsAtEnd)
    {
        this->Size-=NbCharsAtStart;
        memmove(this->String,&this->String[NbCharsAtStart],(this->Size+1)*sizeof(TCHAR));// (this->Size+1) to move \0 to
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RemoveEnd
// Object: remove last n char of the string
// Parameters :
//      SIZE_T NbCharsAtEnd : number of char to remove
// Return : return TRUE
//-----------------------------------------------------------------------------
BOOL COutputString::RemoveEnd(SIZE_T NbCharsAtEnd)
{
    // if string size is smaller than required char numbers
    if ( this->Size <= NbCharsAtEnd)
        this->Clear();
    else // (this->Size > NbCharsAtEnd)
    {
        this->Size-=NbCharsAtEnd;
        this->String[this->Size]=0;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: RemovePart
// Object: remove RemovedSize char of the string starting at position RemoveStartIndex
// Parameters :
//      IN RemoveStartIndex : start index at wich removal begins
//      IN SIZE_T RemovedSize : number of char to remove
// Return : return TRUE
//-----------------------------------------------------------------------------
BOOL COutputString::RemovePart(SIZE_T RemoveStartIndex,SIZE_T RemovedSize)
{
    if (RemoveStartIndex == 0)
        this->RemoveStart(RemovedSize);
    else
    {
        SIZE_T MaxRemovalSize;
        MaxRemovalSize = this->Size-RemoveStartIndex;
        if (RemovedSize>=MaxRemovalSize)
        {
            this->RemoveEnd(MaxRemovalSize);
        }
        else
        {
            memmove(&this->String[RemoveStartIndex],&this->String[RemoveStartIndex+RemovedSize],RemovedSize*sizeof(TCHAR));
            this->Size-=RemovedSize;
            this->String[this->Size]=0;
        }
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: FindPosition
// Object: find first char position in the string
// Parameters :
//      TCHAR c : char to search for
// Return : 0 based position, -1 on error
//-----------------------------------------------------------------------------
SIZE_T COutputString::FindPosition(TCHAR c)
{
    if (this->Size == 0)
        return (SIZE_T)-1;
    TCHAR* pc;
    SIZE_T Position;
    for (pc = this->String, Position = 0;
        Position<this->Size;
        pc++,Position++
        )
    {
        if (*pc == c)
            return Position;
    }

    // not found
    return (SIZE_T)-1;
}

//-----------------------------------------------------------------------------
// Name: FindPosition
// Object: find last char position in the string
// Parameters :
//      TCHAR c : char to search for
// Return : 0 based position, -1 on error
//-----------------------------------------------------------------------------
SIZE_T COutputString::ReverseFindPosition(TCHAR c)
{
    if (this->Size == 0)
        return (SIZE_T)-1;
    TCHAR* pc;
    SIZE_T Position;
    for (pc = &this->String[this->Size-1], Position = this->Size-1;
        Position;
        pc--,Position--
        )
    {
        if (*pc == c)
            return Position;
    }
    // test for Position == 0
    if (*this->String == c)
        return 0;

    // not found
    return (SIZE_T)-1;
}

BOOL COutputString::IsStartingWith(TCHAR c,BOOL bSensitive)
{
    if (this->Size<1)
        return FALSE;
    if (bSensitive)
        return (*this->String == c);
    else
        return ( _tolower(*this->String) == _tolower(c));
}
BOOL COutputString::IsStartingWith(TCHAR* Str,BOOL bSensitive)
{
    if (Str==NULL)
        return FALSE;
    SIZE_T Len = _tcslen(Str);
    if (this->Size<Len)
        return FALSE;
    if (bSensitive)
    {
        // return (_tcsncmp(this->String,Str,Len)==0);
        return (memcmp( this->String, Str, Len*sizeof(TCHAR) )==0);
    }
    else
        return (_tcsnicmp(this->String,Str,Len)==0);
}

TCHAR COutputString::GetLastChar()
{
    if (this->Size<1)
        return '\0';

    return (this->String[this->Size-1]);
}

BOOL COutputString::IsEndingWith(TCHAR c,BOOL bSensitive)
{
    if (this->Size<1)
        return FALSE;
    if (bSensitive)
        return (this->String[this->Size-1] == c);
    else
        return ( _tolower(this->String[this->Size-1]) == _tolower(c));
}

BOOL COutputString::IsEndingWith(TCHAR* Str,BOOL bSensitive)
{
    if (Str==NULL)
        return FALSE;
    SIZE_T Len = _tcslen(Str);
    if (this->Size<Len)
        return FALSE;

    if (bSensitive)
    {
        // return (_tcsncmp(&this->String[this->Size-Len],Str,Len)==0);
        return (memcmp( &this->String[this->Size-Len],Str,Len*sizeof(TCHAR) )==0);
    }
    else
        return (_tcsnicmp(&this->String[this->Size-Len],Str,Len)==0);
}

SIZE_T COutputString::Replace(TCHAR OldCharToReplace,TCHAR NewReplacementChar)
{
    TCHAR* pc;
    SIZE_T NbOccurrences = 0;

    // for each char of our string
    for(pc = this->String;*pc;pc++)
    {
        if (*pc == OldCharToReplace)
        {
            // increase occurrence count
            NbOccurrences++;

            // replace char in our string
            *pc = NewReplacementChar;
        }
    }

    // return number of occurrence
    return NbOccurrences;
}

SIZE_T COutputString::Replace(TCHAR* OldStringToReplace,TCHAR* NewReplacementString)
{
    SIZE_T OldStrSize;
    SIZE_T NewStrSize;
    SSIZE_T SingleReplacementExpand; // can be 0 if length are equal or negative is new string is shortest than old one
    SIZE_T NewStringMaxSize = this->GetLength()+1;

    // check against OldStringToReplace to avoid a division by 0 next
    if (*OldStringToReplace==0)
        return 0;

    // get length of search and replace string
    OldStrSize = _tcslen(OldStringToReplace);
    NewStrSize = _tcslen(NewReplacementString);

    // get mas size of resulting string
    SingleReplacementExpand = NewStrSize-OldStrSize;
    if ( (SingleReplacementExpand>0) && (NewStrSize!=0) )
        NewStringMaxSize = this->GetLength()*NewStrSize/OldStrSize +2; // +1 for potential division reminder +1 for \0
    else
        NewStringMaxSize = this->GetLength()+1;

    // create temporary string
    COutputString* pTmpStr = new COutputString(NewStringMaxSize);
    TCHAR* pFound;
    TCHAR* pc;
    SIZE_T NbOccurrences = 0;

    for(pc = this->String;*pc;)
    {
        // search next occurrence
        pFound = _tcsstr(pc,OldStringToReplace);

        // if no more occurrence
        if (pFound == NULL)
        {
            // append string end
            pTmpStr->Append(pc);
            break;
        }

        // increase occurrence count
        NbOccurrences++;

        // append data between pc and pFound to temporary string
        if (pc!=pFound)
            pTmpStr->Append(pc,pFound-pc);

        // append new string to temporary string
        pTmpStr->Append(NewReplacementString,NewStrSize);

        // set position after the old string
        pc = pFound+OldStrSize;
    }

    // empty our current string
    this->Clear();

    // grab content from temporary string
    this->Append(pTmpStr);

    // free temporary string
    delete pTmpStr;

    // return number of occurrence
    return NbOccurrences;
}
