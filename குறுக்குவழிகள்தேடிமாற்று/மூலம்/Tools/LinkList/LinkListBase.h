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

#pragma once
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#include "LinkListItem.h"

//#if ( defined(_DEBUG) && defined(CHECK_MEMORY_LEAKS) )
//#include "../MemoryCheck/MemoryLeaksChecker.h"
//#endif

class CLinkListBase
//#if ( defined(_DEBUG) && defined(CHECK_MEMORY_LEAKS) )
//    :public CMemoryLeaksChecker
//#endif
{
protected:
    HANDLE LinkListHeap;
    HANDLE hevtListUnlocked;
    HANDLE hevtInternalLockUnlocked;
    SIZE_T  ItemsNumber;
    BOOL bAllowToAddItemDuringLock;
    DWORD ThreadIdOfOwnerLock;
    SIZE_T ThreadOnwerLockCount;
#ifdef _DEBUG
    DWORD ThreadIdOfExternalLock;
#endif
    BOOL LockOnlyOtherThreads;

    CLinkListBase();
    virtual ~CLinkListBase();
    DWORD LockForExternalCalls();
    DWORD UnlockForExternalCalls();

public:
    CLinkListItem* Head;
    CLinkListItem* Tail;
    DWORD  LockWaitTime;

    virtual CLinkListItem* AddEmptyItem()=0;
    virtual CLinkListItem* AddEmptyItem(BOOL bUserManagesLock)=0;
    virtual CLinkListItem* AddItem(PVOID ItemData)=0;
    virtual CLinkListItem* AddItem(PVOID ItemData,BOOL bUserManagesLock)=0;
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem)=0;
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem,BOOL bUserManagesLock)=0;
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem,PVOID ItemData)=0;
    virtual CLinkListItem* InsertItem(CLinkListItem* PreviousItem,PVOID ItemData,BOOL bUserManagesLock)=0;
    virtual BOOL MoveItemTo(CLinkListItem* pItem,CLinkListItem* PreviousItem,BOOL bUserManagesLock=FALSE);
    virtual BOOL MoveItemToHead(CLinkListItem* pItem,BOOL bUserManagesLock=FALSE);
    virtual BOOL MoveItemToTail(CLinkListItem* pItem,BOOL bUserManagesLock=FALSE);
    virtual BOOL RemoveItem(CLinkListItem* Item,BOOL bUserManagesLock=FALSE)=0;
    virtual BOOL RemoveItemFromItemData(PVOID ItemData,BOOL bUserManagesLock=FALSE)=0;
    virtual BOOL RemoveAllItems(BOOL bUserManagesLock=FALSE)=0;// must be pure virtual has it call the pure virtual RemoveItem method

    void SetHeap(HANDLE HeapHandle);
    HANDLE GetHeap();
    void ReportHeapDestruction();
    
    SIZE_T GetItemsCount();
    SIZE_T GetNumberOfItemsBeforeItem(IN CLinkListItem* pItem,IN BOOL bUserManagesLock=FALSE);
    SIZE_T GetNumberOfItemsAfterItem(IN CLinkListItem* pItem,IN BOOL bUserManagesLock=FALSE);
    PVOID* ToArray(SIZE_T* pdwArraySize,BOOL bUserManagesLock=FALSE);
    SIZE_T Lock(BOOL bAllowToAddItemDuringLock=FALSE,BOOL LockOnlyOtherThreads = FALSE);
    SIZE_T Unlock();
    BOOL IsLocked();
    BOOL IsEmpty();
    BOOL IsItemStillInList(CLinkListItem* pItem,BOOL bUserManagesLock=FALSE);
    BOOL IsItemDataStillInList(PVOID pItemData,BOOL bUserManagesLock=FALSE);
    CLinkListItem* GetItem(SIZE_T ItemIndex,BOOL bUserManagesLock=FALSE);
    static BOOL Copy(CLinkListBase* pDst,CLinkListBase*pSrc,BOOL DstLocked=FALSE,BOOL SrcLocked=FALSE);
    FORCEINLINE DWORD GetThreadIdOfOwnerLock(){return this->ThreadIdOfOwnerLock;}
};
