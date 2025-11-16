#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>
#include <strsafe.h>
#include <math.h>
#include <shlwapi.h>

#include "mygraph.h"

struct graph_struct {
	int min_value, max_value, x0, y0, x1, y1, h, w;
	unsigned int buff_sz, buff_pos, timestep;
	float buff[GRAPH_MAX_BUFF+1];
	RECT rect;
	HWND hwnd;
	HBRUSH hbr_back, hbr_black, hbr_red;
	HPEN hpn_black, hpn_dashed, hpn_red;
	BOOL full_clear;
	TCHAR format_y[8];
};
struct graph_struct graph = {0};

const COLORREF CR_White = RGB(255, 255, 255);
const COLORREF CR_Black = RGB( 0, 0, 0);
const COLORREF CR_DarkBlue = RGB( 0, 0, 128);
const COLORREF CR_DarkGreen = RGB( 0, 128, 0);
const COLORREF CR_BlueGreen = RGB( 0, 128, 128);
const COLORREF CR_Brown = RGB(128, 0, 0);
const COLORREF CR_Olive = RGB(128, 128, 0);
const COLORREF CR_Purple = RGB(128, 0, 128);
const COLORREF CR_DarkGrey = RGB(128, 128, 128);
const COLORREF CR_LightGrey = RGB(192, 192, 192);
const COLORREF CR_Blue = RGB( 0, 0, 255);
const COLORREF CR_Green = RGB( 0, 255, 0);
const COLORREF CR_Cyan = RGB( 0, 255, 255);
const COLORREF CR_Red = RGB(255, 0, 0);
const COLORREF CR_Yellow = RGB(255, 255, 0);
const COLORREF CR_Magenta = RGB(255, 0, 255);


void Graph_Init(HWND hw, int x0, int y0, int w, int h)
{
	graph.hwnd = hw;	
	graph.x0 = x0;
	graph.y0 = y0;
	graph.w = w;
	graph.h = h;
	graph.x1 = graph.x0 + graph.w;
	graph.y1 = graph.y0 + graph.h;
	SetRect(&graph.rect, graph.x0, graph.y0, graph.x1, graph.y1);

	graph.min_value = 0;
	graph.max_value = 100;
	graph.buff_sz = GRAPH_MAX_BUFF / 10;
	graph.buff_pos =0;
	graph.timestep = 1000;
	graph.full_clear = TRUE;
	_tcscpy_s(graph.format_y, 8, TEXT("\0"));

	graph.hbr_back = CreateSolidBrush(CR_LightGrey);
	graph.hbr_black = CreateSolidBrush(CR_Black);
	graph.hbr_red = CreateSolidBrush(CR_Red);
	graph.hpn_black = CreatePen(PS_SOLID, 1, CR_Black);
	graph.hpn_dashed = CreatePen(PS_DOT, 1, CR_Blue);
	graph.hpn_red = CreatePen(PS_DOT, 1, CR_Red);

	Graph_Redraw();
}


void Graph_Destroy(void)
{
	DeleteObject(graph.hbr_back);
	DeleteObject(graph.hbr_black);
	DeleteObject(graph.hbr_red);
	DeleteObject(graph.hpn_black);
	DeleteObject(graph.hpn_dashed);
	DeleteObject(graph.hpn_red);
}


void Graph_SetMinValue(int *value)
{
	if (*value >= graph.max_value)
		*value = graph.max_value - 1;

	if (graph.min_value != *value) {
		graph.min_value = *value;
		Graph_ClearBuff(TRUE);
	}
}


void Graph_SetMaxValue(int *value)
{
	if (*value <= graph.min_value)
		*value = graph.min_value + 1;

	if (graph.max_value != *value) {
		graph.max_value = *value;
		Graph_ClearBuff(TRUE);
	}
}


void Graph_SetBuffSize(int *value)
{
	if (*value < 10)
		*value = 10;
	else if (*value > GRAPH_MAX_BUFF)
		*value = GRAPH_MAX_BUFF;

	if (graph.buff_sz != ((unsigned int)*value)) {
		graph.buff_sz = (unsigned int)*value;
		Graph_ClearBuff(TRUE);
	}
}


