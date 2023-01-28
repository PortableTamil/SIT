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
#include "LinkListSimpleSingleThreaded.h"
#include "../LinkListItemTemplate.h"
#include "../RefCountPointerForLinkListTemplate.h"

template <class T> 
class CLinkListTemplateSingleThreaded
{
protected:
    CLinkListSimpleSingleThreaded* pLinkListSimple;
public:
    CLinkListTemplateSingleThreaded();
    virtual ~CLinkListTemplateSingleThreaded();
    CLinkListItemTemplate<T>* GetHead();
    CLinkListItemTemplate<T>* GetTail();
    DWORD bDeleteObjectMemoryWhenRemovingObjectFromList:1;
    DWORD bSmartPointerObjectSoUseReleaseInsteadOfDelete:1;// if TRUE class T must be derivated from CLinkListRefCountBase, see RefCountPointerForLinkListTemplate.h

    CLinkListItemTemplate<T>* AddEmptyItem();
    CLinkListItemTemplate<T>* AddItem(T* pObject);
    CLinkListItemTemplate<T>* InsertItem(CLinkListItemTemplate<T>* PreviousItem);
    CLinkListItemTemplate<T>* InsertItem(CLinkListItemTemplate<T>* PreviousItem,T* pObject);
    BOOL MoveItemTo(CLinkListItemTemplate<T>* pItem,CLinkListItemTemplate<T>* pPreviousItem);
    BOOL MoveItemToHead(CLinkListItemTemplate<T>* pItem);
    BOOL MoveItemToTail(CLinkListItemTemplate<T>* pItem);
    BOOL RemoveItem(CLinkListItemTemplate<T>* Item);
    BOOL RemoveItemFromItemData(T* pObject);
    BOOL RemoveAllItems();

    void SetHeap(HANDLE HeapHandle);
    void ReportHeapDestruction();
    SIZE_T GetItemsCount();
    SIZE_T GetNumberOfItemsBeforeItem(IN CLinkListItemTemplate<T>* pItem);
    SIZE_T GetNumberOfItemsAfterItem(IN CLinkListItemTemplate<T>* pItem);
    T** ToArray(SIZE_T* pdwArraySize);
    BOOL IsItemStillInList(CLinkListItemTemplate<T>* pItem);
    BOOL IsItemDataStillInList(T* pData);
    CLinkListItemTemplate<T>* GetItem(SIZE_T ItemIndex);
    static BOOL Copy(CLinkListTemplateSingleThreaded<T>* pDst,CLinkListTemplateSingleThreaded<T>* pSrc);
};
