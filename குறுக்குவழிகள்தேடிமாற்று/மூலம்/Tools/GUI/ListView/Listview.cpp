/*
Copyright (C) 2004-2016 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004-2016 Jacquelin POTIER <jacquelin.potier@free.fr>

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
// Object: class helper for listview control
//-----------------------------------------------------------------------------

#include "listview.h"
#include "../Dialog/DialogModelessProtection.h"
#include "../../String/AnsiUnicodeConvert.h"
#include "../../RegularExpression/RegularExpression.h"

// Notice : Require the ComCtl32.dll version 6 to make SetView method work
//          --> requires a manifest that specifies that version 6 of the dynamic-link library (DLL)

#define CListview_REPLACED_CHAR_ARRAY_SIZE 4
#define CListview_REPLACED_CHAR_MAX_INCREASE 6
TCHAR* pppszReplacedChar[CListview_REPLACED_CHAR_ARRAY_SIZE][2]={   
                                                                    {_T("&"),_T("&amp;")},
                                                                    {_T("<"),_T("&lt;")},
                                                                    {_T(">"),_T("&gt;")},
                                                                    {_T(" "),_T("&nbsp;")},
                                                                 };

#define CListview_CUSTOM_DRAW_EX_IMAGE_SIZE 16
#define CListview_CUSTOM_DRAW_EX_IMAGE_SPACER_BEFORE_TEXT 2
#define CListview_CUSTOM_DRAW_EX_MAX_SHOWN_CHAR 1024

#ifndef DefineTcharStringLength
    #define DefineTcharStringLengthWithEndingZero(x) (sizeof(x)/sizeof(TCHAR))
    #define DefineTcharStringLength(x) (sizeof(x)/sizeof(TCHAR)-1)
#endif

#ifndef BreakInDebugMode
    #ifdef _DEBUG
        #define BreakInDebugMode if ( ::IsDebuggerPresent() ) { ::DebugBreak();}
    #else 
        #define BreakInDebugMode 
    #endif
#endif

//-----------------------------------------------------------------------------
// Name: CListview
// Object: Constructor. Assume that the common control DLL is loaded.
// Parameters :
//     in  : HWND hWndListView : handle to listview
//     out :
//     return : 
//-----------------------------------------------------------------------------
CListview::CListview(HWND hWndListView,IListviewEvents* pListViewEventsHandler, IListViewItemsCompare* pListViewComparator)
{
    this->hWndListView=hWndListView;
    this->hWndParent=GetParent(this->hWndListView);
    this->bOwnerControlIsDialog=TRUE;
    this->pListViewEventsHandler = pListViewEventsHandler;
    this->pListViewComparator = pListViewComparator;

    // Ensure that the common control DLL is loaded.
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // set lock for multi threading access
    this->hevtUnlocked=CreateEvent(NULL,FALSE,TRUE,NULL);

    // set sorting vars
    this->bSortAscending=TRUE;
    this->iLastSortedColumn=-1;
    this->ItemKey=0;
    this->stSortingType=CListview::SortingTypeString;

    this->bSelectionCallBackEnable=TRUE;

    this->pSortArray = NULL;
    this->ppszReplacedChar = NULL;

    this->pPopUpMenu=new CPopUpMenu();
    this->pSubMenuCopySelectedAs = NULL;
    this->pSubMenuCopyAllAs = NULL;

    this->MenuIdCopySelected=this->pPopUpMenu->Add(_T("Copy Selected"));
    this->MenuIdCopyAll=this->pPopUpMenu->Add(_T("Copy All"));
    this->MenuIdSaveSelected=this->pPopUpMenu->Add(_T("Save Selected"));
    this->MenuIdSaveAll=this->pPopUpMenu->Add(_T("Save All"));
    this->MenuIdClear=(UINT)(-1);
    this->MenuIdCopySelectedAs     =(UINT)(-1);
    this->MenuIdCopySelectedAs_Rtf =(UINT)(-1);
    this->MenuIdCopySelectedAs_Csv =(UINT)(-1);
    this->MenuIdCopySelectedAs_Html=(UINT)(-1);
    this->MenuIdCopySelectedAs_Xml =(UINT)(-1);
    this->MenuIdCopyAllAs     =(UINT)(-1);
    this->MenuIdCopyAllAs_Rtf =(UINT)(-1);
    this->MenuIdCopyAllAs_Csv =(UINT)(-1);
    this->MenuIdCopyAllAs_Html=(UINT)(-1);
    this->MenuIdCopyAllAs_Xml =(UINT)(-1);

    this->PopUpMenuEnabled=TRUE;
    this->ColumnSortingEnabled=TRUE;
    this->DefaultCustomDrawEnabled=TRUE;
    this->DefaultCustomDrawExEnabled=FALSE;
    this->DefaultCustomDrawColor=CLISTVIEW_DEFAULT_CUSTOM_DRAW_COLOR;

    this->HeapListViewItemParams=::HeapCreate(0,CLISTVIEW_DEFAULT_HEAP_SIZE,0);

    this->hImgListNormal=NULL;
    this->hImgListSmall=NULL;
    this->hImgListState=NULL;
    this->hImgListColumns=NULL;

    this->hWndToolTip=NULL;
    this->bToolTipInfoLastActionIsMouseAction = FALSE;
    this->ToolTipInfoLastActionLastMousePoint.x=0;
    this->ToolTipInfoLastActionLastMousePoint.y=0;

    this->bAutoScrollDelayRefreshEnabled = FALSE;
    this->AutoScrollRequiredIndex = 0;
    this->AutoScrollAppliedIndex = 0;
    this->AutoScrollLastTickUpdate = 0;
    this->AutoScrollMinRefreshIntervalInMs = 1000;

    this->RtfSaveAsTable = TRUE;
    this->pRtfColumnInfoArray = NULL;

    this->DisableRedrawCounter = 0;
    this->bRedrawEnabled = TRUE;

    // internal tmp buffer
    ::InitializeCriticalSection(&this->InternalTmpBufferCriticalSection);
    this->pInternalTmpBuffer=NULL;
    this->InternalTmpBufferSize=0;
    this->bIconsForSubItemEnabled=FALSE;

    this->EnableDoubleBuffering(TRUE);
}
CListview::~CListview()
{
    this->pListViewEventsHandler = NULL;
    this->pListViewComparator = NULL;

    if (this->bAutoScrollDelayRefreshEnabled)
        ::KillTimer(this->hWndListView,(UINT_PTR)this);

    this->FreeCListviewItemParam(FALSE);
    if (this->pInternalTmpBuffer)
        ::HeapFree(this->HeapListViewItemParams,0,this->pInternalTmpBuffer);
    ::DeleteCriticalSection(&this->InternalTmpBufferCriticalSection);

    CloseHandle(this->hevtUnlocked);
    delete this->pPopUpMenu;

    if (this->pSubMenuCopySelectedAs)
        delete this->pSubMenuCopySelectedAs;

    if (this->pSubMenuCopyAllAs)
        delete this->pSubMenuCopyAllAs;

    this->EnableTooltip(FALSE);

    if (this->hImgListNormal)
        ImageList_Destroy(this->hImgListNormal);
    if (this->hImgListSmall)
        ImageList_Destroy(this->hImgListSmall);
    if (this->hImgListState)
        ImageList_Destroy(this->hImgListState);
    if (this->hImgListColumns)
        ImageList_Destroy(this->hImgListColumns);
}

// AddCopySelectedAsPopupMenu : return Id of added sub menu
UINT CListview::AddCopySelectedAsPopupMenu(UINT Index,HINSTANCE hInstance,int IdIconCopySelectedAs,int IdIconRtf,int IdIconCsv,int IdIconHtml,int IdIconXml,int Width,int Height)
{
    HICON IconCopySelectedAs=(HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IdIconCopySelectedAs),IMAGE_ICON,Width,Height,LR_DEFAULTCOLOR|LR_SHARED);
    HICON IconRtf=(HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IdIconRtf),IMAGE_ICON,Width,Height,LR_DEFAULTCOLOR|LR_SHARED);
    HICON IconCsv=(HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IdIconCsv),IMAGE_ICON,Width,Height,LR_DEFAULTCOLOR|LR_SHARED);
    HICON IconHtml=(HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IdIconHtml),IMAGE_ICON,Width,Height,LR_DEFAULTCOLOR|LR_SHARED);
    HICON IconXml=(HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IdIconXml),IMAGE_ICON,Width,Height,LR_DEFAULTCOLOR|LR_SHARED);
    return this->AddCopySelectedAsPopupMenu(Index,IconCopySelectedAs,IconRtf,IconCsv,IconHtml,IconXml);
}

// AddCopySelectedAsPopupMenu : return Id of added sub menu
UINT CListview::AddCopySelectedAsPopupMenu(UINT Index,HICON IconCopySelectedAs,HICON IconRtf,HICON IconCsv,HICON IconHtml,HICON IconXml)
{
    if (this->pSubMenuCopySelectedAs)
    {
        BreakInDebugMode;
        return this->MenuIdCopySelectedAs;
    }

    this->pSubMenuCopySelectedAs = new CPopUpMenu(this->pPopUpMenu);
    this->MenuIdCopySelectedAs_Rtf = this->pSubMenuCopySelectedAs->Add(TEXT("Copy As Rich Text Format"),IconRtf);
    this->MenuIdCopySelectedAs_Csv = this->pSubMenuCopySelectedAs->Add(TEXT("Copy As Csv"),IconCsv);
    this->MenuIdCopySelectedAs_Html = this->pSubMenuCopySelectedAs->Add(TEXT("Copy As Html"),IconHtml);
    this->MenuIdCopySelectedAs_Xml = this->pSubMenuCopySelectedAs->Add(TEXT("Copy As Xml"),IconXml);
    this->MenuIdCopySelectedAs = this->pPopUpMenu->AddSubMenu(TEXT("Copy Selected As"),this->pSubMenuCopySelectedAs,IconCopySelectedAs,Index);
    return this->MenuIdCopySelectedAs;
}

// AddCopyAllAsPopupMenu : return Id of added sub menu
UINT CListview::AddCopyAllAsPopupMenu(UINT Index,HINSTANCE hInstance,int IdIconCopyAllAs,int IdIconRtf,int IdIconCsv,int IdIconHtml,int IdIconXml,int Width,int Height)
{
    HICON IconCopyAllAs=(HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IdIconCopyAllAs),IMAGE_ICON,Width,Height,LR_DEFAULTCOLOR|LR_SHARED);
    HICON IconRtf=(HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IdIconRtf),IMAGE_ICON,Width,Height,LR_DEFAULTCOLOR|LR_SHARED);
    HICON IconCsv=(HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IdIconCsv),IMAGE_ICON,Width,Height,LR_DEFAULTCOLOR|LR_SHARED);
    HICON IconHtml=(HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IdIconHtml),IMAGE_ICON,Width,Height,LR_DEFAULTCOLOR|LR_SHARED);
    HICON IconXml=(HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IdIconXml),IMAGE_ICON,Width,Height,LR_DEFAULTCOLOR|LR_SHARED);
    return this->AddCopyAllAsPopupMenu(Index,IconCopyAllAs,IconRtf,IconCsv,IconHtml,IconXml);
}

// AddCopyAllAsPopupMenu : return Id of added sub menu
UINT CListview::AddCopyAllAsPopupMenu(UINT Index,HICON IconCopyAllAs,HICON IconRtf,HICON IconCsv,HICON IconHtml,HICON IconXml)
{
    if (this->pSubMenuCopyAllAs)
    {
        BreakInDebugMode;
        return this->MenuIdCopyAllAs;
    }

    this->pSubMenuCopyAllAs = new CPopUpMenu(this->pPopUpMenu);
    this->MenuIdCopyAllAs_Rtf = this->pSubMenuCopyAllAs->Add(TEXT("Copy As Rich Text Format"),IconRtf);
    this->MenuIdCopyAllAs_Csv = this->pSubMenuCopyAllAs->Add(TEXT("Copy As Csv"),IconCsv);
    this->MenuIdCopyAllAs_Html = this->pSubMenuCopyAllAs->Add(TEXT("Copy As Html"),IconHtml);
    this->MenuIdCopyAllAs_Xml = this->pSubMenuCopyAllAs->Add(TEXT("Copy As Xml"),IconXml);
    this->MenuIdCopyAllAs = this->pPopUpMenu->AddSubMenu(TEXT("Copy All As"),this->pSubMenuCopyAllAs,IconCopyAllAs,Index);
    return this->MenuIdCopyAllAs;
}

//-----------------------------------------------------------------------------
// Name: FreeCListviewItemParam
// Object: remove all CListviewItemParamBase associated with items
//         ListView MUST BE LOCKED WHEN CALLING THIS FUNCTION
// Parameters :
//     in  : BOOL NeedToReallocMemory : TRUE if items param are going to be used again
//                                      FALSE if no more items param are going to be used (should be FALSE only in destructor)
//     out :
//     return : HWND handle to the control
//-----------------------------------------------------------------------------
void CListview::FreeCListviewItemParam(BOOL NeedToReallocMemory)
{
    // calls CListview::FreeCListviewItemParamInternal
    return this->FreeCListviewItemParamInternal(NeedToReallocMemory);
}
void CListview::FreeCListviewItemParamInternal(BOOL NeedToReallocMemory)
{

    // slow way
    //LV_ITEM Item={0};
    //CListviewItemParamBase* pListviewItemParam;
    //Item.mask=LVIF_PARAM;

    //int NbItems=this->GetItemCount();
    //for (int ItemIndex=0;ItemIndex<NbItems;ItemIndex++)
    //{
    //    Item.iItem=ItemIndex;
    //    // try to get our own object associated with item
    //    if (!ListView_GetItem(this->hWndListView,&Item))
    //        return;
    //    // check if object is valid
    //    pListviewItemParam=(CListviewItemParamBase*)Item.lParam;
    //    if (IsBadReadPtr(pListviewItemParam,sizeof(CListviewItemParamBase)))
    //        return;

    //    delete pListviewItemParam;
    //}

    // high speed way (MSDN : you don't need to destroy allocated memory by calling HeapFree before calling HeapDestroy)
    ::HeapDestroy(this->HeapListViewItemParams);
    this->pInternalTmpBuffer=NULL;
    this->InternalTmpBufferSize=0;
    if (NeedToReallocMemory)
        this->HeapListViewItemParams=HeapCreate(0,CLISTVIEW_DEFAULT_HEAP_SIZE,0);
}
//-----------------------------------------------------------------------------
// Name: Clear
// Object: Clear the listview.
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::Clear()
{
    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);
    ListView_DeleteAllItems(this->hWndListView);
    // destroy memory after having removed items
    this->FreeCListviewItemParam(TRUE);
    this->ItemKey=0;
    SetEvent(this->hevtUnlocked);
}

//-----------------------------------------------------------------------------
// Name: RemoveSelectedItems
// Object: Remove Selected Items from the listview.
// Parameters :
//       BOOL bSelectedIsNotChecked 
// return : TRUE
//-----------------------------------------------------------------------------
BOOL CListview::RemoveSelectedItems(BOOL bSelectedIsNotChecked)
{
    int NbAlreadyLoadedItems = this->GetItemCount();
    // do parsing in reverse order as item are going to be removed
    for (int Cnt = NbAlreadyLoadedItems-1; Cnt>=0; Cnt--)
    {
        if (this->IsItemSelected(Cnt,bSelectedIsNotChecked))
            this->RemoveItem(Cnt);
    }
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: InitListViewColumns
// Object: Sets columns headers and some styles.
// Parameters :
//     in  : 
//     out :
//     return : TRUE if successful,FALSE on error
//-----------------------------------------------------------------------------
BOOL CListview::InitListViewColumns(int NbColumns,PCOLUMN_INFO pColumnInfo)
{ 
    int iCol; 

    // remove columns
    this->RemoveAllColumns();

    // Add the columns. 
    for (iCol = 0; iCol < NbColumns; iCol++) 
	{ 
        if (!this->SetColumn(iCol,pColumnInfo[iCol].pcName,pColumnInfo[iCol].iWidth,pColumnInfo[iCol].iTextAlign))
            return FALSE;
    } 
    return TRUE; 
}


//-----------------------------------------------------------------------------
// Name: SetView
// Object: Set listview view style. works only for XP and upper OS
//          require use of commctls 6 or upper
// Parameters :
//     in  : DWORD iView : new view to apply (LV_VIEW_DETAILS,LV_VIEW_ICON,LV_VIEW_LIST,LV_VIEW_SMALLICON,LV_VIEW_TILE, ...)
//     out :
//     return : TRUE if successful,FALSE on error
//-----------------------------------------------------------------------------
DWORD CListview::SetView(DWORD iView)
{
    return ListView_SetView(this->hWndListView,iView);
}

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add items at the end of listview
// Parameters :
//     in  : TCHAR* pcText : text string to add
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItem(TCHAR* pcText)
{
    return this->AddItemAndSubItems(1,&pcText);
}

//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add items at the end of listview
// Parameters :
//     in  : TCHAR* pcText : text string to add
//           BOOL ScrollToItem : TRUE to scroll to added item
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItem(TCHAR* pcText,BOOL ScrollToItem)
{
    return this->AddItem(pcText,NULL,ScrollToItem);
}
//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add items at the end of listview
// Parameters :
//     in  : TCHAR* pcText : text string to add
//           LPVOID UserData : User data to associate to item
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItem(TCHAR* pcText,LPVOID UserData)
{
    return this->AddItem(pcText,UserData,FALSE);
}
//-----------------------------------------------------------------------------
// Name: AddItem
// Object: Add items at the end of listview
// Parameters :
//     in  : TCHAR* pcText : text string to add
//           LPVOID UserData : User data to associate to item
//           BOOL ScrollToItem : TRUE to scroll to added item
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItem(TCHAR* pcText,LPVOID UserData,BOOL ScrollToItem)
{
    return this->AddItemAndSubItems(1,&pcText,this->GetItemCount(),ScrollToItem,UserData);
}
int CListview::AddItem(TCHAR* pcText,LPVOID UserData,LPVOID UserData2,BOOL ScrollToItem)
{
    return this->AddItemAndSubItems(1,&pcText,this->GetItemCount(),ScrollToItem,UserData,UserData2);
}

//-----------------------------------------------------------------------------
// Name: AddItemAndSubItems
// Object: Add items and sub items contains in array of string ppcText at the 
//          end of listview
// Parameters :
//     in  : int NbItem : number of string in ppcText
//           TCHAR** ppcText : array of Item/SubItems string 
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItemAndSubItems(int NbItem,TCHAR** ppcText)
{
    return this->AddItemAndSubItems(NbItem,ppcText,FALSE);
}

//-----------------------------------------------------------------------------
// Name: AddItemAndSubItems
// Object: Add items and sub items contains in array of string ppcText at the 
//          end of listview
// Parameters :
//     in  : int NbItem : number of string in ppcText
//           TCHAR** ppcText : array of Item/SubItems string 
//           BOOL ScrollToItem : TRUE to scroll to inserted item
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItemAndSubItems(int NbItem,TCHAR** ppcText,BOOL ScrollToItem)
{
    return this->AddItemAndSubItems(NbItem,ppcText,ScrollToItem,(LPVOID)NULL);
}

//-----------------------------------------------------------------------------
// Name: AddItemAndSubItems
// Object: Add items and sub items contains in array of string ppcText at the 
//          end of listview
// Parameters :
//     in  : int NbItem : number of string in ppcText
//           TCHAR** ppcText : array of Item/SubItems string 
//           BOOL ScrollToItem : TRUE to scroll to inserted item
//           LPVOID UserData : User data to associate to item
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItemAndSubItems(int NbItem,TCHAR** ppcText,BOOL ScrollToItem,LPVOID UserData,LPVOID UserData2)
{
    return this->AddItemAndSubItems(NbItem,ppcText,this->GetItemCount(),ScrollToItem,UserData,UserData2);
}

//-----------------------------------------------------------------------------
// Name: AddItemAndSubItems
// Object: Add items and sub items contains in array of string ppcText at  
//          position specified by ItemIndex
// Parameters :
//     in  : int NbItem : number of string in ppcText
//           TCHAR** ppcText : array of Item/SubItems string 
//           int ItemIndex : position in listview to and the new row
//           BOOL ScrollToItem : TRUE to scroll to inserted item
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItemAndSubItems(int NbItem,TCHAR** ppcText,int ItemIndex,BOOL ScrollToItem)
{
    return this->AddItemAndSubItems(NbItem,ppcText,ItemIndex,ScrollToItem,(LPVOID)NULL);
}

//-----------------------------------------------------------------------------
// Name: AddItemAndSubItems
// Object: Add items and sub items contains in array of string ppcText at  
//          position specified by ItemIndex
// Parameters :
//     in  : int NbItem : number of string in ppcText
//           TCHAR** ppcText : array of Item/SubItems string 
//           int ItemIndex : position in listview to and the new row
//           BOOL ScrollToItem : TRUE to scroll to inserted item
//           LPVOID UserData : User data to associate to item
//     out :
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItemAndSubItems(int NbItem,TCHAR** ppcText,int ItemIndex,BOOL ScrollToItem,LPVOID UserData,LPVOID UserData2)
{
    CListviewItemParamBase* pListviewItemParam;
    pListviewItemParam=this->CreateListviewItemParam();

    return this->AddItemAndSubItems(NbItem,ppcText,ItemIndex,ScrollToItem,UserData,UserData2,pListviewItemParam);
}


//-----------------------------------------------------------------------------
// Name: AddItemAndSubItems
// Object: Add items and sub items contains in array of string ppcText at  
//          position specified by ItemIndex
// Parameters :
//     in  : int NbItem : number of string in ppcText
//           TCHAR** ppcText : array of Item/SubItems string 
//           int ItemIndex : position in listview to and the new row
//           BOOL ScrollToItem : TRUE to scroll to inserted item
//           LPVOID UserData : User data to associate to item
//           CListviewItemParamBase* pListviewItemParam : an allocated CListviewItemParamBase* where all derived fields are set (CListviewItemParamBase will be adjusted by current function)
//                                                      should be called only by derived class requiring extra data to be set before insertion and drawing 
//                                                      --> should be called if derived class use values in derived class of CListviewItemParamBase for custom drawing
//     return : ItemIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddItemAndSubItems(int NbItem,TCHAR** ppcText,int ItemIndex,BOOL ScrollToItem,LPVOID UserData,LPVOID UserData2,CListviewItemParamBase* pListviewItemParam)
{
    LVITEM lvI = {0};
    
    int SubItemIndex;

    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);
    
    lvI.mask = LVIF_PARAM|LVIF_IMAGE; 
    lvI.iItem= ItemIndex;
	lvI.pszText = ppcText[0];
    if (ppcText[0]!=NULL)
       lvI.mask |= LVIF_TEXT;
    lvI.iImage = -1; // no image by default (avoid image 0 selection by default)
    // get a unique ID for parameter sorting
    if (pListviewItemParam == NULL)
        pListviewItemParam=this->CreateListviewItemParam();
    pListviewItemParam->ItemKey=this->ItemKey;
    pListviewItemParam->UserParam=UserData;
    pListviewItemParam->UserParam2=UserData2;
    lvI.lParam=(LPARAM)pListviewItemParam;
    this->ItemKey++;
    ItemIndex=ListView_InsertItem(this->hWndListView, &lvI);
    if (ItemIndex<0)
    {
        SetEvent(this->hevtUnlocked);
        return -1;
    }
    TCHAR** ppszText;
    for (SubItemIndex=1,ppszText=&ppcText[1];SubItemIndex<NbItem;SubItemIndex++,ppszText++)
    {
        // pszText = ppcText[SubItemIndex];
        if (ppszText)
        {
            ListView_SetItemText(
                                this->hWndListView,
                                ItemIndex,
                                SubItemIndex,
                                *ppszText
                                );
        }
    }

    if (ScrollToItem)
        this->ScrollToDelayedIfRequired(ItemIndex);
    SetEvent(this->hevtUnlocked);
    return ItemIndex;
}

// troubles setting text and next icons, the Draw process is done twice : first for text without icon, 
// and a second time for text with icon. If draw process is slow, it take 2* more time --> do it in one pass
// but anyway m$ (at least on 7) do it in 2 pass (first one setting text and a second one adding icon)
// so our custom draw is called twice :'(
int CListview::AddItemAndSubItemsWithIcons(int NbItem,TCHAR** ppcText,int* pIconIndexArray,int ItemIndex,BOOL ScrollToItem,LPVOID UserData,LPVOID UserData2,CListviewItemParamBase* pListviewItemParam)
{
    if (pIconIndexArray == NULL)
        return this->AddItemAndSubItems(NbItem,ppcText,ItemIndex,ScrollToItem,UserData,UserData2,pListviewItemParam);

    LVITEM lvI = {0};
    
    int SubItemIndex;

    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);
    
    lvI.mask = LVIF_PARAM|LVIF_IMAGE; 
    lvI.iItem= ItemIndex;
	lvI.pszText = ppcText[0];
    if (ppcText[0]!=NULL)
       lvI.mask |= LVIF_TEXT;
    lvI.iImage = pIconIndexArray[0]; // no image by default (avoid image 0 selection by default)
    // get a unique ID for parameter sorting
    if (pListviewItemParam == NULL)
        pListviewItemParam=this->CreateListviewItemParam();
    pListviewItemParam->ItemKey=this->ItemKey;
    pListviewItemParam->UserParam=UserData;
    pListviewItemParam->UserParam2=UserData2;
    lvI.lParam=(LPARAM)pListviewItemParam;
    this->ItemKey++;
    ItemIndex=ListView_InsertItem(this->hWndListView, &lvI);
    if (ItemIndex<0)
    {
        SetEvent(this->hevtUnlocked);
        return -1;
    }
    TCHAR** ppszText;
    int* pCurrentIconIndex;
    lvI.mask = LVIF_IMAGE;
    for (SubItemIndex=1,ppszText=&ppcText[1],pCurrentIconIndex=&pIconIndexArray[1];
         SubItemIndex<NbItem;
         SubItemIndex++,ppszText++,pCurrentIconIndex++)
    {
        lvI.iSubItem = SubItemIndex;
        // pszText = ppcText[SubItemIndex];
        if (ppszText && *ppszText)
        {
            lvI.pszText = *ppszText;
            lvI.mask |= LVIF_TEXT;
        }
        else
            lvI.mask &= ~LVIF_TEXT;
        lvI.iImage =*pCurrentIconIndex;
        
        ListView_SetItem(this->hWndListView,&lvI);
    }

    if (ScrollToItem)
        this->ScrollToDelayedIfRequired(ItemIndex);
    SetEvent(this->hevtUnlocked);
    return ItemIndex;
}

BOOL CListview::EnableAutoScrollDelayRefresh(BOOL bEnable,UINT MinRefreshIntervalInMs)
{
    if (bEnable)
    {
        if (this->bAutoScrollDelayRefreshEnabled)
            ::KillTimer(this->hWndListView,(UINT_PTR)this);
        this->AutoScrollMinRefreshIntervalInMs=MinRefreshIntervalInMs;
        this->bAutoScrollDelayRefreshEnabled = (BOOL)::SetTimer(this->hWndListView,(UINT_PTR)this,MinRefreshIntervalInMs,CListview::AutoScrollTimerProc);
        return this->bAutoScrollDelayRefreshEnabled;
    }
    else
    {
        if (!this->bAutoScrollDelayRefreshEnabled)
            return FALSE;
        this->bAutoScrollDelayRefreshEnabled = FALSE;
        return ::KillTimer(this->hWndListView,(UINT_PTR)this);
    }
}

VOID CALLBACK CListview::AutoScrollTimerProc(HWND hWnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
{
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(dwTime);
    CListview* pListview = (CListview*)idEvent;

    // don't apply scrolling until redraw is disabled
    if (!pListview->bRedrawEnabled)
        return;

    if (pListview->AutoScrollRequiredIndex != pListview->AutoScrollAppliedIndex)
    {
        pListview->AutoScrollLastTickUpdate = ::GetTickCount();
        pListview->AutoScrollAppliedIndex = pListview->AutoScrollRequiredIndex;
        pListview->ScrollTo(pListview->AutoScrollRequiredIndex);
    }
}

//-----------------------------------------------------------------------------
// Name: Move
// Object: Move item at position OldIndex to position NewIndex
// Parameters :
//     int OldIndex : index of item to move
//     int NewIndex : new index of item
// return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Move(int OldIndex,int NewIndex)
{
    BOOL RetValue = TRUE;
    BOOL bCurrentRet;
    LV_ITEM lvi = {0};
    int Cnt;

    if (OldIndex<0)
        return FALSE;
    if (NewIndex<0)
        return FALSE;
    if (NewIndex>=this->GetItemCount())
        return FALSE;

    // adjust new index value after Old index removal
    if (NewIndex>OldIndex)
        NewIndex++;

    // if no change needs to be done
    if (OldIndex == NewIndex)
        return TRUE;

    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);

    DWORD MaxLen = this->GetItemTextLenMaxForRow(OldIndex);
    // TCHAR* psz = new TCHAR[MaxLen];
    TCHAR* psz = (TCHAR*)this->GetInternalTmpBuffer(MaxLen*sizeof(TCHAR));

    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM
#if (_WIN32_IE >= 0x0300)
                | LVIF_INDENT | LVIF_NORECOMPUTE
#endif
//#if (_WIN32_WINNT >= 0x0501)
//                | LVIF_GROUPID | LVIF_COLUMNS 
//#endif
//#if _WIN32_WINNT >= 0x0600
//                | LVIF_COLFMT
//#endif
                ;

    lvi.iItem = OldIndex;
    lvi.pszText = psz;
    lvi.cchTextMax = MaxLen;

    // Create the new item, with all the info of the old...
    ListView_GetItem(this->hWndListView, &lvi);

    lvi.iItem = NewIndex;
    NewIndex = ListView_InsertItem(this->hWndListView, &lvi);
    if (NewIndex<0)
    {
        SetEvent(this->hevtUnlocked);
        //delete[] psz;
        this->ReleaseInternalTmpBuffer();
        return FALSE;
    }

    // if created item is above the old one, then the old one moves down one
    if(NewIndex < OldIndex)
        OldIndex++;

    // for sub items take only text
    lvi.mask = LVIF_TEXT;

    // Copy all the sub items from the old to the new
    for(Cnt = 1; Cnt < this->GetColumnCount(); Cnt++)
    {
        lvi.iSubItem = Cnt;

        lvi.iItem = OldIndex;// update old index
        ListView_GetItem(this->hWndListView, &lvi);

        lvi.iItem = NewIndex;
        // RetValue = RetValue && ListView_SetItem(this->hWndListView, &lvi); bug and optimization : function not called as soon as RetValue is FALSE
        bCurrentRet = ListView_SetItem(this->hWndListView, &lvi);
        RetValue = RetValue && bCurrentRet;
    }
    ListView_DeleteItem(this->hWndListView, OldIndex);

    SetEvent(this->hevtUnlocked);
    //delete[] psz;
    this->ReleaseInternalTmpBuffer();
    return RetValue;
}

BOOL CListview::MoveAllSelectedUp()
{
    int LastMoveIndex = -1;
    int NewIndex;
    BOOL bRetValue = TRUE;
    BOOL bTmpRetValue;
    // we have to start with the first element
    for (int Cnt= 0;Cnt<this->GetItemCount();Cnt++)
    {
        if (!this->IsItemSelected(Cnt,TRUE))
            continue;
        NewIndex = Cnt-1;
        if ( (NewIndex>=0) && (NewIndex>LastMoveIndex) )
        {
            bTmpRetValue = this->Move(Cnt,NewIndex);
            bRetValue = bRetValue && bTmpRetValue;
            LastMoveIndex = NewIndex;
        }
        else
            LastMoveIndex = Cnt;

        this->SetSelectedState(LastMoveIndex,TRUE,FALSE,TRUE);
    }

    return bRetValue;
}

BOOL CListview::MoveAllSelectedDown()
{
    int LastMoveIndex = this->GetItemCount()-1;
    int NewIndex;
    BOOL bRetValue = TRUE;
    BOOL bTmpRetValue;
    // we have to start with the last element
    for (int Cnt= this->GetItemCount()-1;Cnt>=0;Cnt--)
    {
        if (!this->IsItemSelected(Cnt,TRUE))
            continue;
        NewIndex = Cnt+1;
        if ( (NewIndex<this->GetItemCount()) && (NewIndex<=LastMoveIndex) ) // NewIndex<=LastMoveIndex as inserting at LastMoveIndex is possible (we will be inserted before item)
        {
            bTmpRetValue = this->Move(Cnt,NewIndex);
            bRetValue = bRetValue && bTmpRetValue;
            LastMoveIndex = NewIndex;
        }
        else
            LastMoveIndex = Cnt;

        this->SetSelectedState(LastMoveIndex,TRUE,FALSE,TRUE);
    }

    return bRetValue;
}

//-----------------------------------------------------------------------------
// Name: ScrollTo
// Object: scroll to item index
// Parameters :
//     in  : int ItemIndex : index of item
//     out :
//     return :
//-----------------------------------------------------------------------------
void CListview::ScrollTo(int ItemIndex)
{
    ListView_EnsureVisible(this->hWndListView,ItemIndex,FALSE);
}

//-----------------------------------------------------------------------------
// Name: ScrollToDelayedIfRequired
// Object: scroll to item index, assuming a potential delay according to EnableAutoScrollDelayRefresh
// Parameters :
//     in  : int ItemIndex : index of item
//     out :
//     return :
//-----------------------------------------------------------------------------
void CListview::ScrollToDelayedIfRequired(int ItemIndex)
{
    this->AutoScrollRequiredIndex = ItemIndex;
    if (this->bAutoScrollDelayRefreshEnabled)
    {
        if ( (::GetTickCount() - this->AutoScrollLastTickUpdate) > this->AutoScrollMinRefreshIntervalInMs)
        {
            this->AutoScrollLastTickUpdate = ::GetTickCount();
            this->AutoScrollAppliedIndex = ItemIndex;
            this->ScrollTo(ItemIndex);
        }
        // else let the timer proc do is job
    }
    else
        this->ScrollTo(ItemIndex);
}


//-----------------------------------------------------------------------------
// Name: RemoveItem
// Object: remove an item from list
// Parameters :
//     in  : int ItemIndex : index of item to remove
//     out :
//     return : TRUE if successful,FALSE on error
//-----------------------------------------------------------------------------
BOOL CListview::RemoveItem(int ItemIndex)
{
    BOOL RetValue;
    CListviewItemParamBase* pListviewItemParam = this->GetListviewItemParamInternal(ItemIndex);

    // remove item before freeing memory
    RetValue = ListView_DeleteItem(this->hWndListView,ItemIndex);
    if (pListviewItemParam)
    {
        this->DestroyListviewItemParam(pListviewItemParam);
    }

    return RetValue;
}

//-----------------------------------------------------------------------------
// Name: GetSelectedIndex
// Object: GetFirst selected item index 
//          ONLY FOR NON CHECKED LISTVIEW STYLE
// Parameters :
//     in  :
//     out :
//     return : first selected item index, or -1 if no selected item
//-----------------------------------------------------------------------------
int CListview::GetSelectedIndex()
{
    for (int cnt=0;cnt<this->GetItemCount();cnt++)
    {
        if (ListView_GetItemState(this->hWndListView,cnt,LVIS_SELECTED)==LVIS_SELECTED)
            return cnt;
    }
    return -1;
}

//-----------------------------------------------------------------------------
// Name: SetSelectedIndex
// Object: Select item at the specified index  
// Parameters :
//     in  : int ItemIndex : index of item to select
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetSelectedIndex(int ItemIndex)
{
    this->SetSelectedIndex(ItemIndex,FALSE);
}
//-----------------------------------------------------------------------------
// Name: SetSelectedIndex
// Object: Select item at the specified index  
// Parameters :
//     in  : int ItemIndex : index of item to select
//           BOOL bScrollToItem : TRUE to scroll to item and make item visible
//           BOOL bSetFocus : TRUE to set focus to listview
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetSelectedIndex(int ItemIndex,BOOL bScrollToItem,BOOL bSetFocus)
{
    ListView_SetItemState (this->hWndListView,
                          ItemIndex,
                          LVIS_FOCUSED | LVIS_SELECTED, // item state
                          0x000F);                      // mask 
    if (bScrollToItem)
        this->ScrollTo(ItemIndex);
    if (bSetFocus)
        this->SetFocus();
}


//-----------------------------------------------------------------------------
// Name: IsItemSelected
// Object: Check if item at the specified index is selected.
// Parameters :
//     in  : int ItemIndex : index of item to check
//     out :
//     return : TRUE if item is checked for checked listview, TRUE if item is selected for 
//              non checked listview
//-----------------------------------------------------------------------------
BOOL CListview::IsItemSelected(int ItemIndex)
{
    if (this->IsCheckedListView())
        return ListView_GetCheckState(this->hWndListView,ItemIndex);
    // else

        return (ListView_GetItemState(
                                        this->hWndListView,
                                        ItemIndex,
                                        LVIS_SELECTED)==LVIS_SELECTED);
}

//-----------------------------------------------------------------------------
// Name: IsItemSelected
// Object: Check if item at the specified index is selected
// Parameters :
//     in  : int ItemIndex : index of item to check
//           BOOL bSelectedIsNotChecked 
//     out :
//     return : TRUE if item is selected
//-----------------------------------------------------------------------------
BOOL CListview::IsItemSelected(int ItemIndex,BOOL bSelectedIsNotChecked)
{   
    if (bSelectedIsNotChecked)
        return (ListView_GetItemState(
                                    this->hWndListView,
                                    ItemIndex,
                                    LVIS_SELECTED)==LVIS_SELECTED);
    // else
        return this->IsItemSelected(ItemIndex);
}

//-----------------------------------------------------------------------------
// Name: SetSelectedState
// Object: set item at the specified index to a selected state
// Parameters :
//     in  : int ItemIndex : index of item to check
//           BOOL bSelected : TRUE to select, FALSE to unselect
//     out :
//     return : TRUE if item is selected
//-----------------------------------------------------------------------------
void CListview::SetSelectedState(int ItemIndex,BOOL bSelected)
{
    this->SetSelectedState(ItemIndex,bSelected,TRUE);
}


//-----------------------------------------------------------------------------
// Name: SetSelectedState
// Object: set item at the specified index to a selected state
// Parameters :
//     in  : int ItemIndex : index of item to check
//           BOOL bSelected : TRUE to select, FALSE to unselect
//           BOOL bSetFocus : TRUE if focus must be set (must be at least one for selecting in non checked mode)
//     out :
//     return : TRUE if item is selected
//-----------------------------------------------------------------------------
void CListview::SetSelectedState(int ItemIndex,BOOL bSelected,BOOL bSetFocus,BOOL bSelectedIsNotChecked)
{
    if (this->IsCheckedListView() && (!bSelectedIsNotChecked) )
    {
        ListView_SetCheckState(this->hWndListView,ItemIndex,bSelected);
    }
    else
    {
        if (bSetFocus)
            this->SetFocus();

        UINT state;
        if (bSelected)
            state=LVIS_SELECTED;
        else
            state=0;
        ListView_SetItemState (this->hWndListView,
                              ItemIndex,
                              state,    // item state
                              0x000F);  // mask 

    }
}


//-----------------------------------------------------------------------------
// Name: SelectAll
// Object: Select all items
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::SelectAll(BOOL bSelectedIsNotChecked)
{
    this->SetAllItemsSelectedState(TRUE,bSelectedIsNotChecked);
}

//-----------------------------------------------------------------------------
// Name: UnselectAll
// Object: Unselect all items
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::UnselectAll(BOOL bSelectedIsNotChecked)
{
    this->SetAllItemsSelectedState(FALSE,bSelectedIsNotChecked);
}

//-----------------------------------------------------------------------------
// Name: SetAllItemState
// Object: set the state of all items to selected or unselected
// Parameters :
//     in  : BOOL bSelected : TRUE to select, FALSE to unselect
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetAllItemsSelectedState(BOOL bSelected,BOOL bSelectedIsNotChecked)
{
    int ItemCount;

    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);

    // get number of items
    ItemCount=this->GetItemCount();

    // set focus to listview to avoid to do it each time an item is selected
    this->SetFocus();

    // for each item
    for (int cnt=0;cnt<ItemCount;cnt++)
        this->SetSelectedState(cnt,bSelected,FALSE,bSelectedIsNotChecked);

    SetEvent(this->hevtUnlocked);
}

//-----------------------------------------------------------------------------
// Name: UncheckSelected
// Object: Uncheck all selected items. Available only for checked list view
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::UncheckSelected()
{
    if (!this->IsCheckedListView())
        return;

    int ItemCount;

    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);

    // get number of items
    ItemCount=this->GetItemCount();

    // set focus to listview to avoid to do it each time an item is selected
    this->SetFocus();

    // for each item
    for (int cnt=0;cnt<ItemCount;cnt++)
    {
        if (this->IsItemSelected(cnt,TRUE))
            this->SetSelectedState(cnt,FALSE,FALSE);
    }

    SetEvent(this->hevtUnlocked);
}
//-----------------------------------------------------------------------------
// Name: CheckSelected
// Object: check all selected items. Available only for checked list view
// Parameters :
//     in  : 
//     out :
//     return : 
//-----------------------------------------------------------------------------
void CListview::CheckSelected()
{
    if (!this->IsCheckedListView())
        return;

    int ItemCount;

    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);

    // get number of items
    ItemCount=this->GetItemCount();

    // set focus to listview to avoid to do it each time an item is selected
    this->SetFocus();

    // for each item
    for (int cnt=0;cnt<ItemCount;cnt++)
    {
        if (this->IsItemSelected(cnt,TRUE))
            this->SetSelectedState(cnt,TRUE,FALSE);
    }

    SetEvent(this->hevtUnlocked);
}

//-----------------------------------------------------------------------------
// Name: GetSelectedCount
// Object: return number of selected items
// Parameters :
//     in  : 
//     out :
//     return : number of selected items
//-----------------------------------------------------------------------------
int CListview::GetSelectedCount(BOOL bSelectedIsNotChecked)
{
    if (!bSelectedIsNotChecked)
    {
        if (this->IsCheckedListView())
            return this->GetCheckedCount();
    }
    return ListView_GetSelectedCount(this->hWndListView);
}
//-----------------------------------------------------------------------------
// Name: GetCheckedCount
// Object: return number of selected items
//          WORK ONLY FOR LISTVIEW WITH CHECKBOXES STYLE
// Parameters :
//     in  : 
//     out :
//     return : number of selected items
//-----------------------------------------------------------------------------
int CListview::GetCheckedCount()
{
    int NbChecked=0;
    int ItemCount;

    WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);

    // get number of items
    ItemCount=this->GetItemCount();

    // for each item
    for (int cnt=0;cnt<ItemCount;cnt++)
        // if item is checked
        if (ListView_GetCheckState(this->hWndListView,cnt))
            // increase NbChecked
            NbChecked++;

    SetEvent(this->hevtUnlocked);

    return NbChecked;
}

//-----------------------------------------------------------------------------
// Name: IsCheckedListView
// Object: allow to know if listview has checked state
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if checked listview, FALSE else
//-----------------------------------------------------------------------------
BOOL CListview::IsCheckedListView()
{
    return (ListView_GetExtendedListViewStyle(this->hWndListView)&LVS_EX_CHECKBOXES);
}

//-----------------------------------------------------------------------------
// Name: GetItemTextBase
// Object: get item text
// Parameters :
//     in  : int ItemIndex : item index
//           int SubItemIndex : sub item index
//           int pcTextMaxSize : max buffer size for pcText
//     out : TCHAR* pcText : subitem text
//     return : 
//-----------------------------------------------------------------------------
DWORD CListview::GetItemTextBase(int ItemIndex,int SubItemIndex,TCHAR* pcText,int pcTextMaxSize)
{
    *pcText=0;
    LVITEM lvi;
    lvi.cchTextMax=pcTextMaxSize;
    lvi.pszText=pcText;
    lvi.iSubItem=SubItemIndex;
    DWORD retValue = (DWORD)SendMessage(this->hWndListView,LVM_GETITEMTEXT,ItemIndex,(LPARAM)&lvi);
    pcText[pcTextMaxSize-1]=0;
    return retValue;
}

DWORD CListview::GetItemText(int ItemIndex,int SubItemIndex,TCHAR* pcText,int pcTextMaxSize)
{
    return this->GetItemTextBase(ItemIndex,SubItemIndex,pcText,pcTextMaxSize);
}
//-----------------------------------------------------------------------------
// Name: GetItemTextLen
// Object: get item text len in TCHAR including \0
// Parameters :
//     in  : int ItemIndex : item index
//           int SubItemIndex : sub item index
//     out : 
//     return : size of text in TCHAR including \0
//-----------------------------------------------------------------------------
DWORD CListview::GetItemTextLen(int ItemIndex,int SubItemIndex)
{
    // LVITEM lvi;
    TCHAR* psz;
    DWORD pszSize=1024;
    DWORD CopySize=pszSize;

    // lvi.iSubItem=SubItemIndex;
    
    // do a loop increasing size
    // as soon as size of text copied to lvi.pszText is smaller than buffer size
    // we got the real size
    while ((CopySize==pszSize)||(CopySize==pszSize-1))
    {
        pszSize*=2;
        // psz=new TCHAR[pszSize];
        psz=(TCHAR*)this->GetInternalTmpBuffer(pszSize*sizeof(TCHAR));
        // use internal GetItemTextBase as derived class can add char to representation
        //lvi.cchTextMax=pszSize;
        //lvi.pszText=psz;
        // CopySize=(DWORD)SendMessage(this->hWndListView,LVM_GETITEMTEXT,ItemIndex,(LPARAM)&lvi);
        CopySize=this->GetItemText(ItemIndex,SubItemIndex,psz,pszSize);
        // delete[] psz
        this->ReleaseInternalTmpBuffer();
    }

    return CopySize+1;
}

//-----------------------------------------------------------------------------
// Name: GetItemTextLenMaxForColumn
// Object: get max text len of all items and subitems in the listview for the column
// Parameters :
//     in  : DWORD ColumnIndex 
//           BOOL bOnlySelected : if we ignore unselected items
//     out : 
//     return : max size of text in TCHAR including \0 of all item of column in the list
//-----------------------------------------------------------------------------
DWORD CListview::GetItemTextLenMaxForColumn(DWORD ColumnIndex, BOOL bOnlySelected, BOOL bSelectedIsNotChecked)
{
    int iNbRow=this->GetItemCount();

    // get max text len
    DWORD MaxTextLen=0;
    DWORD TextLen=0;
    // for each row
    for (int cnt=0;cnt<iNbRow;cnt++)
    {
        // if we save only selected items
        if (bOnlySelected)
        {
            // if item is not selected
            if (!this->IsItemSelected(cnt,bSelectedIsNotChecked))
                continue;
        }

        TextLen=this->GetItemTextLen(cnt,ColumnIndex);
        if (MaxTextLen<=TextLen)
            MaxTextLen=TextLen;
    }

    return MaxTextLen;
}

//-----------------------------------------------------------------------------
// Name: GetItemTextLenMaxForRow
// Object: get max text len of all items and subitems in the row
// Parameters :
//     in  : DWORD RowIndex 
//     return : max size of text in TCHAR including \0 of all item of row in the list
//-----------------------------------------------------------------------------
DWORD CListview::GetItemTextLenMaxForRow(DWORD RowIndex)
{
    int iNbColumn=this->GetColumnCount();

    if (iNbColumn<0)
        iNbColumn=1;

    // get max text len
    DWORD MaxTextLen=0;
    DWORD TextLen=0;
    // for each row
    for (int cnt=0;cnt<iNbColumn;cnt++)
    {
        TextLen=this->GetItemTextLen(RowIndex,cnt);
        if (MaxTextLen<=TextLen)
            MaxTextLen=TextLen;
    }

    return MaxTextLen;
}

//-----------------------------------------------------------------------------
// Name: GetItemTextLenMax
// Object: get max text len of all items and subitems in the listview
// Parameters :
//     in  : BOOL bOnlySelected : if we ignore unselected items
//     out : 
//     return : max size of text in TCHAR including \0 of all item and subitems in the list
//-----------------------------------------------------------------------------
DWORD CListview::GetItemTextLenMax(BOOL bOnlySelected)
{
    int iNbRow=this->GetItemCount();
    int iNbColumn=this->GetColumnCount();

    if (iNbColumn<0)
        iNbColumn=1;

    // get max text len
    DWORD MaxTextLen=0;
    DWORD TextLen=0;
    // for each row
    for (int cnt=0;cnt<iNbRow;cnt++)
    {
        // if we save only selected items
        if (bOnlySelected)
        {
            // if item is not selected
            if (!this->IsItemSelected(cnt,TRUE))
                continue;
        }

        // for each column
        for (int cnt2=0;cnt2<iNbColumn;cnt2++)
        {
            TextLen=this->GetItemTextLen(cnt,cnt2);
            if (MaxTextLen<=TextLen)
                MaxTextLen=TextLen;
            
        }
    }

    return MaxTextLen;
}

//-----------------------------------------------------------------------------
// Name: GetColumnName
// Object: get column name
// Parameters :
//     in  : int ColumnIndex : column index
//           TCHAR* Name : column text
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CListview::GetColumnName(int ColumnIndex,TCHAR* Name,int NameMaxSize)
{
    HWND hWndHeader = ListView_GetHeader(this->hWndListView);
    HDITEM hdi = { 0 };
    hdi.mask = HDI_TEXT;
    hdi.pszText = Name;
    hdi.cchTextMax = NameMaxSize;
    return Header_GetItem(hWndHeader, ColumnIndex, &hdi);
}

//-----------------------------------------------------------------------------
// Name: SetColumnName
// Object: set column name
// Parameters :
//     in  : int ColumnIndex : column index
//           TCHAR* Name : column text
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CListview::SetColumnName(int ColumnIndex,TCHAR* Name)
{
    LVCOLUMN lvc = { 0 };
    if (this->GetColumnCount()<=ColumnIndex)
        return this->AddColumn(Name,100,LVCFMT_LEFT,ColumnIndex);

    lvc.mask = LVCF_TEXT;	  
    lvc.pszText = Name;

    return ListView_SetColumn(this->hWndListView, ColumnIndex,&lvc);
}
//-----------------------------------------------------------------------------
// Name: SetColumn
// Object: set column name
// Parameters :
//     in  : int ColumnIndex : column index
//           TCHAR* Name : column text
//           int iWidth : width of column in pixels
//           int iTextAlign : LVCFMT_XX
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CListview::SetColumn(int ColumnIndex,TCHAR* Name,int iWidth,int iTextAlign)
{
    LVCOLUMN lvc = { 0 };
    if (this->GetColumnCount()<=ColumnIndex)
        return this->AddColumn(Name,iWidth,iTextAlign,ColumnIndex);

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;	  
    lvc.pszText = Name;
    lvc.cx = iWidth;
    lvc.fmt =iTextAlign;
    
    return ListView_SetColumn(this->hWndListView, ColumnIndex,&lvc);
}

//-----------------------------------------------------------------------------
// Name: AddColumn
// Object: set column name
// Parameters :
//     in  : int ColumnIndex : column index
//           TCHAR* Name : column text
//           int iWidth : width of column in pixels
//           int iTextAlign : LVCFMT_XX
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CListview::AddColumn(TCHAR* Name,int iWidth,int iTextAlign,int ColumnIndex)
{
    LVCOLUMN lvc = { 0 };

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;	  

    lvc.pszText = Name;
    lvc.cx = iWidth;
    lvc.fmt =iTextAlign;

    return ListView_InsertColumn(this->hWndListView, ColumnIndex,&lvc);
}

//-----------------------------------------------------------------------------
// Name: AddColumn
// Object: set column name
// Parameters :
//     in  : TCHAR* Name : column text
//           int iWidth : width of column in pixels
//           int iTextAlign : LVCFMT_XX
//     out : 
//     return : ColumnIndex if successful,-1 on error
//-----------------------------------------------------------------------------
int CListview::AddColumn(TCHAR* Name,int iWidth,int iTextAlign)
{
    int iCol=this->GetColumnCount();
    if (iCol==-1)
        iCol++;
    return this->AddColumn(Name,iWidth,iTextAlign,iCol);
}

//-----------------------------------------------------------------------------
// Name: RemoveColumn
// Object: Remove column. 
//          Notice : column 0 can't be removed, so only it's name and it's content will be free
// Parameters :
//     in  : int ColumnIndex : 0 based column index
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CListview::RemoveColumn(int ColumnIndex)
{
    if (ColumnIndex==0)
    {
        this->Clear();
        return this->SetColumnName(0,_T(""));
    }
    return ListView_DeleteColumn(this->hWndListView,ColumnIndex);
}

//-----------------------------------------------------------------------------
// Name: RemoveAllColumns
// Object: Remove all columns
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
BOOL CListview::RemoveAllColumns()
{
    this->Clear();
    int NbColumns=this->GetColumnCount();
    // column 0 can't be remove, so set it's name to an empty name
    this->SetColumnName(0,TEXT(""));
    // remove other columns
    for (int cnt=NbColumns-1;cnt>=1;cnt--)
        this->RemoveColumn(cnt);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: HideColumn
// Object: hide specified column
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::HideColumn(int ColumnIndex)
{
    ListView_SetColumnWidth(this->hWndListView, ColumnIndex,0);
}

//-----------------------------------------------------------------------------
// Name: ShowColumn
// Object: show specified column
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::ShowColumn(int ColumnIndex)
{
    this->ShowColumn(ColumnIndex,LIST_VIEW_COLUMN_DEFAULT_WIDTH);
}
//-----------------------------------------------------------------------------
// Name: ShowColumn
// Object: show specified column
// Parameters :
//     in  : int Size : size of visible column
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::ShowColumn(int ColumnIndex,int Size)
{
    ListView_SetColumnWidth(this->hWndListView, ColumnIndex,Size);
}

//-----------------------------------------------------------------------------
// Name: IsColumnVisible
// Object: check if specified column is visible
// Parameters :
//     in  : 
//     out : 
//     return : TRUE if visible, FALSE else
//-----------------------------------------------------------------------------
BOOL CListview::IsColumnVisible(int ColumnIndex)
{
    return (ListView_GetColumnWidth(this->hWndListView,ColumnIndex)!=0);
}

//-----------------------------------------------------------------------------
// Name: SetItemText
// Object: add item if required and set item text
// Parameters :
//     in  : int ItemIndex : item index
//           TCHAR* pcText : item text
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetItemText(int ItemIndex,TCHAR* pcText)
{
    this->SetItemText(
    ItemIndex,
    0,
    pcText
    );
}
//-----------------------------------------------------------------------------
// Name: SetItemText
// Object: add item if required and set item text
// Parameters :
//     in  : int ItemIndex : item index
//           int SubItemIndex subitem index
//           TCHAR* pcText : subitem text
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetItemText(int ItemIndex,int SubItemIndex,TCHAR* pcText)
{
    LVITEM lvI;
    memset(&lvI,0,sizeof(LVITEM));
    lvI.iItem=ItemIndex;
    TCHAR* Array[1];

    // if item don't exist
    if (!ListView_GetItem(this->hWndListView,&lvI))
    {
        Array[0]=_T("");
        // create it
        this->AddItemAndSubItems(1,Array,ItemIndex,FALSE);
    }

    ListView_SetItemText(
                        this->hWndListView,
                        ItemIndex,
                        SubItemIndex,
                        pcText
                        );
}

//-----------------------------------------------------------------------------
// Name: SetItemIndent
// Object: set indent of item
//          Notice : Indent is in icon size count
// Parameters :
//     in  : 
//     out : 
//     return : number of items
//-----------------------------------------------------------------------------
BOOL CListview::SetItemIndent(int ItemIndex,int Indent)
{
    LVITEM Item={0};
    Item.mask = LVIF_INDENT;
    Item.iItem = ItemIndex;
    Item.iSubItem = 0; // don't work for subitems
    Item.iIndent = Indent;
    
    return (BOOL)ListView_SetItem(this->hWndListView,&Item);
}

//-----------------------------------------------------------------------------
// Name: GetItemCount
// Object: get number of items
// Parameters :
//     in  : 
//     out : 
//     return : number of items
//-----------------------------------------------------------------------------
int CListview::GetItemCount()
{
    return ListView_GetItemCount(this->hWndListView);
}
//-----------------------------------------------------------------------------
// Name: GetColumnCount
// Object: get number of columns
// Parameters :
//     in  : 
//     out : 
//     return : number of columns
//-----------------------------------------------------------------------------
int CListview::GetColumnCount()
{
    return Header_GetItemCount(ListView_GetHeader(this->hWndListView));
}

//-----------------------------------------------------------------------------
// Name: SetStyle
// Object: set style helper
// Parameters :
//     in  : BOOL bFullRowSelect : true for full row select
//           BOOL bGridLines : true for gridLines
//           BOOL bSendClickNotification : true for sending click notifications : 
//                                          if TRUE sends an LVN_ITEMACTIVATE notification code to the parent window when the user clicks an item
//           BOOL bCheckBoxes : TRUE for check box style listview
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetStyle(BOOL bFullRowSelect,BOOL bGridLines,BOOL bSendClickNotification,BOOL bCheckBoxes)
{   
    DWORD dwStyles=ListView_GetExtendedListViewStyle(this->hWndListView);
    if (bFullRowSelect)
        dwStyles|=LVS_EX_FULLROWSELECT;
    else
        dwStyles&=~LVS_EX_FULLROWSELECT;
    if (bGridLines)
        dwStyles|=LVS_EX_GRIDLINES;
    else
        dwStyles&=~LVS_EX_GRIDLINES;
    if (bSendClickNotification)
    {
        dwStyles|=LVS_EX_ONECLICKACTIVATE;
        dwStyles|=LVS_EX_TWOCLICKACTIVATE;
    }
    else
    {
        dwStyles&=~LVS_EX_ONECLICKACTIVATE;
        dwStyles&=~LVS_EX_TWOCLICKACTIVATE;
    }

    if (bCheckBoxes)
        dwStyles|=LVS_EX_CHECKBOXES;
    else
        dwStyles&=~LVS_EX_CHECKBOXES;

    ListView_SetExtendedListViewStyle(this->hWndListView,dwStyles);
}

//-----------------------------------------------------------------------------
// Name: ReSort
// Object: Do the same action as the last sort command
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::ReSort()
{
    // if never sorted
    if (this->iLastSortedColumn==-1)
        return;
    this->Sort(this->iLastSortedColumn,this->bSortAscending);
}

//-----------------------------------------------------------------------------
// Name: Sort
// Object: Sort content of the listview based on the first column.
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::Sort()
{
    this->Sort(0);
}
//-----------------------------------------------------------------------------
// Name: Sort
// Object: Sort content of the listview based on the column with index ColumnIndex.
//         first time a column is selected, sorting is ascending, and next 
//         column sorting direction is auto changed until sort is call for another column index
// Parameters :
//     in  : int ColumnIndex : 0 based column index
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::Sort(int ColumnIndex)
{
    int ColumnCount=this->GetColumnCount();
    // adjust column count in case of null index
    if (ColumnCount==-1)
        ColumnCount=0;
    else// modify ColumnCount for 0 base index
        ColumnCount--;
    if (ColumnIndex>ColumnCount)
        return;

    if (ColumnIndex==this->iLastSortedColumn)
        this->bSortAscending=!this->bSortAscending;
    else
        this->bSortAscending=TRUE;

    this->Sort(ColumnIndex,this->bSortAscending);

}
//-----------------------------------------------------------------------------
// Name: Sort
// Object: Sort content of the listview based on the column with index ColumnIndex.
//         You can call Sort with same column index without specifying the direction
//         if you just want to inverse order (it will be managed internally)
// Parameters :
//     in  : int ColumnIndex : 0 based column index
//           BOOL bAscending : TRUE for ascending sorting
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::Sort(int ColumnIndex,BOOL bAscending)
{
    // update iLastSortedColumn before calling ListView_SortItems
    this->iLastSortedColumn=ColumnIndex;
    this->bSortAscending=bAscending;
    // sort
    WaitForSingleObject(hevtUnlocked,LIST_VIEW_TIMEOUT);
    HCURSOR hOldCursor=SetCursor(LoadCursor(NULL,IDC_WAIT));

    // make a local copy of {lparam,string} as content of listview is unstable during sorting
    // make a new array of ItemKey size to store string at lparam position in the array
    this->pSortArray=new TCHAR*[this->ItemKey];
    memset (this->pSortArray,0,this->ItemKey*sizeof(TCHAR*));

    DWORD nbItem=this->GetItemCount();
    DWORD cnt;
    LV_ITEM Item={0};
    CListviewItemParamBase* pListviewItemParam;
    Item.mask=LVIF_PARAM|LVIF_TEXT;
    TCHAR psz[MAX_PATH];
    *psz=0;
    Item.iSubItem=this->iLastSortedColumn;
    Item.pszText=psz;
    Item.cchTextMax=MAX_PATH;
    // for each item of list
    for (cnt=0;cnt<nbItem;cnt++)
    {
        Item.iItem=cnt;
        if (ListView_GetItem(this->hWndListView,&Item))
        {
            pListviewItemParam=(CListviewItemParamBase*)Item.lParam;
            if (pListviewItemParam)
                this->pSortArray[pListviewItemParam->ItemKey]=_tcsdup(Item.pszText);
        }
    }

    this->DoListViewSortItems();

    for (cnt=0;cnt<this->ItemKey;cnt++)
    {
        if (this->pSortArray[cnt])
            free(this->pSortArray[cnt]);
    }
    delete[] this->pSortArray;

    // show up/down arrow on headers
    HDITEM HeaderItem;
    HWND hHeader=ListView_GetHeader(this->hWndListView);
    HeaderItem.mask=HDI_FORMAT;
    for (cnt=0;cnt<(DWORD)this->GetColumnCount();cnt++)
    {
        Header_GetItem(hHeader,cnt,&HeaderItem);

        // if current modify index
        if (cnt==(DWORD)ColumnIndex)
        {
            // show up or down arrow
            if (bAscending)
            {
                HeaderItem.fmt|=HDF_SORTUP;
                HeaderItem.fmt&=~HDF_SORTDOWN;
            }
            else
            {
                HeaderItem.fmt|=HDF_SORTDOWN;
                HeaderItem.fmt&=~HDF_SORTUP;
            }
        }
        else
        {
            // remove any previous arrow
            HeaderItem.fmt&=~HDF_SORTUP;
            HeaderItem.fmt&=~HDF_SORTDOWN;
        }
        Header_SetItem(hHeader,cnt,&HeaderItem);
    }

    SetCursor(hOldCursor);
    SetEvent(this->hevtUnlocked);
}

void CListview::DoListViewSortItems()
{
    ListView_SortItems(this->hWndListView,CListview::Compare,this);
}

//-----------------------------------------------------------------------------
// Name: RemoveSortingHeaderInfo
// Object: remove sorting column header info (example you want to add unsorted elements after having done sorting)
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::RemoveSortingHeaderInfo()
{
    // if sorting has never been done
    if (this->iLastSortedColumn==-1)
        return; // --> nothing to do

    // else remove HDF_SORTUP/HDF_SORTDOWN info
    HWND hHeader=ListView_GetHeader(this->hWndListView);

    HDITEM HeaderItem;
    HeaderItem.mask=HDI_FORMAT;
    Header_GetItem(hHeader,this->iLastSortedColumn,&HeaderItem);
    // remove any previous arrow
    HeaderItem.fmt&=~HDF_SORTUP;
    HeaderItem.fmt&=~HDF_SORTDOWN;
    Header_SetItem(hHeader,this->iLastSortedColumn,&HeaderItem);

    this->iLastSortedColumn=-1;
}

//-----------------------------------------------------------------------------
// Name: Sorting compare
// Object: 
// Parameters :
//     in  : LPARAM lParam1 : Item lParam given when item was inserted
//           LPARAM lParam2 : Item lParam given when item was inserted
//           LPARAM lParamSort : CListview* pointer to CListview object being sort
//     out : 
//     return : a negative value if the first item should precede the second,
//              a positive value if the first item should follow the second, 
//              or zero if the two items are equivalent
//-----------------------------------------------------------------------------
int CALLBACK CListview::Compare(LPARAM lParam1, LPARAM lParam2,LPARAM lParamSort)
{
    CListview* pCListview=(CListview*)lParamSort;
    CListviewItemParamBase* Param1;
    CListviewItemParamBase* Param2;

    Param1=(CListviewItemParamBase*)lParam1;
    Param2=(CListviewItemParamBase*)lParam2;

    int iAscendingFlag;

    if (pCListview->bSortAscending)
        iAscendingFlag=1;
    else
        iAscendingFlag=-1;
    if (Param1==NULL)
        return (-iAscendingFlag);
    if (IsBadReadPtr(Param1,sizeof(CListviewItemParamBase*)))
        return (-iAscendingFlag);
    if (Param2==NULL)
        return (iAscendingFlag);
    if (IsBadReadPtr(Param2,sizeof(CListviewItemParamBase*)))
        return (iAscendingFlag);

    if (IsBadReadPtr(pCListview->pSortArray[Param1->ItemKey],sizeof(TCHAR*)))
        return (-iAscendingFlag);
    if (IsBadReadPtr(pCListview->pSortArray[Param2->ItemKey],sizeof(TCHAR*)))
        return (iAscendingFlag);
    
    if (pCListview->pListViewComparator)
    {
        return pCListview->pListViewComparator->OnListViewSortingCompare(pCListview,
                                                                         pCListview->iLastSortedColumn,
                                                                         pCListview->pSortArray[Param1->ItemKey],
                                                                         pCListview->pSortArray[Param2->ItemKey],
                                                                         pCListview->bSortAscending
                                                                         );
    }
    else
    {
        // compare strings
        switch (pCListview->stSortingType)
        {
            case CListview::SortingTypeNumber:
                {
                    INT64 i1=0;_stscanf(pCListview->pSortArray[Param1->ItemKey],TEXT("%I64d"),&i1);
                    INT64 i2=0;_stscanf(pCListview->pSortArray[Param2->ItemKey],TEXT("%I64d"),&i2);
                    // return (int)((i1-i2)*iAscendingFlag); wrong
                    if (i1==i2)
                        return 0;
                    return (i1 > i2)?iAscendingFlag:-iAscendingFlag;
                    
                }
            case CListview::SortingTypeNumberUnsigned:
                {
                    UINT64 i1=0;_stscanf(pCListview->pSortArray[Param1->ItemKey],TEXT("%I64u"),&i1);
                    UINT64 i2=0;_stscanf(pCListview->pSortArray[Param2->ItemKey],TEXT("%I64u"),&i2);
                    // return (int)(INT64)((i1-i2)*iAscendingFlag); wrong
                    if (i1==i2)
                        return 0;
                    return (i1 > i2)?iAscendingFlag:-iAscendingFlag;
                }
            case CListview::SortingTypeHexaNumber:
                {
                    UINT64 i1=0;_stscanf(pCListview->pSortArray[Param1->ItemKey],TEXT("0x%I64x"),&i1);
                    UINT64 i2=0;_stscanf(pCListview->pSortArray[Param2->ItemKey],TEXT("0x%I64x"),&i2);
                    // return (int)(INT64)((i1-i2)*iAscendingFlag); wrong
                    if (i1==i2)
                        return 0;
                    return (i1 > i2)?iAscendingFlag:-iAscendingFlag;
                }
            case CListview::SortingTypeDouble:
            {
                double d1=0.0;_stscanf(pCListview->pSortArray[Param1->ItemKey],TEXT("%g"),&d1);
                double d2=0.0;_stscanf(pCListview->pSortArray[Param2->ItemKey],TEXT("%g"),&d2);
                // return (int)(INT64)((d1-d2)*iAscendingFlag); wrong
                if (d1==d2)
                    return 0;
                return (d1 > d2)?iAscendingFlag:-iAscendingFlag;
            }

            case CListview::SortingTypeUserParamAsSIZE_T:
            {
                if (Param1->UserParam == Param2->UserParam)
                    return 0;
                return (((SIZE_T)Param1->UserParam) > ((SIZE_T)Param2->UserParam))?iAscendingFlag:-iAscendingFlag;
            }
            case CListview::SortingTypeUserParamAsSSIZE_T:
            {
                if (Param1->UserParam == Param2->UserParam)
                    return 0;
                return (((SIZE_T)Param1->UserParam) > ((SIZE_T)Param2->UserParam))?iAscendingFlag:-iAscendingFlag;
            }

            case CListview::SortingTypeUserParam2AsSIZE_T:
                {
                    if (Param1->UserParam2 == Param2->UserParam2)
                        return 0;
                    return (((SIZE_T)Param1->UserParam2) > ((SIZE_T)Param2->UserParam2))?iAscendingFlag:-iAscendingFlag;
                }
            case CListview::SortingTypeUserParam2AsSSIZE_T:
                {
                    if (Param1->UserParam2 == Param2->UserParam2)
                        return 0;
                    return (((SIZE_T)Param1->UserParam2) > ((SIZE_T)Param2->UserParam2))?iAscendingFlag:-iAscendingFlag;
                }


            default:
            case CListview::SortingTypeString:
                return (_tcsicmp(pCListview->pSortArray[Param1->ItemKey],pCListview->pSortArray[Param2->ItemKey])*iAscendingFlag);
        }
    }
}

//-----------------------------------------------------------------------------
// Name: SetSortingType
// Object: Set the sorting type 
// Parameters :
//     in  : CListview::SortingType type : new type of sorting
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetSortingType(CListview::SortingType type)
{
    this->stSortingType=type;
}

BOOL CListview::SetItemUserData(int ItemIndex,LPVOID UserData,LPVOID UserData2)
{
    LV_ITEM Item={0};
    CListviewItemParamBase* pListviewItemParam;
    Item.mask=LVIF_PARAM;
    Item.iItem=ItemIndex;
    // try to get our own object associated with item
    if (!ListView_GetItem(this->hWndListView,&Item))
        return FALSE;
    // check if object is valid
    pListviewItemParam=(CListviewItemParamBase*)Item.lParam;
    if (pListviewItemParam==NULL)
        return FALSE;
    if (IsBadReadPtr(pListviewItemParam,sizeof(CListviewItemParamBase)))
        return FALSE;

    // set the user part of our object
    pListviewItemParam->UserParam=UserData;
    pListviewItemParam->UserParam2=UserData2;

    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: GetItemUserData
// Object: Get user data associated with item
// Parameters :
//     in  : int ItemIndex : index of item
//           LPVOID* pUserData : pointer to value to retrieve
//     out : 
//     return : TRUE on success, FALSE on error
//-----------------------------------------------------------------------------
BOOL CListview::GetItemUserData(int ItemIndex,LPVOID* pUserData,LPVOID* pUserData2)
{
    BOOL bRetValue = TRUE;
    // check pointer
    if (pUserData)
    {
        if (::IsBadWritePtr(pUserData,sizeof(LPVOID)))
            bRetValue = FALSE;
        else
            *pUserData = NULL;
    }

    if (pUserData2)
    {
        if (::IsBadWritePtr(pUserData2,sizeof(LPVOID)))
            bRetValue = FALSE;
        else
            *pUserData2=NULL;
    }

    if (bRetValue == FALSE)
        return FALSE;
    if ( (pUserData==NULL) && (pUserData2==NULL) )
        return FALSE;

    LV_ITEM Item={0};
    CListviewItemParamBase* pListviewItemParam;
    Item.mask=LVIF_PARAM;
    Item.iItem=ItemIndex;
    // try to get our own object associated with item
    if (!ListView_GetItem(this->hWndListView,&Item))
        return FALSE;
    // check if object is valid
    pListviewItemParam=(CListviewItemParamBase*)Item.lParam;
    if (pListviewItemParam==NULL)
        return FALSE;
    if (::IsBadReadPtr(pListviewItemParam,sizeof(CListviewItemParamBase)))
        return FALSE;

    // return the user part of our object to user
    if (pUserData)
        *pUserData = pListviewItemParam->UserParam;

    if (pUserData2)
        *pUserData2 = pListviewItemParam->UserParam2;

    return TRUE;
}

CListview::CListviewItemParamBase* CListview::GetListviewItemParamInternal(int ItemIndex)
{
    LV_ITEM Item={0};
    Item.mask=LVIF_PARAM;
    Item.iItem=ItemIndex;
    // try to get our own object associated with item
    if (!ListView_GetItem(this->hWndListView,&Item))
        return NULL;
    return (CListviewItemParamBase*)Item.lParam;
}
BOOL CListview::SetListviewItemParamInternal(int ItemIndex,CListviewItemParamBase* pListviewItemParam)
{
    LV_ITEM Item={0};
    Item.mask=LVIF_PARAM;
    Item.lParam = (LPARAM)pListviewItemParam;
    Item.iItem=ItemIndex;
    return ListView_SetItem(this->hWndListView,&Item);
}

//-----------------------------------------------------------------------------
// Name: Find
// Object: search for item string in listview
// Parameters :
//     in  : Text : text to search for
//     out : 
//     return : index on success, -1 if not found
//-----------------------------------------------------------------------------
int CListview::Find(TCHAR* Text)
{
    return this->Find(Text,-1,FALSE);
}
//-----------------------------------------------------------------------------
// Name: Find
// Object: search for item string in listview
// Parameters :
//     in  : Text : text to search for
//           BOOL bPartial : TRUE for partial matching : Checks to see if the item text begins with Text
//     out : 
//     return : index on success, -1 if not found
//-----------------------------------------------------------------------------
int CListview::Find(TCHAR* Text,BOOL bPartial)

{
    return this->Find(Text,-1,bPartial);
}
//-----------------------------------------------------------------------------
// Name: Find
// Object: search for item string in listview
// Parameters :
//     in  : Text : text to search for
//           int StartIndex : Index of the item to begin the search
//           BOOL bPartial : TRUE for partial matching : Checks to see if the item text begins with Text
//     out : 
//     return : index on success, -1 if not found
//-----------------------------------------------------------------------------
int CListview::Find(TCHAR* Text,int StartIndex,BOOL bPartial)
{
    LVFINDINFO FindInfo={0};
    FindInfo.psz=Text;
    FindInfo.flags=LVFI_STRING;
    if (bPartial)
        FindInfo.flags|=LVFI_PARTIAL;
    return ListView_FindItem(this->hWndListView,StartIndex,&FindInfo);
}

//-----------------------------------------------------------------------------
// Name: ShowColumnHeaders
// Object: show or hides columns headers
// Parameters :
//     in  : BOOL bShow : True to show columns headers
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::ShowColumnHeaders(BOOL bShow)
{
    LONG_PTR Style=GetWindowLongPtr(this->hWndListView,GWL_STYLE);

    if (bShow)
        Style&=~LVS_NOCOLUMNHEADER;        
    else
        Style|=LVS_NOCOLUMNHEADER;

    SetWindowLongPtr(this->hWndListView,GWL_STYLE,Style);
    
}

//-----------------------------------------------------------------------------
// Name: SetTransparentBackGround
// Object: make background of listview to become transparent
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetTransparentBackGround()
{
    ListView_SetBkColor(this->hWndListView,CLR_NONE);
    ListView_SetTextBkColor(this->hWndListView,CLR_NONE);
    if (this->hWndListView)
        ::InvalidateRect(this->hWndListView,0,TRUE);
}

//-----------------------------------------------------------------------------
// Name: InvertSelection
// Object: change the selected state of all items
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::InvertSelection()
{
    int NbItems;
    BOOL bSelectCallback;

    WaitForSingleObject(hevtUnlocked,LIST_VIEW_TIMEOUT);

    // store current selection callback state
    bSelectCallback=this->bSelectionCallBackEnable;
    NbItems=this->GetItemCount();

    // for each item in list
    for (int ItemIndex=0;ItemIndex<NbItems;ItemIndex++)
    {
        // reverse it's state
        this->SetSelectedState(ItemIndex,!this->IsItemSelected(ItemIndex),FALSE);

        // disable callback after first selection
        if (this->bSelectionCallBackEnable)
            this->bSelectionCallBackEnable=FALSE;
    }

    // restore selection callback state
    this->bSelectionCallBackEnable=bSelectCallback;

    SetEvent(this->hevtUnlocked);
}


void CListview::EnableDefaultCustomDraw(BOOL bEnable)
{
    this->DefaultCustomDrawEnabled=bEnable;
}
void CListview::EnablePopUpMenu(BOOL bEnable)
{
    this->PopUpMenuEnabled=bEnable;
}
void CListview::EnableColumnSorting(BOOL bEnable)
{
    this->ColumnSortingEnabled=bEnable;
    if (!bEnable)
        this->RemoveSortingHeaderInfo();
}
void CListview::EnableColumnReOrdering(BOOL bEnable)
{
    DWORD Style=ListView_GetExtendedListViewStyle(this->hWndListView);
    if (bEnable)
        Style |=LVS_EX_HEADERDRAGDROP;
    else
        Style &=~LVS_EX_HEADERDRAGDROP;
    ListView_SetExtendedListViewStyle(this->hWndListView,Style);
}
void CListview::SetDefaultCustomDrawColor(COLORREF ColorRef)
{
    this->DefaultCustomDrawColor=ColorRef;
}

void CListview::SetDefaultMenuText(TCHAR* Clear,TCHAR* CopySelected,TCHAR* CopyAll,TCHAR* SaveSelected,TCHAR* SaveAll)
{
    if (this->MenuIdClear!=(UINT)(-1))
        this->pPopUpMenu->SetText(this->MenuIdClear,Clear);

    this->pPopUpMenu->SetText(this->MenuIdCopySelected,CopySelected);
    this->pPopUpMenu->SetText(this->MenuIdCopyAll,CopyAll);
    this->pPopUpMenu->SetText(this->MenuIdSaveSelected,SaveSelected);
    this->pPopUpMenu->SetText(this->MenuIdSaveAll,SaveAll);
}

void CListview::SetDefaultMenuIcons(HINSTANCE hInstance,int IdIconClear,int IdIconCopySelected,int IdIconCopyAll,int IdIconSaveSelected,int IdIconSaveAll,int Width,int Height)
{
    if (this->MenuIdClear!=(UINT)(-1))
        this->pPopUpMenu->SetIcon(this->MenuIdClear,IdIconClear,hInstance,Width,Height);

    this->pPopUpMenu->SetIcon(this->MenuIdCopySelected,IdIconCopySelected,hInstance,Width,Height);
    this->pPopUpMenu->SetIcon(this->MenuIdCopyAll,IdIconCopyAll,hInstance,Width,Height);
    this->pPopUpMenu->SetIcon(this->MenuIdSaveSelected,IdIconSaveSelected,hInstance,Width,Height);
    this->pPopUpMenu->SetIcon(this->MenuIdSaveAll,IdIconSaveAll,hInstance,Width,Height);
}
void CListview::SetDefaultMenuIcons(HICON IconClear,HICON IconCopySelected,HICON IconCopyAll,HICON IconSaveSelected,HICON IconSaveAll)
{
    if (this->MenuIdClear!=(UINT)(-1))
        this->pPopUpMenu->SetIcon(this->MenuIdClear,IconClear);

    this->pPopUpMenu->SetIcon(this->MenuIdCopySelected,IconCopySelected);
    this->pPopUpMenu->SetIcon(this->MenuIdCopyAll,IconCopyAll);
    this->pPopUpMenu->SetIcon(this->MenuIdSaveSelected,IconSaveSelected);
    this->pPopUpMenu->SetIcon(this->MenuIdSaveAll,IconSaveAll);
}

//-----------------------------------------------------------------------------
// Name: OnNotify
// Object: a WM_NOTIFY message helper for list view and it's associated columns
//         Must be called in the WndProc when receiving a WM_NOTIFY message 
//          if you want to reorder listview by clicking columns
//         EXAMPLE :
//         assuming mpListview is NULL before creation and after deletion, an example can be the following 
//         where mpListview is a CListview*
//              LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//              switch (uMsg)
//              {
//                  case WM_NOTIFY:
//                      if (mpListview)
//                          mpListview->OnNotify(wParam,lParam);
//                      break;
//              }
// Parameters :
//     in  : WPARAM wParam : WndProc wParam
//           LPARAM lParam : WndProc lParam
//     out : 
//     return : TRUE if message have been internally proceed
//-----------------------------------------------------------------------------
BOOL CListview::OnNotify(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    NMHDR* pnmh=((NMHDR*)lParam);

    // listview messages
    if (pnmh->hwndFrom==this->hWndListView)
    {
        switch (pnmh->code) 
        {
        case NM_CUSTOMDRAW:
            if (this->DefaultCustomDrawEnabled)
            {
                // do it in all cases, so if called from WindowProc, we can access result by
                // GetWindowLongPtr(hWndWindow, DWLP_MSGRESULT)
                // if (this->bOwnerControlIsDialog)
                // {
                // must use SetWindowLongPtr to return value from dialog proc
                if (this->DefaultCustomDrawExEnabled)
                    SetWindowLongPtr(this->hWndParent, DWLP_MSGRESULT, this->OnCustomDrawInternalEx(wParam,lParam));
                else
                    SetWindowLongPtr(this->hWndParent, DWLP_MSGRESULT, this->OnCustomDrawInternal(wParam,lParam));
                return TRUE;
                // }
            }
            else
                return FALSE; // mark message as not treated
        case LVN_ITEMCHANGED:
            this->OnItemChangedInternal(wParam,lParam);
            return TRUE;
        case NM_CLICK:
            this->OnClickInternal(wParam,lParam);
            return TRUE;
        case NM_DBLCLK:
            this->OnDoubleClickInternal(wParam,lParam);
            return TRUE;
        case NM_RCLICK:
            this->OnRightClickInternal(wParam,lParam);
            return TRUE;
        case LVN_KEYDOWN:
            {
                LPNMLVKEYDOWN pnkd = (LPNMLVKEYDOWN) lParam;
                BOOL bCtrlKeyDown;
                // get control key state
                bCtrlKeyDown=GetKeyState(VK_CONTROL) & 0x8000;
                switch(pnkd->wVKey)
                {
                case 'C':
                case 'c':
                case VK_INSERT:
                    this->CopyToClipBoard(TRUE,TRUE);
                    return TRUE;
                case 'a':
                case 'A':
                    this->SelectAll(TRUE);
                    return TRUE;
                case VK_APPS:
                    this->ShowPopUpMenu();
                    return TRUE;
                }
            }
            break;
        }
    }
    // listview headers messages
    else if (pnmh->hwndFrom==ListView_GetHeader(this->hWndListView))
    {
        LPNMHEADER pnmheader;
        pnmheader = (LPNMHEADER) lParam;
        switch (pnmh->code) 
        {
        // on header click
        case HDN_ITEMCLICKA:
        case HDN_ITEMCLICKW:
            if (this->ColumnSortingEnabled)
            {
                // sort the clicked column ascending or descending, depending last sort
                this->Sort(pnmheader->iItem);
                return TRUE;
            }
            break;
        //case HDN_ENDDRAG:
        //    OutputDebugString(_T("HDN_ENDDRAG"));
        //    {
        //    LRESULT NbColumns = Header_GetItemCount(pnmh->hwndFrom);
        //    int* lpiArray = new int[NbColumns];
        //    Header_GetOrderArray(pnmh->hwndFrom,NbColumns,lpiArray);
        //    DebugBreak;
        //    }
        //    break;
        case HDN_BEGINTRACKA:
        case HDN_BEGINTRACKW:
            if (pnmheader->pitem->cxy ==0) // avoid tracking for hidden columns
            {
                SetWindowLongPtr(this->hWndParent, DWLP_MSGRESULT, (LONG_PTR)TRUE);// MUST set in case of dialog proc
                return TRUE;// item is hidden return TRUE to prevent tracking
            }
            break;
        case HDN_ENDTRACKA:
        case HDN_ENDTRACKW:
            if (pnmheader->pitem->cxy <5) 
                // avoid 0 value to allow tracking for user manually hidden column
                ::PostMessage(this->hWndListView,LVM_SETCOLUMNWIDTH,pnmheader->iItem,5);
            break;
        }
    }

    return FALSE;
}

void CListview::OnRightClickInternal(WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    BOOL bDisplayPopUpMenu = TRUE;

    if (!this->PopUpMenuEnabled)
        return;

    LPNMITEMACTIVATE pnmitem;
    RECT Rect;
    pnmitem = (LPNMITEMACTIVATE) lParam;

    if (this->pListViewEventsHandler)
    {
        LVHITTESTINFO HitTestInfos={0};
        HitTestInfos.pt = pnmitem->ptAction;
        ListView_SubItemHitTest(this->hWndListView,&HitTestInfos);
        this->pListViewEventsHandler->OnListViewItemRightClick(this,HitTestInfos.iItem,HitTestInfos.iSubItem,((HitTestInfos.flags & LVHT_ONITEMICON)==LVHT_ONITEMICON),&bDisplayPopUpMenu);
    }

    if (bDisplayPopUpMenu)
    {
        // get listview position
        ::GetWindowRect(this->hWndListView,&Rect);
        this->ShowPopUpMenu(Rect.left+pnmitem->ptAction.x,Rect.top+pnmitem->ptAction.y);
    }
}

void CListview::ShowPopUpMenu()
{
    POINT pt;
    ::GetCursorPos(&pt);
    this->ShowPopUpMenu(pt.x,pt.y);
}

void CListview::ShowPopUpMenu(LONG X,LONG Y)
{
    UINT uiRetPopUpMenuItemID;
    // show popupmenu
    uiRetPopUpMenuItemID=this->pPopUpMenu->Show(X,Y,this->hWndListView);
    if (uiRetPopUpMenuItemID==0)// no menu selected
    {
        // do nothing
        return;
    }
    else if (uiRetPopUpMenuItemID==this->MenuIdCopySelected)
        this->CopyToClipBoard(TRUE,TRUE);
    else if (uiRetPopUpMenuItemID==this->MenuIdCopyAll)
        this->CopyToClipBoard(FALSE,TRUE);
    else if (uiRetPopUpMenuItemID==this->MenuIdSaveSelected)
        this->Save(TRUE,TRUE);
    else if (uiRetPopUpMenuItemID==this->MenuIdSaveAll)
        this->Save(FALSE,TRUE);
    else if (uiRetPopUpMenuItemID==this->MenuIdClear)
        this->Clear();
    else if (uiRetPopUpMenuItemID==this->MenuIdCopySelectedAs     )
    { // submenu nothing to do
    }
    else if (uiRetPopUpMenuItemID==this->MenuIdCopySelectedAs_Rtf )
        this->CopyToClipBoard(TRUE,TRUE,StoringTypeRTF);
    else if (uiRetPopUpMenuItemID==this->MenuIdCopySelectedAs_Csv )
        this->CopyToClipBoard(TRUE,TRUE,StoringTypeCSV);
    else if (uiRetPopUpMenuItemID==this->MenuIdCopySelectedAs_Html)
        this->CopyToClipBoard(TRUE,TRUE,StoringTypeHTML);
    else if (uiRetPopUpMenuItemID==this->MenuIdCopySelectedAs_Xml )
        this->CopyToClipBoard(TRUE,TRUE,StoringTypeXML);
    else if (uiRetPopUpMenuItemID==this->MenuIdCopyAllAs     )
    { // submenu nothing to do
    }
    else if (uiRetPopUpMenuItemID==this->MenuIdCopyAllAs_Rtf )
        this->CopyToClipBoard(FALSE,TRUE,StoringTypeRTF);
    else if (uiRetPopUpMenuItemID==this->MenuIdCopyAllAs_Csv )
        this->CopyToClipBoard(FALSE,TRUE,StoringTypeCSV);
    else if (uiRetPopUpMenuItemID==this->MenuIdCopyAllAs_Html)
        this->CopyToClipBoard(FALSE,TRUE,StoringTypeHTML);
    else if (uiRetPopUpMenuItemID==this->MenuIdCopyAllAs_Xml )
        this->CopyToClipBoard(FALSE,TRUE,StoringTypeXML);
    else
    {
        // menu is not processed internally
        if (this->pListViewEventsHandler)
            this->pListViewEventsHandler->OnListViewPopUpMenuClick(this,uiRetPopUpMenuItemID);
    }
}

void CListview::OnClickInternal(WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
    if (this->pListViewEventsHandler)
    {
        LVHITTESTINFO HitTestInfos={0};
        HitTestInfos.pt = lpnmitem->ptAction;
        ListView_SubItemHitTest(this->hWndListView,&HitTestInfos);
        this->pListViewEventsHandler->OnListViewItemClick(this,HitTestInfos.iItem,HitTestInfos.iSubItem,((HitTestInfos.flags & LVHT_ONITEMICON)==LVHT_ONITEMICON));
    }
}

void CListview::OnDoubleClickInternal(WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) lParam;
    if (this->pListViewEventsHandler)
    {
        LVHITTESTINFO HitTestInfos={0};
        HitTestInfos.pt = lpnmitem->ptAction;
        ListView_SubItemHitTest(this->hWndListView,&HitTestInfos);
        this->pListViewEventsHandler->OnListViewItemDoubleClick(this,HitTestInfos.iItem,HitTestInfos.iSubItem,((HitTestInfos.flags & LVHT_ONITEMICON)==LVHT_ONITEMICON));
    }
}

// WARNING : when click again the same item, an OnListViewItemUnselect + OnListViewItemSelect events will be raised
void CListview::OnItemChangedInternal(WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    if (!this->bSelectionCallBackEnable)
        return;

    LPNMLISTVIEW pnmv;
    pnmv = (LPNMLISTVIEW) lParam;
    // if item selection has changed
    if (pnmv->iItem>=0)
    {
        // if new state is selected
        if ((pnmv->uNewState & LVIS_SELECTED)&& (!(pnmv->uOldState & LVIS_SELECTED)))
        {
            // WARNING : when click again the same item, an OnListViewItemUnselect + OnListViewItemSelect events will be raised
            if (this->pListViewEventsHandler)
                this->pListViewEventsHandler->OnListViewItemSelect(this,pnmv->iItem,pnmv->iSubItem);
        }// if unselected
        else if ((pnmv->uOldState & LVIS_SELECTED)&& (!(pnmv->uNewState & LVIS_SELECTED)))
        {
            // WARNING : when click again the same item, an OnListViewItemUnselect + OnListViewItemSelect events will be raised
            if (this->pListViewEventsHandler)
                this->pListViewEventsHandler->OnListViewItemUnselect(this,pnmv->iItem,pnmv->iSubItem);
        }
    }
}

COLORREF CListview::GetForegroundColor(int ItemIndex,int SubitemIndex)
{
    return this->GetForegroundColorBase(ItemIndex,SubitemIndex);
}
COLORREF CListview::GetForegroundColorBase(int ItemIndex,int SubitemIndex)
{
    UNREFERENCED_PARAMETER(ItemIndex);
    UNREFERENCED_PARAMETER(SubitemIndex);
    return RGB(0,0,0); // black color
}

COLORREF CListview::GetBackgroundColor(int ItemIndex,int SubitemIndex)
{
    return this->GetBackgroundColorBase(ItemIndex,SubitemIndex);
}

COLORREF CListview::GetBackgroundColorBase(int ItemIndex,int SubitemIndex)
{
    UNREFERENCED_PARAMETER(SubitemIndex);
    if (ItemIndex%2==0)
        return RGB(255,255,255); // white color
    else
        // set a background color
        return this->DefaultCustomDrawColor;
}

LRESULT CListview::OnCustomDrawInternal(WPARAM wParam,LPARAM lParam)
{
    // WARNING WS_EX_COMPOSITED used in parent control can cause troubles with listview custom drawing !!!

    UNREFERENCED_PARAMETER(wParam);
    if (!this->DefaultCustomDrawEnabled)
        return CDRF_DODEFAULT;

    LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

    switch(lplvcd->nmcd.dwDrawStage) 
    {
    case CDDS_PREPAINT : //Before the paint cycle begins
        //request notifications for individual listview items
        // ::OutputDebugString(TEXT("CListview::OnCustomDrawInternal: CDDS_PREPAINT\r\n"));
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT: //Before an item is drawn
        // lplvcd->nmcd.dwItemSpec // item index
        // to request notification for subitems
        // ::OutputDebugString(TEXT("CListview::OnCustomDrawInternal: CDDS_ITEMPREPAINT\r\n"));
        return CDRF_NOTIFYSUBITEMDRAW;

    // for subitem customization
    case CDDS_SUBITEM | CDDS_ITEMPREPAINT: //Before a subitem is drawn
        // for subitem customization
        // case CDDS_SUBITEM | CDDS_ITEMPREPAINT: //Before a subitem is drawn
        // lplvcd->nmcd.dwItemSpec // item number
        // lplvcd->iSubItem // subitem number
        // // customize subitem appearance for column 0
        // lplvcd->clrText   = RGB(255,255,255);
        // lplvcd->clrTextBk = RGB(255,0,0);
        // //To set a custom font:
        // //SelectObject(lplvcd->nmcd.hdc, <your custom HFONT>);
        // return CDRF_NEWFONT;
        // ::OutputDebugString(TEXT("CListview::OnCustomDrawInternal: CDDS_SUBITEM | CDDS_ITEMPREPAINT\r\n"));
        //customize item appearance
        lplvcd->clrText = this->GetForegroundColor((int)lplvcd->nmcd.dwItemSpec,lplvcd->iSubItem);
        lplvcd->clrTextBk = this->GetBackgroundColor((int)lplvcd->nmcd.dwItemSpec,lplvcd->iSubItem);
        return CDRF_DODEFAULT;
    }
    return CDRF_DODEFAULT;
}


// virtual to be sub classed in case of owner draw content
LRESULT CListview::OnCustomDrawInternalEx(WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

    switch(lplvcd->nmcd.dwDrawStage) 
    {
    case CDDS_PREPAINT: //Before the paint cycle begins
        //request notifications for individual listview items
        return CDRF_NOTIFYITEMDRAW;

    //    return (CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT);
    //case CDDS_POSTPAINT:
    //    return CDRF_DODEFAULT;

    case CDDS_ITEMPREPAINT: //Before an item is drawn
        // lplvcd->nmcd.dwItemSpec // item index
        // to request notification for subitems
        return CDRF_NOTIFYSUBITEMDRAW;

    // for subitem customization
    case CDDS_SUBITEM | CDDS_ITEMPREPAINT: //Before a subitem is drawn
        // lplvcd->nmcd.dwItemSpec // item number
        // lplvcd->iSubItem // subitem number
        // // customize subitem appearance for column 0
        // lplvcd->clrText   = RGB(255,255,255);
        // lplvcd->clrTextBk = RGB(255,0,0);
        // //To set a custom font:
        // //SelectObject(lplvcd->nmcd.hdc, <your custom HFONT>);
        {
            HDC hDC = lplvcd->nmcd.hdc;
            // HWND hWnd = ::WindowFromDC(lplvcd->nmcd.hdc);
            int ItemIndex = (int)lplvcd->nmcd.dwItemSpec;
            int SubItemIndex = lplvcd->iSubItem;

            /*
            if (lplvcd->nmcd.uItemState == 0) // ms bug ? what this state ?
            {

                RECT Rect={0};
                if (memcmp(&lplvcd->nmcd.rc,&Rect,sizeof(RECT))==0)
                {
                    // if lplvcd->nmcd.rc == {0} lplvcd->iSubItem could be wrong ?
# FIX ME ms bug ? may this->GetSubItemRect(ItemIndex, SubItemIndex,&SubItemRect) should be called")
//#ifdef _DEBUG
//                    if (::IsDebuggerPresent())
//                        ::DebugBreak();
//#endif
                    return CDRF_DODEFAULT;
                }
            }
            */
            if (!this->IsColumnVisible(lplvcd->iSubItem))
                return CDRF_SKIPDEFAULT; // nothing to draw

            //customize item appearance
            lplvcd->clrText = this->GetForegroundColor(ItemIndex,SubItemIndex);
            lplvcd->clrTextBk = this->GetBackgroundColor(ItemIndex,SubItemIndex);

            // draw all columns to avoid little rendering differences
            //if (SubItemIndex != this->PropertyColumnIndex) //detect which subitem is being drawn
            //    return CDRF_DODEFAULT;
            
            RECT SubItemRect = { 0 };
            if (!this->GetSubItemRect(ItemIndex, SubItemIndex,&SubItemRect))
                return CDRF_DODEFAULT;
           
            // if item is selected
            if (lplvcd->nmcd.uItemState & CDIS_SELECTED)
            {
                BOOL bSetSelectColor = FALSE;
                DWORD ExStyle = ListView_GetExtendedListViewStyle(this->hWndListView);
                if ( ExStyle & LVS_EX_FULLROWSELECT)
                    bSetSelectColor = TRUE;

                if (bSetSelectColor)
                {
                    LONG_PTR Style = GetWindowLongPtr(this->hWndListView,GWL_STYLE);
                    if (Style & LVS_SHOWSELALWAYS)
                    {
                        // (lplvcd->nmcd.uItemState & CDIS_SELECTED) sucks in this case we have to check it manually
                        // I finally get the answer from the win2k src code :
                        //     "
                        //      // NOTE:  This is a bug.  We should set CDIS_SELECTED only if the item
                        //      // really is selected.  But this bug has existed forever so who knows
                        //      // what apps are relying on it.  Standard workaround is for the client
                        //      // to do a GetItemState and reconfirm the LVIS_SELECTED flag.
                        //      // That's what we do in ListView_DrawImageEx.
                        //      if (plvdi->plv->ci.style & LVS_SHOWSELALWAYS) {
                        //          plvdi->nmcd.nmcd.uItemState |= CDIS_SELECTED;
                        //      }
                        //     "
                        bSetSelectColor = this->IsItemSelected(ItemIndex,TRUE);
                        if (bSetSelectColor)
                        {
                            // assume listview don't have focus (else we will get bad color with multiple selection ctrl/shift or ctrl+a)
                            if (::GetFocus()!=this->hWndListView)
                            {
                                // in show always mode, if we don't have focus, use different selection color
                                if ( !(lplvcd->nmcd.uItemState & CDIS_FOCUS) )
                                {
                                    lplvcd->clrTextBk = ::GetSysColor(COLOR_3DFACE);
                                    bSetSelectColor = FALSE;
                                }
                            }
                        }
                    }
                }
                if (bSetSelectColor)
                {
                    lplvcd->clrTextBk = ::GetSysColor(COLOR_HIGHLIGHT);
                    lplvcd->clrText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
                }

                // if (ExStyle & LVS_EX_BORDERSELECT) to implement
            }
        
            HBRUSH hBrush = ::CreateSolidBrush(lplvcd->clrTextBk);
            ::FillRect(hDC,&SubItemRect,hBrush);
            ::DeleteObject(hBrush);

            BOOL bGetImageIndexFromListView = TRUE;
            int iImage=-1;
            if (bGetImageIndexFromListView)
            {
                // get image index
                LV_ITEM Item={0};
                Item.iItem = ItemIndex;
                Item.iSubItem = SubItemIndex;
                Item.iImage = -1; // seems that LVIF_IMAGE not taken into account if no icon has been set in listview (Item.iImage not modified by ListView_GetItem)
                                  // so provide a default "no image" value
                Item.mask = LVIF_IMAGE;
                if (!ListView_GetItem(this->hWndListView,&Item))
                    return CDRF_DODEFAULT;
                iImage = Item.iImage;
            }

            if (iImage>=0)
            {
                ImageList_Draw( this->hImgListSmall, iImage,hDC,SubItemRect.left,SubItemRect.top,ILD_TRANSPARENT);
                SubItemRect.left+=CListview_CUSTOM_DRAW_EX_IMAGE_SIZE+CListview_CUSTOM_DRAW_EX_IMAGE_SPACER_BEFORE_TEXT;
            }

            // Draw text with matching colors
            ::SetBkColor(hDC,lplvcd->clrTextBk);
            ::SetBkMode(hDC, OPAQUE);
            ::SetTextColor(hDC, lplvcd->clrText);
            if (this->DrawItemText(hDC,&SubItemRect,ItemIndex,SubItemIndex))
                return CDRF_SKIPDEFAULT;
            else
                return CDRF_DODEFAULT;
        }
    }  

    return CDRF_DODEFAULT;
}

