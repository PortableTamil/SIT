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
// Object: provides template linked list
//-----------------------------------------------------------------------------

#pragma once
#include <Windows.h>

#ifndef ReleaseAndCleanIfNotNull
    // usage : ReleaseAndCleanIfNotNull(this->pObject)
    #define ReleaseAndCleanIfNotNull(x) if (x){x->Release(); x=NULL;}
#endif

class CLinkListRefCountBase
{
protected:
    volatile LONG RefCount;
public:
    CLinkListRefCountBase(void) { this->RefCount = 1; }
    virtual ~CLinkListRefCountBase() { }
    virtual LONG AddRef(void) 
    { 
        // ++this->RefCount; 
        // return this->RefCount;
        return ::InterlockedIncrement(&this->RefCount);
    }
    virtual LONG Release(void)
    {
        // LONG localRefCount = --this->RefCount;
        LONG localRefCount =::InterlockedDecrement(&this->RefCount);
        if (localRefCount==0)
            delete this;
        // never return this->RefCount : object referenced by this can be destroyed !!!
        return localRefCount;
    }
};