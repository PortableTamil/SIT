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

#include "stringconverter.h"

//-----------------------------------------------------------------------------
// Name: StringToBYTE
// Object: convert string to BYTE
// Parameters :
//     in  : TCHAR* strValue
//     out : BYTE* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToBYTE(TCHAR* strValue,BYTE* pValue)
{
    DWORD dw;
    BOOL bRet = StringToDWORD(strValue,&dw);
    *pValue = (BYTE)(dw & 0xff);
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: StringToWORD
// Object: convert string to WORD
// Parameters :
//     in  : TCHAR* strValue
//     out : WORD* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToWORD(TCHAR* strValue,WORD* pValue)
{
    DWORD dw;
    BOOL bRet = StringToDWORD(strValue,&dw);
    *pValue = (WORD)(dw & 0xffff);
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: StringToDWORD
// Object: convert string to DWORD
// Parameters :
//     in  : TCHAR* strValue
//     out : DWORD* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToDWORD(TCHAR* strValue,DWORD* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%x"),pValue);
    else
        iScanfRes=_stscanf(strValue,_T("%u"),pValue);

    return (iScanfRes==1);
}

//-----------------------------------------------------------------------------
// Name: StringToPBYTE
// Object: convert string to PBYTE
// Parameters :
//     in  : TCHAR* strValue
//     out : PBYTE* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToPBYTE(TCHAR* strValue,PBYTE* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%p"),pValue);
    else
        iScanfRes=_stscanf(strValue,UNSIGNED_VALUE_DISPLAY,(SIZE_T*)pValue);

    return (iScanfRes==1);
}

//-----------------------------------------------------------------------------
// Name: StringToSSIZE_T_Or_SIZE_T
// Object: convert string to SSIZE_T or SIZE_T
// Parameters :
//     in  : TCHAR* strValue
//     out : SIZE_T* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToSSIZE_T_Or_SIZE_T(TCHAR* strValue,SIZE_T* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%p"),(PBYTE*)pValue);
    else if(_tcsnicmp(strValue,_T("-"),1)==0) // signed value
        iScanfRes=_stscanf(strValue,SIGNED_VALUE_DISPLAY,pValue);
    else // unsigned value
        iScanfRes=_stscanf(strValue,UNSIGNED_VALUE_DISPLAY,pValue);

    return (iScanfRes==1);
}

//-----------------------------------------------------------------------------
// Name: StringToSIZE_T
// Object: convert string to SIZE_T
// Parameters :
//     in  : TCHAR* strValue
//     out : SIZE_T* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToSIZE_T(TCHAR* strValue,SIZE_T* pValue)
{
    return CStringConverter::StringToPBYTE(strValue,(PBYTE*)pValue);
}

//-----------------------------------------------------------------------------
// Name: StringToSignedInt
// Object: convert string to INT
// Parameters :
//     in  : TCHAR* strValue
//     out : int* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToSignedInt(TCHAR* strValue,INT* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%I32X"),pValue);
    else
        iScanfRes=_stscanf(strValue,_T("%I32d"),pValue);

    return (iScanfRes==1);
}
//-----------------------------------------------------------------------------
// Name: StringToSignedInt64
// Object: convert string to INT64
// Parameters :
//     in  : TCHAR* strValue
//     out : INT64* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToSignedInt64(TCHAR* strValue,INT64* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%I64X"),pValue);
    else
        iScanfRes=_stscanf(strValue,_T("%I64d"),pValue);

    return (iScanfRes==1);
}
//-----------------------------------------------------------------------------
// Name: StringToUnsignedInt
// Object: convert string to UINT
// Parameters :
//     in  : TCHAR* strValue
//     out : INT64* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToUnsignedInt(TCHAR* strValue,UINT* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%I32X"),pValue);
    else
        iScanfRes=_stscanf(strValue,_T("%I32u"),pValue);

    return (iScanfRes==1);
}
//-----------------------------------------------------------------------------
// Name: StringToUnsignedInt64
// Object: convert string to UINT64
// Parameters :
//     in  : TCHAR* strValue
//     out : INT64* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToUnsignedInt64(TCHAR* strValue,UINT64* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%I64X"),pValue);
    else
        iScanfRes=_stscanf(strValue,_T("%I64u"),pValue);

    return (iScanfRes==1);
}