BOOL CListview::DrawItemText(HDC hDC,IN RECT* const pSubItemRect,int ItemIndex,int SubItemIndex)
{
    return this->DrawItemTextInternal(hDC,pSubItemRect,ItemIndex,SubItemIndex);
}

BOOL CListview::DrawItemTextInternal(HDC hDC,IN RECT* const pSubItemRect,int ItemIndex,int SubItemIndex)
{
    LONG MaxTextWidth = pSubItemRect->right-pSubItemRect->left;
    if (MaxTextWidth<0)
        return TRUE;

    TCHAR Text[CListview_CUSTOM_DRAW_EX_MAX_SHOWN_CHAR];
    *Text=0;

    // get text content
    // use this->GetItemTextPropertyListview instead of message sending to allow 
    // override of GetItemTextPropertyListview, and so custom text retrieval in derived controls
    //LV_ITEM Item={0};
    //Item.iItem = ItemIndex;
    //Item.iSubItem = SubItemIndex;
    //Item.pszText = Text;
    //Item.cchTextMax = CPropertyListview_MAX_SHOWN_CHAR-1;
    //Item.mask = LVIF_TEXT;
    //if (!ListView_GetItem(this->hWndListView,&Item))
    //    return FALSE;
    this->GetItemText(ItemIndex,SubItemIndex,Text,CListview_CUSTOM_DRAW_EX_MAX_SHOWN_CHAR-1);

    // assume string is terminated
    Text[CListview_CUSTOM_DRAW_EX_MAX_SHOWN_CHAR-1]=0;

    // HFONT OldFont = ::SelectObject(hDC,hNewFont);
    // ::SetBkColor(hDC,TextBkColor);
    // ::SetBkMode(hDC, OPAQUE);
    // ::SetTextColor(hDC, TextColor);
    // ::DrawText(hDC, Text, -1, pSubItemRect, DT_LEFT | DT_VCENTER);
    SIZE TextSize;
    SIZE_T TextLen = _tcslen(Text);
    this->EllipseTextToFitWidth(hDC,Text,&TextLen,MaxTextWidth,&TextSize);
    ::ExtTextOut(hDC,pSubItemRect->left,pSubItemRect->top+(pSubItemRect->bottom - pSubItemRect->top - TextSize.cy) / 2,
                ETO_CLIPPED,pSubItemRect,
                Text,(UINT)TextLen,
                NULL);
    // ::SelectObject(hDC,OldFont);
    return TRUE;
}
//-----------------------------------------------------------------------------
// Name: EllipseTextToFitWidth
// Object: ellipse text to fit a specified width
// Parameters :
//      IN HDC hDC : used device context
//      IN OUT TCHAR* Text : provided buffer must be at least 5 TCHARS
//      IN OUT SIZE_T* pTextLength : size of Text
//      IN LONG MaxTextWidth : maximum required width (subitem rect width)
//      OUT SIZE* pTextSize : size used by text printing
// Return : TRUE if text has been ellipsed
//-----------------------------------------------------------------------------
BOOL CListview::EllipseTextToFitWidth(IN HDC hDC,IN OUT TCHAR* Text,IN OUT SIZE_T* pTextLength,IN LONG MaxTextWidth, OUT SIZE* pTextSize)
{
    BOOL bRetValue=FALSE;
    SIZE TextSize={0};
    SIZE_T TextLen = *pTextLength;
    if (TextLen>0)
    {
        // compute full text required width
        ::GetTextExtentPoint32(hDC, Text, (int)TextLen, &TextSize);

        LONG Margin = (LONG)(TextSize.cx/TextLen); // use 1 char margin (assume at least a single point is visible in case of ellipse)
        if (MaxTextWidth>=Margin)
            MaxTextWidth-=Margin;

        // if the required width is more than the available width
        if (TextSize.cx>MaxTextWidth)
        {
            // compute the overflow
            LONG OverFlowSize = TextSize.cx-MaxTextWidth;

            // get width of char one by one beginning with the end of Text
            // as soon as the sum of width is greater than the overflow
            // we can end the string
            SIZE TextEndSize={0};
            TCHAR* pszEnd;
            LONG CumulatedEndTextLen;
            for (pszEnd = &Text[TextLen-1], CumulatedEndTextLen=0;
                pszEnd>Text;
                pszEnd--
                )
            {
                // compute single char required width
                ::GetTextExtentPoint32(hDC, pszEnd, 1, &TextEndSize);
                CumulatedEndTextLen+=TextEndSize.cx;
                // if sum of chars width is greater than the overflow size, we have removed enough letters
                if (CumulatedEndTextLen >= OverFlowSize)
                {
                    if (pszEnd == Text)
                        *(pszEnd+1)=0; // assume to let at least one letter
                    else
                        *pszEnd=0;
                    break;
                }
            }

            if ( (pszEnd-Text) > 3 )
            {
                // replace 3 last letter by '.'
                _tcscpy(pszEnd-3,_T("..."));
            }
            else
            {
                if (TextLen>1)
                {
                    // assume to let first letter
                    _tcscpy(&Text[1],_T("..."));
                }
                else // in case of char by char drawing disturbing because seems likes there is no change --> better to put single point
                    _tcscpy(Text,_T("..."));
            }
            bRetValue=TRUE;
            TextLen = _tcslen(Text);
            TextSize.cx=0;
            TextSize.cy=0;
            ::GetTextExtentPoint32(hDC, Text, (int)TextLen, &TextSize);
            *pTextLength = TextLen;
        }
    }
    // increment by 1 else part of text can overlap
    TextSize.cx++;

    *pTextSize=TextSize;
    return bRetValue;
}

