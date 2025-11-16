
//  _____________________________________________________________________________________
//
//  main.auto.c
//
//  Within Auto C this file is will be read-only. You should not modify the contents of
//  this file. To use this code with your modifications, copy a function from this file
//  to another file and give the function a different name. Then, add code in the main
//  window procedure to call your function next to the code that calls the auto-generated
//  function and comment out the code that calls the auto-generated function.
//  _____________________________________________________________________________________

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "main.auto.h"


HWND hwnd_lblPort, hwnd_lblSerialData, hwnd_lblUpdatePeriod, hwnd_lblUpdatePeriodVal,
hwnd_lblParsedData, hwnd_lblGraphMin, hwnd_lblGraphMax, hwnd_lblGraphBuffSz, hwnd_cbPortList,
hwnd_btnSerialPortConnect, hwnd_txtSerialData, hwnd_sldUpdatePeriod, hwnd_txtParsedData,
hwnd_btnSerialPortRefresh, hwnd_btnSerialPortDisconnect, hwnd_btnSerialDataClear,
hwnd_btnParsedDataClear, hwnd_udMaxPort, hwnd_cbGraphType, hwnd_txtGraphMin, hwnd_txtGraphMax,
hwnd_chkLogData, hwnd_txtGraphBuffSz, hwnd_btnGraphClear;

HFONT Consolas_10pt;
HFONT Consolas_8pt;



double WindowScaling(void)
{
    static double Result;
    if (Result) return Result;
    HDC hDC = CreateCompatibleDC(NULL);
    Result = GetDeviceCaps(hDC, LOGPIXELSY) / 96.0;
    DeleteDC(hDC);
    return Result;
}