void Graph_SetTimestep(int *value)
{
	if (*value < 1)
		*value = 1;

	if (graph.timestep != ((unsigned int)*value)) {
		graph.timestep = (unsigned int)*value;
		Graph_ClearBuff(TRUE);
	}
}


void Graph_SetFormatY(TCHAR *format)
{
	if (_tcslen(format) < 8)
		_tcscpy_s(graph.format_y, 8, format);
	else
		_tcscpy_s(graph.format_y, 8, TEXT("ERROR"));
}


void Graph_AddBuffValue(int value)
{
	if (value > graph.max_value)
		value = graph.max_value;
	else if (value < graph.min_value)
		value = graph.min_value;

	if (graph.buff_pos >= graph.buff_sz)
		Graph_ClearBuff(FALSE);
	graph.buff[graph.buff_pos++] = (float)value;

	Graph_Redraw();
}


void Graph_AddBuffValueF(float value)
{
	if (value > ((float)graph.max_value))
		value = (float)graph.max_value;
	else if (value < ((float)graph.min_value))
		value = (float)graph.min_value;

	if (graph.buff_pos >= graph.buff_sz)
		Graph_ClearBuff(FALSE);
	graph.buff[graph.buff_pos++] = value;

	Graph_Redraw();
}


void Graph_ClearBuff(BOOL full)
{
	graph.buff_pos =0;
	graph.full_clear = full;
	Graph_Redraw();
}


static void Graph_Redraw(void)
{
	InvalidateRect(graph.hwnd, &graph.rect, TRUE);
}


static int Graph_CheckMarkValue(TCHAR *txt_buff, int buff_sz, int max_len, BOOL is_sec)
{
	int l = _tcslen(txt_buff);
	if (l > max_len) {
		if (is_sec) {
			float val = (float)((StrToInt(txt_buff)) / 1000.0f);
			_stprintf(txt_buff, buff_sz, TEXT("%0.2f"), val);
			l = _tcslen(txt_buff);
		}
		if (l > max_len) {
			l = max_len;
			txt_buff[l] = '\0';
			if (txt_buff[l-1] == '.')
				txt_buff[--l] = '\0';
		}
	}

	return l;
}