//-----------------------------------------------------------------------------
// Name: ShowClearPopupMenu
// Object: Show or hide the Clear popupmenu (by default Clear menu is hidden)
// Parameters :
//     in  : BOOL bVisible : TRUE to show it, FLASE to hide it
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::ShowClearPopupMenu(BOOL bVisible)
{
    // assume menu is removed before adding it
    this->pPopUpMenu->Remove(this->MenuIdClear);
    this->MenuIdClear=(UINT)(-1);

    // if we should show menu
    if (bVisible)
        // show it
        this->MenuIdClear=this->pPopUpMenu->Add(_T("Clear"));
}

//-----------------------------------------------------------------------------
// Name: SetWatermarkImage
// Object: set a watermark / background image
//         Thanks to Stephan of TortoiseSVN team http://tortoisesvn.net/blog/3
// Parameters :
//     in  : HBITMAP hBmp
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::SetWatermarkImage(HBITMAP hBmp)
{
    LVBKIMAGE LvBkImage={0};
    LvBkImage.hbm=hBmp;
    LvBkImage.ulFlags= LVBKIF_TYPE_WATERMARK;
    return ListView_SetBkImage(this->hWndListView,&LvBkImage);
}

//-----------------------------------------------------------------------------
// Name: SetSelectionCallbackState
// Object: Allow to disable selections callback for a while and then reenable them
// Parameters :
//     in  : BOOL bEnable : new state. 
//                          TRUE to activate SetUnselectItemCallback and SetSelectItemCallback
//                          FALSE to disable SetUnselectItemCallback and SetSelectItemCallback
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::SetSelectionCallbackState(BOOL bEnable)
{
    this->bSelectionCallBackEnable=bEnable;
}

