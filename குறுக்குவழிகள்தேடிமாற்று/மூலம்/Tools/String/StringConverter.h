/*
Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004 Jacquelin POTIER <jacquelin.potier@free.fr>

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
// Object: convert string to single type : byte, DWORD, INT, ...
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include "TrimString.h"

#ifdef  _WIN64
    #define HEX_VALUE_DISPLAY _T("%I64X")
    #define HEX_VALUE_DISPLAY_FIXED_SIZE _T("%016I64X")
    #define UNSIGNED_VALUE_DISPLAY _T("%I64u")
    #define SIGNED_VALUE_DISPLAY _T("%I64d")
#else
    #define HEX_VALUE_DISPLAY _T("%I32X")
    #define HEX_VALUE_DISPLAY_FIXED_SIZE _T("%08I32X")
    #define UNSIGNED_VALUE_DISPLAY _T("%I32u")
    #define SIGNED_VALUE_DISPLAY _T("%I32d")
#endif

class CStringConverter
{
public:
    static BOOL StringToBYTE(TCHAR* strValue,BYTE* pValue);
    static BOOL StringToWORD(TCHAR* strValue,WORD* pValue);
    static BOOL StringToDWORD(TCHAR* strValue,DWORD* pValue);
    static BOOL StringToPBYTE(TCHAR* strValue,PBYTE* pValue);
    static BOOL StringToSIZE_T(TCHAR* strValue,SIZE_T* pValue);
    static BOOL StringToSSIZE_T_Or_SIZE_T(TCHAR* strValue,SIZE_T* pValue);
    static BOOL StringToSignedInt(TCHAR* strValue,INT* pValue);
    static BOOL StringToSignedInt64(TCHAR* strValue,INT64* pValue);
    static BOOL StringToUnsignedInt(TCHAR* strValue,UINT* pValue);
    static BOOL StringToUnsignedInt64(TCHAR* strValue,UINT64* pValue);
    static BOOL StringToSignedOrUnsignedInt32(TCHAR* strValue,UINT32* pValue);
    static BOOL StringToSignedOrUnsignedInt64(TCHAR* strValue,UINT64* pValue);
    static BOOL StringToDouble(TCHAR* strValue,double* pValue);
    static BOOL StringToFloat(TCHAR* strValue,float* pValue);
    //-----------------------------------------------------------------------------
    // Name: ValueToBinaryString
    // Object: print string binary representation into strBinValue
    // Parameters :
    //     IN  T Value
    //     OUT TCHAR* strBinValue : must have a length at least of (sizeof(T)*8+1)
    //     IN  BOOL bRemoveStartingZero : if TRUE useless 0 at the begin of representation will be trashed
    //     return : TRUE on success
    //-----------------------------------------------------------------------------
    template <class T> static BOOL ValueToBinaryString(T const Value,OUT TCHAR* strBinValue,BOOL bRemoveStartingZero = FALSE)
    {
        T LocalValue;
        TCHAR* pc;
        LocalValue = Value;
        strBinValue[sizeof(T)*8] = 0;
        // fill strBinValue backward
        for (pc = &strBinValue[sizeof(T)*8-1] ; pc>=strBinValue; pc--)
        {
            (LocalValue&1) ? *pc = '1' : *pc = '0';
            LocalValue = LocalValue>>1;
            if (bRemoveStartingZero && (LocalValue==0))
            {
                if (pc!=strBinValue)
                {
                    // as we filled string backward, we have to move memory
                    memmove(strBinValue,pc,(&strBinValue[sizeof(T)*8]-pc+1)*sizeof(TCHAR));
                }
                break;
            }
        }

        return TRUE;
    }

    static BOOL BinaryStringToValue(IN TCHAR* String,IN SIZE_T SizeInBytes,OUT ULONG64* pValue,OUT TCHAR** pStringPositionAfterScan);
};