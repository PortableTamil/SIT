/*
Copyright (C) 2004-2020 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004-2020 Jacquelin POTIER <jacquelin.potier@free.fr>

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

#include "linklistbase.h"

#ifndef BreakInDebugMode
    #ifdef _DEBUG
        #define BreakInDebugMode if ( ::IsDebuggerPresent() ) { ::DebugBreak();}
    #else 
        #define BreakInDebugMode 
    #endif
#endif

CLinkListBase::CLinkListBase()
//#if ( defined(_DEBUG) && defined(CHECK_MEMORY_LEAKS) )
//    :CMemoryLeaksChecker(TEXT("CLinkListBase"))
//#endif
{
    this->Head=NULL;
    this->Tail=NULL;
    // use process heap by default
    this->LinkListHeap=GetProcessHeap();
    this->hevtListUnlocked=CreateEvent(NULL,FALSE,TRUE,NULL);
    this->hevtInternalLockUnlocked=CreateEvent(NULL,FALSE,TRUE,NULL);
    
    this->ItemsNumber=0;
    this->LockWaitTime=INFINITE;
    this->bAllowToAddItemDuringLock=FALSE;
    this->ThreadIdOfOwnerLock = NULL; // 0 is an invalid Id according to ::GetThreadId MSDN documentation
    this->ThreadOnwerLockCount = 0;
    this->LockOnlyOtherThreads = FALSE;
#ifdef _DEBUG
    this->ThreadIdOfExternalLock = NULL; // 0 is an invalid Id according to ::GetThreadId MSDN documentation
#endif
}

CLinkListBase::~CLinkListBase(void)
{
    // close unlocked event
    if (this->hevtListUnlocked)
    {
        CloseHandle(this->hevtListUnlocked);
        this->hevtListUnlocked=NULL;
    }
    if (this->hevtInternalLockUnlocked)
    {
        CloseHandle(this->hevtInternalLockUnlocked);
        this->hevtInternalLockUnlocked=NULL;
    }
}


//-----------------------------------------------------------------------------
// Name: SetHeap
// Object: allow to specify a heap for items allocation
//              Allocating all elements in a specified heap allow a quick deletion
//              by calling CLinkListBase::Lock; // assume list is locked before destroying memory
//                         ::HeapDestroy();
//                         CLinkListBase::ReportHeapDestruction();
//                              NewHeap=::HeapCreate(0,0,0);[optional,only if you want to use list again]
//                              CLinkListBase::SetHeap(NewHeap);[optional,only if you want to use list again]
//                         CLinkListBase::Unlock();
//
// Parameters :
//     in  : HANDLE HeapHandle : specified heap handle
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CLinkListBase::SetHeap(HANDLE HeapHandle)
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
HANDLE CLinkListBase::GetHeap()
{
    return this->LinkListHeap;
}

//-----------------------------------------------------------------------------
// Name: ReportHeapDestruction
// Object: SEE CLinkListBase::SetHeap DOCUMENTATION FOR USING SEQUENCE
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
void CLinkListBase::ReportHeapDestruction()
{
    // all memory is destroyed, just reset some fields
    this->Head=NULL;
    this->Tail=NULL;
    this->ItemsNumber=0;
    this->LinkListHeap=::GetProcessHeap();
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
BOOL CLinkListBase::IsEmpty()
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
//           BOOL bUserManagesLock : TRUE is user manages Lock
//     out :
//     return : TRUE if item is still in list
//-----------------------------------------------------------------------------
BOOL CLinkListBase::IsItemStillInList(CLinkListItem* pItem,BOOL bUserManagesLock)
{
    BOOL bRet=FALSE;

    // lock list if needed
    if (!bUserManagesLock)
        this->LockForExternalCalls();

    WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);// must be after this->LockForExternalCalls call

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

    SetEvent(this->hevtInternalLockUnlocked);

    // unlock list if needed
    if (!bUserManagesLock)
        this->UnlockForExternalCalls();

    return bRet;
}

//-----------------------------------------------------------------------------
// Name: IsItemDataStillInList
// Object: check if an item content is still in the list
// Parameters :
//     in  : PVOID pItemData
//           BOOL bUserManagesLock : TRUE is user manages Lock
//     out :
//     return : TRUE if item is still in list
//-----------------------------------------------------------------------------
BOOL CLinkListBase::IsItemDataStillInList(PVOID pItemData,BOOL bUserManagesLock)
{
    BOOL bRet=FALSE;

    // lock list if needed
    if (!bUserManagesLock)
        this->LockForExternalCalls();

    WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);// must be after this->LockForExternalCalls call

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

    SetEvent(this->hevtInternalLockUnlocked);

    // unlock list if needed
    if (!bUserManagesLock)
        this->UnlockForExternalCalls();

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
SIZE_T CLinkListBase::GetItemsCount()
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
SIZE_T CLinkListBase::GetNumberOfItemsBeforeItem(IN CLinkListItem* pItem,IN BOOL bUserManagesLock)
{
    SIZE_T ItemCount;
    CLinkListItem* pTmpItem;

    if (pItem==NULL)
        return 0;

    // wait for synchro
    if(!bUserManagesLock)
        this->Lock(FALSE,TRUE);

    // until there is previous items
    for (ItemCount=0,pTmpItem = pItem->PreviousItem;pTmpItem;pTmpItem=pTmpItem->PreviousItem)
    {
        ItemCount++;
    }

    if(!bUserManagesLock)
        this->Unlock();

    return ItemCount;
}

//-----------------------------------------------------------------------------
// Name: GetNumberOfItemsAfterItem
// Object: get the number of items after provided item in the list
// Parameters :
//     IN CLinkListItem* pItem 
//     return : the number of items after provided item in the list
//-----------------------------------------------------------------------------
SIZE_T CLinkListBase::GetNumberOfItemsAfterItem(IN CLinkListItem* pItem,IN BOOL bUserManagesLock)
{
    SIZE_T ItemCount;
    CLinkListItem* pTmpItem;

    if (pItem==NULL)
        return 0;


    // wait for synchro
    if(!bUserManagesLock)
        this->Lock(FALSE,TRUE);

    // until there is next items
    for (ItemCount=0,pTmpItem = pItem->NextItem;pTmpItem;pTmpItem=pTmpItem->NextItem)
    {
        ItemCount++;
    }

    if(!bUserManagesLock)
        this->Unlock();

    return ItemCount;
}

//-----------------------------------------------------------------------------
// Name: Lock
// Object: wait for list to be unlocked, and lock it
//          useful to avoid item removal when parsing list 
// Parameters :
//     in  : BOOL bAllowToAddItemDuringLock : TRUE if items can be added into list
//                                             despite lock (useful for list parsing only)
//           BOOL bLockOnlyOtherThreads : if TRUE allow recursive call of function locking the list 
//                                       (list will be unlocked only after all matching Unlock() calls have been done)
//     out :
//     return : ThreadOnwerLockCount
//-----------------------------------------------------------------------------
SIZE_T CLinkListBase::Lock(BOOL bAllowToAddItemDuringLock,BOOL bLockOnlyOtherThreads)
{
    DWORD dwRet;

    // if shared lock and not first shared lock
    if (bLockOnlyOtherThreads && (this->ThreadIdOfOwnerLock == ::GetCurrentThreadId()))
    {
        this->ThreadOnwerLockCount++;
        dwRet = WAIT_OBJECT_0;
    }
    else // exclusive lock or first shared lock
    {
#ifdef _DEBUG
        if (this->ThreadIdOfOwnerLock == ::GetCurrentThreadId())
        {
            // something goes wrong. Potential deadlock !!!
            BreakInDebugMode;
        }
#endif
        dwRet=::WaitForSingleObject(this->hevtListUnlocked,this->LockWaitTime);
        this->ThreadOnwerLockCount = 1;
    }
    ::WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);
    this->ThreadIdOfOwnerLock = ::GetCurrentThreadId();
    this->LockOnlyOtherThreads = bLockOnlyOtherThreads;
    this->bAllowToAddItemDuringLock=bAllowToAddItemDuringLock;
    ::SetEvent(this->hevtInternalLockUnlocked);
    return this->ThreadOnwerLockCount;
}

//-----------------------------------------------------------------------------
// Name: Unlock
// Object: unlock list
// Parameters :
//     in  : 
//     out :
//     return : number of remaining locks
//-----------------------------------------------------------------------------
SIZE_T CLinkListBase::Unlock()
{
#ifdef _DEBUG
    // check for cross thread unlock
    if (this->ThreadIdOfOwnerLock != ::GetCurrentThreadId())
    {
        BreakInDebugMode;
    }
#endif
    // assume we are not adding an item before releasing lock
    if (this->bAllowToAddItemDuringLock)
    {
        ::WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);
        this->bAllowToAddItemDuringLock=FALSE;
        ::SetEvent(this->hevtInternalLockUnlocked);
    }

    this->ThreadOnwerLockCount--;
    if (this->ThreadOnwerLockCount==0)
    {
        this->ThreadIdOfOwnerLock = NULL;
        ::SetEvent(this->hevtListUnlocked);
    }
    return this->ThreadOnwerLockCount;
}

//-----------------------------------------------------------------------------
// Name: LockForExternalCalls used internally only when this->hevtInternalLockUnlocked is already set
// Object: wait for list to be unlocked, and lock it
//          useful to avoid item removal when parsing list 
// Parameters :
//     in  : BOOL bAllowToAddItemDuringLock : TRUE if items can be added into list
//                                             despite lock (useful for list parsing only)
//     out :
//     return : WaitForSingleObject result
//-----------------------------------------------------------------------------
DWORD CLinkListBase::LockForExternalCalls()
{
    DWORD dwRet;
#ifdef _DEBUG
    DWORD ThreadId = ::GetCurrentThreadId();
    if (this->ThreadIdOfExternalLock == ThreadId)
    {
        BreakInDebugMode;
    }
#endif

    dwRet = ::WaitForSingleObject(this->hevtListUnlocked,this->LockWaitTime);

#ifdef _DEBUG
    this->ThreadIdOfExternalLock = ThreadId;
#endif
    return dwRet;
}
//-----------------------------------------------------------------------------
// Name: UnlockForExternalCalls used internally only when this->hevtInternalLockUnlocked is already set
// Object: unlock list
// Parameters :
//     in  : 
//     out :
//     return : SetEvent result
//-----------------------------------------------------------------------------
DWORD CLinkListBase::UnlockForExternalCalls()
{
#ifdef _DEBUG
    this->ThreadIdOfExternalLock = NULL;
#endif
    return ::SetEvent(this->hevtListUnlocked);
}

//-----------------------------------------------------------------------------
// Name: ToArray
// Object: return an array of pointer to ItemData
//         CALLER HAVE TO FREE ARRAY calling "delete[] Array;"
// Parameters :
//     in  : BOOL bUserManagesLock : TRUE is user manages lock
//     out : SIZE_T* pdwArraySize : returned array size
//     return : array of pointer to elements, NULL on error or if no elements
//-----------------------------------------------------------------------------
PVOID* CLinkListBase::ToArray(SIZE_T* pdwArraySize,BOOL bUserManagesLock)
{
    PVOID* Array;
    SIZE_T Counter;

    // check param
    if (::IsBadWritePtr(pdwArraySize,sizeof(SIZE_T)))
        return NULL;

    if (!bUserManagesLock)
        // wait for synchro
        this->LockForExternalCalls();

    ::WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);// must be after this->LockForExternalCalls call

    // get number of Items
    *pdwArraySize=this->GetItemsCount();
    
    if (*pdwArraySize == 0)
    {
        ::SetEvent(this->hevtInternalLockUnlocked);

        if (!bUserManagesLock)
            this->UnlockForExternalCalls();
        
        return NULL;
    }

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

    ::SetEvent(this->hevtInternalLockUnlocked);

    if (!bUserManagesLock)
        this->UnlockForExternalCalls();
    
    return Array;
}

//-----------------------------------------------------------------------------
// Name: IsLocked
// Object: allow to know if list is currently locked
// Parameters :
//     return : TRUE if locked, FALSE else
//-----------------------------------------------------------------------------
BOOL CLinkListBase::IsLocked()
{
    switch(::WaitForSingleObject(this->hevtListUnlocked,0))
    {
    case WAIT_TIMEOUT:
        return TRUE;
    case WAIT_OBJECT_0:
        // restore unlocked state reseted by WaitForSingleObject
        ::SetEvent(this->hevtListUnlocked);
        return FALSE;
    default:
        return FALSE;
    }
}

//-----------------------------------------------------------------------------
// Name: GetItem
// Object: Get item at specified address
// Parameters :
//         in : SIZE_T ItemIndex : 0 based item index
//              BOOL bUserManagesLock : TRUE if user manages lock
//     return : CLinkListItem if found, NULL else
//-----------------------------------------------------------------------------
CLinkListItem* CLinkListBase::GetItem(SIZE_T ItemIndex,BOOL bUserManagesLock)
{
    // do a quick index checking
    if (ItemIndex>=this->GetItemsCount())
        return NULL;

    // lock list if needed
    if (!bUserManagesLock)
        this->LockForExternalCalls();

    ::WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);// must be after this->LockForExternalCalls call

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

    ::SetEvent(this->hevtInternalLockUnlocked);

    // unlock list if needed
    if (!bUserManagesLock)
        this->UnlockForExternalCalls();

    return pReturnedItem;
}

//-----------------------------------------------------------------------------
// Name: Copy
// Object: copy all elements from pSrc to pDst
// Parameters :
//     in     : CLinkListBase* pSrc : source list
//              BOOL DstLocked : TRUE if user manages pDst Lock
//              BOOL SrcLocked : TRUE if user manages pSrc Lock
//     in out : CLinkListBase* pDst destination list 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CLinkListBase::Copy(CLinkListBase* pDst,CLinkListBase* pSrc,BOOL DstLocked,BOOL SrcLocked)
{
    if (::IsBadReadPtr(pSrc,sizeof(CLinkListBase))||::IsBadReadPtr(pDst,sizeof(CLinkListBase)))
        return FALSE;

    if(!DstLocked)
        pDst->Lock(FALSE,TRUE);
    if(!SrcLocked)
        pSrc->Lock(FALSE,TRUE);

    // clear pDst content
    pDst->RemoveAllItems(TRUE);

    // for each item of pSrc
    CLinkListItem* pItem;
    for (pItem=pSrc->Head;pItem;pItem=pItem->NextItem)
    {
        pDst->AddItem(pItem->ItemData,TRUE);
    }

    if(!DstLocked)
        pDst->Unlock();
    if(!SrcLocked)
        pSrc->Unlock();

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
BOOL CLinkListBase::MoveItemTo(CLinkListItem* pItem,CLinkListItem* PreviousItem,BOOL bUserManagesLock)
{
#if _DEBUG
    // assume item is still in list
    if (!this->IsItemStillInList(pItem,bUserManagesLock))
    {
        if (IsDebuggerPresent())
            DebugBreak();
        return FALSE;
    }
#endif

    if ( (pItem->PreviousItem == PreviousItem) || (pItem == PreviousItem) )
        return TRUE;

    if(!bUserManagesLock)
        this->LockForExternalCalls();

#ifdef _DEBUG
    DWORD dwRet = WaitForSingleObject(this->hevtInternalLockUnlocked,5000);// must be after this->LockForExternalCalls call
    if (dwRet == WAIT_TIMEOUT)
    {
        DebugBreak(); // DeadLock !!!
    }
#else
    WaitForSingleObject(this->hevtInternalLockUnlocked,INFINITE);// must be after this->LockForExternalCalls call
#endif

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

    SetEvent(this->hevtInternalLockUnlocked);
    if(!bUserManagesLock)
        this->UnlockForExternalCalls();

    return TRUE;
}

BOOL CLinkListBase::MoveItemToHead(CLinkListItem* pItem,BOOL bUserManagesLock)
{
    return this->MoveItemTo(pItem,NULL,bUserManagesLock);
}

BOOL CLinkListBase::MoveItemToTail(CLinkListItem* pItem,BOOL bUserManagesLock)
{
    return this->MoveItemTo(pItem,this->Tail,bUserManagesLock);
}
