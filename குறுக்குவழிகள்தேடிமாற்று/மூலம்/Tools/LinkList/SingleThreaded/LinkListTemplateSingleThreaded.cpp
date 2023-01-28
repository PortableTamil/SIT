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
#include "LinkListTemplateSingleThreaded.h"

// # pragma message (__FILE__ " Information : don't forget #include \"LinkListTemplateSingleThreaded.cpp\"\r\n")

template <class T> 
CLinkListTemplateSingleThreaded<T>::CLinkListTemplateSingleThreaded()
{
    this->bDeleteObjectMemoryWhenRemovingObjectFromList=TRUE;
    this->bSmartPointerObjectSoUseReleaseInsteadOfDelete = FALSE;
    this->pLinkListSimple=new CLinkListSimpleSingleThreaded();
}
template <class T> 
CLinkListTemplateSingleThreaded<T>::~CLinkListTemplateSingleThreaded(void)
{
    // remove all items
    this->RemoveAllItems();
    delete this->pLinkListSimple;
}
template <class T>
CLinkListItemTemplate<T>* CLinkListTemplateSingleThreaded<T>::GetHead()
{
    return (CLinkListItemTemplate<T>*)this->pLinkListSimple->Head;
}
template <class T>
CLinkListItemTemplate<T>* CLinkListTemplateSingleThreaded<T>::GetTail()
{
    return (CLinkListItemTemplate<T>*)this->pLinkListSimple->Tail;
}

