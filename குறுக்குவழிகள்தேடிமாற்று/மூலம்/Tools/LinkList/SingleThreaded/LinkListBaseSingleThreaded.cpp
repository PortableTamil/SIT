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
// Object: Link list base interface
//-----------------------------------------------------------------------------

#include "LinkListBaseSingleThreaded.h"

CLinkListBaseSingleThreaded::CLinkListBaseSingleThreaded()
{
    this->Head=NULL;
    this->Tail=NULL;
    // use process heap by default
    this->LinkListHeap=GetProcessHeap();
    
    this->ItemsNumber=0;
}

CLinkListBaseSingleThreaded::~CLinkListBaseSingleThreaded(void)
{

}


//-----------------------------------------------------------------------------
// Name: SetHeap
// Object: allow to specify a heap for items allocation
//              Allocating all elements in a specified heap allow a quick deletion
//              by calling CLinkListBaseSingleThreaded::Lock; // assume list is locked before destroying memory
//                         ::HeapDestroy();
//                         CLinkListBaseSingleThreaded::ReportHeapDestruction();
//                              NewHeap=::HeapCreate(0,0,0);[optional,only if you want to use list again]
//                              CLinkListBaseSingleThreaded::SetHeap(NewHeap);[optional,only if you want to use list again]
//                         CLinkListBaseSingleThreaded::Unlock();
//
// Parameters :
//     in  : HANDLE HeapHandle : specified heap handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CLinkListBaseSingleThreaded::SetHeap(HANDLE HeapHandle)
{
    this->LinkListHeap=HeapHandle;
}
//-----------------------------------------------------------------------------
// Name: GetHeap
// Object: return the specified heap for items allocation
//
// Parameters :
//     in  : 
//     out :
//     return : HANDLE HeapHandle : specified heap handle
//-----------------------------------------------------------------------------
HANDLE CLinkListBaseSingleThreaded::GetHeap()
{
    return this->LinkListHeap;
}

//-----------------------------------------------------------------------------
// Name: ReportHeapDestruction
// Object: SEE CLinkListBaseSingleThreaded::SetHeap DOCUMENTATION FOR USING SEQUENCE
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
void CLinkListBaseSingleThreaded::ReportHeapDestruction()
{
    // all memory is destroyed, just reset some fields
    this->Head=NULL;
    this->Tail=NULL;
    this->ItemsNumber=0;
}