//-----------------------------------------------------------------------------
// Name: SetColumnIconIndex
// Object: use icon index in column icon list for specified column
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::SetColumnIconIndex(int ColumnIndex,int IconIndexInColumnIconList)
{
    // get header handle
    HWND Header=ListView_GetHeader(this->hWndListView);
    if (!Header)
        return FALSE;

    HDITEM HeaderItem={0};
    HeaderItem.mask= HDI_FORMAT;
    // get current format
    Header_GetItem(Header,ColumnIndex,&HeaderItem);

    HeaderItem.mask=HDI_IMAGE | HDI_FORMAT;
    HeaderItem.iImage=IconIndexInColumnIconList;
    // add image format
    HeaderItem.fmt|=HDF_IMAGE; //add |HDF_BITMAP_ON_RIGHT to display on right
    

    return Header_SetItem(Header,ColumnIndex,&HeaderItem);
}
//-----------------------------------------------------------------------------
// Name: SetItemIconIndex
// Object: use icon index in icon list for specified item
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::SetItemIconIndex(int ItemIndex,int IconIndex)
{
    return this->SetItemIconIndex(ItemIndex,0,IconIndex);
}
//-----------------------------------------------------------------------------
// Name: SetItemIconIndex
// Object: use icon index in icon list for specified item
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::SetItemIconIndex(int ItemIndex,int SubItemIndex,int IconIndex) // listview should have LVS_EX_SUBITEMIMAGES style
{
    LVITEM Item={0};
    Item.mask=LVIF_IMAGE;
    Item.iItem=ItemIndex;
    Item.iImage=IconIndex;
    Item.iSubItem=SubItemIndex;

    if (SubItemIndex>0 && (this->bIconsForSubItemEnabled==FALSE))
        this->EnableIconsForSubItem(TRUE);

    return ListView_SetItem(this->hWndListView,&Item);
}