//-----------------------------------------------------------------------------
// Name: RemoveAllItems
// Object: Remove all items from the list
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplateSingleThreaded<T>::RemoveAllItems()
{
    while (this->pLinkListSimple->Head)
    {
        this->RemoveItem((CLinkListItemTemplate<T>*)(this->pLinkListSimple->Head));
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add Item to the link list allocating necessary memory for pObject
// Parameters :
//     in  : 
//     out :
//     return : CLinkListItemTemplate<T>* Item : pointer to the added item
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplateSingleThreaded<T>::AddEmptyItem()
{
    return this->AddItem(NULL);
}

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add Item to the link list allocating necessary memory for pObject
// Parameters :
//     in  : T* pObject : Pointer to object
//     out :
//     return : CLinkListItemTemplate<T>* Item : pointer to the added item
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplateSingleThreaded<T>::AddItem(T* pObject)
{
    return this->InsertItem((CLinkListItemTemplate<T>*)this->pLinkListSimple->Tail,pObject);
}

//-----------------------------------------------------------------------------
// Name: InsertItem
// Object: insert Item to the link list after PreviousItem,
//              allocating necessary memory for pObject
// Parameters :
//     in  : CLinkListItemTemplate<T>* PreviousItem : item after which new item will be added
//                                          Null if added item must be the first one
//     out :
//     return : CLinkListItemTemplate<T>* Item : pointer to the added item
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplateSingleThreaded<T>::InsertItem(CLinkListItemTemplate<T>* PreviousItem)
{
    return this->InsertItem(PreviousItem,NULL);
}

//-----------------------------------------------------------------------------
// Name: InsertItem
// Object: insert Item to the link list after PreviousItem,
//              allocating necessary memory for pObject and copying pObject
// Parameters :
//     in  : CLinkListItemTemplate<T>* PreviousItem : item after which new item will be added
//                                          Null if added item must be the first one
//           T* pObject : Pointer to object
//     out :
//     return : CLinkListItemTemplate<T>* Item : pointer to the added item
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplateSingleThreaded<T>::InsertItem(CLinkListItemTemplate<T>* PreviousItem,T* pObject)
{
    return (CLinkListItemTemplate<T>*)this->pLinkListSimple->InsertItem((CLinkListItem*)PreviousItem,(PVOID)pObject);
}

//-----------------------------------------------------------------------------
// Name: MoveItemTo
// Object: move a linked list item to another position in the list
// Parameters :
//     in     : CLinkListItem* pItem : Item to move
//              CLinkListItem* PreviousItem : where to move (previous item of destination)
//              BOOL bUserManagesLock : TRUE is user manages Lock
//     return : TRUE on success
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplateSingleThreaded<T>::MoveItemTo(CLinkListItemTemplate<T>* pItem,CLinkListItemTemplate<T>* PreviousItem)
{
    return this->pLinkListSimple->MoveItemTo((CLinkListItem*)pItem,(CLinkListItem*)PreviousItem);
}

template <class T> 
BOOL CLinkListTemplateSingleThreaded<T>::MoveItemToHead(CLinkListItemTemplate<T>* pItem)
{
    return this->MoveItemTo(pItem,NULL);
}

template <class T> 
BOOL CLinkListTemplateSingleThreaded<T>::MoveItemToTail(CLinkListItemTemplate<T>* pItem)
{
    return this->MoveItemTo(pItem,this->Tail);
}

//-----------------------------------------------------------------------------
// Name: RemoveItem
// Object: Remove item from list and free memory pointed by pObject
// Parameters :
//     in  : CLinkListItemTemplate<T>* Item : pointer to the item to remove
//     out :
//     return : 
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplateSingleThreaded<T>::RemoveItem(CLinkListItemTemplate<T>* Item)
{
    T* pObject=Item->ItemData;
    BOOL bRet = this->pLinkListSimple->RemoveItem((CLinkListItem*)Item);
    if (bRet && this->bDeleteObjectMemoryWhenRemovingObjectFromList)
    {
        if (this->bSmartPointerObjectSoUseReleaseInsteadOfDelete)
            ((CLinkListRefCountBase*)pObject)->Release();
        else
            delete pObject;
    }
    return bRet;
}

//-----------------------------------------------------------------------------
// Name: RemoveItemFromItemData
// Object: Remove an item identify by its data
//          Data should be unique in list, else first matching item will be removed
// Parameters :
//     in  : T* pObject : pointer to object
//     out :
//     return : 
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplateSingleThreaded<T>::RemoveItemFromItemData(T* pObject)
{
    BOOL bRet = this->pLinkListSimple->RemoveItemFromItemData((PVOID)pObject);
    if (bRet && this->bDeleteObjectMemoryWhenRemovingObjectFromList)
        delete pObject;
    return bRet;
}


//-----------------------------------------------------------------------------
// Name: SetHeap
// Object: allow to specify a heap for items allocation
//              Allocating all elements in a specified heap allow a quick deletion
//              by calling CLinkListTemplateSingleThreaded<T>::Lock; // assume list is locked before destroying memory
//                         ::HeapDestroy();
//                         CLinkListTemplateSingleThreaded<T>::ReportHeapDestruction();
//                              NewHeap=::HeapCreate(0,0,0);[optionnal,only if you want to use list again]
//                              CLinkListTemplateSingleThreaded<T>::SetHeap(NewHeap);[optionnal,only if you want to use list again]
//                         CLinkListTemplateSingleThreaded<T>::Unlock();
//
// Parameters :
//     in  : HANDLE HeapHandle : specified heap handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
template <class T> 
void CLinkListTemplateSingleThreaded<T>::SetHeap(HANDLE HeapHandle)
{
    this->pLinkListSimple->SetHeap(HeapHandle);
}

//-----------------------------------------------------------------------------
// Name: ReportHeapDestruction
// Object: SEE CLinkListTemplateSingleThreaded<T>::SetHeap DOCUMENTATION FOR USING SEQUENCE
//          if all elements have been created and allocated in specified heap
//         a quick memory freeing can be done by HeapDestroy.
//         If HeapDestroy has been called, all items must be removed from the list
//         without calling HeapFree
//         So this function remove all items from list without trying to free memory
//         USE IT ONLY IF MEMORY HAS BEEN REALLY DESTROYED, ELSE ALLOCATED MEMORY WON'T BE FREED
//
// Parameters :
//     in  : HANDLE HeapHandle : specified heap handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
template <class T> 
void CLinkListTemplateSingleThreaded<T>::ReportHeapDestruction()
{
    this->pLinkListSimple->ReportHeapDestruction();
}

//-----------------------------------------------------------------------------
// Name: IsItemStillInList
// Object: check if an item is still in the list
//          Notice : it's quite note a secure way as original item can have 
//                   been replaced by another one (if HeapAlloc returns the same address 
//                   for a new item)
//                   but it's assume at least memory is still available with the same type of object
//          in conclusion : it gives a safer way than IsBadReadPtr :)
// Parameters :
//     in  : CLinkListItemTemplate<T>* pItem
//     out :
//     return : TRUE if item is still in list
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplateSingleThreaded<T>::IsItemStillInList(CLinkListItemTemplate<T>* pItem)
{
    return this->pLinkListSimple->IsItemStillInList((CLinkListItem*)pItem);
}

//-----------------------------------------------------------------------------
// Name: IsItemDataStillInList
// Object: check if an item content is still in the list
// Parameters :
//     in  : T* pItemData
//     out :
//     return : TRUE if item is still in list
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplateSingleThreaded<T>::IsItemDataStillInList(T* pData)
{
    return this->pLinkListSimple->IsItemDataStillInList((PVOID) pData);
}

//-----------------------------------------------------------------------------
// Name: GetItemsCount
// Object: get number of items in the list
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
template <class T> 
SIZE_T CLinkListTemplateSingleThreaded<T>::GetItemsCount()
{
    return this->pLinkListSimple->GetItemsCount();
}

//-----------------------------------------------------------------------------
// Name: GetNumberOfItemsBeforeItem
// Object: get the number of items before provided item in the list
// Parameters :
//     IN CLinkListItem* pItem 
//     return : the number of items before provided item in the list
//-----------------------------------------------------------------------------
template <class T> 
SIZE_T CLinkListTemplateSingleThreaded<T>::GetNumberOfItemsBeforeItem(IN CLinkListItemTemplate<T>* pItem)
{
    return this->pLinkListSimple->GetNumberOfItemsBeforeItem((CLinkListItem*)pItem);
}

//-----------------------------------------------------------------------------
// Name: GetNumberOfItemsAfterItem
// Object: get the number of items after provided item in the list
// Parameters :
//     IN CLinkListItem* pItem 
//     return : the number of items after provided item in the list
//-----------------------------------------------------------------------------
template <class T> 
SIZE_T CLinkListTemplateSingleThreaded<T>::GetNumberOfItemsAfterItem(IN CLinkListItemTemplate<T>* pItem)
{
    return this->pLinkListSimple->GetNumberOfItemsAfterItem((CLinkListItem*)pItem);
}

//-----------------------------------------------------------------------------
// Name: ToArray
// Object: return an array of pointer to ItemData
//         CALLER HAVE TO FREE ARRAY calling "delete[] Array;"
// Parameters :
//     in  : 
//     out : SIZE_T* pdwArraySize : returned array size
//     return : array of pointer to elements, NULL on error or if no elements
//-----------------------------------------------------------------------------
template <class T> 
T** CLinkListTemplateSingleThreaded<T>::ToArray(SIZE_T* pdwArraySize)
{
    return (T**)this->pLinkListSimple->ToArray(pdwArraySize);
}

//-----------------------------------------------------------------------------
// Name: GetItem
// Object: Get item at specified address
// Parameters :
//         in : SIZE_T ItemIndex : 0 based item index
//     return : CLinkListItemTemplate<T> if found, NULL else
//-----------------------------------------------------------------------------
template <class T> 
CLinkListItemTemplate<T>* CLinkListTemplateSingleThreaded<T>::GetItem(SIZE_T ItemIndex)
{
    return (CLinkListItemTemplate<T>*)this->pLinkListSimple->GetItem(ItemIndex);
}

//-----------------------------------------------------------------------------
// Name: Copy
// Object: copy all elements from pSrc to pDst
// Parameters :
//     in     : CLinkListTemplateSingleThreaded<T>* pSrc : source list
//     in out : CLinkListTemplateSingleThreaded<T>* pDst destination list 
//     return : TRUE on success
//-----------------------------------------------------------------------------
template <class T> 
BOOL CLinkListTemplateSingleThreaded<T>::Copy(CLinkListTemplateSingleThreaded<T>* pDst,CLinkListTemplateSingleThreaded<T>* pSrc)
{
    if (IsBadReadPtr(pSrc,sizeof(CLinkListTemplateSingleThreaded<T>))||IsBadReadPtr(pDst,sizeof(CLinkListTemplateSingleThreaded<T>)))
        return FALSE;

    // clear pDst content
    pDst->RemoveAllItems();

    // for each item of pSrc
    CLinkListItem* pItem;
    for (pItem=pSrc->Head;pItem;pItem=pItem->NextItem)
    {
        pDst->AddItem(pItem->ItemData);
    }

    return TRUE;
}