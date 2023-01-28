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
// Object: provides object creation in a specific heap
//-----------------------------------------------------------------------------

/*
using example:

class CFoo
{
public:
    int a;
    int b;
    CFoo()
    {
        a=1;
        b=2;
    }
    ~CFoo()
    {
        b=3;
    }
};

void main()
{
    HANDLE Heap = ::HeapCreate(HEAP_GROWABLE,4096,0);

    CPlacementNew<CFoo> PlacementNew(Heap);
    CFoo* pObj;
    pObj = PlacementNew.CreateObject();
    pObj->a = 4;
    PlacementNew.DestroyObject(pObj);

    ::HeapDestroy(Heap);
}
*/

#pragma once

#include <new>

template <class T> 
class CPlacementNew
{
protected:
    HANDLE Heap;
public:
    CPlacementNew(HANDLE Heap)
    {
        this->Heap = Heap;
    }
    ~CPlacementNew(void)
    {

    }

    T* CreateObject()
    {
        T* pObject = NULL;

        // allocate memory
        void* Memory = ::HeapAlloc(this->Heap,HEAP_ZERO_MEMORY,sizeof(T));
        if (!Memory)
            return NULL;

        try
        {
            // call default T constructor
            pObject = new (Memory) T();
        }
        catch (...)
        {
            // release allocated memory
            ::HeapFree(this->Heap,0,Memory);

            // rethrow exception
            throw;
        }

        return pObject;
    }

    void DestroyObject(T* pObject)
    {
        if (pObject==NULL)
            return;

        // call destructor
        pObject->~T();

        // release allocated memory
        ::HeapFree(this->Heap,0,pObject);
    }
};