//-----------------------------------------------------------------------------
// Name: EnableIconsForSubItem
// Object: allow icon for sub items (first column item icon index must be set to -1 to avoid image)
//          Notice : called automatically when SubItemIndex>0 for SetItemIconIndex call
// Parameters :
//     in  : 
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::EnableIconsForSubItem(BOOL bEnable)
{
    DWORD Styles=ListView_GetExtendedListViewStyle(this->hWndListView);
    
    if (bEnable)
        Styles|=LVS_EX_SUBITEMIMAGES;
    else
        Styles&=~LVS_EX_SUBITEMIMAGES;
    ListView_SetExtendedListViewStyle(this->hWndListView,Styles);
    this->bIconsForSubItemEnabled=bEnable;
}

//-----------------------------------------------------------------------------
// Name: AddIcon
// Object: remove an icon from the specified list
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::RemoveIcon(tagImageLists ImageList,int IconIndex)
{
    HIMAGELIST hImgList;
    switch(ImageList)
    {
    case ImageListNormal:
        hImgList=this->hImgListNormal;
        break;
    case ImageListSmall:
        hImgList=this->hImgListSmall;
        break;
    case ImageListState:
        hImgList=this->hImgListState;
        break;
    case ImageListColumns:
        hImgList=this->hImgListColumns;
        break;
    default:
        return FALSE;
    }
    if (!hImgList)
        return FALSE;

    return ImageList_Remove(hImgList,IconIndex);
}
//-----------------------------------------------------------------------------
// Name: RemoveAllIcons
// Object: remove all icons from the specified list
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::RemoveAllIcons(tagImageLists ImageList)
{
    return this->RemoveIcon(ImageList,-1);
}
//-----------------------------------------------------------------------------
// Name: AddIcon
// Object: add an icon to the specified list
// Parameters :
//     in  : 
//     out : 
//     return : index of icon in image list
//-----------------------------------------------------------------------------
int CListview::AddIcon(tagImageLists ImageList,HMODULE hModule,int IdIcon)
{
    HICON hicon=(HICON)LoadImage(hModule, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,0,0,LR_DEFAULTCOLOR|LR_SHARED); 
    if (!hicon)
        return -1;

    return this->AddIcon(ImageList,hModule,IdIcon,0,0);
}
//-----------------------------------------------------------------------------
// Name: AddIcon
// Object: add an icon to the specified list
// Parameters :
//     in  : 
//     out : 
//     return : index of icon in image list
//-----------------------------------------------------------------------------
int CListview::AddIcon(tagImageLists ImageList,HMODULE hModule,int IdIcon,int Width,int Height)
{
    HICON hicon=(HICON)LoadImage(hModule, MAKEINTRESOURCE(IdIcon),IMAGE_ICON,Width,Height,LR_DEFAULTCOLOR|LR_SHARED); 
    if (!hicon)
        return -1;

    return this->AddIcon(ImageList,hicon);
}
//-----------------------------------------------------------------------------
// Name: AddIcon
// Object: add an icon to the specified list
// Parameters :
//     in  : 
//     out : 
//     return : index of icon in image list
//-----------------------------------------------------------------------------
int CListview::AddIcon(tagImageLists ImageList,HICON hIcon)
{
    HIMAGELIST hImgList;
    switch(ImageList)
    {
    case ImageListNormal:
        // if list doesn't exists
        if (!this->hImgListNormal)
        {
            // create it
            this->hImgListNormal=ImageList_Create(32, 32, ILC_MASK|ILC_COLOR32, 20, 5);
            if (!this->hImgListNormal)
                return -1;
            // associate it to corresponding list
            ListView_SetImageList(this->hWndListView,this->hImgListNormal,LVSIL_NORMAL);
        }
        hImgList=this->hImgListNormal;
        break;
    case ImageListSmall:
        // if list doesn't exists
        if (!this->hImgListSmall)
        {
            // create it
            this->hImgListSmall=ImageList_Create(16, 16, ILC_MASK|ILC_COLOR32, 20, 5);
            if (!this->hImgListSmall)
                return -1;
            // associate it to corresponding list
            ListView_SetImageList(this->hWndListView,this->hImgListSmall,LVSIL_SMALL);
        }
        hImgList=this->hImgListSmall;
        break;
    case ImageListState:
        // if list doesn't exists
        if (!this->hImgListState)
        {
            // create it
            this->hImgListState=ImageList_Create(16, 16, ILC_MASK|ILC_COLOR32, 20, 5);
            if (!this->hImgListState)
                return -1;
            // associate it to corresponding list
            ListView_SetImageList(this->hWndListView,this->hImgListState,LVSIL_STATE);
        }
        hImgList=this->hImgListState;
        break;
    case ImageListColumns:
        // if list doesn't exists
        if (!this->hImgListColumns)
        {
            // get header handle
            HWND Header=ListView_GetHeader(this->hWndListView);
            if (!Header)
                return -1;
            // create it
            this->hImgListColumns=ImageList_Create(16, 16, ILC_MASK|ILC_COLOR32, 20, 5);
            if (!this->hImgListColumns)
                return -1;
            // associate it to corresponding list
            Header_SetImageList(Header,this->hImgListColumns);
        }
        hImgList=this->hImgListColumns;
        break;
    default:
        return -1;
    }
    
    // add icon to list
    return ImageList_AddIcon(hImgList, hIcon);
}

BOOL CListview::GetItemRect(int ItemIndex,RECT* pRect)
{
    return ListView_GetItemRect(this->hWndListView, ItemIndex, pRect, LVIR_BOUNDS);
}
BOOL CListview::GetSubItemRect(int ItemIndex,int SubItemIndex,RECT* pRect)
{
    if (pRect==NULL)
        return FALSE;

    if (SubItemIndex == 0)
    {
        // ListView_GetSubItemRect nice m$ bug : returns full line width for sub item 0
        BOOL bRet;

        // get line rect
        bRet = ListView_GetSubItemRect(this->hWndListView, ItemIndex, 0, LVIR_BOUNDS, pRect);

        int NbColumns = this->GetColumnCount();
        if (bRet && (NbColumns>1) )
        {
            int cnt;
            int* ColumnsOrder= new int[NbColumns];
            // try to get column array
            if (!ListView_GetColumnOrderArray(this->hWndListView,NbColumns,ColumnsOrder))
            {
                for (cnt=0;cnt<NbColumns;cnt++)
                {
                    ColumnsOrder[cnt] = cnt;
                }
            }

            RECT TmpRect;
            int SubItem0Position = 0;
            int MaxUnder=pRect->left;
            int MinUpper=pRect->right;
            int ColumnWidth;

            for (cnt=0;cnt<NbColumns;cnt++)
            {
                if (ColumnsOrder[cnt] == 0)
                {
                    SubItem0Position = cnt;
                    break;
                }
            }
                
            for (cnt=0;cnt<NbColumns;cnt++)
            {
                // if under item, adjust pRect->left
                // test all previous rect, because if some headers are hidden, left=size=0
                if ( cnt < SubItem0Position )
                {
                    // why we use get column width : because ListView_GetSubItemRect is fully wrong when item as an icon,
                    //                               the returned rect size is at least the size of the icon, even if the column width is 0...
                    ColumnWidth = this->GetColumnWidth(ColumnsOrder[cnt]);
                    if (ColumnWidth>0)
                    {
                        ListView_GetSubItemRect(this->hWndListView, ItemIndex, ColumnsOrder[cnt], LVIR_BOUNDS, &TmpRect);
                        MaxUnder = __max(MaxUnder,TmpRect.left+ColumnWidth);
                    }
                }

                // if next item, adjust pRect->right
                // test all next rect, because if some headers are hidden, left=size=0
                if ( SubItem0Position < cnt )
                {
                    ListView_GetSubItemRect(this->hWndListView, ItemIndex, ColumnsOrder[cnt], LVIR_BOUNDS, &TmpRect);
                    ColumnWidth = this->GetColumnWidth(ColumnsOrder[cnt]);
                    if (ColumnWidth>0)
                        MinUpper = __min(MinUpper,TmpRect.left);
                }
            }

            pRect->left = MaxUnder;
            pRect->right = MinUpper;

            delete[] ColumnsOrder;
        }

        return bRet;
    }
    else if (SubItemIndex == this->GetColumnCount()-1)
    {
        // nice m$ bug : full line width is returned (only SubItemRect.left is wrong)
        BOOL bRet;
        // get item rect
        bRet = ListView_GetSubItemRect(this->hWndListView, ItemIndex, 0, LVIR_BOUNDS, pRect);
        // hopefully the value returned by ListView_GetColumnWidth is correct
        int ColumnWidth = this->GetColumnWidth(SubItemIndex);
        pRect->left = pRect->right-ColumnWidth;
        return bRet;
    }
    else
        return ListView_GetSubItemRect(this->hWndListView, ItemIndex, SubItemIndex, LVIR_BOUNDS, pRect);
}

void CListview::Invalidate()
{
    if (this->hWndListView)
        ::InvalidateRect(this->hWndListView,NULL,FALSE);
}
void CListview::Redraw()
{
    if (this->hWndListView)
        ::RedrawWindow(this->hWndListView,NULL,NULL,RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW);
}

// enable or disable redraw
void CListview::EnableRedraw(BOOL bEnable)
{
    ::SendMessage(this->hWndListView, WM_SETREDRAW, bEnable, 0);
    if (bEnable && this->hWndListView)
        ::RedrawWindow(this->hWndListView, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);

    this->bRedrawEnabled = bEnable;
}

// enable or disable redraw with counted enabled / disabled (avoid disable redraw query to be override by another disable + enable sequence)
// return the count of disabled times (redraw begin again when returned counter is <=0)
int CListview::EnableRedrawCounted()
{
    this->DisableRedrawCounter--;
    if (this->DisableRedrawCounter==0) // send message only once
        this->EnableRedraw(TRUE);
    return this->DisableRedrawCounter;
}
int CListview::DisableRedrawCounted()
{
    this->DisableRedrawCounter++;
    if (this->DisableRedrawCounter==1) // send message only once
        this->EnableRedraw(FALSE); 
    return this->DisableRedrawCounter;
}

void CListview::EnableDoubleBuffering(BOOL bEnable)
{
    LONG_PTR StyleEx;
    StyleEx = ListView_GetExtendedListViewStyle(this->hWndListView);
    if (bEnable)
        StyleEx|=LVS_EX_DOUBLEBUFFER;
    else
        StyleEx&=~LVS_EX_DOUBLEBUFFER;
    ListView_SetExtendedListViewStyle(this->hWndListView,StyleEx|LVS_EX_DOUBLEBUFFER);
}


LRESULT CALLBACK CListview::SubClassListView(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
    UNREFERENCED_PARAMETER(uIdSubclass);
    CListview* pListView = (CListview*)dwRefData;
    switch(uMsg)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
            pListView->bToolTipInfoLastActionIsMouseAction = FALSE;
            break;
        case WM_MOUSEMOVE:
            {
            #ifndef GET_X_LPARAM
            #define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
            #define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
            #endif 
    
                LONG xPos = GET_X_LPARAM(lParam); 
                LONG yPos = GET_Y_LPARAM(lParam); 
                // don't update mouse info if mouse has not moved
                if ( (pListView->ToolTipInfoLastActionLastMousePoint.x==xPos) && (pListView->ToolTipInfoLastActionLastMousePoint.y==yPos) )
                    break;
                pListView->ToolTipInfoLastActionLastMousePoint.x=xPos;
                pListView->ToolTipInfoLastActionLastMousePoint.y=yPos;
                //TCHAR Msg[64];
                //_sntprintf(Msg,_countof(Msg),_T("Mouse Move x:%d, y%d \r\n"),xPos,yPos);
                //::OutputDebugString(Msg);
                pListView->bToolTipInfoLastActionIsMouseAction = TRUE;
            }
            break;
        case WM_MOUSEWHEEL:
            pListView->bToolTipInfoLastActionIsMouseAction = TRUE;
            break;

        case WM_NOTIFY:
            switch(((LPNMHDR)lParam)->code)
            {
            case TTN_GETDISPINFO:
                {
                    if (pListView->pListViewEventsHandler)
                    {
                        LPNMTTDISPINFO lpnmtdi = (LPNMTTDISPINFO) lParam;
                        LVHITTESTINFO HitTestInfo={0};
                        int Ret;

                        // assume to display nothing if user don't fill infos
                        lpnmtdi->lpszText=_T("");
                        *lpnmtdi->szText=0;
                        lpnmtdi->hinst = NULL;
                        lpnmtdi->uFlags=0;
                        // m$ is happy to query tooltip for key board events and position it at 0,0
                        // so we have to filter these dummy messages
                        if (!pListView->bToolTipInfoLastActionIsMouseAction)
                            return 0;
                        if (!pListView->bRedrawEnabled)
                            return 0;
                        ::GetCursorPos(&HitTestInfo.pt);
                        ::ScreenToClient(pListView->GetControlHandle(),&HitTestInfo.pt);
                        // ListView_HitTestEx : not supported by xp
                        // Ret = ListView_HitTestEx(pListView->GetControlHandle(),&HitTestInfo);
                        // ListView_HitTest doesn't fill the HitTestInfo.iSubItem :'( ... thank you m$
                        Ret = ListView_HitTest(pListView->GetControlHandle(),&HitTestInfo);
                        if ( (Ret ==-1)
                            || ( (HitTestInfo.flags & (LVHT_ONITEMICON | LVHT_ONITEMLABEL | LVHT_ONITEMSTATEICON ) ) == 0 )
                            )
                        {
                            return 0;
                        }
                        // RECT Rect={0};
                        //::GetWindowRect(pListView->hWndToolTip,&Rect);
                        //if ( (Rect.left == 0) && (Rect.top == 0) )
                        //{
                        //    // ::SetWindowPos(pListView->hWndToolTip,NULL,HitTestInfo.pt.x+::GetSystemMetrics(SM_CXCURSOR),HitTestInfo.pt.y,0,0,SWP_NOREDRAW | SWP_NOZORDER | SWP_NOSIZE );
                        //    return 0;
                        //}

                        pListView->pListViewEventsHandler->OnListViewToolTipTextQuery(pListView,HitTestInfo.iItem,pListView->hWndToolTip,lpnmtdi);
                        return 0;
                    }
                }
                break;
            }
        break;
    }
    return ::DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

HWND CListview::GetTooltipControlHandle()
{
    return this->hWndToolTip;
}

BOOL CListview::EnableTooltip(BOOL bEnable)
{
    if (bEnable)
    {
        if (this->hWndToolTip)
            return TRUE;

        // get the tooltip object
        this->hWndToolTip = ListView_GetToolTips(this->hWndListView);

        // multi lines tooltip : if you let it to -1 (auto size), tooltip is adjust on first line size
        // so if next lines are larger than the first one, you will miss some words
        // By putting larger size tooltip will be auto adjusted to larger line
        // TTM_SETMAXTIPWIDTH : width of control, text length can be larger. In that case it will be put on multiple lines
        ::SendMessage(this->hWndToolTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM) 400 );

        // add sub classing
        UINT_PTR uIdSubclass=(UINT_PTR)this;
        DWORD_PTR dwRefData=(DWORD_PTR)this;

        ::SetWindowSubclass(this->hWndListView,CListview::SubClassListView,uIdSubclass,dwRefData);

        // LVN_GETINFOTIP is sent only when on first listview column --> sucks
        //DWORD dwStyle = ListView_GetExtendedListViewStyle(this->hWndListView);
        //ListView_SetExtendedListViewStyle(this->hWndListView,dwStyle|LVS_EX_INFOTIP);
    }
    else
    {
        if (this->hWndToolTip==NULL)
            return TRUE;

        // remove sub classing
        UINT_PTR uIdSubclass=(UINT_PTR)this;
        ::RemoveWindowSubclass(this->hWndListView,CListview::SubClassListView,uIdSubclass);

        //DWORD dwStyle = ListView_GetExtendedListViewStyle(this->hWndListView);
        //ListView_SetExtendedListViewStyle(this->hWndListView,dwStyle & (~LVS_EX_INFOTIP) );

        this->hWndToolTip = NULL;
    }
    return TRUE;
}