void CreateChildWindows(HWND hwndMainWindow, HINSTANCE hInstance)
{
    INITCOMMONCONTROLSEX iccex;

    iccex.dwICC = ICC_WIN95_CLASSES;
    iccex.dwSize = sizeof(iccex);
    InitCommonControlsEx(&iccex);

    HDC hDC = GetDC(hwndMainWindow);
    Consolas_10pt = CreateFont(-MulDiv(10, GetDeviceCaps(hDC, LOGPIXELSY), 72),0,0,0,FW_NORMAL,0,0,0,0,0,0,0,0,"Consolas");
    Consolas_8pt = CreateFont(-MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72),0,0,0,FW_NORMAL,0,0,0,0,0,0,0,0,"Consolas");
    ReleaseDC(hwndMainWindow, hDC);

    hwnd_lblPort = CreateChildWindow(0, "Static", "Serial port (10 max):",
        SS_RIGHT | WS_VISIBLE,
        154, 11, 165, 22, hwndMainWindow, lblPort);

    SetWindowFont(hwnd_lblPort, Consolas_10pt, TRUE);

    hwnd_lblSerialData = CreateChildWindow(0, "Static", "Serial port stream:",
        WS_VISIBLE,
        550, 44, 154, 17, hwndMainWindow, lblSerialData);

    SetWindowFont(hwnd_lblSerialData, Consolas_10pt, TRUE);

    hwnd_lblUpdatePeriod = CreateChildWindow(0, "Static", "Update period:",
        WS_VISIBLE,
        418, 11, 99, 33, hwndMainWindow, lblUpdatePeriod);

    SetWindowFont(hwnd_lblUpdatePeriod, Consolas_10pt, TRUE);

    hwnd_lblUpdatePeriodVal = CreateChildWindow(0, "Static", "1000 ms",
        WS_VISIBLE,
        638, 11, 66, 33, hwndMainWindow, lblUpdatePeriodVal);

    SetWindowFont(hwnd_lblUpdatePeriodVal, Consolas_10pt, TRUE);

    hwnd_lblParsedData = CreateChildWindow(0, "Static", "Parsed data:",
        WS_VISIBLE,
        418, 44, 99, 22, hwndMainWindow, lblParsedData);

    SetWindowFont(hwnd_lblParsedData, Consolas_10pt, TRUE);

    hwnd_lblGraphMin = CreateChildWindow(0, "Static", "Min:",
        SS_RIGHT | WS_VISIBLE,
        99, 66, 33, 22, hwndMainWindow, lblGraphMin);

    SetWindowFont(hwnd_lblGraphMin, Consolas_10pt, TRUE);

    hwnd_lblGraphMax = CreateChildWindow(0, "Static", "Max:",
        SS_RIGHT | WS_VISIBLE,
        176, 66, 33, 22, hwndMainWindow, lblGraphMax);

    SetWindowFont(hwnd_lblGraphMax, Consolas_10pt, TRUE);

    hwnd_lblGraphBuffSz = CreateChildWindow(0, "Static", "Buffer:",
        SS_RIGHT | WS_VISIBLE,
        253, 66, 55, 22, hwndMainWindow, lblGraphBuffSz);

    SetWindowFont(hwnd_lblGraphBuffSz, Consolas_10pt, TRUE);

    hwnd_cbPortList = CreateChildWindow(0, "ComboBox", NULL,
        CBS_NOINTEGRALHEIGHT | WS_VSCROLL | CBS_DROPDOWNLIST | WS_VISIBLE,
        330, 11, 66, 100, hwndMainWindow, cbPortList);

    SetWindowFont(hwnd_cbPortList, Consolas_8pt, TRUE);

    hwnd_btnSerialPortConnect = CreateChildWindow(0, "Button", "Connect",
        BS_MULTILINE | BS_PUSHBUTTON | WS_VISIBLE,
        330, 33, 77, 22, hwndMainWindow, btnSerialPortConnect);

    SetWindowFont(hwnd_btnSerialPortConnect, Consolas_8pt, TRUE);

    hwnd_txtSerialData = CreateChildWindow(WS_EX_CLIENTEDGE, "Edit", NULL,
        ES_WANTRETURN | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL | ES_READONLY | WS_VISIBLE,
        550, 66, 154, 275, hwndMainWindow, txtSerialData);

    SetWindowFont(hwnd_txtSerialData, Consolas_8pt, TRUE);

    hwnd_sldUpdatePeriod = CreateChildWindow(0, "msctls_trackbar32", NULL,
        TBS_AUTOTICKS | TBS_TOOLTIPS | WS_VISIBLE,
        528, 11, 110, 33, hwndMainWindow, sldUpdatePeriod);

    SendMessage(hwnd_sldUpdatePeriod, TBM_SETRANGE, 0, MAKELPARAM(3, 12));
    SendMessage(hwnd_sldUpdatePeriod, TBM_SETLINESIZE, 0, 1);
    SendMessage(hwnd_sldUpdatePeriod, TBM_SETPAGESIZE, 0, 5);
    SendMessage(hwnd_sldUpdatePeriod, TBM_SETTICFREQ, 1, 0);
    SendMessage(hwnd_sldUpdatePeriod, TBM_SETPOS, (WPARAM)TRUE, 4);

    hwnd_txtParsedData = CreateChildWindow(WS_EX_CLIENTEDGE, "Edit", NULL,
        ES_WANTRETURN | ES_MULTILINE | WS_HSCROLL | WS_VSCROLL | ES_READONLY | WS_VISIBLE,
        418, 66, 121, 275, hwndMainWindow, txtParsedData);

    SetWindowFont(hwnd_txtParsedData, Consolas_8pt, TRUE);

    hwnd_btnSerialPortRefresh = CreateChildWindow(0, "Button", "Refresh",
        BS_MULTILINE | BS_PUSHBUTTON | WS_VISIBLE,
        154, 33, 77, 22, hwndMainWindow, btnSerialPortRefresh);

    SetWindowFont(hwnd_btnSerialPortRefresh, Consolas_8pt, TRUE);

    hwnd_btnSerialPortDisconnect = CreateChildWindow(0, "Button", "Disconnect",
        BS_MULTILINE | BS_PUSHBUTTON | WS_VISIBLE,
        242, 33, 77, 22, hwndMainWindow, btnSerialPortDisconnect);

    SetWindowFont(hwnd_btnSerialPortDisconnect, Consolas_8pt, TRUE);

    hwnd_btnSerialDataClear = CreateChildWindow(0, "Button", "Clear",
        BS_MULTILINE | BS_PUSHBUTTON | WS_VISIBLE,
        638, 363, 66, 22, hwndMainWindow, btnSerialDataClear);

    SetWindowFont(hwnd_btnSerialDataClear, Consolas_8pt, TRUE);

    hwnd_btnParsedDataClear = CreateChildWindow(0, "Button", "Clear",
        BS_MULTILINE | BS_PUSHBUTTON | WS_VISIBLE,
        418, 363, 66, 22, hwndMainWindow, btnParsedDataClear);

    SetWindowFont(hwnd_btnParsedDataClear, Consolas_8pt, TRUE);

    hwnd_udMaxPort = CreateChildWindow(0, "msctls_updown32", NULL,
        WS_VISIBLE,
        396, 10, 11, 23, hwndMainWindow, udMaxPort);

    SendMessage(hwnd_udMaxPort, UDM_SETRANGE, 0, MAKELPARAM(255, 1));
    SendMessage(hwnd_udMaxPort, UDM_SETPOS, 0, MAKELPARAM(1, 0));
    MoveWindow(hwnd_udMaxPort, 396 * WindowScaling(), 10 * WindowScaling(), 11 * WindowScaling(), 23 * WindowScaling(), TRUE);

    hwnd_cbGraphType = CreateChildWindow(0, "ComboBox", NULL,
        CBS_NOINTEGRALHEIGHT | WS_VSCROLL | CBS_DROPDOWNLIST | WS_VISIBLE,
        11, 66, 88, 100, hwndMainWindow, cbGraphType);

    SetWindowFont(hwnd_cbGraphType, Consolas_8pt, TRUE);

    hwnd_txtGraphMin = CreateChildWindow(WS_EX_CLIENTEDGE, "Edit", "0",
        ES_WANTRETURN | WS_VISIBLE,
        132, 66, 44, 22, hwndMainWindow, txtGraphMin);

    SetWindowFont(hwnd_txtGraphMin, Consolas_8pt, TRUE);

    hwnd_txtGraphMax = CreateChildWindow(WS_EX_CLIENTEDGE, "Edit", "100",
        ES_WANTRETURN | WS_VISIBLE,
        209, 66, 44, 22, hwndMainWindow, txtGraphMax);

    SetWindowFont(hwnd_txtGraphMax, Consolas_8pt, TRUE);

    hwnd_chkLogData = CreateChildWindow(0, "Button", "Log",
        BS_AUTOCHECKBOX | BS_MULTILINE | BS_LEFT | WS_VISIBLE,
        363, 66, 44, 22, hwndMainWindow, chkLogData);

    SetWindowFont(hwnd_chkLogData, Consolas_8pt, TRUE);

    hwnd_txtGraphBuffSz = CreateChildWindow(WS_EX_CLIENTEDGE, "Edit", "100",
        ES_WANTRETURN | WS_VISIBLE,
        308, 66, 44, 22, hwndMainWindow, txtGraphBuffSz);

    SetWindowFont(hwnd_txtGraphBuffSz, Consolas_8pt, TRUE);

    hwnd_btnGraphClear = CreateChildWindow(0, "Button", "C",
        BS_MULTILINE | BS_PUSHBUTTON | WS_VISIBLE,
        11, 363, 22, 22, hwndMainWindow, btnGraphClear);

    SetWindowFont(hwnd_btnGraphClear, Consolas_8pt, TRUE);

    return;
}