//-----------------------------------------------------------------------------
// Name: StringToSignedOrUnsignedInt32
// Object: convert string to UINT32 or UINT32
// Parameters :
//     in  : TCHAR* strValue
//     out : UINT32* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToSignedOrUnsignedInt32(TCHAR* strValue,UINT32* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%X"),pValue);
    else if(_tcsnicmp(strValue,_T("-"),1)==0) // signed value
        iScanfRes=_stscanf(strValue,_T("%d"),pValue);
    else
        iScanfRes=_stscanf(strValue,_T("%u"),pValue);

    return (iScanfRes==1);
}

//-----------------------------------------------------------------------------
// Name: StringToSignedOrUnsignedInt64
// Object: convert string to INT64 or UINT64
// Parameters :
//     in  : TCHAR* strValue
//     out : UINT64* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToSignedOrUnsignedInt64(TCHAR* strValue,UINT64* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);

    int iScanfRes;
    // look for 0x format
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
        iScanfRes=_stscanf(strValue+2,_T("%I64X"),pValue);
    else if(_tcsnicmp(strValue,_T("-"),1)==0) // signed value
        iScanfRes=_stscanf(strValue,_T("%I64d"),pValue);
    else
        iScanfRes=_stscanf(strValue,_T("%I64u"),pValue);

    return (iScanfRes==1);
}

//-----------------------------------------------------------------------------
// Name: StringToDouble
// Object: convert string to double
// Parameters :
//     in  : TCHAR* strValue
//     out : double* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToDouble(TCHAR* strValue,double* pValue)
{
    *pValue=0;
    CTrimString::TrimString(strValue);
    if(_tcsnicmp(strValue,_T("0x"),2)==0)
    {
        ULONG64 Value;
        if (CStringConverter::StringToUnsignedInt64(strValue,&Value))
        {
            *pValue = (double)Value;
            return TRUE;
        }
        else
            return FALSE;
    }
    return (_stscanf(strValue,_T("%lg"),pValue)==1);
}