//-----------------------------------------------------------------------------
// Name: CopyToClipBoard
// Object: Copy all listview content to clipboard
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::CopyToClipBoard()
{
    return this->CopyToClipBoard(FALSE);
}
//-----------------------------------------------------------------------------
// Name: CopyToClipBoard
// Object: Copy listview content to clipboard
// Parameters :
//     in  : BOOL bOnlySelected : TRUE to copy only selected items
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::CopyToClipBoard(BOOL bOnlySelected)
{
    return this->CopyToClipBoard(bOnlySelected,FALSE);
}

//-----------------------------------------------------------------------------
// Name: CopyToClipBoard
// Object: Copy listview content to clipboard
// Parameters :
//     in  : BOOL bOnlySelected : TRUE to copy only selected items
//           BOOL bOnlyVisibleColumns : TRUE to copy only visible columns
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::CopyToClipBoard(BOOL bOnlySelected,BOOL bOnlyVisibleColumns)
{
    return this->CopyToClipBoard(bOnlySelected,bOnlyVisibleColumns,StoringTypeTXT);
}

//-----------------------------------------------------------------------------
// Name: CopyToClipBoard
// Object: Copy listview content to clipboard
// Parameters :
//     in  : BOOL bOnlySelected : TRUE to copy only selected items
//           BOOL bOnlyVisibleColumns : TRUE to copy only visible columns
//           tagStoringType CopyType : same types are saving type are available
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::CopyToClipBoard(BOOL bOnlySelected,BOOL bOnlyVisibleColumns,tagStoringType CopyType)
{
    return this->CopyToClipBoardEx(bOnlySelected,bOnlyVisibleColumns,CopyType,NULL,0);
}

BOOL CListview::CopyToClipBoard(BOOL bOnlySelected,IN int* ColumnsIndexToSaveArray,IN SIZE_T ColumnsIndexToSaveArraySize,tagStoringType CopyType)
{
    return this->CopyToClipBoardEx(bOnlySelected,FALSE,CopyType,ColumnsIndexToSaveArray,ColumnsIndexToSaveArraySize);
}

BOOL CListview::CopyToClipBoardEx(BOOL bOnlySelected,BOOL bOnlyVisibleColumns,tagStoringType CopyType,IN int* ColumnsIndexToSaveArray,IN SIZE_T ColumnsIndexToSaveArraySize)
{
    COutputString OutputString;
    this->GenericStoring(StoreMode_COPY_TO_CLIP_BOARD,(PVOID)&OutputString,CopyType,bOnlySelected,bOnlyVisibleColumns,ColumnsIndexToSaveArray,ColumnsIndexToSaveArraySize);

    SIZE_T StringSizeIncludingEndChar = OutputString.GetLength()+1;

    if (CopyType==StoringTypeCSV)
    {
        CHAR* pAnsi;
        BOOL bRetValue = FALSE;

        //#ifndef CF_CSV
        //    #define CF_CSV          TEXT("Csv")
        //#endif

        //UINT uiFormat = ::RegisterClipboardFormat(CF_CSV);

        CAnsiUnicodeConvert::TcharToAnsi(OutputString.GetString(),&pAnsi);
        if (pAnsi)
        {
            bRetValue = CClipboard::SetClipboardData(this->hWndListView,CF_TEXT,(PBYTE)pAnsi,strlen(pAnsi)+1);
            //if (uiFormat)
            //    bRetValue = CClipboard::SetClipboardData(this->hWndListView,uiFormat,(PBYTE)pAnsi,strlen(pAnsi)+1);
            free(pAnsi);
        }
        return bRetValue;
    }
    else if (CopyType==StoringTypeRTF)
    {
        #ifndef CF_RTF
                // Clipboard formats - use as parameter to RegisterClipboardFormat() 
                #define CF_RTF 			TEXT("Rich Text Format")
                #define CF_RTFNOOBJS 	TEXT("Rich Text Format Without Objects")
                #define CF_RETEXTOBJ 	TEXT("RichEdit Text and Objects")
        #endif

        UINT uiFormat = ::RegisterClipboardFormat(CF_RTF);
        if (uiFormat == 0)
            return FALSE;

        CHAR* pAnsi;
        BOOL bRetValue = FALSE;
        CAnsiUnicodeConvert::TcharToAnsi(OutputString.GetString(),&pAnsi);
        if (pAnsi)
        {
            // bRetValue = CClipboard::SetClipboardData(this->hWndListView,CF_TEXT,(PBYTE)pAnsi,strlen(pAnsi)+1);
            bRetValue = CClipboard::SetClipboardData(this->hWndListView,uiFormat,(PBYTE)pAnsi,strlen(pAnsi)+1);
            free(pAnsi);
        }

        return bRetValue;
    }
    else
    {
        UINT uiFormat;
#if (defined(UNICODE)||defined(_UNICODE))
        uiFormat = CF_UNICODETEXT;
#else
        uiFormat = CF_TEXT;
#endif
        // copy data to clipboard
        CClipboard::SetClipboardData(this->hWndListView,uiFormat,(PBYTE)OutputString.GetString(),StringSizeIncludingEndChar*sizeof(TCHAR));

        //if (CopyType==StoringTypeHTML)
        //{
        //    #ifndef CF_HTML
        //        #define CF_HTML     TEXT("HTML Format")
        //    #endif
        //    UINT uiFormat = ::RegisterClipboardFormat(CF_HTML);
        //    if (uiFormat)
        //        CClipboard::SetClipboardData(this->hWndListView,uiFormat,(PBYTE)OutputString.GetString(),StringSizeIncludingEndChar*sizeof(TCHAR));
        //}
    }    

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview. This function shows an interface to user
//         for choosing file
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save()
{
    return this->Save(FALSE);
}

//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview. This function shows an interface to user
//         for choosing file
// Parameters :
//     in  : BOOL bOnlySelected : TRUE to save only selected items
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save(BOOL bOnlySelected)
{
    return this->Save(bOnlySelected,FALSE);
}

//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview. This function shows an interface to user
//         for choosing file
// Parameters :
//     in  : BOOL bOnlySelected : TRUE to save only selected items
//           BOOL bOnlyVisibleColumns : TRUE to save only visible columns
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save(BOOL bOnlySelected,BOOL bOnlyVisibleColumns)
{
    return this->Save(bOnlySelected,bOnlyVisibleColumns,_T("xml"));
}

//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview. This function shows an interface to user
//         for choosing file
// Parameters :
//     in  : BOOL bOnlySelected : TRUE to save only selected items
//           BOOL bOnlyVisibleColumns : TRUE to save only visible columns
//           TCHAR* DefaultExtension : default extension (xml, html or txt)
//           pfSpecializedSavingFunction SpecializedSavingFunction : function specialized fo saving
//                 use it only for non standard saving (like saving data that are not in listview)
//           LPVOID UserParam : UserParam passed on each call of SpecializedSavingFunction
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save(BOOL bOnlySelected,BOOL bOnlyVisibleColumns,TCHAR* DefaultExtension)
{

    TCHAR pszFile[MAX_PATH]=_T("");

    // open dialog
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=this->hWndParent;
    ofn.hInstance=NULL;
    ofn.lpstrFilter=_T("xml\0*.xml\0txt\0*.txt\0html\0*.html\0csv\0*.csv\0rtf\0*.rtf\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_NOREADONLYRETURN|OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt=DefaultExtension;
    ofn.lpstrFile=pszFile;
    ofn.nMaxFile=MAX_PATH;

    if (_tcsicmp(DefaultExtension,_T("xml"))==0)
        ofn.nFilterIndex=1;
    else if (_tcsicmp(DefaultExtension,_T("txt"))==0)
        ofn.nFilterIndex=2;
    else if (_tcsicmp(DefaultExtension,_T("html"))==0)
        ofn.nFilterIndex=3;
    else if (_tcsicmp(DefaultExtension,_T("csv"))==0)
        ofn.nFilterIndex=4;
    else // if (_tcsicmp(DefaultExtension,_T("rtf"))==0)
        ofn.nFilterIndex=5;
  
    if (!GetSaveFileNameModelessProtected(&ofn))
        return TRUE;

    return this->Save(ofn.lpstrFile,bOnlySelected,bOnlyVisibleColumns);

}

//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview in the specified file.
//         Save depends of extension name. Save will be done in xml for .xml files
//         else it will be done in text
//         Warning Only saves done in xml can be reloaded
// Parameters :
//     in  : TCHAR* pszFileName : output file (full pathname)
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save(TCHAR* pszFileName)
{
    return this->Save(pszFileName,FALSE);
}
//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview in the specified file.
//         Save depends of extension name. Save will be done in xml for .xml files
//         else it will be done in text
//         Warning Only saves done in xml can be reloaded
// Parameters :
//     in  : TCHAR* pszFileName : output file (full pathname)
//           BOOL bOnlySelected : TRUE to save only selected items
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save(TCHAR* pszFileName,BOOL bOnlySelected)
{
    return this->Save(pszFileName,bOnlySelected,FALSE);
}

//-----------------------------------------------------------------------------
// Name: Save
// Object: Save content of the listview in the specified file.
//         Save depends of extension name. Can be ".xml", ".html", ".csv"
//         else it will be done in text
//         Warning Only saves done in xml can be reloaded
// Parameters :
//     in  : TCHAR* pszFileName : output file (full pathname)
//           BOOL bOnlySelected : TRUE to save only selected items
//           BOOL bOnlyVisibleColumns : TRUE to save only visible columns
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Save(TCHAR* pszFileName,BOOL bOnlySelected,BOOL bOnlyVisibleColumns)
{
    DWORD RetValue;
    HANDLE hFile;

    hFile = NULL;
    RetValue = FALSE;

    if (!pszFileName)
        return FALSE;

    if (!*pszFileName)
        return FALSE;

    tagStoringType StoringType;
    // default saving type
    StoringType=StoringTypeTXT;

    TCHAR* pFileExt = _tcsrchr(pszFileName,'.');
    if (pFileExt)
    {
        // point after '.'
        pFileExt++;

        // check HTML extension
        if (_tcsicmp(_T("html"),pFileExt)==0)
            StoringType=StoringTypeHTML;

        if (_tcsicmp(_T("xml"),pFileExt)==0)
            StoringType=StoringTypeXML;

        if (_tcsicmp(_T("csv"),pFileExt)==0)
            StoringType=StoringTypeCSV;

        if (_tcsicmp(_T("rtf"),pFileExt)==0)
            StoringType=StoringTypeRTF;

        // else storing type has already been defaulted as StoringTypeTXT 
        // --> nothing to do
        //  StoringType=StoringTypeTXT;
    }

    if (!CTextFile::CreateTextFile(pszFileName,&hFile))
    {
        CAPIError::ShowLastError(this->hWndParent);
        goto CleanUp;
    }

    if ( (StoringType == StoringTypeCSV) || (StoringType == StoringTypeRTF) )
    {
        // specific case
        // CSV and RTF don't support unicode
        // --> remove unicode file header if any by going to begin of file
        ::SetFilePointer(hFile,0,NULL,FILE_BEGIN);
    }

    this->GenericStoring(StoreMode_SAVE,(PVOID)hFile,StoringType,bOnlySelected,bOnlyVisibleColumns,NULL,0);

    RetValue = TRUE;
CleanUp:
    if (hFile)
        ::CloseHandle(hFile);
    
    if (RetValue)
    {
        CDialogBase_DisableClosing(this->hWndParent);
        if (::MessageBox(this->hWndListView,_T("Save Successfully Completed\r\nDo you want to open saved document now ?"),_T("Information"),MB_YESNO|MB_ICONINFORMATION)==IDYES)
        {
            if (::ShellExecute(this->hWndListView,_T("open"),pszFileName,NULL,NULL,SW_SHOWNORMAL)<(HINSTANCE)33)
            {
                ::MessageBox(this->hWndListView,_T("Error opening associated viewer"),_T("Error"),MB_OK|MB_ICONERROR);
            }
        }
        CDialogBase_EnableClosing(this->hWndParent);
    }

    return RetValue;
}

void CListview::GenericStoring(StoreMode StorageMode,PVOID StorageData,tagStoringType StoringType,BOOL bOnlySelected,BOOL bOnlyVisibleColumns,IN int* ColumnsIndexToSaveArray,IN SIZE_T ColumnsIndexToSaveArraySize)
{
    ::WaitForSingleObject(this->hevtUnlocked,LIST_VIEW_TIMEOUT);

    int iNbRow;
    int iNbColumn;
    int ItemIndex;
    int SubItemIndex;
    BOOL bSaveCurrentColumn;
    SIZE_T Cnt; 

    iNbRow=this->GetItemCount();
    iNbColumn=this->GetColumnCount();
    if (iNbColumn<0)
        iNbColumn=1;

    BOOL bStoreListviewHeadersNames=FALSE; // store headers only if more than 1 item
    int NbSelectedItems;
    // for each row
    for (ItemIndex=0,NbSelectedItems=0;ItemIndex<iNbRow;ItemIndex++)
    {
        // if we save only selected items
        if (bOnlySelected)
        {
            // if item is not selected
            if (!this->IsItemSelected(ItemIndex,TRUE))
                continue;
        }
        NbSelectedItems++;
        if (NbSelectedItems>1)
        {
            bStoreListviewHeadersNames = TRUE;
            break; // currently we are only interested by affecting bStoreListviewHeadersNames so we stop loop
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // store header
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (StoringType == StoringTypeRTF)
    {
        this->FillStoreRtfColorTable(bOnlySelected,bOnlyVisibleColumns,ColumnsIndexToSaveArray,ColumnsIndexToSaveArraySize);
        if (this->RtfSaveAsTable)
        {
            this->pRtfColumnInfoArray=new RTF_CELL_INFO[this->GetColumnCount()];
            this->FillStoreRtfColumnPercentArray(bOnlyVisibleColumns,ColumnsIndexToSaveArray,ColumnsIndexToSaveArraySize);
        }
    }

    COutputString OutputString;
    this->GetStoreContentDocumentHeader(StoringType,&OutputString);
    this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // store user extra post body begin (copyright / link)
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    OutputString.Clear();
    this->GetStoreContentDocumentPostBodyBegin(StoringType,&OutputString);
    this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // store table begin
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    OutputString.Clear();
    this->GetStoreContentDocumentTableBegin(StoringType,&OutputString);
    this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());

    BOOL bLastColumn;
    int LastColumnIndexToSave = 0;
    for (SubItemIndex=0;SubItemIndex<iNbColumn;SubItemIndex++)
    {
        if (ColumnsIndexToSaveArray)
        {
            bSaveCurrentColumn = FALSE;
            for (Cnt=0;Cnt<ColumnsIndexToSaveArraySize;Cnt++)
            {
                if (ColumnsIndexToSaveArray[Cnt]==SubItemIndex)
                {
                    bSaveCurrentColumn = TRUE;
                    break;
                }
            }
            if (!bSaveCurrentColumn)
                continue;
        }
        else if (bOnlyVisibleColumns)
        {
            if (!this->IsColumnVisible(SubItemIndex))
                continue;
        }
        LastColumnIndexToSave = SubItemIndex;
    }

    if (bStoreListviewHeadersNames)
    {

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // listview columns header saving
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        // row start
        OutputString.Clear();
        this->GetStoreContentTableHeadersRowBegin(StoringType,&OutputString);
        this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());

        // for each column
        for (SubItemIndex=0;SubItemIndex<iNbColumn;SubItemIndex++)
        {
            if (ColumnsIndexToSaveArray)
            {
                bSaveCurrentColumn = FALSE;
                for (Cnt=0;Cnt<ColumnsIndexToSaveArraySize;Cnt++)
                {
                    if (ColumnsIndexToSaveArray[Cnt]==SubItemIndex)
                    {
                        bSaveCurrentColumn = TRUE;
                        break;
                    }
                }
                if (!bSaveCurrentColumn)
                    continue;
            }
            else if (bOnlyVisibleColumns)
            {
                if (!this->IsColumnVisible(SubItemIndex))
                    continue;
            }

            bLastColumn = (SubItemIndex == LastColumnIndexToSave);

            // column start
            OutputString.Clear();
            this->GetStoreContentTableHeadersColumnBegin(SubItemIndex,StoringType,&OutputString);
            this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());

            // column content
            OutputString.Clear();
            this->GetStoreContentTableHeader(SubItemIndex,StoringType,&OutputString);
            this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());

            // column end
            OutputString.Clear();
            this->GetStoreContentTableHeadersColumnEnd(SubItemIndex,bLastColumn,StoringType,&OutputString);
            this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());
        }
        // end of row
        OutputString.Clear();
        this->GetStoreContentTableHeadersRowEnd(StoringType,&OutputString);
        this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // listview items saving
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    int NbSelected = 0;
    int NbSelectedStored = 0;
    if (bOnlySelected)
        NbSelected = this->GetSelectedCount(TRUE);

    BOOL bLastRow = FALSE;

    if ( (!bOnlySelected) || (NbSelected>0) )
    {
        // for each row
        for (ItemIndex=0;ItemIndex<iNbRow;ItemIndex++)
        {
            // if we save only selected items
            if (bOnlySelected)
            {
                // if item is not selected
                if (!this->IsItemSelected(ItemIndex,TRUE))
                    continue;

                bLastRow = (NbSelectedStored+1==NbSelected);
            }
            else
                bLastRow = (ItemIndex+1==iNbRow);

            // row start
            OutputString.Clear();
            this->GetStoreContentItemsRowBegin(ItemIndex,StoringType,&OutputString);
            this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());

            // for each column
            for (SubItemIndex=0;SubItemIndex<iNbColumn;SubItemIndex++)
            {
                if (ColumnsIndexToSaveArray)
                {
                    bSaveCurrentColumn = FALSE;
                    for (Cnt=0;Cnt<ColumnsIndexToSaveArraySize;Cnt++)
                    {
                        if (ColumnsIndexToSaveArray[Cnt]==SubItemIndex)
                        {
                            bSaveCurrentColumn = TRUE;
                            break;
                        }
                    }
                    if (!bSaveCurrentColumn)
                        continue;
                }
                else if (bOnlyVisibleColumns)
                {
                    if (!this->IsColumnVisible(SubItemIndex))
                        continue;
                }

                bLastColumn = (SubItemIndex == LastColumnIndexToSave);

                // column start
                OutputString.Clear();
                this->GetStoreContentItemsColumnBegin(ItemIndex,SubItemIndex,StoringType,&OutputString);
                this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());

                // call saving func
                OutputString.Clear();
                this->GetStoreContentItem(ItemIndex,SubItemIndex,StoringType,&OutputString);
                this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());

                // column end
                OutputString.Clear();
                this->GetStoreContentItemsColumnEnd(ItemIndex,SubItemIndex,bLastColumn,StoringType,&OutputString);
                this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());
            }
            // end of row
            OutputString.Clear();
            this->GetStoreContentItemsRowEnd(ItemIndex,bLastRow,StoringType,&OutputString);
            this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());

            
            if (bOnlySelected)
            {
                NbSelectedStored++;
                if (NbSelectedStored == NbSelected)
                    break;
            }
        }
        // end of listview
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // store table end
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    OutputString.Clear();
    this->GetStoreContentDocumentTableEnd(StoringType,&OutputString);
    this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // store user extra pre body end (copyright / link)
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    OutputString.Clear();
    this->GetStoreContentDocumentPreBodyEnd(StoringType,&OutputString);
    this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());



    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // store footer
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    OutputString.Clear();
    this->GetStoreContentDocumentFooter(StoringType,&OutputString);
    this->StoreItemContentWithoutContentConvertion(StorageMode,StorageData,StoringType,OutputString.GetString(),OutputString.GetLength());


    if (StoringType == StoringTypeRTF)
    {
        if (this->pRtfColumnInfoArray)
        {
            delete[] this->pRtfColumnInfoArray;
            this->pRtfColumnInfoArray=NULL;
        }
    }

    ::SetEvent(this->hevtUnlocked);
}

void CListview::FillStoreRtfColorTable(BOOL bStoreOnlySelectedRow,BOOL bStoreOnlyVisibleColumns,IN int* ColumnsIndexToSaveArray,IN SIZE_T ColumnsIndexToSaveArraySize)
{
    // Notice : vector multi thread access is protected by this->hevtUnlocked of GenericStoring
    # pragma message (__FILE__ " Information : CListview::FillStoreRtfColorTable: Very slow this function should be overrided for rtf saving better performance")

    int iNbRow;
    int iNbColumn;
    COLORREF Color;
    BOOL bSaveCurrentColumn;
    SIZE_T Cnt;

    // reset array content
    this->RtfColorList.clear();

    iNbRow=this->GetItemCount();
    iNbColumn=this->GetColumnCount();
    if (iNbColumn<0)
        iNbColumn=1;

    // for each row
    for (int ItemIndex=0;ItemIndex<iNbRow;ItemIndex++)
    {
        // if we save only selected items
        if (bStoreOnlySelectedRow)
        {
            // if item is not selected
            if (!this->IsItemSelected(ItemIndex,TRUE))
                continue;
        }

        // for each column
        for (int SubItemIndex=0;SubItemIndex<iNbColumn;SubItemIndex++)
        {
            if (ColumnsIndexToSaveArray)
            {
                bSaveCurrentColumn = FALSE;
                for (Cnt=0;Cnt<ColumnsIndexToSaveArraySize;Cnt++)
                {
                    if (ColumnsIndexToSaveArray[Cnt]==SubItemIndex)
                    {
                        bSaveCurrentColumn = TRUE;
                        break;
                    }
                }
                if (!bSaveCurrentColumn)
                    continue;
            }
            else if (bStoreOnlyVisibleColumns)
            {
                if (!this->IsColumnVisible(SubItemIndex))
                    continue;
            }

            Color = this->GetBackgroundColor(ItemIndex,SubItemIndex);
            this->AddStoreRtfColorIndexIfNeeded(Color);

            Color = this->GetForegroundColor(ItemIndex,SubItemIndex);
            this->AddStoreRtfColorIndexIfNeeded(Color);
        }
    }
}

// AddStoreRtfColorIndexIfNeeded return color index for color
UINT CListview::AddStoreRtfColorIndexIfNeeded(COLORREF Color)
{
    // Notice : vector multi thread access is protected by this->hevtUnlocked of GenericStoring
    UINT Index = this->GetStoreRtfColorIndex(Color);
    if (Index!=(UINT)-1)
        return Index;

    this->RtfColorList.push_back(Color);
    return (UINT)(this->RtfColorList.size()-1); // return 0 based index
}

// return the color table 0 based index, (UINT)-1 on error
UINT CListview::GetStoreRtfColorIndex(COLORREF Color)
{
    // Notice : vector multi thread access is protected by this->hevtUnlocked of GenericStoring
    std::vector<COLORREF>::iterator Iter;
    UINT Index;
    for (Iter=this->RtfColorList.begin(),Index=0;
         Iter!=this->RtfColorList.end();
         ++Iter,Index++
         )
    {
        if (*Iter == Color)
            return Index;
    }
    return (UINT)-1;
}