void Graph_Paint(HDC *p_hdc)
{
	int i, n, l, txt_buff_sz = 32;
	TCHAR txt_buff[txt_buff_sz];

	FillRect(*p_hdc, &graph.rect, graph.hbr_back);

	int zpo = 32; //zero point offset
	int	gzx = graph.x0 + zpo, gzy = graph.y1 - zpo;  //graph zero x-y
	Graph_PaintCircle(p_hdc, &graph.hpn_black, &graph.hbr_black, gzx, gzy, 5);

	int mpo = 10, mpof = mpo - 2; //max point offset
	int gmx = gzx + graph.w - zpo - mpo, gmy = gzy - graph.h + zpo + mpo; //max x-y
	Graph_PaintLine(p_hdc, &graph.hpn_black, gzx, gzy, gmx+mpof, gzy); //x-axis
	Graph_PaintLine(p_hdc, &graph.hpn_black, gzx, gzy, gzx, gmy-mpof); //y-axis

	int gmh = 3; //mark height
	int gtw = gmx - gzx, gth = gzy - gmy; //total width-height
	int gal = 5; //arrow length
	int tox = 7; //text offset x
	int toy = 5; //text offset y
	int tlc = 5; //text len coeff
	int gmss = 10; //min step size
	//int gpsx = Graph_Min(gtw / graph.buff_sz, gmss); //pixel step x
	//int gisx = graph.buff_sz / ((gtw-gpsx) / gpsx); //int step value x
	float gsvx = (float)(((float)(graph.timestep * graph.buff_sz)) / ((float)gtw / (float)gmss)); //step value y
	float gsvy = (float)((float)(graph.max_value - graph.min_value)) / ((float)gth / (float)gmss); //step value y
	n = 0;
	for (i = gmss; i < gtw; i+= gmss) { //draw x-marks
		Graph_PaintLine(p_hdc, &graph.hpn_black, gzx+i, gzy-gmh, gzx+i, gzy+gmh);
		_stprintf(txt_buff, txt_buff_sz, TEXT("%0.2f"), (float)(gsvx*((float)(++n))));
		l = Graph_CheckMarkValue(txt_buff, txt_buff_sz, 5, TRUE);
		Graph_PaintTextRotated(p_hdc, gzx+i-tox, gzy+toy+tlc*l, 90, 5, 12, FALSE, FALSE, FW_REGULAR, txt_buff, CR_Black);
	}
	n = 0;
	tox = 4;
	toy = 5;
	for (i = gmss; i < gth; i+= gmss) { //draw y-marks
		Graph_PaintLine(p_hdc, &graph.hpn_black, gzx-gmh, gzy-i, gzx+gmh, gzy-i);
		_stprintf(txt_buff, txt_buff_sz, TEXT("%0.2f"), (float)(((float)graph.min_value)+gsvy*((float)(++n))));
		l = Graph_CheckMarkValue(txt_buff, txt_buff_sz, 5, FALSE);
		Graph_PaintTextRotated(p_hdc, gzx-tox-tlc*l, gzy-i-toy, 0, 5, 12, FALSE, FALSE, FW_REGULAR, txt_buff, CR_Black);
	}

	Graph_PaintLine(p_hdc, &graph.hpn_black, gmx+mpof, gzy, gmx+mpof-gal, gzy-gal); //x-axis arrow
	Graph_PaintLine(p_hdc, &graph.hpn_black, gmx+mpof, gzy, gmx+mpof-gal, gzy+gal); //x-axis arrow
	Graph_PaintLine(p_hdc, &graph.hpn_black, gzx, gmy-mpof, gzx-gal, gmy-mpof+gal); //y-axis arrow
	Graph_PaintLine(p_hdc, &graph.hpn_black, gzx, gmy-mpof, gzx+gal, gmy-mpof+gal); //y-axis arrow

	int gub = (graph.full_clear ? graph.buff_pos : (int)graph.buff_sz); //upper bound
	int gcpx = gzx, gcpy = gzy, gcpx0, gcpy0; //cur pos x-y
	BOOL is_glp; //is last pos
	for (i = 0; i < gub; ++i) {
		gcpx0 = gcpx;
		gcpx = gzx + (int)((float)i*((float)(((float)gtw)/((float)graph.buff_sz))));
		gcpy0 = gcpy;
		gcpy = gzy - (int) ( (float)gth*((float)(((graph.buff[i] < graph.min_value) ? graph.min_value : graph.buff[i]) - graph.min_value)/(float)(graph.max_value-graph.min_value)) );
		
		if (i != graph.buff_pos)
			Graph_PaintLine(p_hdc, &graph.hpn_black, gcpx0, gcpy0, gcpx, gcpy);

		is_glp = (i == (graph.buff_pos-1) ? TRUE : FALSE);
		if ( (is_glp) && (graph.buff[i] != 0.0f) ) {
			SetBkMode(*p_hdc, TRANSPARENT); 
			Graph_PaintLine(p_hdc, &graph.hpn_dashed, gcpx, gcpy+4, gcpx, gzy);
			if (!graph.full_clear)
				Graph_PaintLine(p_hdc, &graph.hpn_dashed, gcpx, gcpy-4, gcpx, gmy);
			Graph_PaintLine(p_hdc, &graph.hpn_dashed, gcpx-4, gcpy, gzx, gcpy);
			SetBkMode(*p_hdc, OPAQUE); 
			_stprintf(txt_buff, txt_buff_sz, TEXT("(%0.2fs; %0.2f%s)"), ((float)(graph.timestep*i))/1000.0f, graph.buff[i], graph.format_y);
			Graph_PaintTextRotated(p_hdc, gcpx + 5 - ((gcpx > (gzx + gtw/2)) ? 90 : 0), gcpy-15 + ((gcpy > (gzy - gth/2)) ? 0 : 20), 0, 5, 12, TRUE, FALSE, FW_REGULAR, txt_buff, CR_Blue);
		}

		if ( (!is_glp) && ((gtw / graph.buff_sz) < 3) )
			Graph_PaintDot(p_hdc, &graph.hbr_black, gcpx, gcpy);
		else
			Graph_PaintCircle(p_hdc, &graph.hpn_black, (is_glp ? &graph.hbr_red : &graph.hbr_black), gcpx, gcpy, (is_glp ? 6 : 3));
	}

	_stprintf(txt_buff, txt_buff_sz, TEXT("X-STEP: %0.2fs"), gsvx / 1000.0f);
	Graph_PaintTextRotated(p_hdc, 330, 90, 0, 5, 12, TRUE, FALSE, FW_REGULAR, txt_buff, CR_Black);
	_stprintf(txt_buff, txt_buff_sz, TEXT("Y-STEP: %0.2f"), gsvy);
	Graph_PaintTextRotated(p_hdc, 330, 100, 0, 5, 12, TRUE, FALSE, FW_REGULAR, txt_buff, CR_Black);
}