//-----------------------------------------------------------------------------
// Name: IsEmpty
// Object: check if list is empty
//
// Parameters :
//     in  : 
//     out :
//     return : TRUE if list is empty
//-----------------------------------------------------------------------------
BOOL CLinkListBaseSingleThreaded::IsEmpty()
{
    return (this->Head == NULL);
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
//     in  : CLinkListItem* pItem
//     out :
//     return : TRUE if item is still in list
//-----------------------------------------------------------------------------
BOOL CLinkListBaseSingleThreaded::IsItemStillInList(CLinkListItem* pItem)
{
    BOOL bRet=FALSE;

    CLinkListItem* pListItem;
    // parse list
    for (pListItem=this->Head;pListItem;pListItem=pListItem->NextItem)
    {
        // if Cnt match Index
        if (pListItem==pItem)
        {
            bRet=TRUE;
            break;
        }
    }

    return bRet;
}


//-----------------------------------------------------------------------------
// Name: IsItemDataStillInList
// Object: check if an item content is still in the list
// Parameters :
//     in  : PVOID pItemData
//     out :
//     return : TRUE if item is still in list
//-----------------------------------------------------------------------------
BOOL CLinkListBaseSingleThreaded::IsItemDataStillInList(PVOID pItemData)
{
    BOOL bRet=FALSE;

    CLinkListItem* pListItem;
    // parse list
    for (pListItem=this->Head;pListItem;pListItem=pListItem->NextItem)
    {
        // if Cnt match Index
        if (pListItem->ItemData==pItemData)
        {
            bRet=TRUE;
            break;
        }
    }

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: GetItemsCount
// Object: get number of items in the list
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
SIZE_T CLinkListBaseSingleThreaded::GetItemsCount()
{
    return this->ItemsNumber;
}

//-----------------------------------------------------------------------------
// Name: GetNumberOfItemsBeforeItem
// Object: get the number of items before provided item in the list
// Parameters :
//     IN CLinkListItem* pItem 
//     return : the number of items before provided item in the list
//-----------------------------------------------------------------------------
SIZE_T CLinkListBaseSingleThreaded::GetNumberOfItemsBeforeItem(IN CLinkListItem* pItem)
{
    SIZE_T ItemCount;
    CLinkListItem* pTmpItem;

    if (pItem==NULL)
        return 0;

    // until there is previous items
    for (ItemCount=0,pTmpItem = pItem->PreviousItem;pTmpItem;pTmpItem=pTmpItem->PreviousItem)
    {
        ItemCount++;
    }

    return ItemCount;
}

//-----------------------------------------------------------------------------
// Name: GetNumberOfItemsAfterItem
// Object: get the number of items after provided item in the list
// Parameters :
//     IN CLinkListItem* pItem 
//     return : the number of items after provided item in the list
//-----------------------------------------------------------------------------
SIZE_T CLinkListBaseSingleThreaded::GetNumberOfItemsAfterItem(IN CLinkListItem* pItem)
{
    SIZE_T ItemCount;
    CLinkListItem* pTmpItem;

    if (pItem==NULL)
        return 0;

    // until there is next items
    for (ItemCount=0,pTmpItem = pItem->NextItem;pTmpItem;pTmpItem=pTmpItem->NextItem)
    {
        ItemCount++;
    }

    return ItemCount;
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
PVOID* CLinkListBaseSingleThreaded::ToArray(SIZE_T* pdwArraySize)
{

    PVOID* Array;
    SIZE_T Counter;

    // check param
    if (IsBadWritePtr(pdwArraySize,sizeof(SIZE_T)))
        return NULL;

    // get number of Items
    *pdwArraySize=this->GetItemsCount();
    
    if (*pdwArraySize == 0)
        return NULL;

    // we just allocate an array to pointer list
    // this is the speediest way as we don't need 
    // to do memory allocation for each Item
    Array=new PVOID[*pdwArraySize];
    CLinkListItem* CurrentItem = this->Head;
    for (Counter=0;Counter<*pdwArraySize;Counter++)// as while (CurrentItem->NextItem != NULL)) but more secure
    {
        Array[Counter]=CurrentItem->ItemData;
        CurrentItem = CurrentItem->NextItem;
    }
    
    return Array;
}

//-----------------------------------------------------------------------------
// Name: GetItem
// Object: Get item at specified address
// Parameters :
//         in : SIZE_T ItemIndex : 0 based item index
//     return : CLinkListItem if found, NULL else
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListBaseSingleThreaded::GetItem(SIZE_T ItemIndex)
{
    // do a quick index checking
    if (ItemIndex>=this->GetItemsCount())
        return NULL;

    CLinkListItem* pReturnedItem=NULL;
    CLinkListItem* pItem;
    SIZE_T Cnt=0;
    // parse list
    for (pItem=this->Head;pItem;pItem=pItem->NextItem,Cnt++)
    {
        // if Cnt match Index
        if (Cnt==ItemIndex)
        {
            pReturnedItem=pItem;
            break;
        }
    }
    return pReturnedItem;
}

//-----------------------------------------------------------------------------
// Name: Copy
// Object: copy all elements from pSrc to pDst
// Parameters :
//     in     : CLinkListBaseSingleThreaded* pSrc : source list
//     in out : CLinkListBaseSingleThreaded* pDst destination list 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CLinkListBaseSingleThreaded::Copy(CLinkListBaseSingleThreaded* pDst,CLinkListBaseSingleThreaded* pSrc)
{
    if (IsBadReadPtr(pSrc,sizeof(CLinkListBaseSingleThreaded))||IsBadReadPtr(pDst,sizeof(CLinkListBaseSingleThreaded)))
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

//-----------------------------------------------------------------------------
// Name: MoveItemTo
// Object: move a linked list item to another position in the list
// Parameters :
//     in     : CLinkListItem* pItem : Item to move
//              CLinkListItem* PreviousItem : where to move (previous item of destination)
//              BOOL bUserManagesLock : TRUE is user manages Lock
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CLinkListBaseSingleThreaded::MoveItemTo(CLinkListItem* pItem,CLinkListItem* PreviousItem)
{
#if _DEBUG
    // assume item is still in list
    if (!this->IsItemStillInList(pItem))
    {
        if (IsDebuggerPresent())
            DebugBreak();
        return FALSE;
    }
#endif

    if ( (pItem->PreviousItem == PreviousItem) || (pItem == PreviousItem) )
        return TRUE;

    // remove link to pItem, and conserve double link list
    if (pItem->PreviousItem)
        pItem->PreviousItem->NextItem = pItem->NextItem;
    if (pItem->NextItem)
        pItem->NextItem->PreviousItem = pItem->PreviousItem;

    // add new link to pItem at wanted position
    if (PreviousItem)
    {
        // change next item previous item
        if (PreviousItem->NextItem)
        {
            pItem->NextItem = PreviousItem->NextItem;
            PreviousItem->NextItem->PreviousItem = pItem;
        }
        else
        {
            pItem->NextItem = NULL;
            this->Tail = pItem;
        }

        // change previous item next item
        PreviousItem->NextItem = pItem;

        pItem->PreviousItem = PreviousItem;
    }
    else
    {
        if (this->Head)
        {
            this->Head->PreviousItem = pItem;
            pItem->NextItem = this->Head;
        }

        pItem->PreviousItem = NULL;
        this->Head = pItem;
    }

    return TRUE;
}

BOOL CLinkListBaseSingleThreaded::MoveItemToHead(CLinkListItem* pItem)
{
    return this->MoveItemTo(pItem,NULL);
}

BOOL CLinkListBaseSingleThreaded::MoveItemToTail(CLinkListItem* pItem)
{
    return this->MoveItemTo(pItem,this->Tail);
}