// IN OUT COutputString* pOutputString : string inside which header content will be append
void CListview::GetStoreContentDocumentHeader(IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    #define CListview_HTML_HEADER \
_T("<HTML><BODY>\r\n")\
_T("<style type=\"text/css\"><!--table, th, td {border: 1px solid black;} table {margin: 0px; padding: 0px; border-spacing: 0px; border-collapse: collapse;}--></style>\r\n")

#if (defined(UNICODE)||defined(_UNICODE))
    #define STRINGIFY(x) L#x
#else
    #define STRINGIFY(x) #x
#endif
    #define TOSTRING(x) STRINGIFY(x)

    // use default value because some editors (like word pad at least 6.1) don't care about width and size
    // only top and bottom margin won't affect layout, so we can put them to 0
    #define RTF_PAPER_WIDTH 12240
    #define RTF_PAPER_WIDTH_STRING TOSTRING(RTF_PAPER_WIDTH)
    #define RTF_MARGIN_LEFT 1800
    #define RTF_MARGIN_LEFT_STRING TOSTRING(RTF_MARGIN_LEFT)
    #define RTF_MARGIN_RIGHT 1800
    #define RTF_MARGIN_RIGHT_STRING TOSTRING(RTF_MARGIN_RIGHT)

    #define RTF_PAPER_HEIGTH_STRING _T("15840")
    #define RTF_PAPER_LANDSCAPE _T("") // _T("\\landscape") // empty for portrait, landscape option not taken into account by some readers
    #define CListview_RTF_HEADER_PART1     _T("{\\rtf1\\ansi\\paperw") RTF_PAPER_WIDTH_STRING _T("\\paperh") RTF_PAPER_HEIGTH_STRING _T("\\margl") RTF_MARGIN_LEFT_STRING _T("\\margr") RTF_MARGIN_RIGHT_STRING _T("\\margt0\\margb0") RTF_PAPER_LANDSCAPE _T("{\\fonttbl{\\f0\\fmodern\\fcharset0\\fprq1 Courier New;}}")
    #define CListview_RTF_HEADER_PART2     _T("\\f0\\fs20")


    switch (StoringType)
    {
    case StoringTypeXML:
        pOutputString->Append(_T("<LISTVIEW>"),DefineTcharStringLength(_T("<LISTVIEW>")));
        break;
    case StoringTypeHTML:
        pOutputString->Append(CListview_HTML_HEADER,DefineTcharStringLength(CListview_HTML_HEADER));
        break;
    case StoringTypeRTF:
        {
            COLORREF Color;

            pOutputString->Append(CListview_RTF_HEADER_PART1,DefineTcharStringLength(CListview_RTF_HEADER_PART1));        
            pOutputString->Append(_T("{\\colortbl"));

            std::vector<COLORREF>::iterator Iter;
            for (Iter=this->RtfColorList.begin();Iter!=this->RtfColorList.end();++Iter)
            {
                Color=*Iter;
                pOutputString->AppendSnprintf(64,_T("\\red%u\\green%u\\blue%u;"),GetRValue(Color),GetGValue(Color),GetBValue(Color));
            }

            pOutputString->Append(_T("}"));
            pOutputString->Append(CListview_RTF_HEADER_PART2,DefineTcharStringLength(CListview_RTF_HEADER_PART2));
        }
        break;
    }    
}
// IN OUT COutputString* pOutputString : string inside which header content will be append
void CListview::GetStoreContentDocumentFooter(IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    switch (StoringType)
    {
    case StoringTypeXML:
        pOutputString->Append(_T("</LISTVIEW>"),DefineTcharStringLength(_T("</LISTVIEW>")));
        break;
    case StoringTypeHTML:
        pOutputString->Append(_T("</BODY></HTML>"),DefineTcharStringLength(_T("</BODY></HTML>")));
        break;
    case StoringTypeRTF:
        pOutputString->Append(_T("}"),DefineTcharStringLength(_T("}")));
        break;
    }
}

void CListview::GetStoreContentDocumentTableBegin(IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    switch (StoringType)
    {
    case StoringTypeHTML:
        pOutputString->Append(_T("<TABLE align=\"center\">\r\n"),DefineTcharStringLength(_T("<TABLE align=\"center\">\r\n")));
        break;
    }
}
void CListview::GetStoreContentDocumentTableEnd(IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    switch (StoringType)
    {
    case StoringTypeHTML:
        pOutputString->Append(_T("</TABLE>"),DefineTcharStringLength(_T("</TABLE>")));
        break;
    }
}
// only here for derivation (specific software copyright or link)
void CListview::GetStoreContentDocumentPostBodyBegin(IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    UNREFERENCED_PARAMETER(StoringType);
    UNREFERENCED_PARAMETER(pOutputString);
}
// only here for derivation (specific software copyright or link)
void CListview::GetStoreContentDocumentPreBodyEnd(IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    UNREFERENCED_PARAMETER(StoringType);
    UNREFERENCED_PARAMETER(pOutputString);
}


void CListview::FillStoreRtfColumnPercentArray(BOOL bStoreOnlyVisibleColumns,IN int* ColumnsIndexToSaveArray,IN SIZE_T ColumnsIndexToSaveArraySize)
{
    BOOL bSaveCurrentColumn;
    int NbColumns = this->GetColumnCount();
    int FullWidth;
    int CurrentCellWidth;
    int CurrentCellOrigin;
    int SubItemIndex;
    SIZE_T CntArray;
    // first loop to get full width
    for (SubItemIndex=0,FullWidth=0;
         SubItemIndex<NbColumns;
         SubItemIndex++
         )
    {
        if (ColumnsIndexToSaveArray)
        {
            bSaveCurrentColumn = FALSE;
            for (CntArray=0;CntArray<ColumnsIndexToSaveArraySize;CntArray++)
            {
                if (ColumnsIndexToSaveArray[CntArray]==SubItemIndex)
                {
                    bSaveCurrentColumn = TRUE;
                    break;
                }
            }
            if (!bSaveCurrentColumn)
                continue;
        }
        else if (bStoreOnlyVisibleColumns)
        {
            if (!this->IsColumnVisible(SubItemIndex))
                continue;
        }
        FullWidth+= this->GetColumnWidth(SubItemIndex);
    }

    // 2nd loop to get percent
    for (SubItemIndex=0,CurrentCellOrigin=0;SubItemIndex<NbColumns;SubItemIndex++)
    {
        
        if (ColumnsIndexToSaveArray)
        {
            bSaveCurrentColumn = FALSE;
            for (CntArray=0;CntArray<ColumnsIndexToSaveArraySize;CntArray++)
            {
                if (ColumnsIndexToSaveArray[CntArray]==SubItemIndex)
                {
                    bSaveCurrentColumn = TRUE;
                    break;
                }
            }
            if (!bSaveCurrentColumn)
            {
                this->pRtfColumnInfoArray[SubItemIndex].bIsVisible = FALSE;
                *this->pRtfColumnInfoArray[SubItemIndex].CellWidth = 0;
                continue;
            }
        }
        else if (bStoreOnlyVisibleColumns)
        {
            if (!this->IsColumnVisible(SubItemIndex))
            {
                this->pRtfColumnInfoArray[SubItemIndex].bIsVisible = FALSE;
                *this->pRtfColumnInfoArray[SubItemIndex].CellWidth = 0;
                continue;
            }
        }

        this->pRtfColumnInfoArray[SubItemIndex].bIsVisible = TRUE;
        // compute percent of width translated to rtf page size
        CurrentCellWidth = ( (RTF_PAPER_WIDTH-RTF_MARGIN_LEFT-RTF_MARGIN_RIGHT)*this->GetColumnWidth(SubItemIndex))/FullWidth;
        // compute and of current cell (aka start of the next one)
        CurrentCellOrigin+=CurrentCellWidth;
        // add cell information into table
        _sntprintf(this->pRtfColumnInfoArray[SubItemIndex].CellWidth,CLISTVIEW_RTF_CELL_WIDTH_STRING_LENGTH,_T("\\cellx%d"),CurrentCellOrigin);
    }
}

void CListview::GetStoreContentTableHeadersRowBegin(IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    switch (StoringType)
    {
    case StoringTypeXML:
        pOutputString->Append(_T("<HROW>"),DefineTcharStringLength(_T("<HROW>")));
        break;
    case StoringTypeHTML:
        pOutputString->Append(_T("<TR>"),DefineTcharStringLength(_T("<TR>")));
        break;
    case StoringTypeRTF:
        if (this->RtfSaveAsTable)
        {
            pOutputString->Append( _T("\\trowd"),DefineTcharStringLength(_T("\\trowd")));

            // for each column
            int NbColumns = this->GetColumnCount();
            int SubItemIndex;
            for (SubItemIndex=0;SubItemIndex<NbColumns;SubItemIndex++)
            {
                if (this->pRtfColumnInfoArray[SubItemIndex].bIsVisible)
                    pOutputString->Append(this->pRtfColumnInfoArray[SubItemIndex].CellWidth);
            }

            pOutputString->Append( _T("\\pard\\intbl "),DefineTcharStringLength(_T("\\pard\\intbl ")));
        }
        break;
    }
}

void CListview::GetStoreContentTableHeadersRowEnd(IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    switch (StoringType)
    {
    case StoringTypeXML:
        pOutputString->Append(_T("</HROW>"),DefineTcharStringLength(_T("</HROW>")));
        break;
    case StoringTypeHTML:
        pOutputString->Append(_T("</TR>\r\n"),DefineTcharStringLength(_T("</TR>\r\n")));
        break;
    case StoringTypeCSV:
    case StoringTypeTXT:
        pOutputString->Append(_T("\r\n"),DefineTcharStringLength(_T("\r\n")));
        break;
    case StoringTypeRTF:
        if (this->RtfSaveAsTable)
            pOutputString->Append(_T("\\row "),DefineTcharStringLength(_T("\\row ")));
        else
            pOutputString->Append(_T("\\par"),DefineTcharStringLength(_T("\\par")));
        break;
    }
}
void CListview::GetStoreContentTableHeadersColumnBegin(IN int SubItemIndex,IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    UNREFERENCED_PARAMETER(SubItemIndex);
    switch (StoringType)
    {
    case StoringTypeXML:
        pOutputString->Append(_T("<HCOLUMN>"),DefineTcharStringLength(_T("<HCOLUMN>")));
        break;
    case StoringTypeHTML:
        pOutputString->Append(_T("<TH style=\"background-color:#F1F1F1;\">"),DefineTcharStringLength(_T("<TH style=\"background-color:#F1F1F1;\">")));
        break;
    case StoringTypeCSV:
        pOutputString->Append(_T("\""),DefineTcharStringLength(_T("\"")));
        break;
    }
}

void CListview::GetStoreContentTableHeadersColumnEnd(IN int SubItemIndex,IN BOOL bLastColumn,IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    UNREFERENCED_PARAMETER(SubItemIndex);
    switch (StoringType)
    {
    case StoringTypeXML:
        pOutputString->Append(_T("</HCOLUMN>"),DefineTcharStringLength(_T("</HCOLUMN>")));
        break;
    case StoringTypeHTML:
        pOutputString->Append(_T("</TH>"),DefineTcharStringLength(_T("</TH>")));
        break;
    case StoringTypeCSV:
        pOutputString->Append(_T("\""),DefineTcharStringLength(_T("\"")));
        if (!bLastColumn)
            pOutputString->Append(_T(";"),DefineTcharStringLength(_T(";")));
        break;
    case StoringTypeTXT:
        if (!bLastColumn)
            pOutputString->Append(_T("\t"),DefineTcharStringLength(_T("\t")));
        break;
    case StoringTypeRTF:
        if (this->RtfSaveAsTable)
        {
            // \\cell End of table cell
            pOutputString->Append(_T("\\cell "),DefineTcharStringLength(_T("\\cell ")));
        }
        else
        {
            if (!bLastColumn)
            {
                // \\tab Tab character
                pOutputString->Append(_T("\\tab "),DefineTcharStringLength(_T("\\tab ")));
            }
        }
        break;
    }
}

void CListview::GetStoreContentTableHeader(IN int SubItemIndex,IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    #define MAX_COLUMN_NAME 256
    TCHAR ColumnName[MAX_COLUMN_NAME];
    *ColumnName=0;
    this->GetColumnName(SubItemIndex,ColumnName,_countof(ColumnName)-1);
    ColumnName[_countof(ColumnName)-1]=0;
    this->AddItemContentToOutputString(pOutputString,StoringType,ColumnName,_tcslen(ColumnName),TRUE);
}

void CListview::GetStoreContentItemsRowBegin(IN int ItemIndex,IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    UNREFERENCED_PARAMETER(ItemIndex);
    switch (StoringType)
    {
    case StoringTypeXML:
        pOutputString->Append(_T("<ROW>"),DefineTcharStringLength(_T("<ROW>")));
        break;
    case StoringTypeHTML:
        pOutputString->Append(_T("<TR>"),DefineTcharStringLength(_T("<TR>")));
        break;
    case StoringTypeRTF:
        if (this->RtfSaveAsTable)
        {
            COLORREF Color;
            UINT ColorTableIndex;

            // The table row begins with the \trowd control word and ends with the \row control word
            pOutputString->Append( _T("\\trowd"),DefineTcharStringLength(_T("\\trowd")));

            // for each column
            int NbColumns = this->GetColumnCount();
            int SubItemIndex;
            for (SubItemIndex=0;SubItemIndex<NbColumns;SubItemIndex++)
            {
                if (this->pRtfColumnInfoArray[SubItemIndex].bIsVisible)
                {
                    Color = this->GetBackgroundColor(ItemIndex,SubItemIndex);
                    ColorTableIndex = this->GetStoreRtfColorIndex(Color);
                    pOutputString->AppendSnprintf(64,_T("\\clcbpat%u "),ColorTableIndex);
                    pOutputString->Append(this->pRtfColumnInfoArray[SubItemIndex].CellWidth);
                }
            }
            // Every paragraph that is contained in a table row must have the \intbl control word specified or inherited from the previous paragraph
            // \\pard Resets to default paragraph properties
            // \\intbl Paragraph is part of a table
            pOutputString->Append( _T("\\pard\\intbl "),DefineTcharStringLength(_T("\\pard\\intbl ")));
        }
        else
        {
            // \\pard Resets to default paragraph properties
            pOutputString->Append( _T("\\pard "),DefineTcharStringLength(_T("\\pard ")));
        }
        break;
    }
}
void CListview::GetStoreContentItemsRowEnd(IN int ItemIndex,IN BOOL bLastRow,IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    UNREFERENCED_PARAMETER(ItemIndex);
    switch (StoringType)
    {
    case StoringTypeXML:
        pOutputString->Append(_T("</ROW>"),DefineTcharStringLength(_T("</ROW>")));
        break;
    case StoringTypeHTML:
        pOutputString->Append(_T("</TR>\r\n"),DefineTcharStringLength(_T("</TR>\r\n")));
        break;
    case StoringTypeRTF:
        if (this->RtfSaveAsTable)
        {
            // \\row End of table row
            pOutputString->Append(_T("\\row "),DefineTcharStringLength(_T("\\row ")));
        }
        else
        {
            if (!bLastRow)
            {
                // \\par New paragraph
                pOutputString->Append(_T("\\par "),DefineTcharStringLength(_T("\\par ")));
            }
        }
        break;
    case StoringTypeCSV:
    case StoringTypeTXT:
        if (!bLastRow)
            pOutputString->Append(_T("\r\n"),DefineTcharStringLength(_T("\r\n")));
        break;
    }
}
void CListview::GetStoreContentItemsColumnBegin(IN int ItemIndex,IN int SubItemIndex,IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    switch (StoringType)
    {
    case StoringTypeXML:
        pOutputString->Append(_T("<COLUMN>"),DefineTcharStringLength(_T("<COLUMN>")));
        break;
    case StoringTypeHTML:
        {
            COLORREF BackGroundColor;
            COLORREF ForeGroundColor;
            BackGroundColor = this->GetBackgroundColor(ItemIndex,SubItemIndex);
            ForeGroundColor = this->GetForegroundColor(ItemIndex,SubItemIndex);
            if ( BackGroundColor==RGB(255,255,255) && ForeGroundColor==RGB(0,0,0) )
                pOutputString->Append(_T("<TD>"),DefineTcharStringLength(_T("<TD>")));
            else
            {
                pOutputString->Append(_T("<TD style=\""),DefineTcharStringLength(_T("<TD style=\"")));
                DWORD dwR;
                DWORD dwG;
                DWORD dwB;
                if ( BackGroundColor!=RGB(255,255,255) )
                {
                    dwR = GetRValue(BackGroundColor);
                    dwG = GetGValue(BackGroundColor);
                    dwB = GetBValue(BackGroundColor);
                    pOutputString->AppendSnprintf(64,_T("background-color:#%02X%02X%02X;"),dwR, dwG, dwB);
                }
                if ( ForeGroundColor!=RGB(0,0,0) )
                {
                    dwR = GetRValue(ForeGroundColor);
                    dwG = GetGValue(ForeGroundColor);
                    dwB = GetBValue(ForeGroundColor);
                    pOutputString->AppendSnprintf(64,_T("color:#%02X%02X%02X"),dwR, dwG, dwB);
                }
                pOutputString->Append(_T("\">"),DefineTcharStringLength(_T("\">")));
            }
        }
        break;
    case StoringTypeCSV:
        pOutputString->Append(_T("\""),DefineTcharStringLength(_T("\"")));
        break;
    case StoringTypeRTF:
        {
            COLORREF Color;
            UINT ColorTableIndex;

            if (!this->RtfSaveAsTable)
            {
                Color = this->GetBackgroundColor(ItemIndex,SubItemIndex);
                ColorTableIndex = this->GetStoreRtfColorIndex(Color);
                pOutputString->AppendSnprintf(64,_T("\\highlight%u "),ColorTableIndex);
            }

            Color = this->GetForegroundColor(ItemIndex,SubItemIndex);
            ColorTableIndex = this->GetStoreRtfColorIndex(Color);
            pOutputString->AppendSnprintf(64,_T("\\cf%u "),ColorTableIndex);
        }
        break;
    }
}
void CListview::GetStoreContentItemsColumnEnd(IN int ItemIndex,IN int SubItemIndex,IN BOOL bLastColumn,IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    UNREFERENCED_PARAMETER(ItemIndex);
    UNREFERENCED_PARAMETER(SubItemIndex);
    switch (StoringType)
    {
    case StoringTypeXML:
        pOutputString->Append(_T("</COLUMN>"),DefineTcharStringLength(_T("</COLUMN>")));
        break;
    case StoringTypeHTML:
        pOutputString->Append(_T("</TD>"),DefineTcharStringLength(_T("</TD>")));
        break;
    case StoringTypeCSV:
        pOutputString->Append(_T("\""),DefineTcharStringLength(_T("\"")));
        if (!bLastColumn)
            pOutputString->Append(_T(";"),DefineTcharStringLength(_T(";")));
        break;
    case StoringTypeTXT:
        if (!bLastColumn)
            pOutputString->Append(_T("\t"),DefineTcharStringLength(_T("\t")));
        break;
    case StoringTypeRTF:
        if (this->RtfSaveAsTable)
        {
            // \\cell End of table cell
            pOutputString->Append(_T("\\cell "),DefineTcharStringLength(_T("\\cell ")));
        }
        else
        {
            if (!bLastColumn)
            {
                // \\tab Tab character
                pOutputString->Append(_T("\\tab "),DefineTcharStringLength(_T("\\tab ")));
            }
        }
        break;
    }
}

//-----------------------------------------------------------------------------
// Name: SpecializedSavingFunction
// Object: generic item saving function in case no user SpecializedSavingFunction was provided
// Parameters :
//     in  : HANDLE SavingHandle : handle provided by Save function
//           int ItemIndex : item index of item to be saved
//           int SubItemIndex : subitem index of item to be saved
//     out : 
//     return : 
//-----------------------------------------------------------------------------
void CListview::GetStoreContentItem(IN int ItemIndex,IN int SubItemIndex,IN tagStoringType StoringType,IN OUT COutputString* pOutputString)
{
    UNREFERENCED_PARAMETER(StoringType);
    TCHAR* pszItemText;
    DWORD ItemTextSize=4096;// try to minimize the number of loops and reallocation
    DWORD ItemTextLen;
    pszItemText=new TCHAR[ItemTextSize];
    ItemTextLen=this->GetItemText(ItemIndex,SubItemIndex,pszItemText,ItemTextSize);

    // do a loop increasing size
    // as soon as size of text copied to lvi.pszText is smaller than buffer size
    // we got the real size
    while ((ItemTextLen==ItemTextSize)||(ItemTextLen==ItemTextSize-1))
    {
        delete[] pszItemText;
        ItemTextSize*=2;
        pszItemText=new TCHAR[ItemTextSize];
        if (!pszItemText)
            return;

        ItemTextLen=this->GetItemText(ItemIndex,SubItemIndex,pszItemText,ItemTextSize);
    }
    this->AddItemContentToOutputString(pOutputString,StoringType,pszItemText,ItemTextLen,TRUE);
    delete[] pszItemText;
}

void CListview::GetItemTextAsOutputString(IN int ItemIndex,IN int SubItemIndex,IN OUT COutputString* pOutputString)
{
    DWORD ItemTextLen;
    DWORD ItemTextSize=(DWORD)pOutputString->GetMaxSize();
    
    pOutputString->Clear();
    ItemTextLen=this->GetItemText(ItemIndex,SubItemIndex,pOutputString->GetString(),(int)pOutputString->GetMaxSize());

    // do a loop increasing size
    // as soon as size of text copied to lvi.pszText is smaller than buffer size
    // we got the real size
    while ((ItemTextLen==ItemTextSize)||(ItemTextLen==ItemTextSize-1))
    {
        ItemTextSize*=2;
        if (ItemTextSize<4096)
            ItemTextSize = 4096;
        pOutputString->Clear(); // clear before increase capacity to avoid memcpy
        pOutputString->IncreaseCapacity(ItemTextSize);
        ItemTextLen=this->GetItemText(ItemIndex,SubItemIndex,pOutputString->GetString(),(int)pOutputString->GetMaxSize());
    }
    pOutputString->UpdateAfterExternalContentChange();
}

//-----------------------------------------------------------------------------
// Name: StoreItemContent
// Object: used to store (write or clipboard buffer filling) content of a SpecializedSavingFunction, convert Content
//         in xml, html, csv or txt according to user saving type choice
//         just use it as a WriteFile
// Parameters :
//     StoreMode StorageMode : saving or copying to clipboard mode
//     HANDLE FileHandle : file handle
//     tagStoringType FileType : file type (txt, xml, html, ...)
//     TCHAR* Content : content to write to file
// return : 
//-----------------------------------------------------------------------------
void CListview::StoreItemContentWithoutContentConvertion(StoreMode StorageMode,PVOID StoreData,tagStoringType FileType,TCHAR* Content,SIZE_T ContentLength)
{
    if (StorageMode == StoreMode_COPY_TO_CLIP_BOARD)
        this->AddItemContentToOutputString((COutputString*)StoreData,FileType,Content,ContentLength,FALSE);
    else // if (StorageMode == StoreMode_SAVE)
        this->ItemContentSave((HANDLE)StoreData,FileType,Content,FALSE);
}

void CListview::AddItemContentToOutputString(COutputString* pOutputString,tagStoringType FileType,TCHAR* pszContent,BOOL bConvertContent)
{
    this->AddItemContentToOutputString(pOutputString,FileType,pszContent,_tcslen(pszContent),bConvertContent);
}

void CListview::AddItemContentToOutputString(COutputString* pOutputString,tagStoringType FileType,TCHAR* pszContent,SIZE_T ContentLength,BOOL bConvertContent)
{
    if (IsBadReadPtr(pszContent,sizeof(TCHAR*)))
        return;
    
    if (!bConvertContent)
    {
        pOutputString->Append(pszContent);
        return;
    }

    switch (FileType)
    {
        // in case of XML and HTML saving, replace disturbing chars
    case StoringTypeXML:
        this->ConvertHTML_XML_MarkupsAndAddToOutputString(pOutputString,pszContent,ContentLength);
        break;
    case StoringTypeHTML:
        {
            // avoid empty cells on table some browsers remove them
            if (*pszContent==0)// only '\0' (TextLen is len with '\0')
                pOutputString->Append(_T("&nbsp;"),DefineTcharStringLength(_T("&nbsp;")));
            else
                this->ConvertHTML_XML_MarkupsAndAddToOutputString(pOutputString,pszContent,ContentLength);
        }
        break;
    case StoringTypeCSV:
        // if not empty string
        if (*pszContent)
        {
            TCHAR* pc;
            TCHAR* pcEnd;
            TCHAR c;
            // replace all strings defined in ppszReplacedChar
            for (pc=pszContent,pcEnd = pc + ContentLength;
                 pc<pcEnd;
                 pc++)
            {
                c=*pc;
                if (c==0)
                    break;

                // replace " by ""
                if (c=='"')
                    pOutputString->Append( _T("\"\""),DefineTcharStringLength(_T("\"\"")) );
                else
                    pOutputString->Append(c);
            }
        }
        break;
    case StoringTypeRTF:
        // if not empty string
        if (*pszContent)
        {
            TCHAR* pc;
            TCHAR* pcEnd;
            TCHAR c;
            // replace all strings defined in ppszReplacedChar
            for (pc=pszContent,pcEnd = pc + ContentLength;
                pc<pcEnd;
                pc++)
            {
                c=*pc;
                if (c==0)
                    break;

                if( (c == '\\') || (c == '{') || (c == '}'))
                {
                    pOutputString->Append('\\');
                    pOutputString->Append(c);
                }
                else if (c <= 0x7f)
                {
                    pOutputString->Append(c);
                }
                else
                {
                    pOutputString->Append( _T("\\u"), DefineTcharStringLength(_T("\\u")) );
                    pOutputString->AppendSnprintf(5,_T("%04u"),c);
                    pOutputString->Append('?');
                }
            }
        }
        break;
    default:
        pOutputString->Append(pszContent);
        break;
    }
}

// SIZE_T ContentLength : length of text that will be added to pOutputString. -1 to add full pszContent string
void CListview::ConvertHTML_XML_MarkupsAndAddToOutputString(COutputString* pOutputString,TCHAR* pszContent,SIZE_T ContentLength)
{
    TCHAR* pc;
    TCHAR* pcEnd;
    TCHAR c;
    // replace all strings defined in ppszReplacedChar
    for (pc=pszContent,pcEnd = pc + ContentLength;
        pc<pcEnd;
        pc++)
    {
        c=*pc;
        if (c==0)
            break;
        switch(c)
        {
        case '&':
            pOutputString->Append(_T("&amp;"),DefineTcharStringLength(_T("&amp;")));
            break;
        case '<':
            pOutputString->Append(_T("&lt;"),DefineTcharStringLength(_T("&lt;")));
            break;
        case '>':
            pOutputString->Append(_T("&gt;"),DefineTcharStringLength(_T("&gt;")));
            break;
        case ' ':
            pOutputString->Append(_T("&nbsp;"),DefineTcharStringLength(_T("&nbsp;")));
            break;
        default:
            pOutputString->Append(pc,1);
            break;
        }
    }
}

