#ifndef __MYGRAPH_H
#define __MYGRAPH_H

#define GRAPH_MAX_BUFF 1000

void Graph_Init(HWND hw, int x0, int y0, int w, int h);
void Graph_SetMinValue(int *value);
void Graph_SetMaxValue(int *value);
void Graph_SetBuffSize(int *value);
void Graph_SetTimestep(int *value);
void Graph_SetFormatY(TCHAR *format);
void Graph_AddBuffValue(int value);
void Graph_AddBuffValueF(float value);
void Graph_ClearBuff(BOOL full);
static void Graph_Redraw(void);
void Graph_Paint(HDC *p_hdc);
static void Graph_PaintCircle(HDC *p_hdc, HPEN *p_hpn, HBRUSH *p_hbr, int cx, int cy, int d);
static void Graph_PaintLine(HDC *p_hdc, HPEN *p_hpn, int x0, int y0, int x1, int y1);
void Graph_Destroy(void);
static void Graph_PaintTextRotated(HDC *p_hdc, int x, int y, int angle, BYTE width, BYTE height, BOOL italic, BOOL underline, LONG weight, TCHAR *lpszRotate, COLORREF color);
static int Graph_CheckMarkValue(TCHAR *txt_buff, int buff_sz, int max_len, BOOL is_sec);
static void Graph_PaintDot(HDC *p_hdc, HBRUSH *p_hbr, int x, int y);

#endif /* __MYGRAPH_H */