//-----------------------------------------------------------------------------
// Name: StringToFloat
// Object: convert string to float
// Parameters :
//     in  : TCHAR* strValue
//     out : float* pValue
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::StringToFloat(TCHAR* strValue,float* pValue)
{
    *pValue=0.0;
    double dValue=0.0;
    if (!CStringConverter::StringToDouble(strValue,&dValue))
        return FALSE;
    
    *pValue=(float)dValue;
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: BinaryStringToValue
// Object: convert binary string to ULONG64
// Parameters :
//     IN TCHAR* String : String to scan
//     IN SIZE_T SizeInBytes : wanted type size in byte ( 1 for byte , 2 for word, 4 for dword, 8 for qword)
//     OUT ULONG64* pValue : resulting value embeded in largest possible type
//     OUT TCHAR** pStringPositionAfterScan : position in String after type scan (allow to manage a sequence of type representation)
// return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CStringConverter::BinaryStringToValue(IN TCHAR* String,IN SIZE_T SizeInBytes,OUT ULONG64* pValue,OUT TCHAR** pStringPositionAfterScan)
{
    DWORD u0;
    DWORD u1;
    DWORD u2;
    DWORD u3;
    DWORD u4;
    DWORD u5;
    DWORD u6;
    DWORD u7;
    SIZE_T CntValidBytes;
    SIZE_T CntChar;
    TCHAR* pc;
    BYTE ByteArray[8];
    BYTE b;
    int Cnt;

    *pValue=0;
    *pStringPositionAfterScan = String;

    for ( CntValidBytes = 0; CntValidBytes < SizeInBytes; CntValidBytes++,String+=CntChar)
    {
        if (*String == 0)
        {
            *pStringPositionAfterScan = String;
            break;
        }

        // in case of Y bits scan for based from split X bits (Y>X) (may to remove)
        while ( (*String!=0) && (*String!='0') && (*String!='1') )
        {
            String++;
        }

        ///////////////////////////////////////////////////
        // manage case of not complete string representation
        ///////////////////////////////////////////////////
        for (CntChar = 0, pc = String,*pStringPositionAfterScan = String; 
             (CntChar<8) && ( (*pc == '0') || (*pc == '1') );
             CntChar++, pc++, (*pStringPositionAfterScan)++ //  (*pStringPositionAfterScan)++ : do not remove parenthesis due to operation priority
             )
        {

        }

        switch(CntChar)
        {
        default:
        case 0:
            return FALSE;
        case 1:
            // do scan to get the 1 bit
            if (_stscanf(String,_T("%1u"),&u0) != 1 )
                return FALSE;

            // make byte from the 1 bit
            b = (BYTE)u0;

            break;
        case 2:
            // do scan to get the 2 bits
            if (_stscanf(String,_T("%1u%1u"),&u0,&u1) != 2 )
                return FALSE;

            // make byte from the 2 bits
            b = (BYTE)( (u0<<1) | u1 );

            break;
        case 3:
            // do scan to get the 3 bits
            if (_stscanf(String,_T("%1u%1u%1u"),&u0,&u1,&u2) != 3 )
                return FALSE;

            // make byte from the 3 bits
            b = (BYTE)( (u0<<2) | (u1<<1) | u2 );

            break;
        case 4:
            // do scan to get the 4 bits
            if (_stscanf(String,_T("%1u%1u%1u%1u"),&u0,&u1,&u2,&u3) != 4 )
                return FALSE;

            // make byte from the 4 bits
            b = (BYTE)( (u0<<3) | (u1<<2) | (u2<<1) | u3 );

            break;
        case 5:
            // do scan to get the 5 bits
            if (_stscanf(String,_T("%1u%1u%1u%1u%1u"),&u0,&u1,&u2,&u3,&u4) != 5 )
                return FALSE;

            // make byte from the 5 bits
            b = (BYTE)( (u0<<4) | (u1<<3) | (u2<<2) | (u3<<1) | u4 );

            break;
        case 6:
            // do scan to get the 6 bits
            if (_stscanf(String,_T("%1u%1u%1u%1u%1u%1u"),&u0,&u1,&u2,&u3,&u4,&u5) != 6 )
                return FALSE;

            // make byte from the 6 bits
            b = (BYTE)( (u0<<5) | (u1<<4) | (u2<<3) | (u3<<2) | (u4<<1) | u5 );

            break;
        case 7:
            // do scan to get the 7 bits
            if (_stscanf(String,_T("%1u%1u%1u%1u%1u%1u%1u"),&u0,&u1,&u2,&u3,&u4,&u5,&u6) != 7 )
                return FALSE;

            // make byte from the 7 bits
            b = (BYTE)( (u0<<6) | (u1<<5) | (u2<<4) | (u3<<3) | (u4<<2) | (u5<<1) | u6 );

            break;

        case 8:
            // do scan to get the 8 bits
            if (_stscanf(String,_T("%1u%1u%1u%1u%1u%1u%1u%1u"),&u0,&u1,&u2,&u3,&u4,&u5,&u6,&u7) != 8 )
                return FALSE;

            // make byte from the 8 bits
            b = (BYTE)( (u0<<7) | (u1<<6) | (u2<<5) | (u3<<4) | (u4<<3) | (u5<<2) | (u6<<1) | u7 );

            break;
        }

        // note : unknown number of bytes and bytes stored from MSB to LSB --> *pValue |= (b << ( (UnknownValue - CntValidBytes) * 8) );
        ByteArray[CntValidBytes] = b;
    }

    if (CntValidBytes > 0)
        CntValidBytes--;

    for (Cnt = 0;Cnt<=(int)CntValidBytes;Cnt++)
    {
        b = ByteArray[Cnt];
        *pValue |= ((ULONG64)b << ( (CntValidBytes-Cnt) * 8 ) );
    }
   
    return TRUE;
}