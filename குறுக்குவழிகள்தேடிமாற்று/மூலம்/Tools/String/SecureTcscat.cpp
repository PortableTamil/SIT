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
// Object: make a strcat function with reallocation if needed
//         limitation : string must be created with "new" and delete with "delete"
//-----------------------------------------------------------------------------

#include "SecureTcscat.h"

//-----------------------------------------------------------------------------
// Name: Secure_tcscat
// Object: make a strcat function with reallocation if needed
//         limitation : string must be created with "new" and delete with "delete[] "
// Parameters :
//     IN OUT TCHAR** pString : result string may be reallocated
//     TCHAR* StringToAdd : string to be append to *pString
//     IN OUT SIZE_T* pStringMaxSize : IN *pString max size (including \0 aka memory size), OUT new *pString max size
//     return : *pString
//-----------------------------------------------------------------------------
TCHAR* CSecureTcscat::Secure_tcscat(IN OUT TCHAR** pString,TCHAR* StringToAdd,IN OUT SIZE_T* pStringMaxSize)
{
    TCHAR* CurrentString=*pString;
    SIZE_T CurrentSize = _tcslen(CurrentString);
    SIZE_T StringToAddSize=_tcslen(StringToAdd);
    if ( ( CurrentSize + StringToAddSize + 1) > *pStringMaxSize)
    {
        // store current string buffer to TmpString
        TCHAR* TmpString = CurrentString;
        // double string size at least and assume that size is enough to store StringToAdd
        *pStringMaxSize = __max(*pStringMaxSize*2,*pStringMaxSize+StringToAddSize+1);
        // allocate a new buffer
        *pString = new TCHAR[*pStringMaxSize];
        CurrentString=*pString;
        // copy previous content to new allocated string
        _tcscpy(CurrentString,TmpString);
        // delete previously allocated buffer
        delete[] TmpString;
    }
    // do the _tcscat
    // optimize as we know current string size
    // _tcscat(*pString,StringToAdd);
    _tcscpy(&CurrentString[CurrentSize],StringToAdd);
    return *pString;
}