void CListview::ConvertHTML_XML_MarkupsAndSaveContent(HANDLE FileHandle,TCHAR* pszContent,SIZE_T ContentLength)
{
    TCHAR* pc;
    TCHAR* pcEnd;
    TCHAR c;
    // replace all strings defined in ppszReplacedChar
    for (pc=pszContent,pcEnd = pc + ContentLength;
        pc<pcEnd;
        pc++)
    {
        c=*pc;
        if (c==0)
            break;
        switch(c)
        {
        case '&':
            CTextFile::WriteText(FileHandle,_T("&amp;"),DefineTcharStringLength(_T("&amp;")));
            break;
        case '<':
            CTextFile::WriteText(FileHandle,_T("&lt;"),DefineTcharStringLength(_T("&lt;")));
            break;
        case '>':
            CTextFile::WriteText(FileHandle,_T("&gt;"),DefineTcharStringLength(_T("&gt;")));
            break;
        case ' ':
            CTextFile::WriteText(FileHandle,_T("&nbsp;"),DefineTcharStringLength(_T("&nbsp;")));
            break;
        default:
            CTextFile::WriteText(FileHandle,pc,1);
            break;
        }
    }
}

void CListview::ItemContentSave(HANDLE FileHandle ,tagStoringType FileType,TCHAR* Content,BOOL bConvertContent)
{
    this->ItemContentSave(FileHandle ,FileType,Content,_tcslen(Content),bConvertContent);
}

void CListview::ItemContentSave(HANDLE FileHandle ,tagStoringType FileType,TCHAR* Content,SIZE_T ContentLength,BOOL bConvertContent)
{
    if (IsBadReadPtr(Content,sizeof(TCHAR*)))
        return;

    if (!bConvertContent)
    {
        if ( (FileType == StoringTypeCSV) || (FileType == StoringTypeRTF) )
        {
            if (*Content)
            {
                DWORD NbBytesWritten;
                char* pszAnsi;
                size_t nbChar;
                nbChar = ContentLength+1;

#if (defined(UNICODE)||defined(_UNICODE))
                CAnsiUnicodeConvert::UnicodeToAnsi(Content,&pszAnsi);
                if (pszAnsi)
                {
#else
                pszAnsi=Content;
#endif
                    ::WriteFile(FileHandle,pszAnsi,(DWORD)(nbChar-1),&NbBytesWritten,NULL);
#if (defined(UNICODE)||defined(_UNICODE))
                    free(pszAnsi);
                }
#endif
            }
        }
        else
        {
            if (*Content)
                CTextFile::WriteText(FileHandle,Content);
        }

        return;
    }

    switch (FileType)
    {
        // in case of XML and HTML saving, replace disturbing chars
    case StoringTypeXML:
        this->ConvertHTML_XML_MarkupsAndSaveContent(FileHandle,Content,ContentLength);
        break;
    case StoringTypeHTML:
        {
            // avoid empty cells on table some browsers remove them
            if (*Content==0)// only '\0' (TextLen is len with '\0')
                CTextFile::WriteText(FileHandle,_T("&nbsp;"),DefineTcharStringLength(_T("&nbsp;")));
            else
                this->ConvertHTML_XML_MarkupsAndSaveContent(FileHandle,Content,ContentLength);
        }
        break;
    case StoringTypeCSV:
        {
            if (*Content)
            {
                char* pszAnsi;
                CAnsiUnicodeConvert::TcharToAnsi(Content,&pszAnsi);
                if (pszAnsi)
                {
                    DWORD NbBytesWritten;
                    char* pc;
                    // replace " by ""
                    pc=strtok(pszAnsi,"\"");
                    while(pc)
                    {
                        ::WriteFile(FileHandle,pc,(DWORD)strlen(pc)/* *sizeof(CHAR)*/,&NbBytesWritten,NULL);
                        pc=strtok(NULL,"\"");
                        if (pc)
                            ::WriteFile(FileHandle,"\"\"",2 /* *sizeof(CHAR)*/,&NbBytesWritten,NULL);
                    }
                    free(pszAnsi);
                }
            }
        }
        break;
    case StoringTypeRTF:
        // if not empty string
        if (*Content)
        {
            char* pszAnsi;
            CAnsiUnicodeConvert::TcharToAnsi(Content,&pszAnsi);
            if (pszAnsi)
            {
                DWORD NbBytesWritten;
                char* Lastpc;
                char* pc;
                char c;
                char Tmp[64];
                int TmpSize;

                // replace all strings defined in ppszReplacedChar
                for (pc=pszAnsi,Lastpc=pszAnsi;;pc++)
                {
                    c=*pc;
                    if (c==0)
                        break;

                    if( (c == '\\') || (c == '{') || (c == '}'))
                    {
                        // write before special char content
                        ::WriteFile(FileHandle,Lastpc,(DWORD)(pc-Lastpc /* *sizeof(CHAR)*/),&NbBytesWritten,NULL);
                        // write special char escape
                        ::WriteFile(FileHandle,"\\",1,&NbBytesWritten,NULL);
                        Lastpc=pc;
                    }
                    else if (c=='\t')
                    {
                        // write before special char content
                        ::WriteFile(FileHandle,Lastpc,(DWORD)(pc-Lastpc /* *sizeof(CHAR)*/),&NbBytesWritten,NULL);
                        // write special char escape
                        // \\tab Tab character
                        ::WriteFile(FileHandle,"\\tab",DefineTcharStringLength(_T("\\tab")),&NbBytesWritten,NULL);
                        Lastpc=pc+1;
                    }
                    else if (c <= 0x7f)
                        continue;
                    else
                    {
                        // write before special char content
                        ::WriteFile(FileHandle,Lastpc,(DWORD)(pc-Lastpc /* *sizeof(CHAR)*/),&NbBytesWritten,NULL);
                        // write special 
                        TmpSize = _snprintf(Tmp,_countof(Tmp),"\\u%04u?",(DWORD)c);
                        ::WriteFile( FileHandle,Tmp, TmpSize /* *sizeof(CHAR)*/,&NbBytesWritten,NULL);
                        Lastpc=pc;
                    }
                }
                free(pszAnsi);
            }
        }
        break;

    default:
        if (*Content)
            CTextFile::WriteText(FileHandle,Content,ContentLength);
        break;
    }
}

//-----------------------------------------------------------------------------
// Name: Load
// Object: load content of the file and put it in listview. This function shows 
//         an interface to user for choosing file
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Load()
{
    TCHAR pszFile[MAX_PATH]=_T("");

    // open dialog
    OPENFILENAME ofn;
    memset(&ofn,0,sizeof (OPENFILENAME));
    ofn.lStructSize=sizeof (OPENFILENAME);
    ofn.hwndOwner=this->hWndParent;
    ofn.hInstance=NULL;
    ofn.lpstrFilter=_T("xml\0*.xml\0");
    ofn.nFilterIndex = 1;
    ofn.Flags=OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
    ofn.lpstrDefExt=_T("xml");
    ofn.lpstrFile=pszFile;
    ofn.nMaxFile=MAX_PATH;
    
    if (!GetOpenFileNameModelessProtected(&ofn))
        return TRUE;

    return this->Load(ofn.lpstrFile);
}

//-----------------------------------------------------------------------------
// Name: Load
// Object: load content of the file and put it in listview
// Parameters :
//     in  : 
//     out : 
//     return : TRUE on success
//-----------------------------------------------------------------------------
BOOL CListview::Load(TCHAR* pszFileName)
{
    TCHAR pszMsg[MAX_PATH];
    TCHAR* psz=NULL;
    TCHAR* pszTagBegin;
    TCHAR* pszTagEnd;
    TCHAR* pszChildTagEnd;
    TCHAR* pszChildTagBegin;
    TCHAR* pszCurrentFilePos;
    TCHAR* pszOut;

    DWORD dwCurrentRow;
    DWORD dwCurrentColumn;
    BOOL bUnicodeFile=FALSE;
    TCHAR* pszFile;
    DWORD dwpszSize;
    dwpszSize=4096;// try to minimize the number of loops and reallocation
    

    if (!pszFileName)
        return FALSE;

    if (!*pszFileName)
        return FALSE;

    this->Clear();

    
    if (!CTextFile::Read(pszFileName,&pszFile,&bUnicodeFile))
        return FALSE;

    // search Listview tag
    pszCurrentFilePos=pszFile;
    pszTagBegin=_tcsstr(pszCurrentFilePos,_T("<LISTVIEW>"));
    if (!pszTagBegin)
    {
        _tcscpy(pszMsg,_T("Syntax error in file "));
        _tcscat(pszMsg,pszFileName);
        _tcscat(pszMsg,_T("\r\nNo LISTVIEW Tag found"));
        MessageBoxModelessProtected(this->hWndParent,pszMsg,_T("Error"),MB_OK|MB_ICONERROR);
        return FALSE;
    }
    // go after Listview tag
    pszCurrentFilePos+=10;
    // search end of listview tag
    pszTagEnd=_tcsstr(pszCurrentFilePos,_T("</LISTVIEW>"));
    if (!pszTagEnd)
    {
        _tcscpy(pszMsg,_T("Syntax error in file "));
        _tcscat(pszMsg,pszFileName);
        _tcscat(pszMsg,_T("\r\nNo LISTVIEW end Tag found"));
        MessageBoxModelessProtected(this->hWndParent,pszMsg,_T("Error"),MB_OK|MB_ICONERROR);
        return FALSE;
    }
    // ends string at begin of listviw tag end
    *pszTagEnd=0;

    // loop into row
    pszCurrentFilePos=_tcsstr(pszCurrentFilePos,_T("<ROW>"));
    dwCurrentRow=0;
    psz=new TCHAR[dwpszSize];
    while (pszCurrentFilePos>0)
    {
        // go after Row tag
        pszCurrentFilePos+=5;
        // search end of row tag
        pszTagEnd=_tcsstr(pszCurrentFilePos,_T("</ROW>"));
        if (!pszTagEnd)
        {
            _tcscpy(pszMsg,_T("Syntax error in file "));
            _tcscat(pszMsg,pszFileName);
            _tcscat(pszMsg,_T("\r\nNo ROW end Tag found"));
            MessageBoxModelessProtected(this->hWndParent,pszMsg,_T("Error"),MB_OK|MB_ICONERROR);

            delete[] psz;
            return FALSE;
        }
        // search begin of column tag
        pszChildTagBegin=_tcsstr(pszCurrentFilePos,_T("<COLUMN>"));
        // loop into columns
        dwCurrentColumn=0;
        while ((pszTagEnd>pszChildTagBegin)&&(pszChildTagBegin))
        {
            // go after column tag
            pszCurrentFilePos+=8;
            // search end of column tag
            pszChildTagEnd=_tcsstr(pszCurrentFilePos,_T("</COLUMN>"));
            if(!pszChildTagEnd)
            {
                if (psz)
                    delete[] psz;

                _tcscpy(pszMsg,_T("Syntax error in file "));
                _tcscat(pszMsg,pszFileName);
                _tcscat(pszMsg,_T("\r\nNo COLUMN end Tag found"));
                MessageBoxModelessProtected(this->hWndParent,pszMsg,_T("Error"),MB_OK|MB_ICONERROR);
                return FALSE;
            }
            if ((DWORD)(pszChildTagEnd-pszCurrentFilePos+1)>dwpszSize)
            {
                // if already allocated
                if (psz)
                    delete[] psz;

                // allocate new size
                dwpszSize=(DWORD)(pszChildTagEnd-pszCurrentFilePos+1);
                psz=new TCHAR[dwpszSize];
            }
            _tcsncpy(psz,pszCurrentFilePos,pszChildTagEnd-pszCurrentFilePos);
            psz[pszChildTagEnd-pszCurrentFilePos]=0;
            pszOut=new TCHAR[pszChildTagEnd-pszCurrentFilePos+1];
            // convert xml-like back to normal text
            this->Convert(psz,pszOut,FALSE);
            this->SetItemText(dwCurrentRow,dwCurrentColumn,pszOut);
            delete[] pszOut;

            // go after Column end tag 
            pszCurrentFilePos=pszChildTagEnd+9;
            // search begin of next column tag
            pszChildTagBegin=_tcsstr(pszCurrentFilePos,_T("<COLUMN>"));
            dwCurrentColumn++;
        } // end of column loop

        // go after row tag end
        pszCurrentFilePos=pszTagEnd+6;
        // search next row
        pszCurrentFilePos=_tcsstr(pszCurrentFilePos,_T("<ROW>"));
        dwCurrentRow++;
    } // end of row loop

    if (psz)
        delete[] psz;

    return TRUE;
}

//-----------------------------------------------------------------------------
// Name: Convert
// Object: convert string in xml like or xml like to string depending bToXML
//          Warning pszOutputString must be large enough no check is done
// Parameters :
//     in  : TCHAR* pszInputString : string to translate
//           BOOL bToXML : TRUE to convert to xml, FALSE to convert from XML
//     out : TCHAR* pszOutputString : result string
//     return : 
//-----------------------------------------------------------------------------
void CListview::Convert(TCHAR* pszInputString,TCHAR* pszOutputString,BOOL bToXML)
{
    if ((!pszInputString)||(!pszOutputString))
        return;

    // try to earn some time
    if (*pszInputString==0)
    {
        *pszOutputString=0;
        return;
    }

    int IndexOriginalString;
    int IndexReplaceString;

    TCHAR* Tmp=new TCHAR[_tcslen(pszInputString)*CListview_REPLACED_CHAR_MAX_INCREASE+1];

    if (bToXML)
    {
        IndexOriginalString=0;
        IndexReplaceString=1;
    }
    else
    {
        IndexOriginalString=1;
        IndexReplaceString=0;
    }

    _tcscpy(Tmp,pszInputString);

    // replace all strings defined in ppszReplacedChar
    for (int cnt=0;cnt<CListview_REPLACED_CHAR_ARRAY_SIZE;cnt++)
    {
        StrReplace(
                    Tmp,
                    pszOutputString,
                    pppszReplacedChar[cnt][IndexOriginalString],
                    pppszReplacedChar[cnt][IndexReplaceString]
                    );
        _tcscpy(Tmp,pszOutputString);
    }

    delete[] Tmp;
}

//-----------------------------------------------------------------------------
// Name: StrReplace
// Object: replace pszOldStr by pszNewStr in pszInputString and put the result in 
//         pszOutputString.
//         Warning pszOutputString must be large enough no check is done
// Parameters :
//     in  : TCHAR* pszInputString : string to translate
//           TCHAR* pszOldStr : string to replace
//           TCHAR* pszNewStr : replacing string
//     out : TCHAR* pszOutputString : result string
//     return : 
//-----------------------------------------------------------------------------
void CListview::StrReplace(TCHAR* pszInputString,TCHAR* pszOutputString,TCHAR* pszOldStr,TCHAR* pszNewStr)
{
    TCHAR* pszPos;
    TCHAR* pszOldPos;
    int SearchedItemSize;

    if ((!pszInputString)||(!pszOutputString)||(!pszOldStr)||(!pszNewStr))
        return;

    *pszOutputString=0;

    pszOldPos=pszInputString;
    // get searched item size
    SearchedItemSize=(int)_tcslen(pszOldStr);
    // look for next string to replace
    pszPos=_tcsstr(pszInputString,pszOldStr);
    while(pszPos)
    {
        // copy unchanged data
        _tcsncat(pszOutputString,pszOldPos,pszPos-pszOldPos);
        // copy replace string
        _tcscat(pszOutputString,pszNewStr);
        // update old position
        pszOldPos=pszPos+SearchedItemSize;
        // look for next string to replace
        pszPos=_tcsstr(pszOldPos,pszOldStr);
    }
    // copy remaining data
    _tcscat(pszOutputString,pszOldPos);
}

// GetInternalTmpBuffer : WARNING RequiredSize is in Byte count not TCHAR count !!!
LPVOID CListview::GetInternalTmpBuffer(SIZE_T RequiredSize)
{
    ::EnterCriticalSection(&this->InternalTmpBufferCriticalSection);
#ifdef _DEBUG
    if (this->InternalTmpBufferCriticalSection.RecursionCount>1)
        BreakInDebugMode;
#endif

    BOOL bNeedToAlloc = FALSE;
    if (this->pInternalTmpBuffer==NULL)
        bNeedToAlloc = TRUE;
    else if( this->InternalTmpBufferSize < RequiredSize )
        bNeedToAlloc = TRUE;
    if (bNeedToAlloc)
    {
        this->InternalTmpBufferSize = RequiredSize;
        if (this->pInternalTmpBuffer)
            ::HeapFree(this->HeapListViewItemParams,0,this->pInternalTmpBuffer);
        this->pInternalTmpBuffer = (PBYTE)::HeapAlloc(this->HeapListViewItemParams,0,this->InternalTmpBufferSize);
    }
    return this->pInternalTmpBuffer;
}
void CListview::ReleaseInternalTmpBuffer()
{
    ::LeaveCriticalSection(&this->InternalTmpBufferCriticalSection);
}

BOOL CListview::FindFirstText(IN TCHAR* SearchedString, IN int ColumnIndex,IN BOOL bRegularExpressionSearch)
{
    return this->FindNextTextInternal(SearchedString,0,ColumnIndex,bRegularExpressionSearch);
}

BOOL CListview::FindNextText(IN TCHAR* SearchedString, IN int ColumnIndex,IN BOOL bRegularExpressionSearch)
{
    int ItemIndex = this->GetSelectedIndex();
    if (ItemIndex<0) // no item selected
        ItemIndex = 0;
    else
        ItemIndex++;
    return this->FindNextTextInternal(SearchedString,ItemIndex,ColumnIndex,bRegularExpressionSearch);
}

// FindPreviousText:find the previous occurrence starting to selected item
// SearchedString: string or regular expression to search
// ColumnIndex : 0 based index, -1 to search in all columns
// bRegularExpressionSearch : TRUE if SearchedString is a regular expression, FALSE else
BOOL CListview::FindPreviousText(IN TCHAR* SearchedString, IN int ColumnIndex,IN BOOL bRegularExpressionSearch)
{
    int NbItems = this->GetItemCount();
    int Index = this->GetSelectedIndex();

    if (Index <= 0)
        return FALSE;

    if (NbItems<=0)
        return FALSE;

    if ( Index >= NbItems) 
        Index=NbItems;

    Index--;

    if (Index<0)
        return FALSE;

    BOOL bRetValue = FALSE;

    TCHAR* SearchedStringLower=NULL;
    TCHAR* String = NULL;
    DWORD TextLen;
    DWORD CurrentItemTextLen;
    LONG_PTR Styles;
    int CntColumn;
    int CntColumnMinValue;
    int CntColumnMaxValue;

    CRegularExpression* pRegularExpression = NULL;

    TextLen = (DWORD)_tcslen(SearchedString);
    TextLen+=1;// +1 for \0
    SearchedStringLower = (TCHAR*)_alloca((TextLen)*sizeof(TCHAR));
    _tcscpy(SearchedStringLower,SearchedString);
        
    if (bRegularExpressionSearch)
    {
        pRegularExpression = new CRegularExpression(SearchedString);
        if (!pRegularExpression->IsRegExpressionValid())
            goto CleanUp;
    }
    else
    {
        // convert SearchedText to lower case only once
        _tcslwr(SearchedStringLower);
    }

    TextLen = __max(256,TextLen);
    String = new TCHAR[TextLen]; 

    // set single selection style
    Styles=GetWindowLongPtr(this->GetControlHandle(),GWL_STYLE);
    SetWindowLongPtr(this->GetControlHandle(),GWL_STYLE,Styles|LVS_SINGLESEL);

    if (ColumnIndex == -1) 
    {
        CntColumnMinValue = 0; 
        CntColumnMaxValue = this->GetColumnCount();
    }
    else
    {
        CntColumnMinValue = ColumnIndex;
        CntColumnMaxValue = ColumnIndex+1;
    }
    
    // search for next matching item in listview
    for(int cnt=Index;cnt>=0;cnt--)
    {
        for (CntColumn = CntColumnMinValue; CntColumn<CntColumnMaxValue; CntColumn++)
        {
            // get item text
            CurrentItemTextLen = this->GetItemTextLen(cnt,CntColumn);
            if (CurrentItemTextLen>TextLen)
            {
                TextLen = __max(TextLen*2,CurrentItemTextLen+1);
                delete[] String;
                String = new TCHAR[TextLen]; 
            }
            this->GetItemText(cnt,CntColumn,String,TextLen);

            if (bRegularExpressionSearch)
            {
                if (pRegularExpression->DoesMatch(String))
                    bRetValue =TRUE;
            }
            else
            {
                // convert to lower case
                _tcslwr(String);
                // check if searched string is found
                if (_tcsstr(String,SearchedStringLower))
                    bRetValue =TRUE;
            }

            if (bRetValue)
            {
                // ::SetFocus(this->GetControlHandle());
                this->SetSelectedIndex(cnt,TRUE);
                // restore style
                SetWindowLongPtr(this->GetControlHandle(),GWL_STYLE,Styles);

                goto CleanUp;
            }
        }
    }

    // restore style
    SetWindowLongPtr(this->GetControlHandle(),GWL_STYLE,Styles);

CleanUp:
    if (String)
        delete[] String;

    if (pRegularExpression)
        delete pRegularExpression;

    return bRetValue;
}

// FindNextTextInternal: find the next occurrence
// SearchedString: string or regular expression to search
// Index: Index at which to start start the search (included)
// ColumnIndex : 0 based index, -1 to search in all columns
// bRegularExpressionSearch : TRUE if SearchedString is a regular expression, FALSE else
BOOL CListview::FindNextTextInternal(IN TCHAR* SearchedString, IN int Index, IN int ColumnIndex,IN BOOL bRegularExpressionSearch)
{
    BOOL bRetValue = FALSE;
    TCHAR* SearchedStringLower=NULL;
    TCHAR* String = NULL;
    DWORD TextLen;
    DWORD CurrentItemTextLen;
    LONG_PTR Styles;
    int CntColumn;
    int CntColumnMinValue;
    int CntColumnMaxValue;

    CRegularExpression* pRegularExpression = NULL;

    if (Index<0)
        Index=0;

    if (Index>=this->GetItemCount())
        goto CleanUp;

    TextLen = (DWORD)_tcslen(SearchedString);
    TextLen+=1;// +1 for \0
    SearchedStringLower = (TCHAR*)_alloca((TextLen)*sizeof(TCHAR));
    _tcscpy(SearchedStringLower,SearchedString);

    if (bRegularExpressionSearch)
    {
        pRegularExpression = new CRegularExpression(SearchedString);
        if (!pRegularExpression->IsRegExpressionValid())
            goto CleanUp;
    }
    else
    {
        // convert SearchedText to lower case only once
        _tcslwr(SearchedStringLower);
    }

    TextLen = __max(256,TextLen);
    String = new TCHAR[TextLen]; 

    // set single selection style
    Styles=GetWindowLongPtr(this->GetControlHandle(),GWL_STYLE);
    SetWindowLongPtr(this->GetControlHandle(),GWL_STYLE,Styles|LVS_SINGLESEL);

    if (ColumnIndex == -1) 
    {
        CntColumnMinValue = 0; 
        CntColumnMaxValue = this->GetColumnCount();
    }
    else
    {
        CntColumnMinValue = ColumnIndex;
        CntColumnMaxValue = ColumnIndex+1;
    }

    // search for next matching item in listview
    for(int cnt=Index;cnt<this->GetItemCount();cnt++)
    {
        for (CntColumn = CntColumnMinValue; CntColumn<CntColumnMaxValue; CntColumn++)
        {
            // get item text
            CurrentItemTextLen = this->GetItemTextLen(cnt,CntColumn);
            if (CurrentItemTextLen>TextLen)
            {
                TextLen = __max(TextLen*2,CurrentItemTextLen+1);
                delete[] String;
                String = new TCHAR[TextLen]; 
            }
            this->GetItemText(cnt,CntColumn,String,TextLen);

            if (bRegularExpressionSearch)
            {
                if (pRegularExpression->DoesMatch(String))
                    bRetValue =TRUE;
            }
            else
            {
                // convert to lower case
                _tcslwr(String);
                // check if searched string is found
                if (_tcsstr(String,SearchedStringLower))
                    bRetValue = TRUE;
            }

            if (bRetValue)
            {
                this->SetSelectedIndex(cnt,TRUE);

                // restore style
                SetWindowLongPtr(this->GetControlHandle(),GWL_STYLE,Styles);

                bRetValue = TRUE;
                goto CleanUp;
            }
        }
    }

    // restore style
    SetWindowLongPtr(this->GetControlHandle(),GWL_STYLE,Styles);

CleanUp:
    if (String)
        delete[] String;

    if (pRegularExpression)
        delete pRegularExpression;

    return bRetValue;
}
