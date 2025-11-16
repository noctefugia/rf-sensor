
//  _____________________________________________________________________________________
//
//  main.c
//
//  An SDK application employing menus and common controls. This file is one of a multi-
//  file project. Functions defined in main.auto.c are called from within this file.
//  _____________________________________________________________________________________
//
#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "main.auto.h"
#include <tchar.h>
#include <shlwapi.h>
#include "mygraph.h"

//  These are prototypes for the functions that appear in this file:
//  _____________________________________________________________________________________

    LRESULT CALLBACK MainWindowProc(HWND hWndParent, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void Scrollbar_Click(HWND hWndParent, HWND hwndCtl, UINT wNotifyCode, int nPos);
    void Text_Change(HWND hWndParent, int wID, HWND hwndCtl, UINT wNotifyCode);
    void ListBox_Click(HWND hWndParent, int wID, HWND hwndCtl, UINT wNotifyCode);
    BOOL MouseOverControl(HWND hWndParent, HWND hwndCtl, UINT nHittest, UINT wMouseMsg);
    void Button_Click(HWND hWndParent, int wID, HWND hwndCtl, UINT wNotifyCode);

    extern HWND hwnd_lblPort, hwnd_lblSerialData, hwnd_lblUpdatePeriod, hwnd_lblUpdatePeriodVal,
    hwnd_lblParsedData, hwnd_lblGraphMin, hwnd_lblGraphMax, hwnd_lblGraphBuffSz, hwnd_cbPortList,
    hwnd_btnSerialPortConnect, hwnd_txtSerialData, hwnd_sldUpdatePeriod, hwnd_txtParsedData,
    hwnd_btnSerialPortRefresh, hwnd_btnSerialPortDisconnect, hwnd_btnSerialDataClear,
    hwnd_btnParsedDataClear, hwnd_udMaxPort, hwnd_cbGraphType, hwnd_txtGraphMin, hwnd_txtGraphMax,
    hwnd_chkLogData, hwnd_txtGraphBuffSz, hwnd_btnGraphClear;

    #define APP_VERSION "v1.0"
#define MAX_SERIAL_PORT 10
#define IDT_UPDATE_TIMER 1
#define SERIAL_BUFFER_SIZE 128
#define SERIAL_BUFFER_LINES 150
#define BYTE_L(val) ((BYTE)((val) & (BYTE)0xFF))
#define BYTE_H(val) ((BYTE)(BYTE_L((val) >> (BYTE)8)))
#define WORD_HL(value_h, value_l) ((UINT16)((((UINT16)value_h) << 8) | value_l))
#define DWORD_HML(value_h, value_m, value_l) ((UINT32)((((UINT32)value_h) << 16) | (((UINT16)value_m) << 8) | value_l))

#define DBG_BUFF_SZ 256
#define DEBUG_INT(expr) \
	{ _stprintf(DBG_BUFF, DBG_BUFF_SZ, TEXT(#expr " = %i"), expr); MessageBox(NULL, DBG_BUFF, TEXT("DEBUG"), 0); }
#define DEBUG_STR(expr) \
	{ _stprintf(DBG_BUFF, DBG_BUFF_SZ, TEXT(#expr " = %s"), expr); MessageBox(NULL, DBG_BUFF, TEXT("DEBUG"), 0); }
TCHAR DBG_BUFF[DBG_BUFF_SZ];

#define LIGHT_RECT_X0 490
#define LIGHT_RECT_Y0 345
#define LIGHT_RECT_W 142
#define LIGHT_RECT_H 40

const LPCTSTR STR_RESOURCE_LOAD_FAIL = TEXT("Failed to load resource");
void EnumSerialPorts(UINT max_port);
void Form_Init(HINSTANCE hInstance, HWND hWnd);
void Form_UpdateTimer(void);
void MsgBox(LPCSTR buff, DWORD err_code);
void PrintCommState(DCB dcb, LPCTSTR name);
BOOL OpenComPort(HANDLE *hCom, LPCSTR port_name, DWORD bdr, BYTE byte_sz, BYTE parity, BYTE stop_bits);
void ControlAppendText(int nIDDlgItem, LPCSTR new_buff, BOOL scroll);
void ControlClearText(int nIDDlgItem);
void ControlSetText(int nIDDlgItem, LPCSTR buff);
void CRC16(UINT16 *crc, BYTE data);
void ParseComData(BYTE *buff, DWORD buff_sz);
void ProcessComPacket(BYTE cmd_id, BYTE *payload_buff, BYTE payload_sz);
void LogStart(void);
void LogStop(void);
void LogWrite(char *buff);

#define GRAPH_COUNT 3
const TCHAR graph_list[GRAPH_COUNT + 1][16] = {
	TEXT("Humidity"),
	TEXT("Temperature"),
	TEXT("Lux"),
	TEXT("UNDEFINED")
};

struct bitmap_struct {
	HBITMAP hbmp;
	BITMAP  bmp;
};

typedef enum {
	GM_HUMIDITY,
	GM_TEMPERATURE,
	GM_LUX,
	GM_UNDEFINED
} GraphMode_TypeDef;

struct system_struct {
	struct bitmap_struct logo;
	UINT update_period, serial_buffer_index, max_cport;
	HWND hwnd_main;
	HANDLE hcom;
	BOOL com_flag, new_data_flag, log_flag, first_req_flag;
	INT16 accel_buff[3];
	float temperature, humidity;
	UINT32 color_buff[4], lux;
	FILE *fp_log;
	RECT light_rect;
	GraphMode_TypeDef graph_mode;
};
struct system_struct sys = {0};


void MsgBox(LPCSTR buff, DWORD err_code)
{
	TCHAR cap_buff[2][8] = {TEXT("ERROR"), TEXT("INFO")};
	TCHAR msg_buff[255];
	BOOL is_error = (err_code != ERROR_SUCCESS) ? TRUE : FALSE;

	if (is_error)
		_stprintf(msg_buff, 255, TEXT("%s\nCode: %u"), buff, err_code);

	MessageBox(sys.hwnd_main, (is_error ? msg_buff : buff), cap_buff[(is_error ? 0 : 1)], MB_OK|(is_error ? MB_ICONWARNING : MB_ICONINFORMATION));
}


void LogStart(void)
{
	char filename[255];
	SYSTEMTIME st;
	GetLocalTime(&st);

	sprintf_s(filename, 255, "log_%02d_%02d_%04d.txt", st.wDay, st.wMonth, st.wYear);
	sys.fp_log = fopen(filename, "a");
	if (sys.fp_log != NULL) {
		fprintf(sys.fp_log, "TIME\tACCEL_X\tACCEL_Y\tACCEL_Z\tHMDT\tTEMP\tCOLOR_I\tCOLOR_G\tCOLOR_R\tCOLOR_B\tLUX\n");
		sys.log_flag = TRUE;
	} else {
		MsgBox(TEXT("LogStart failed"), 1);
	}
}


void LogStop(void)
{
	if ( (sys.log_flag) || (sys.fp_log != NULL) ) {
		fclose(sys.fp_log);
		sys.log_flag = FALSE;
	}
}


void LogWrite(char *buff)
{
	if (sys.log_flag) {
		SYSTEMTIME st;
		GetLocalTime(&st);
		char full_buff[255];
		sprintf_s(full_buff, 255, "%02d:%02d:%02d\t%s\n", st.wHour, st.wMinute, st.wSecond, buff);
		int res = fputs(full_buff, sys.fp_log);
		if (res < 0) {
			LogStop();
			MsgBox(TEXT("LogWrite failed"), 1);
		} else {
			fflush(sys.fp_log);
		}
	}
}


void EnumSerialPorts(UINT max_port)
{
	TCHAR port_name[8];
	COMMCONFIG cc;
	DWORD dwSize = sizeof(COMMCONFIG);

	SendMessage(GetItem(cbPortList), (UINT)CB_RESETCONTENT, (WPARAM)0, (LPARAM)0); 
	for (UINT i=1; i<max_port; i++) {
		_stprintf(port_name, 8, TEXT("COM%u"), i);

		if (GetDefaultCommConfig(port_name, &cc, &dwSize))
			SendMessage(GetItem(cbPortList), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)port_name); 
	}
	SendMessage(GetItem(cbPortList), CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
}


void Form_Init(HINSTANCE hInstance, HWND hWnd)
{
	UINT i;

	sys.logo.hbmp = NULL;
	sys.update_period = 1000;
	sys.hwnd_main = hWnd;
	sys.com_flag = FALSE;
	sys.serial_buffer_index = 0;
	sys.new_data_flag = FALSE;
	sys.log_flag = FALSE;
	sys.fp_log = NULL;
	sys.graph_mode = GM_HUMIDITY;
	sys.first_req_flag = FALSE;
	SetRect(&sys.light_rect, LIGHT_RECT_X0, LIGHT_RECT_Y0, LIGHT_RECT_X0 + LIGHT_RECT_W, LIGHT_RECT_Y0 + LIGHT_RECT_H);

	sys.max_cport = MAX_SERIAL_PORT;
	SendMessage(GetItem(udMaxPort), (UINT)UDM_SETPOS, (WPARAM)0, (LPARAM)sys.max_cport); 

	EnableWindow(GetItem(btnSerialPortDisconnect), FALSE);
	EnumSerialPorts(sys.max_cport);

	sys.logo.hbmp = LoadBitmap(hInstance, MAKEINTRESOURCE(8002));
	if (sys.logo.hbmp)
		GetObject(sys.logo.hbmp, sizeof(sys.logo.bmp), &sys.logo.bmp);

	for (i = 0; i < GRAPH_COUNT; ++i)
		SendMessage(GetItem(cbGraphType), (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)graph_list[i]); 
	SendMessage(GetItem(cbGraphType), CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	Graph_Init(sys.hwnd_main, 10, 90, 400, 295);
	Graph_SetFormatY(TEXT("%"));

	SetTimer(hWnd, IDT_UPDATE_TIMER, sys.update_period, (TIMERPROC)NULL);
}



void Form_UpdateTimer(void)
{
	BOOL fSuccess; 
	COMSTAT coms; 
	DWORD error_flags, out_buff_sz = 4, in_buff_sz = SERIAL_BUFFER_SIZE, nbytes_read = 0; 
	BYTE out_buff[out_buff_sz], in_buff[in_buff_sz], i;
	UINT16 crc = 0;
	TCHAR msg_buff[255];

	sys.new_data_flag = FALSE;

	if (sys.com_flag) {
		fSuccess = ReadFile(sys.hcom, in_buff, in_buff_sz, &nbytes_read, NULL); 
		if(!fSuccess) { 
			MsgBox(TEXT("Failed to read from serial port!"), GetLastError());
			CloseHandle(sys.hcom); 
			sys.com_flag = FALSE;
		} else {
			if (nbytes_read > 0) {
				ParseComData(in_buff, nbytes_read);
				_tcscpy_s(msg_buff, 255, TEXT("<<"));
				for (i = 0; i < nbytes_read; ++i)
					_stprintf(msg_buff, 255, TEXT("%s %02X"), msg_buff, in_buff[i]); 
				_tcscat_s(msg_buff, 255, TEXT("\r\n"));
				ControlAppendText(txtSerialData, msg_buff, TRUE);
				++sys.serial_buffer_index;
			}
			PurgeComm(sys.hcom, PURGE_RXABORT|PURGE_RXCLEAR);
		}

		out_buff[0] = 1; //msg size
		out_buff[1] = 1; //msg no (REQUEST_SENSOR_DATA)
		for (i = 0; i < (out_buff_sz - 2); ++i)
			CRC16(&crc, out_buff[i]);
		out_buff[2] = BYTE_H(crc);
		out_buff[3] = BYTE_L(crc);

		ClearCommError(sys.hcom, &error_flags, &coms); 
		fSuccess = WriteFile(sys.hcom, out_buff, out_buff_sz, &out_buff_sz, NULL); 
		if(!fSuccess) { 
			MsgBox(TEXT("Failed to write to serial port!"), GetLastError());
			CloseHandle(sys.hcom); 
			sys.com_flag = FALSE;
		} else {
			PurgeComm(sys.hcom, PURGE_TXABORT|PURGE_TXCLEAR);
			_tcscpy_s(msg_buff, 255, TEXT(">>"));
			for (i = 0; i < out_buff_sz; ++i)
				_stprintf(msg_buff, 255, TEXT("%s %02X"), msg_buff, out_buff[i]); 
			_tcscat_s(msg_buff, 255, TEXT("\r\n"));
			ControlAppendText(txtSerialData, msg_buff, TRUE);
			if ((++sys.serial_buffer_index) > SERIAL_BUFFER_LINES) {
				ControlClearText(txtSerialData);
				sys.serial_buffer_index = 0;
			}
		}
	}

	if (sys.new_data_flag)
		InvalidateRect(sys.hwnd_main, &sys.light_rect, TRUE);
	else if ( (sys.com_flag) && (!sys.first_req_flag) )
		Graph_AddBuffValueF(0.0f);
	sys.first_req_flag = FALSE;
}


void ParseComData(BYTE *buff, DWORD buff_sz)
{
	UINT i, pos, msg_sz, payload_sz;
	BYTE cur_byte, cmd_id, payload[buff_sz];
	UINT16 crc1, crc2;

	pos = 0;
	ControlClearText(txtParsedData);

	do {
		msg_sz = buff[pos++] + 3; //cmd_sz + payload + crc_h + crc_l
		if (msg_sz > buff_sz) {
			ControlSetText(txtParsedData, "PARSE FAILED\r\n");
			return;
		}

		payload_sz = 0;
		crc1 = crc2 = 0;
		CRC16(&crc2, buff[pos - 1]);
		for (i = 1; i < msg_sz; ++i) {
			cur_byte = buff[pos++];
			if (i == 1)
				cmd_id = cur_byte;
			else if (i == (msg_sz - 2))
				crc1 = (UINT16)(cur_byte << 8);
			else if (i == (msg_sz - 1))
				crc1 |= (UINT16)cur_byte;
			else
				payload[payload_sz++] = cur_byte;

			if (i < (msg_sz - 2))
				CRC16(&crc2, cur_byte);
		}

		if (crc1 == crc2)
			ProcessComPacket(cmd_id, payload, payload_sz);
	} while (pos < buff_sz);
}


void ProcessComPacket(BYTE cmd_id, BYTE *payload_buff, BYTE payload_sz)
{
	UINT i, pos;
	TCHAR msg_buff[255];
	BOOL result;
	TCHAR axes_buff[3][2] = {"X", "Y", "Z"}, color_id_buff[4][2] = {"I", "G", "R", "B"};
	UINT32 hmdt_raw, temp_raw;
	char log_buff[255];

	_stprintf(msg_buff, 255, TEXT("CMD_ID: %u\r\nPAYLOAD_SZ: %u\r\nDESCR: \r\n"), cmd_id, payload_sz); 
	ControlAppendText(txtParsedData, msg_buff, TRUE);

	result = FALSE;
	switch (cmd_id) {
		case 0:
			ControlAppendText(txtParsedData, TEXT("PING\r\n"), TRUE);
			result = TRUE;
			break;

		case 1:
			ControlAppendText(txtParsedData, TEXT("RF OFFLINE\r\n"), TRUE);
			result = TRUE;
			break;

		case 2:
			if (payload_sz == 1) {
				_stprintf(msg_buff, 255, TEXT("RF RETRIES: %u\r\n"), payload_buff[0]); 
				ControlAppendText(txtParsedData, msg_buff, TRUE);
				result = TRUE;
			}
			break;

		case 3:
			if (payload_sz == 24) {
				pos = 0;
				ControlAppendText(txtParsedData, TEXT("RF SENSOR DATA\r\n"), TRUE);
				for (i = 0; i < 3; ++i) {
					sys.accel_buff[i] = (INT16)WORD_HL(payload_buff[pos + i*2 + 1], payload_buff[pos + i*2]);
					_stprintf(msg_buff, 255, TEXT("ACCEL_%s: %i\r\n"), axes_buff[i], sys.accel_buff[i]); 
					ControlAppendText(txtParsedData, msg_buff, TRUE);
				}

				pos += 6;
				hmdt_raw = (UINT32)(DWORD_HML(payload_buff[pos+1], payload_buff[pos+2], payload_buff[pos+3]) >> 4);
				temp_raw = DWORD_HML((payload_buff[pos+3] & 0x0F), payload_buff[pos+4], payload_buff[pos+5]);
				sys.humidity = (float)(((float)hmdt_raw) * 0.000095f);
				sys.temperature = (float)(((float)temp_raw) * 0.000191f - 50.0f);
				result = TRUE;
				_stprintf(msg_buff, 255, TEXT("HMDT: %.2f %%\r\nTEMP: %.2f C\r\n"), sys.humidity, sys.temperature); 
				ControlAppendText(txtParsedData, msg_buff, TRUE);

				pos += 6;
				for (i = 0; i < 4; ++i) {
					sys.color_buff[i] = DWORD_HML(payload_buff[pos + 3*i + 2], payload_buff[pos + 3*i + 1], payload_buff[pos + 3*i + 0]);
					_stprintf(msg_buff, 255, TEXT("COLOR_%s: %u\r\n"), color_id_buff[i], sys.color_buff[i]); 
					ControlAppendText(txtParsedData, msg_buff, TRUE);
				}
				//resolution 16bit (INT=1), GAIN=3, WFAC=1, C1=0.033
				//LUX = (0.8*GREEN_DATA)/(GAIN*INT) * (1 - C1*(IR_DATA/GREEN_DATA)) * WFAC
				sys.lux = ((0.8*sys.color_buff[1])/(3*1)) * (1 - 0.033*(sys.color_buff[0]/((sys.color_buff[1] == 0 ? 1 : sys.color_buff[1])))) * 1;
				_stprintf(msg_buff, 255, TEXT("LUX: %u\r\n"), sys.lux); 
				ControlAppendText(txtParsedData, msg_buff, TRUE);

				sprintf_s(log_buff, 255, "%i\t%i\t%i\t%.2f\t%.2f\t%u\t%u\t%u\t%u\t%u",
					sys.accel_buff[0], sys.accel_buff[1], sys.accel_buff[2],
					sys.humidity, sys.temperature,
					sys.color_buff[0], sys.color_buff[1], sys.color_buff[2], sys.color_buff[3],
					sys.lux
				);
				LogWrite(log_buff);
				if (sys.graph_mode == GM_HUMIDITY)
					Graph_AddBuffValueF(sys.humidity);
				else if (sys.graph_mode == GM_TEMPERATURE)
					Graph_AddBuffValueF(sys.temperature);
				else if (sys.graph_mode == GM_LUX)
					Graph_AddBuffValue((int)sys.lux);
				sys.new_data_flag = TRUE;
			}
			break;

		default:
			ControlAppendText(txtParsedData, TEXT("UNDEFINED CMD\r\n"), TRUE);
			result = TRUE;
	}

	if (!result)
		ControlAppendText(txtParsedData, TEXT("CMD PROCESS ERR\r\n"), TRUE);
	ControlAppendText(txtParsedData, TEXT("\r\n"), FALSE);
}


void CRC16(UINT16 *crc, BYTE data)
{
	BYTE i;
	
	*crc ^= data;
	for (i = 0; i < 8; ++i)
		*crc = (*crc & 1) ? ((*crc >> 1) ^ 0xA001) : (*crc >> 1);
}


BOOL DrawBitmap(HDC *hdc, HDC *ddc, HDC *cdc, struct bitmap_struct *p_bitmap, int x, int y)
{
	if (p_bitmap->hbmp) {
		HBITMAP hbm = CreateCompatibleBitmap(*ddc, p_bitmap->bmp.bmWidth, p_bitmap->bmp.bmHeight);
		HBITMAP hbmOld = (HBITMAP)SelectObject(*hdc, hbm);
		HBITMAP hbmol2 = (HBITMAP)SelectObject(*cdc, p_bitmap->hbmp);
		BitBlt(*hdc, x, y, sys.logo.bmp.bmWidth, p_bitmap->bmp.bmHeight, *cdc, 0, 0, SRCCOPY);
		SelectObject(*hdc, hbmOld);
		SelectObject(*cdc, hbmol2);
		return TRUE;
	} 
	
	return FALSE;
}


void OnPaint(HWND hWnd)
{
	UINT i;
	PAINTSTRUCT ps = {0};
	HDC hdc = BeginPaint(hWnd, &ps);
	
	HDC ddc = GetDC(GetDesktopWindow());
	HDC cdc = CreateCompatibleDC(ddc);

	/* bitmaps begin */
	if (!DrawBitmap(&hdc, &ddc, &cdc, &sys.logo, 10, 10))
		TextOut(hdc, 10, 10, STR_RESOURCE_LOAD_FAIL, _tcslen(STR_RESOURCE_LOAD_FAIL));
	/* bitmaps end */

	if (sys.new_data_flag) {
		BYTE light_byte[3];
		UINT32 max_light = 0, fixed_light[3];

		//1.8706 green-red k
		//1.7673 green-blue k
		fixed_light[1] = sys.color_buff[1];
		fixed_light[0] = (UINT32)(((float)sys.color_buff[2]) * 1.8706f); //1.9425
		fixed_light[2] = (UINT32)(((float)sys.color_buff[3]) * 1.7673f); //1.7315
		for (i = 0; i < 3; ++i)
			if (fixed_light[i] > max_light)
				max_light = fixed_light[i];
		if (max_light == 0)
			max_light = 1;
		for (i = 0; i < 3; ++i)
			light_byte[i] = (BYTE)((UINT32)(fixed_light[i] * 255) / max_light);

		HBRUSH light_hbr = CreateSolidBrush(RGB(light_byte[0], light_byte[1], light_byte[2]));
		FillRect(hdc, &sys.light_rect, light_hbr);
		DeleteObject(light_hbr);
	}
	Graph_Paint(&hdc);

	DeleteDC(cdc);
	ReleaseDC(GetDesktopWindow(), ddc);

	EndPaint(hWnd, &ps);
}


void PrintCommState(DCB dcb, LPCTSTR name)
{
	TCHAR port_buff[255];

	_stprintf(port_buff, 255, TEXT("%s\r\nBaudRate = %d\r\nByteSize = %d\r\nParity = %d\r\nStopBits = %d\r\n"),
		name,
		dcb.BaudRate, 
		dcb.ByteSize, 
		dcb.Parity,
		dcb.StopBits);
	//MsgBox(port_buff, 0);

	ControlAppendText(txtSerialData, port_buff, TRUE);
}


BOOL OpenComPort(HANDLE *hCom, LPCSTR port_name, DWORD bdr, BYTE byte_sz, BYTE parity, BYTE stop_bits)
{
	DCB dcb;
	BOOL fSuccess;

	ControlClearText(txtSerialData);

	//  Open a handle to the specified com port.
	*hCom = CreateFile(port_name,
		GENERIC_READ | GENERIC_WRITE,
		0,      //  must be opened with exclusive-access
		NULL,   //  default security attributes
		OPEN_EXISTING, //  must use OPEN_EXISTING
		0,      //  not overlapped I/O
		NULL ); //  hTemplate must be NULL for comm devices

	if (*hCom == INVALID_HANDLE_VALUE) {
		MsgBox(TEXT("CreateFile failed"), GetLastError());
		return FALSE;
	}

	SecureZeroMemory(&dcb, sizeof(DCB));
	dcb.DCBlength = sizeof(DCB);

	fSuccess = GetCommState(*hCom, &dcb);
	if (!fSuccess) {
		MsgBox(TEXT("GetCommState failed"), GetLastError());
		CloseHandle(*hCom);
		return FALSE;
	}
	PrintCommState(dcb, TEXT("Current config:"));

	/* NEW */
	fSuccess = SetupComm(*hCom, SERIAL_BUFFER_SIZE, SERIAL_BUFFER_SIZE);
	if (!fSuccess) {
		MsgBox(TEXT("SetupComm failed"), GetLastError());
		CloseHandle(*hCom);
		return FALSE;
	}

	COMMTIMEOUTS comt;
	comt.ReadIntervalTimeout=2; 
	comt.ReadTotalTimeoutMultiplier=1; 
	comt.ReadTotalTimeoutConstant=10; 
	comt.WriteTotalTimeoutMultiplier=1; 
	comt.WriteTotalTimeoutConstant=10; 
	fSuccess = SetCommTimeouts(*hCom, &comt);
	if (!fSuccess) {
		MsgBox(TEXT("SetCommTimeouts failed"), GetLastError());
		CloseHandle(*hCom);
		return FALSE;
	}
	/* NEW */

	dcb.BaudRate = bdr;
	dcb.ByteSize = byte_sz;
	dcb.Parity   = parity;
	dcb.StopBits = stop_bits;

	fSuccess = SetCommState(*hCom, &dcb);
	if (!fSuccess) {
		MsgBox(TEXT("SetCommState failed"), GetLastError());
		CloseHandle(*hCom);
		return FALSE;
	}

	fSuccess = GetCommState(*hCom, &dcb);
	if (!fSuccess) {
		MsgBox(TEXT("GetCommState failed"), GetLastError());
		CloseHandle(*hCom);
		return FALSE;
	}
	PrintCommState(dcb, TEXT("New config:")); 

	/* NEW */
	PurgeComm(*hCom, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);
	/* NEW */

	TCHAR msg_buff[255];
	_stprintf(msg_buff, 255, TEXT("* CONNECTED %s *\r\n"), port_name);
	ControlAppendText(txtSerialData, msg_buff, TRUE);
	//MsgBox(msg_buff, 0);

	return TRUE;
}


void ControlAppendText(int nIDDlgItem, LPCSTR new_buff, BOOL scroll)
{
	HWND hWndControl = GetItem(nIDDlgItem);

	SendMessage(hWndControl, EM_SETSEL, 0, -1); //Select all
	SendMessage(hWndControl, EM_SETSEL, -1, -1);//Unselect and stay at the end pos
	SendMessage(hWndControl, EM_REPLACESEL, 0, (LPARAM)new_buff);
	if (!scroll) {
		SendMessage(hWndControl, EM_SETSEL, 0, 0);
		SendMessage(hWndControl, EM_SCROLLCARET, 0, 0); //scroll to start
	}
}


void ControlClearText(int nIDDlgItem)
{
	TCHAR empty[1];
	empty[0] = '\0';
	SetWindowText(GetItem(nIDDlgItem), empty);
}


void ControlSetText(int nIDDlgItem, LPCSTR buff)
{
	HWND hWndControl = GetItem(nIDDlgItem);

	SetWindowText(hWndControl, buff);

	SendMessage(hWndControl, EM_SETSEL, 0, -1); //Select all. 
	SendMessage(hWndControl, EM_SETSEL, -1, -1); //Unselect and stay at the end pos
	SendMessage(hWndControl, EM_SCROLLCARET, 0, 0);
}

//  _____________________________________________________________________________________
//
//  Function WinMain
//
//  The WinMain function is called by the system as the initial entry point for a Win32
//  based application. For a description see WinMain in the Win32 SDK online help file.
//  _____________________________________________________________________________________

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    WNDCLASSEX wincl;

    wincl.cbSize = sizeof(WNDCLASSEX);
    wincl.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wincl.lpfnWndProc = MainWindowProc;
    wincl.cbClsExtra = 0;
    wincl.cbWndExtra = 0;
    wincl.hInstance = hInstance;
    wincl.hIcon = LoadIcon(hInstance, "res1");
    wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
    wincl.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wincl.lpszMenuName = NULL;
    wincl.lpszClassName = "WindowsApp";
    wincl.hIconSm = (HICON)LoadImage(hInstance, "res1", IMAGE_ICON, 16, 16, LR_SHARED);

    if (!RegisterClassEx(&wincl)) return 0;

    HWND hwndMainWindow = CreateWindowEx(WS_EX_CLIENTEDGE, wincl.lpszClassName, TEXT("RF-Sensor PC " APP_VERSION),
        WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        727 * WindowScaling() + GetSystemMetrics(SM_CXFIXEDFRAME) * 2 + GetSystemMetrics(SM_CXEDGE) * 2,
        408 * WindowScaling() + GetSystemMetrics(SM_CYFIXEDFRAME) * 2 + GetSystemMetrics(SM_CYEDGE) * 2 + GetSystemMetrics(SM_CYCAPTION),
        HWND_DESKTOP, NULL, hInstance, NULL);

    ShowWindow(hwndMainWindow, nCmdShow);

    CreateChildWindows(hwndMainWindow, hInstance);

	Form_Init(hInstance, hwndMainWindow);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}


//  _____________________________________________________________________________________
//
//  Function MainWindowProc
//
//  The MainWindowProc function is called repeatedly by the system while the application
//  runs. For a description see WindowProc in the Win32 SDK online help file.
//  _____________________________________________________________________________________

LRESULT CALLBACK MainWindowProc(HWND hWndParent, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_COMMAND:
            HANDLE_WM_COMMAND(hWndParent, wParam, lParam, Text_Change);
            HANDLE_WM_COMMAND(hWndParent, wParam, lParam, ListBox_Click);
            HANDLE_WM_COMMAND(hWndParent, wParam, lParam, Button_Click);
            break;

        case WM_CREATE:
            break;

        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
            break;

        case WM_DESTROY:
			if (sys.com_flag)
				CloseHandle(sys.hcom);
			LogStop();
			Graph_Destroy();
            HANDLE_WM_DESTROY(hWndParent, 0, 0, Form_Unload);
            PostQuitMessage(0);
            break;

        case WM_HSCROLL:
        case WM_VSCROLL:
            HANDLE_WM_HSCROLL(hWndParent, wParam, lParam, Scrollbar_Click);
            break;

        case WM_NOTIFY:
            break;

        case WM_SETCURSOR:
            HANDLE_WM_SETCURSOR(hWndParent, wParam, lParam, MouseOverControl);
            break;

        case WM_SIZE:
            break;

		case WM_PAINT:
			OnPaint(hWndParent);
			break;

		case WM_TIMER:
			if (wParam == IDT_UPDATE_TIMER)
				Form_UpdateTimer();
			break;
    }

    return DefWindowProc(hWndParent, uMsg, wParam, lParam);
}



//  _____________________________________________________________________________________
//
//  Function Button_Click - called with every WM_COMMAND message.
//
//  The code here traps button click events. If you later rename or delete a control, any code
//  added for that control will be removed. (This applies to all auto-generated functions.)
//  _____________________________________________________________________________________

void Button_Click(HWND hWndParent, int wID, HWND hwndCtl, UINT wNotifyCode)
{
    if(wID==btnSerialPortConnect)
    {
		TCHAR ItemBuff[8];
		int ItemIndex = SendMessage(GetItem(cbPortList), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
       	SendMessage(GetItem(cbPortList), (UINT)CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)ItemBuff);
		
		sys.com_flag = FALSE;
		if (OpenComPort(&sys.hcom, ItemBuff, CBR_115200, 8, NOPARITY, ONESTOPBIT)) {
			sys.com_flag = TRUE;
			sys.first_req_flag = TRUE;
		}

		EnableWindow(GetItem(btnSerialPortConnect), FALSE);
        EnableWindow(GetItem(btnSerialPortDisconnect), TRUE);
    }

    if(wID==btnSerialPortRefresh)
    {
		EnableWindow(GetItem(btnSerialPortRefresh), FALSE);
		EnumSerialPorts(sys.max_cport);
		EnableWindow(GetItem(btnSerialPortRefresh), TRUE);
    }

    if(wID==btnSerialPortDisconnect)
    {
		if (sys.com_flag) {
			CloseHandle(sys.hcom);
			ControlAppendText(txtSerialData, TEXT("* DISCONNECTED *\r\n"), TRUE);
			sys.com_flag = FALSE;
		}

        EnableWindow(GetItem(btnSerialPortDisconnect), FALSE);
		EnableWindow(GetItem(btnSerialPortConnect), TRUE);
    }

    if(wID==btnSerialDataClear)
    {
        ControlClearText(txtSerialData);
    }

    if(wID==btnParsedDataClear)
    {
        ControlClearText(txtParsedData);
    }

    if(wID==chkLogData)
    {
		LRESULT state = SendMessage(GetItem(chkLogData), (UINT)BM_GETCHECK, (WPARAM)0, (LPARAM)0); 
        if (state == BST_CHECKED)
			LogStart();
		else if (state == BST_UNCHECKED)
			LogStop();
    }

    if(wID==btnGraphClear)
    {
        Graph_ClearBuff(TRUE);
    }

    if(GetDlgItem(hWndParent, wID)&&!hwndCtl) Button_SetState(GetDlgItem(hWndParent, wID), TRUE);

    return;
}



//  _____________________________________________________________________________________
//
//  Function MouseOverControl - called with every WM_SETCURSOR message.
//
//  The code here executes when the mouse is passed over the various controls.
//  Static controls without the SS_NOTIFY style and Frames are not included.
//  _____________________________________________________________________________________

BOOL MouseOverControl(HWND hWndParent, HWND hwndCtl, UINT nHittest, UINT wMouseMsg)
{
    static HWND last;

    if (nHittest==HTCAPTION) return TRUE;

    if (hwndCtl==last) return TRUE;

    last=hwndCtl;

    if (GetParent(hwndCtl)==HWND_DESKTOP)  // The mouse was moved onto the main window
    {
    }

    switch(GetDlgCtrlID(hwndCtl))
    {
        case btnGraphClear:
            
            break;

        case cbGraphType:
            
            break;

        case txtGraphMin:
            
            break;

        case txtGraphMax:
            
            break;

        case chkLogData:
            
            break;

        case txtGraphBuffSz:
            
            break;

        case udMaxPort:
            
            break;

        case btnSerialDataClear:
            
            break;

        case btnParsedDataClear:
            
            break;

        case btnSerialPortConnect:
            
            break;

        case btnSerialPortRefresh:
            
            break;

        case btnSerialPortDisconnect:
            
            break;

        case txtParsedData:
            
            break;

        case sldUpdatePeriod:
            
            break;

        case txtSerialData:
            
            break;

        case cbPortList:
            
            break;
    }

    return TRUE;
}



//  _____________________________________________________________________________________
//
//  Function ListBox_Click - called with every WM_COMMAND message.
//
//  The code here traps a change in the selected item of a ListBox or ComboBox. For each
//  additional item, make a copy of the auto-generated code and change the numeral near
//  the end of the line; to 1 for the second item, to 2 for the third, etc..
//  _____________________________________________________________________________________

void ListBox_Click(HWND hWndParent, int wID, HWND hwndCtl, UINT wNotifyCode)
{

    if((wID==cbPortList) && (wNotifyCode==CBN_SELCHANGE) && (ComboBox_GetCurSel(hwndCtl)==0))
    {
        
    }

    if ((wID==cbGraphType) && (wNotifyCode==CBN_SELCHANGE))
    {
		int ItemIndex = SendMessage(GetItem(cbGraphType), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
		GraphMode_TypeDef last_mode = sys.graph_mode;
		switch (ItemIndex) {
			case 0:
				sys.graph_mode = GM_HUMIDITY;
				Graph_SetFormatY(TEXT("%"));
				break;
			case 1:
				sys.graph_mode = GM_TEMPERATURE;
				Graph_SetFormatY(TEXT("C"));
				break;
			case 2:
				sys.graph_mode = GM_LUX;
				Graph_SetFormatY(TEXT("lx"));
				break;
			default:
				sys.graph_mode = GM_UNDEFINED;
		}
		if (last_mode != sys.graph_mode)
        	Graph_ClearBuff(TRUE);
    }

    return;
}



//  _____________________________________________________________________________________
//
//  Function Text_Change - called with every WM_COMMAND message.
//
//  The code between the braces executes when the text in a TextBox changes.
//  _____________________________________________________________________________________

void Text_Change(HWND hWndParent, int wID, HWND hwndCtl, UINT wNotifyCode)
{
	TCHAR txt_buff[16];
	INT val;

    if((wID==txtSerialData) && (wNotifyCode==EN_CHANGE))
    {
        
    }

    if((wID==txtParsedData) && (wNotifyCode==EN_CHANGE))
    {
        
    }

    if((wID==txtGraphMin) && (wNotifyCode==EN_KILLFOCUS))
    {
		GetWindowText(GetItem(txtGraphMin), txt_buff, 16);
		val = StrToInt(txt_buff);
		Graph_SetMinValue(&val);
		_itot(val, txt_buff, 10);
		SetWindowText(GetItem(txtGraphMin), txt_buff);
	}

    if((wID==txtGraphMax) && (wNotifyCode==EN_KILLFOCUS))
    {
		GetWindowText(GetItem(txtGraphMax), txt_buff, 16);
		val = StrToInt(txt_buff);
		Graph_SetMaxValue(&val);
		_itot(val, txt_buff, 10);
		SetWindowText(GetItem(txtGraphMax), txt_buff);
    }

    if((wID==txtGraphBuffSz) && (wNotifyCode==EN_KILLFOCUS))
    {
		GetWindowText(GetItem(txtGraphBuffSz), txt_buff, 16);
		val = abs(StrToInt(txt_buff));
		Graph_SetBuffSize(&val);
		_itot(val, txt_buff, 10);
		SetWindowText(GetItem(txtGraphBuffSz), txt_buff);
    }

    if((wID==txtGraphMin) && (wNotifyCode==EN_CHANGE))
    {
        
    }

    if((wID==txtGraphMax) && (wNotifyCode==EN_CHANGE))
    {
        
    }

    if((wID==txtGraphBuffSz) && (wNotifyCode==EN_CHANGE))
    {
        
    }

    return;
}



//  _____________________________________________________________________________________
//
//  Function Scrollbar_Click - called with every WM_HSCROLL and WM_VSCROLL message.
//
//  Used for Sliders and UpDown buttons as well as Scrollbars. When adding code here,
//  you may want to check the notify code and only respond to specific notifications
//  such as SB_ENDSCROLL, SB_THUMBPOSITION, etc.
//  _____________________________________________________________________________________

void Scrollbar_Click(HWND hWndParent, HWND hwndCtl, UINT wNotifyCode, int nPos)
{
	TCHAR lbl_buff[32];

    if(GetDlgCtrlID(hwndCtl)==sldUpdatePeriod)
    {
		if (wNotifyCode == TB_THUMBTRACK) {
			sys.update_period = (UINT)(nPos* 250);
			Graph_SetTimestep(&sys.update_period);
			_stprintf(lbl_buff, 8, TEXT("%u ms"), sys.update_period);
	        SetWindowText(GetItem(lblUpdatePeriodVal), lbl_buff);
			SetTimer(hWndParent, IDT_UPDATE_TIMER, sys.update_period, (TIMERPROC)NULL);
		}
    }

    if(GetDlgCtrlID(hwndCtl)==udMaxPort)
    {
		if (wNotifyCode == 4) {
        	sys.max_cport = nPos;
			_stprintf(lbl_buff, 32, TEXT("Serial port (%u max):"), sys.max_cport);
			SetWindowText(GetItem(lblPort), lbl_buff);
		}
    }

    return;
}
