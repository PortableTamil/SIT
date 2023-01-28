/*
Copyright (C) 2004-2016 Jacquelin POTIER <jacquelin.potier@free.fr>
Dynamic aspect ratio code Copyright (C) 2004-2013 Jacquelin POTIER <jacquelin.potier@free.fr>

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

#pragma once

// required lib : comctl32.lib
#pragma comment (lib,"comctl32")
// require manifest to make SetView work

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 // for xp os
#endif

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#pragma warning (push)
#pragma warning(disable : 4005)// for '_stprintf' : macro redefinition in tchar.h
#include <TCHAR.h>
#pragma warning (pop)
#include <malloc.h>
#pragma warning (push)
#pragma warning(disable : 4995)// for '_stprintf' : macro redefinition in tchar.h
#include <vector>
#pragma warning (pop)
#include "..\menu\popupmenu.h"
#include "..\..\clipboard\clipboard.h"
#include "..\..\File\TextFile.h"
#include "..\..\String\OutputString.h"
#include "..\..\PlacementNew\PlacementNew.h"

#define LIST_VIEW_TIMEOUT 5000
#define LIST_VIEW_COLUMN_DEFAULT_WIDTH 100
#define CLISTVIEW_DEFAULT_CUSTOM_DRAW_COLOR RGB(247,247,247) // (240,240,240)
#define CLISTVIEW_DEFAULT_HEAP_SIZE 4096

class IListViewItemsCompare;
class IListviewEvents;
// use CListviewEventsBase which contains defined empty functions
//class IListviewEvents
//{
//public:
//    virtual void OnListViewItemSelect(CListview* pSender,int ItemIndex,int SubItemIndex)=0;
//    virtual void OnListViewItemUnselect(CListview* pSender,int ItemIndex,int SubItemIndex)=0;
//    virtual void OnListViewItemDoubleClick(CListview* pSender,int ItemIndex,int SubItemIndex,BOOL bOnIcon)=0;
//    virtual void OnListViewItemClick(CListview* pSender,int ItemIndex,int SubItemIndex,BOOL bOnIcon)=0;
//    virtual void OnListViewPopUpMenuClick(CListview* pSender,UINT MenuID)=0;
//    virtual void OnListViewToolTipTextQuery(CListview* pSender,int ItemIndex,HWND hWndToolTip,NMTTDISPINFO* lpnmtdi)=0;
//};
//class IListViewItemsCompare
//{
//public:
//    virtual int OnListViewSortingCompare(CListview* pSender,TCHAR* String1,TCHAR* String2,CListview::SortingType DataSortingType,BOOL Ascending)=0;
//};

class CListview
{
public:
    typedef struct _COLUMN_INFO
    {
        TCHAR   pcName[MAX_PATH]; // column text
        int     iWidth;     // width of column in pixels
        int     iTextAlign; // LVCFMT_XX
    }COLUMN_INFO,*PCOLUMN_INFO;

    enum tagStoringType
    {
        StoringTypeXML,
        StoringTypeHTML,
        StoringTypeCSV,
        StoringTypeTXT,
        StoringTypeRTF,
    };

    enum StoreMode
    {
        StoreMode_COPY_TO_CLIP_BOARD,
        StoreMode_SAVE,
    };

    enum SortingType
    {
        SortingTypeString,
        SortingTypeNumber,
        SortingTypeHexaNumber,
        SortingTypeNumberUnsigned,
        SortingTypeDouble,

        SortingTypeUserParamAsSIZE_T,
        SortingTypeUserParamAsSSIZE_T,
        SortingTypeUserParam2AsSIZE_T,
        SortingTypeUserParam2AsSSIZE_T,
    };

    CPopUpMenu* pPopUpMenu;
    CPopUpMenu* pSubMenuCopySelectedAs;
    CPopUpMenu* pSubMenuCopyAllAs;

    BOOL bOwnerControlIsDialog; // allow to known if owner is a dialog proc (default) or a window proc
    BOOL bToolTipInfoLastActionIsMouseAction;
    POINT ToolTipInfoLastActionLastMousePoint;

    CListview(HWND hWndListView,IListviewEvents* pListViewEventsHandler=NULL,IListViewItemsCompare* pListViewComparator=NULL);
    virtual ~CListview();

    // Sets columns headers and some styles
    BOOL InitListViewColumns(int NbColumns,PCOLUMN_INFO pColumnInfo);

    // clear listview
    void Clear();

    // add item and sub items
    int AddItem(TCHAR* pcText);
    int AddItem(TCHAR* pcText,BOOL ScrollToItem);
    int AddItem(TCHAR* pcText,LPVOID UserData);
    int AddItem(TCHAR* pcText,LPVOID UserData,BOOL ScrollToItem);
    int AddItem(TCHAR* pcText,LPVOID UserData,LPVOID UserData2,BOOL ScrollToItem);
    int AddItemAndSubItems(int NbItem,TCHAR** ppcText);
    int AddItemAndSubItems(int NbItem,TCHAR** ppcText,BOOL ScrollToItem);
    int AddItemAndSubItems(int NbItem,TCHAR** ppcText,BOOL ScrollToItem,LPVOID UserData,LPVOID UserData2=NULL);
    int AddItemAndSubItems(int NbItem,TCHAR** ppcText,int ItemIndex,BOOL ScrollToItem);
    int AddItemAndSubItems(int NbItem,TCHAR** ppcText,int ItemIndex,BOOL ScrollToItem,LPVOID UserData,LPVOID UserData2=NULL);
    BOOL RemoveItem(int ItemIndex);
    BOOL RemoveSelectedItems(BOOL bSelectedIsNotChecked=FALSE);
    BOOL Move(int OldIndex,int NewIndex);
    BOOL MoveAllSelectedUp();
    BOOL MoveAllSelectedDown();

    DWORD SetView(DWORD iView);
    void SetStyle(BOOL bFullRowSelect,BOOL bGridLines,BOOL bSendClickNotification,BOOL bCheckBoxes);
    void SetDefaultCustomDrawColor(COLORREF ColorRef);
    void SetDefaultMenuText(TCHAR* Clear,TCHAR* CopySelected,TCHAR* CopyAll,TCHAR* SaveSelected,TCHAR* SaveAll);
    void SetDefaultMenuIcons(HINSTANCE hInstance,int IdIconClear,int IdIconCopySelected,int IdIconCopyAll,int IdIconSaveSelected,int IdIconSaveAll,int Width,int Height);
    void SetDefaultMenuIcons(HICON IconClear,HICON IconCopySelected,HICON IconCopyAll,HICON IconSaveSelected,HICON IconSaveAll);
    UINT AddCopySelectedAsPopupMenu(UINT Index,HINSTANCE hInstance,int IdIconCopySelectedAs,int IdIconRtf,int IdIconCsv,int IdIconHtml,int IdIconXml,int Width,int Height);
    UINT AddCopySelectedAsPopupMenu(UINT Index,HICON IconCopySelectedAs,HICON IconRtf,HICON IconCsv,HICON IconHtml,HICON IconXml);
    UINT AddCopyAllAsPopupMenu(UINT Index,HINSTANCE hInstance,int IdIconCopyAllAs,int IdIconRtf,int IdIconCsv,int IdIconHtml,int IdIconXml,int Width,int Height);
    UINT AddCopyAllAsPopupMenu(UINT Index,HICON IconCopyAllAs,HICON IconRtf,HICON IconCsv,HICON IconHtml,HICON IconXml);
    void EnableDefaultCustomDraw(BOOL bEnable);
    void EnablePopUpMenu(BOOL bEnable);
    void EnableColumnSorting(BOOL bEnable);
    void EnableColumnReOrdering(BOOL bEnable);

    int GetSelectedIndex();
    int GetSelectedCount(BOOL bSelectedIsNotChecked = FALSE);
    BOOL IsItemSelected(int ItemIndex);
    BOOL IsItemSelected(int ItemIndex,BOOL bSelectedIsNotChecked);
    void SetSelectedIndex(int ItemIndex);
    void SetSelectedIndex(int ItemIndex,BOOL bScrollToItem,BOOL bSetFocus=FALSE);
    void SetSelectedState(int ItemIndex,BOOL bSelected);
    void SetSelectedState(int ItemIndex,BOOL bSelected,BOOL bSetFocus,BOOL bSelectedIsNotChecked = FALSE);
    void SelectAll(BOOL bSelectedIsNotChecked = FALSE);
    void UnselectAll(BOOL bSelectedIsNotChecked = FALSE);
    void SetAllItemsSelectedState(BOOL bSelected,BOOL bSelectedIsNotChecked = FALSE);
    void UncheckSelected();
    void CheckSelected();
    void InvertSelection();

    int GetItemCount();
    int GetColumnCount();
    HWND FORCEINLINE GetControlHandle(){return this->hWndListView;}
    HWND FORCEINLINE GetHeader(){return ListView_GetHeader(this->hWndListView);}

    BOOL GetColumnName(int ColumnIndex,TCHAR* Name,int NameMaxSize);
    BOOL SetColumnName(int ColumnIndex,TCHAR* Name);
    BOOL SetColumn(int ColumnIndex,TCHAR* Name,int iWidth,int iTextAlign);
    int AddColumn(TCHAR* Name,int iWidth,int iTextAlign,int ColumnIndex);
    int AddColumn(TCHAR* Name,int iWidth,int iTextAlign);
    BOOL RemoveColumn(int ColumnIndex);
    BOOL RemoveAllColumns();
    BOOL IsColumnVisible(int ColumnIndex);
    void HideColumn(int ColumnIndex);
    void ShowColumn(int ColumnIndex);
    void ShowColumn(int ColumnIndex,int Size);
    BOOL FORCEINLINE SetColumnWidth(int ColumnIndex,int Size){return ListView_SetColumnWidth(this->hWndListView, ColumnIndex,Size);}
    int  FORCEINLINE GetColumnWidth(int ColumnIndex){return ListView_GetColumnWidth(this->hWndListView, ColumnIndex);}
    BOOL FORCEINLINE SetColumnOrderArray(int ArrayCount, int* Array){return (BOOL)ListView_SetColumnOrderArray(this->hWndListView, ArrayCount, Array);}
    
    void ShowColumnHeaders(BOOL bShow);

    void SetTransparentBackGround();

    virtual DWORD GetItemText(int ItemIndex,int SubItemIndex,TCHAR* pcText,int pcTextMaxSize);
    void GetItemTextAsOutputString(IN int ItemIndex,IN int SubItemIndex,IN OUT COutputString* pOutputString);
    // GetItemTextLen get item text len in TCHAR including \0
    DWORD GetItemTextLen(int ItemIndex,int SubItemIndex);
    DWORD GetItemTextLenMax(BOOL bOnlySelected);
    DWORD GetItemTextLenMaxForColumn(DWORD ColumnIndex, BOOL bOnlySelected, BOOL bSelectedIsNotChecked);
    DWORD GetItemTextLenMaxForRow(DWORD RowIndex);
    void SetItemText(int ItemIndex,TCHAR* pcText);
    void SetItemText(int ItemIndex,int SubItemIndex,TCHAR* pcText);
    BOOL SetItemIndent(int ItemIndex,int Indent);// Indent is in icon size count, works only for items with icon

    BOOL GetItemRect(int ItemIndex,RECT* pRect);
    BOOL GetSubItemRect(int ItemIndex,int SubItemIndex,RECT* pRect);

    void ScrollTo(int ItemIndex);
    void ScrollToDelayedIfRequired(int ItemIndex);

    BOOL OnNotify(WPARAM wParam, LPARAM lParam);
    void SetSelectionCallbackState(BOOL bEnable);
    void ShowClearPopupMenu(BOOL bVisible);

    BOOL SetWatermarkImage(HBITMAP hBmp);// can be used for stretched background too

    // default listview search
    int Find(TCHAR* Text);
    int Find(TCHAR* Text,BOOL bPartial);
    int Find(TCHAR* Text,int StartIndex,BOOL bPartial);

    void Sort();
    void Sort(int ColumnIndex);
    void Sort(int ColumnIndex,BOOL bAscending);
    void ReSort();
    void SetSortingType(CListview::SortingType type);
    void RemoveSortingHeaderInfo();

    BOOL SetItemUserData(int ItemIndex,LPVOID UserData,LPVOID UserData2 = NULL);
    BOOL GetItemUserData(int ItemIndex,LPVOID* pUserData,LPVOID* pUserData2 = NULL);

    HWND SetFocus()
    {
        if (this->bOwnerControlIsDialog) // dialog proc
            return (HWND)::SendMessage(this->hWndParent, WM_NEXTDLGCTL, (WPARAM)this->hWndListView, TRUE);            
        else // window proc
            return ::SetFocus(this->hWndListView);
    }

    BOOL Save();
    BOOL Save(BOOL bOnlySelected);
    BOOL Save(BOOL bOnlySelected,BOOL bOnlyVisibleColumns);
    BOOL Save(BOOL bOnlySelected,BOOL bOnlyVisibleColumns,TCHAR* DefaultExtension);
    BOOL Save(TCHAR* pszFileName);
    BOOL Save(TCHAR* pszFileName,BOOL bOnlySelected);
    BOOL Save(TCHAR* pszFileName,BOOL bOnlySelected,BOOL bOnlyVisibleColumns);

    BOOL Load();
    BOOL Load(TCHAR* pszFileName);

    BOOL CopyToClipBoard();
    BOOL CopyToClipBoard(BOOL bOnlySelected);
    BOOL CopyToClipBoard(BOOL bOnlySelected,BOOL bOnlyVisibleColumns);
    BOOL CopyToClipBoard(BOOL bOnlySelected,BOOL bOnlyVisibleColumns,tagStoringType CopyType);
    BOOL CopyToClipBoard(BOOL bOnlySelected,IN int* ColumnsIndexToSaveArray,IN SIZE_T ColumnsIndexToSaveArraySize,tagStoringType CopyType);
protected:
    BOOL CopyToClipBoardEx(BOOL bOnlySelected,BOOL bOnlyVisibleColumns,tagStoringType CopyType,IN int* ColumnsIndexToSaveArray,IN SIZE_T ColumnsIndexToSaveArraySize);
public:

    enum tagImageLists
    {
        ImageListNormal = LVSIL_NORMAL, // Image list with large icons.
        ImageListSmall = LVSIL_SMALL, // Image list with small icons.
        ImageListState = LVSIL_STATE, // Image list with state images.
        // ImageListGroupHeader = LVSIL_GROUPHEADER, // Image list for group header.
        ImageListColumns
    };

    BOOL bIconsForSubItemEnabled;
    void EnableIconsForSubItem(BOOL bEnable);
    BOOL SetColumnIconIndex(int ColumnIndex,int IconIndexInColumnIconList);
    BOOL SetItemIconIndex(int ItemIndex,int IconIndex);
    BOOL SetItemIconIndex(int ItemIndex,int SubItemIndex,int IconIndex);
    int AddIcon(tagImageLists ImageList,HMODULE hModule,int IdIcon,int Width,int Height);
    int AddIcon(tagImageLists ImageList,HMODULE hModule,int IdIcon);
    int AddIcon(tagImageLists ImageList,HICON hIcon);
    BOOL RemoveIcon(tagImageLists ImageList,int IconIndex);
    BOOL RemoveAllIcons(tagImageLists ImageList);

    // Allow to provide a refresh delay : in case of big number of item insertion with auto scroll, avoid to redraw listview at each item insertion
    BOOL EnableAutoScrollDelayRefresh(BOOL bEnable,UINT MinRefreshIntervalInMs);

    void EnableDoubleBuffering(BOOL bEnable);
    BOOL EnableTooltip(BOOL bEnable);
    HWND GetTooltipControlHandle();

    void Invalidate();
    void Redraw();

    // enable or disable redraw
    void EnableRedraw(BOOL bEnable);

    // enable or disable redraw with counted enabled / disabled (avoid disable redraw query to be override by another disable + enable sequence)
    // return the count of disabled times (redraw begin again when returned counter is <=0)
    int EnableRedrawCounted();
    int DisableRedrawCounted();

    // GetBackgroundColor is virtual to be sub classed
    virtual COLORREF GetBackgroundColor(int ItemIndex,int SubitemIndex);
    virtual COLORREF GetForegroundColor(int ItemIndex,int SubitemIndex);  

    // search
    virtual BOOL FindFirstText(IN TCHAR* SearchedString,IN int ColumnIndex,IN BOOL bRegularExpressionSearch);
    virtual BOOL FindNextText(IN TCHAR* SearchedString,IN int ColumnIndex,IN BOOL bRegularExpressionSearch);
    virtual BOOL FindPreviousText(IN TCHAR* SearchedString,IN int ColumnIndex,IN BOOL bRegularExpressionSearch);
    
protected:
    BOOL FindNextTextInternal(IN TCHAR* SearchedString,IN int Index,IN int ColumnIndex,IN BOOL bRegularExpressionSearch);

    class CListviewItemParamBase // small class to provide user an associated value like Item.lparam
    {
    friend class CPlacementNew<CListviewItemParamBase>;
    public:
        LPVOID UserParam;// allow the user to have a PVOID LPARAM associated with item
        LPVOID UserParam2;// allow the user to have a second PVOID LPARAM associated with item (so he can manage one pointer for presentation and one for extra data)
        DWORD ItemKey; // for CListview private use 
    protected:
        CListviewItemParamBase()
        {
            this->UserParam=NULL;
            this->UserParam2=NULL;
            this->ItemKey=0; 
        }
        ~CListviewItemParamBase()
        {

        }
    };

    // assume sub classes can get their own derived CListviewItemParamXXX 
    virtual CListviewItemParamBase* CreateListviewItemParam()
    {
        CPlacementNew<CListviewItemParamBase> PlacementNew(this->HeapListViewItemParams);
        return PlacementNew.CreateObject();
    }
    virtual void DestroyListviewItemParam(CListviewItemParamBase* pListviewItemParamBase)
    {
        CPlacementNew<CListviewItemParamBase> PlacementNew(this->HeapListViewItemParams);
        PlacementNew.DestroyObject(pListviewItemParamBase);
    }

    CListviewItemParamBase* GetListviewItemParamInternal(int ItemIndex);
    BOOL SetListviewItemParamInternal(int ItemIndex,CListviewItemParamBase* pListviewItemParam);
    HIMAGELIST hImgListNormal;
    HIMAGELIST hImgListSmall;
    HIMAGELIST hImgListState;
    // HIMAGELIST hImgListGroupHeader;
    HIMAGELIST hImgListColumns;

    UINT MenuIdCopySelected;
    UINT MenuIdCopyAll;
    UINT MenuIdSaveSelected;
    UINT MenuIdSaveAll;
    UINT MenuIdClear;

    UINT MenuIdCopySelectedAs;
    UINT MenuIdCopySelectedAs_Rtf ;
    UINT MenuIdCopySelectedAs_Csv ;
    UINT MenuIdCopySelectedAs_Html;
    UINT MenuIdCopySelectedAs_Xml ;

    UINT MenuIdCopyAllAs;
    UINT MenuIdCopyAllAs_Rtf ;
    UINT MenuIdCopyAllAs_Csv ;
    UINT MenuIdCopyAllAs_Html;
    UINT MenuIdCopyAllAs_Xml ;

    HANDLE HeapListViewItemParams;
    
    HWND hWndListView;// handle to listview
    HWND hWndParent;// handle to parent window
    // TCHAR replacement table for xml like loading saving
    TCHAR** ppszReplacedChar;
    // lock var
    HANDLE hevtUnlocked;
    // sorting vars
    BOOL bSortAscending;
    int iLastSortedColumn;
    DWORD ItemKey;

    BOOL PopUpMenuEnabled;
    BOOL ColumnSortingEnabled;
    BOOL DefaultCustomDrawEnabled;
    BOOL DefaultCustomDrawExEnabled;
    COLORREF DefaultCustomDrawColor;
    

    IListViewItemsCompare* pListViewComparator;
    IListviewEvents* pListViewEventsHandler;
    BOOL bSelectionCallBackEnable; // avoid to raise selection callback event on InvertSelection calls
    TCHAR** pSortArray;

    int DisableRedrawCounter;
    BOOL bRedrawEnabled;

    // tmp buffer
    PBYTE pInternalTmpBuffer;
    SIZE_T InternalTmpBufferSize;
    CRITICAL_SECTION InternalTmpBufferCriticalSection;
    LPVOID GetInternalTmpBuffer(SIZE_T RequiredSize);
    void ReleaseInternalTmpBuffer();

    // tooltip
    HWND hWndToolTip;
    static LRESULT CALLBACK SubClassListView(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

    // RTF
    std::vector<COLORREF> RtfColorList;
    BOOL RtfSaveAsTable;// TRUE : table + cells mode, FALSE flat mode (no table)
    #define CLISTVIEW_RTF_CELL_WIDTH_STRING_LENGTH 256
    typedef struct tagRtfCellInfo
    {
        TCHAR CellWidth[CLISTVIEW_RTF_CELL_WIDTH_STRING_LENGTH];
        BOOL bIsVisible;
    }RTF_CELL_INFO;
    RTF_CELL_INFO* pRtfColumnInfoArray;

    // delay auto scroll
    BOOL bAutoScrollDelayRefreshEnabled;
    int AutoScrollRequiredIndex;
    int AutoScrollAppliedIndex;
    UINT AutoScrollMinRefreshIntervalInMs;
    DWORD AutoScrollLastTickUpdate;
    static VOID CALLBACK AutoScrollTimerProc(HWND hWnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);

    virtual void OnRightClickInternal(WPARAM wParam,LPARAM lParam);
    virtual void OnClickInternal(WPARAM wParam,LPARAM lParam);
    virtual void OnDoubleClickInternal(WPARAM wParam,LPARAM lParam);
    virtual void OnItemChangedInternal(WPARAM wParam,LPARAM lParam);
    virtual LRESULT OnCustomDrawInternal(WPARAM wParam,LPARAM lParam);
    virtual LRESULT OnCustomDrawInternalEx(WPARAM wParam,LPARAM lParam);
    virtual BOOL DrawItemText(HDC hDC,IN RECT* const pSubItemRect,int ItemIndex,int SubItemIndex);
    virtual BOOL DrawItemTextInternal(HDC hDC,IN RECT* const pSubItemRect,int ItemIndex,int SubItemIndex);
    BOOL EllipseTextToFitWidth(IN HDC hDC,IN OUT TCHAR* Text,IN OUT SIZE_T* pTextLength,IN LONG MaxTextWidth, OUT SIZE* pTextSize);
    DWORD GetItemTextBase(int ItemIndex,TCHAR* pcText,int pcTextMaxSize);
    DWORD GetItemTextBase(int ItemIndex,int SubItemIndex,TCHAR* pcText,int pcTextMaxSize);
   
    virtual void FreeCListviewItemParam(BOOL NeedToReallocMemory);
    void FreeCListviewItemParamInternal(BOOL NeedToReallocMemory);
    static int CALLBACK Compare(LPARAM lParam1, LPARAM lParam2,LPARAM lParamSort);
    void StrReplace(TCHAR* pszInputString,TCHAR* pszOutputString,TCHAR* pszOldStr,TCHAR* pszNewStr);
    void Convert(TCHAR* pszInputString,TCHAR* pszOutputString,BOOL bToXML);
    SortingType stSortingType;

    // in all the following IN OUT COutputString* pOutputString : string inside which content will be append
    virtual void GetStoreContentDocumentHeader(IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    virtual void GetStoreContentDocumentFooter(IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    virtual void GetStoreContentDocumentTableBegin(IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    virtual void GetStoreContentDocumentTableEnd(IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    // only here for derivation (specific software copyright or link)
    virtual void GetStoreContentDocumentPostBodyBegin(IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    // only here for derivation (specific software copyright or link)
    virtual void GetStoreContentDocumentPreBodyEnd(IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    virtual void GetStoreContentTableHeadersRowBegin(IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    virtual void GetStoreContentTableHeadersRowEnd(IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    virtual void GetStoreContentTableHeadersColumnBegin(IN int SubItemIndex,IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    virtual void GetStoreContentTableHeadersColumnEnd(IN int SubItemIndex,IN BOOL bLastColumn,IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    virtual void GetStoreContentTableHeader(IN int SubItemIndex,IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    virtual void GetStoreContentItemsRowBegin(IN int ItemIndex,IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    virtual void GetStoreContentItemsRowEnd(IN int ItemIndex,IN BOOL bLastRow,IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    virtual void GetStoreContentItemsColumnBegin(IN int ItemIndex,IN int SubItemIndex,IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    virtual void GetStoreContentItemsColumnEnd(IN int ItemIndex,IN int SubItemIndex,IN BOOL bLastColumn,IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    virtual void GetStoreContentItem(IN int ItemIndex,IN int SubItemIndex,IN tagStoringType StoringType,IN OUT COutputString* pOutputString);
    void GenericStoring(IN StoreMode StorageMode,IN PVOID StorageData,IN tagStoringType StoringType,IN BOOL bOnlySelected,IN BOOL bOnlyVisibleColumns,IN int* ColumnsIndexToSaveArray,IN SIZE_T ColumnsIndexToSaveArraySize);
    virtual void FillStoreRtfColorTable(IN BOOL bStoreOnlySelectedRow,IN BOOL bStoreOnlyVisibleColumns,IN int* ColumnsIndexToSaveArray,IN SIZE_T ColumnsIndexToSaveArraySize);
    virtual void FillStoreRtfColumnPercentArray(IN BOOL bStoreOnlyVisibleColumns,IN int* ColumnsIndexToSaveArray,IN SIZE_T ColumnsIndexToSaveArraySize);
    UINT GetStoreRtfColorIndex(COLORREF Color);
    UINT AddStoreRtfColorIndexIfNeeded(COLORREF Color);

    void ConvertHTML_XML_MarkupsAndSaveContent(HANDLE FileHandle,TCHAR* pszContent,SIZE_T ContentLength);
    void ConvertHTML_XML_MarkupsAndAddToOutputString(COutputString* pOutputString,TCHAR* pszContent,SIZE_T ContentLength);
    void ItemContentSave(HANDLE FileHandle,tagStoringType FileType,TCHAR* Content,BOOL bConvertContent);
    void ItemContentSave(HANDLE FileHandle,tagStoringType FileType,TCHAR* Content,SIZE_T ContentLength,BOOL bConvertContent);
    void AddItemContentToOutputString(COutputString* pOutputString,tagStoringType FileType,TCHAR* pszContent,BOOL bConvertContent);
    void AddItemContentToOutputString(COutputString* pOutputString,tagStoringType FileType,TCHAR* pszContent,SIZE_T ContentLength,BOOL bConvertContent);
    void StoreItemContentWithoutContentConvertion(StoreMode StorageMode,PVOID StoreData,tagStoringType FileType,TCHAR* Content,SIZE_T ContentLength);

    int AddItemAndSubItems(int NbItem,TCHAR** ppcText,int ItemIndex,BOOL ScrollToItem,LPVOID UserData,LPVOID UserData2,CListviewItemParamBase* pListviewItemParam);
    int AddItemAndSubItemsWithIcons(int NbItem,TCHAR** ppcText,int* pIconIndexArray,int ItemIndex,BOOL ScrollToItem,LPVOID UserData,LPVOID UserData2,CListviewItemParamBase* pListviewItemParam);

    BOOL IsCheckedListView();
    int GetCheckedCount();
    void ShowPopUpMenu();
    void ShowPopUpMenu(LONG X,LONG Y);

    COLORREF GetBackgroundColorBase(int ItemIndex,int SubitemIndex);
    COLORREF GetForegroundColorBase(int ItemIndex,int SubitemIndex);

    virtual void DoListViewSortItems();
};

class IListviewEvents
{
public:
    virtual void OnListViewItemClick(CListview* pSender,int ItemIndex,int SubItemIndex,BOOL bOnIcon)=0;
    virtual void OnListViewItemDoubleClick(CListview* pSender,int ItemIndex,int SubItemIndex,BOOL bOnIcon)=0;
    // WARNING : when click again the same item, an OnListViewItemUnselect + OnListViewItemSelect events will be raised
    virtual void OnListViewItemSelect(CListview* pSender,int ItemIndex,int SubItemIndex)=0;
    // WARNING : when click again the same item, an OnListViewItemUnselect + OnListViewItemSelect events will be raised
    virtual void OnListViewItemUnselect(CListview* pSender,int ItemIndex,int SubItemIndex)=0;
    virtual void OnListViewPopUpMenuClick(CListview* pSender,UINT MenuID)=0;
    virtual void OnListViewToolTipTextQuery(CListview* pSender,int ItemIndex,HWND hWndToolTip,NMTTDISPINFO* lpnmtdi)=0;
    virtual void OnListViewItemRightClick(IN CListview* pSender,IN int ItemIndex,IN int SubItemIndex,IN BOOL bOnIcon,OUT BOOL* pbDisplayPopupMenu)=0;
};
class CListviewEventsBase:public IListviewEvents
{
public:
    virtual void OnListViewItemClick(CListview* pSender,int ItemIndex,int SubItemIndex,BOOL bOnIcon)
    {
        UNREFERENCED_PARAMETER(pSender);
        UNREFERENCED_PARAMETER(ItemIndex);
        UNREFERENCED_PARAMETER(SubItemIndex);
        UNREFERENCED_PARAMETER(bOnIcon);
    }
    virtual void OnListViewItemDoubleClick(CListview* pSender,int ItemIndex,int SubItemIndex,BOOL bOnIcon)
    {
        UNREFERENCED_PARAMETER(pSender);
        UNREFERENCED_PARAMETER(ItemIndex);
        UNREFERENCED_PARAMETER(SubItemIndex);
        UNREFERENCED_PARAMETER(bOnIcon);
    }

    // WARNING : when click again the same item, an OnListViewItemUnselect + OnListViewItemSelect events will be raised
    virtual void OnListViewItemSelect(CListview* pSender,int ItemIndex,int SubItemIndex)
    {
        UNREFERENCED_PARAMETER(pSender);
        UNREFERENCED_PARAMETER(ItemIndex);
        UNREFERENCED_PARAMETER(SubItemIndex);
    }
    // WARNING : when click again the same item, an OnListViewItemUnselect + OnListViewItemSelect events will be raised
    virtual void OnListViewItemUnselect(CListview* pSender,int ItemIndex,int SubItemIndex)
    {
        UNREFERENCED_PARAMETER(pSender);
        UNREFERENCED_PARAMETER(ItemIndex);
        UNREFERENCED_PARAMETER(SubItemIndex);
    }

    virtual void OnListViewPopUpMenuClick(CListview* pSender,UINT MenuID)
    {
        UNREFERENCED_PARAMETER(pSender);
        UNREFERENCED_PARAMETER(MenuID);
    }

    virtual void OnListViewToolTipTextQuery(CListview* pSender,int ItemIndex,HWND hWndToolTip,NMTTDISPINFO* lpnmtdi)
    {
        UNREFERENCED_PARAMETER(pSender);
        UNREFERENCED_PARAMETER(ItemIndex);
        UNREFERENCED_PARAMETER(hWndToolTip);
        UNREFERENCED_PARAMETER(lpnmtdi);
    }

    virtual void OnListViewItemRightClick(IN CListview* pSender,IN int ItemIndex,IN int SubItemIndex,IN BOOL bOnIcon,OUT BOOL* pbDisplayPopupMenu)
    {
        UNREFERENCED_PARAMETER(pSender);
        UNREFERENCED_PARAMETER(ItemIndex);
        UNREFERENCED_PARAMETER(SubItemIndex);
        UNREFERENCED_PARAMETER(bOnIcon);
        *pbDisplayPopupMenu=TRUE;
    }
};
class IListViewItemsCompare
{
public:
    //sorting callback must return :
    //- negative value if first item is less than second item
    //- positive value else
    virtual int  OnListViewSortingCompare(CListview* pSender,int iColumnIndex,TCHAR* String1,TCHAR* String2,BOOL Ascending)=0;
};