static void Graph_PaintDot(HDC *p_hdc, HBRUSH *p_hbr, int x, int y)
{
	RECT gdrect;
	SetRect(&gdrect, x, y, x+1, y+1);

	SelectObject(*p_hdc, *p_hbr);

	FillRect(*p_hdc, &gdrect, graph.hbr_black);
}


static void Graph_PaintCircle(HDC *p_hdc, HPEN *p_hpn, HBRUSH *p_hbr, int cx, int cy, int d)
{
	SelectObject(*p_hdc, *p_hpn);
	SelectObject(*p_hdc, *p_hbr);

	int x0 = cx - d/2, y0 = cy - d/2;
	Ellipse(*p_hdc, x0, y0, x0+d, y0+d);
}


static void Graph_PaintLine(HDC *p_hdc, HPEN *p_hpn, int x0, int y0, int x1, int y1)
{
	SelectObject(*p_hdc, *p_hpn);

	MoveToEx(*p_hdc, x1, y1, NULL);
	LineTo(*p_hdc, x0, y0);
}


static void Graph_PaintTextRotated(
	HDC *p_hdc,
	int x, int y, int angle,
	BYTE width, BYTE height,
	BOOL italic, BOOL underline, LONG weight,
	TCHAR *lpszRotate,
	COLORREF color
)
{
    HGDIOBJ hfnt, hfntPrev; 
    HRESULT hr; 
    size_t pcch = _tcslen(lpszRotate);

	angle *= 10;

	PLOGFONT plf = (PLOGFONT)LocalAlloc(LPTR, sizeof(LOGFONT)); 

	hr = StringCchCopy(plf->lfFaceName, 9, TEXT("Consolas"));
	plf->lfWeight = weight;
	plf->lfHeight = height;
	plf->lfWidth = width;
	plf->lfItalic = (BYTE)italic;
	plf->lfUnderline = (BYTE)underline;
	plf->lfQuality = CLEARTYPE_QUALITY;
	SetBkMode(*p_hdc, TRANSPARENT); 
	plf->lfEscapement = angle; 
	hfnt = CreateFontIndirect(plf); 
	hfntPrev = SelectObject(*p_hdc, hfnt);
	hr = StringCchLength(lpszRotate, 255, &pcch);
	SetTextColor(*p_hdc, color);
	TextOut(*p_hdc, x, y, lpszRotate, pcch); 

	SelectObject(*p_hdc, hfntPrev); 
	DeleteObject(hfnt); 
	SetBkMode(*p_hdc, OPAQUE);

	LocalFree((LOCALHANDLE)plf); 
}
