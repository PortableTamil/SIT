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
// Object: allow to convert a string with format "str1;str2;str3" to an array of string
//         or a string with format "1;4;6-9;14-20;22;33" to an array of DWORD
//-----------------------------------------------------------------------------

#include "multipleelementsparsing.h"
#include "StringConverter.h"
#include "../LinkList/SingleThreaded/LinkListSimpleSingleThreaded.h"

//-----------------------------------------------------------------------------
// Name: ParseDWORD
// Object: return an array of DWORD containing elements
//         CALLER HAVE TO FREE ARRAY calling "delete[] Array;"
// Parameters :
//     in  : TCHAR* pszText : string containing list of numbers likes "1;4;6-9;14-20;22;33"
//                            meaning list {1,4,6,7,8,9,14,15,16,17,18,19,20,22,33}
//     out : SIZE_T* pArraySize : returned array size
//     return : array of pointer to elements, NULL on error or if no elements
//-----------------------------------------------------------------------------
SIZE_T* CMultipleElementsParsing::ParseSIZE_T(TCHAR* pszText,SIZE_T* pArraySize)
{
    // NOTICE until we use atol, return values are only positive LONG values not DWORD
    CLinkListSimpleSingleThreaded* pLinkList;
    TCHAR* pszOldPos=pszText;
    TCHAR* pszNewPos;
    TCHAR* pszPosSplitter2;
    SIZE_T Begin;
    SIZE_T End;
    SIZE_T Cnt;
    SIZE_T* RetArray;

    if (IsBadWritePtr(pArraySize,sizeof(SIZE_T)))
        return NULL;
    *pArraySize=0;

    if (IsBadReadPtr(pszText,1))
        return NULL;

    pLinkList=new CLinkListSimpleSingleThreaded();
    SIZE_T strSize=_tcslen(pszText);
    // while we can found ;
    while (strSize>(SIZE_T)(pszOldPos-pszText))
    {
        pszNewPos=_tcschr(pszOldPos,CMULTIPLEELEMENTSPARSING_MAJOR_SPLITTER_CHAR);

        // search -
        pszPosSplitter2=_tcschr(pszOldPos,CMULTIPLEELEMENTSPARSING_MINOR_SPLITTER_CHAR);
        // if - exists and is before next ;
        if ((pszPosSplitter2)&&((pszPosSplitter2<pszNewPos)||(pszNewPos==NULL))&&(pszPosSplitter2>pszOldPos))
        {
            // add list of number between Begin and End
            CStringConverter::StringToPBYTE(pszOldPos,(PBYTE*)&Begin);
            CStringConverter::StringToPBYTE(pszPosSplitter2+1,(PBYTE*)&End);
            for(Cnt=Begin;Cnt<=End;Cnt++)
                pLinkList->AddItem((PVOID)Cnt);
        }
        else
        {
            // add only the number
            CStringConverter::StringToPBYTE(pszOldPos,(PBYTE*)&Begin);
            pLinkList->AddItem((PVOID)Begin);
        }

        pszOldPos=pszNewPos+1;
        // if it was the last ;
        if (!pszNewPos)
            break;
    }
    // link list to SIZE_T*
    RetArray=(SIZE_T*)pLinkList->ToArray(pArraySize);

    // free memory
    delete pLinkList;

    // return allocated array
    return RetArray;
}
//-----------------------------------------------------------------------------
// Name: ParseStringArrayFree
// Object: free memory allocated during a ParseString call
// Parameters :
//     in  : TCHAR** pArray : array return by ParseString call
//           SIZE_T ArraySize : array size return by ParseString call
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CMultipleElementsParsing::ParseStringArrayFree(TCHAR** pArray,SIZE_T ArraySize)
{
    SIZE_T Cnt;

    if (!pArray)
        return;

    for (Cnt=0;Cnt<ArraySize;Cnt++)
        free(pArray[Cnt]);
    delete[] pArray;
}
//-----------------------------------------------------------------------------
// Name: ParseString
// Object: return an array of DWORD containing elements
//         CALLER HAVE TO FREE ARRAY calling ParseStringArrayFree(TCHAR** pArray,DWORD dwArraySize)
// Parameters :
//     in  : TCHAR* pszText : string containing list of string likes "str1;str2;str3"
//     out : SIZE_T* pArraySize : returned array size
//     return : array of pointer to elements, NULL on error or if no elements
//-----------------------------------------------------------------------------
TCHAR** CMultipleElementsParsing::ParseString(TCHAR* pszText,SIZE_T* pArraySize)
{
    SIZE_T NbItems;
    SIZE_T TextLen;
    SIZE_T Cnt;
    TCHAR** pArray;
    TCHAR* psz;
    TCHAR* pszLocalText;

    if (IsBadWritePtr(pArraySize,sizeof(SIZE_T)))
        return NULL;

    *pArraySize=0;
    pArray=NULL;

    // count major splitter number in field
    NbItems=0;
    TextLen=_tcslen(pszText);
    for (Cnt=0;Cnt<TextLen;Cnt++)
    {
        if (pszText[Cnt]==CMULTIPLEELEMENTSPARSING_MAJOR_SPLITTER_CHAR)
            NbItems++;
    }
    // allocate memory
    pArray=new TCHAR*[NbItems+1];

    // copy text for not modifying pszText
    pszLocalText=_tcsdup(pszText);

    // for each field
    psz=_tcstok(pszLocalText,CMULTIPLEELEMENTSPARSING_MAJOR_SPLITTER_STRING);
    while(psz && (*pArraySize< (NbItems+1) ) )
    {
        // copy it
        pArray[*pArraySize]=_tcsdup(psz);
        // increase pszFiltersInclusion size
        (*pArraySize)++;
        psz = _tcstok( NULL, CMULTIPLEELEMENTSPARSING_MAJOR_SPLITTER_STRING );
    }
    // free allocated text
    free(pszLocalText);

    return pArray;
}