HWND GetItem(int nIDDlgItem)
{
    switch (nIDDlgItem)
    {
        case -1:
            return GetParent(hwnd_lblPort);
        case lblPort:
            return hwnd_lblPort;
        case lblSerialData:
            return hwnd_lblSerialData;
        case lblUpdatePeriod:
            return hwnd_lblUpdatePeriod;
        case lblUpdatePeriodVal:
            return hwnd_lblUpdatePeriodVal;
        case lblParsedData:
            return hwnd_lblParsedData;
        case lblGraphMin:
            return hwnd_lblGraphMin;
        case lblGraphMax:
            return hwnd_lblGraphMax;
        case lblGraphBuffSz:
            return hwnd_lblGraphBuffSz;
        case cbPortList:
            return hwnd_cbPortList;
        case btnSerialPortConnect:
            return hwnd_btnSerialPortConnect;
        case txtSerialData:
            return hwnd_txtSerialData;
        case sldUpdatePeriod:
            return hwnd_sldUpdatePeriod;
        case txtParsedData:
            return hwnd_txtParsedData;
        case btnSerialPortRefresh:
            return hwnd_btnSerialPortRefresh;
        case btnSerialPortDisconnect:
            return hwnd_btnSerialPortDisconnect;
        case btnSerialDataClear:
            return hwnd_btnSerialDataClear;
        case btnParsedDataClear:
            return hwnd_btnParsedDataClear;
        case udMaxPort:
            return hwnd_udMaxPort;
        case cbGraphType:
            return hwnd_cbGraphType;
        case txtGraphMin:
            return hwnd_txtGraphMin;
        case txtGraphMax:
            return hwnd_txtGraphMax;
        case chkLogData:
            return hwnd_chkLogData;
        case txtGraphBuffSz:
            return hwnd_txtGraphBuffSz;
        case btnGraphClear:
            return hwnd_btnGraphClear;
        default: return NULL;
    }
}



void Form_Unload(HWND hMainWnd)
{
    DeleteFont(Consolas_10pt);
    DeleteFont(Consolas_8pt);
    return;
}

