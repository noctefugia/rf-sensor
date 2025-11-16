
#ifdef __LCC__
#define MANIFEST_RESOURCE RT_MANIFEST
#else
#define MANIFEST_RESOURCE 24
#endif

#define lblPort                                                                   3
#define lblSerialData                                                             4
#define lblUpdatePeriod                                                           5
#define lblUpdatePeriodVal                                                        6
#define lblParsedData                                                             7
#define lblGraphMin                                                               8
#define lblGraphMax                                                               9
#define lblGraphBuffSz                                                           10
#define cbPortList                                                               11
#define btnSerialPortConnect                                                     12
#define txtSerialData                                                            13
#define sldUpdatePeriod                                                          14
#define txtParsedData                                                            15
#define btnSerialPortRefresh                                                     16
#define btnSerialPortDisconnect                                                  17
#define btnSerialDataClear                                                       18
#define btnParsedDataClear                                                       19
#define udMaxPort                                                                20
#define cbGraphType                                                              21
#define txtGraphMin                                                              22
#define txtGraphMax                                                              23
#define chkLogData                                                               25
#define txtGraphBuffSz                                                           26
#define btnGraphClear                                                            27

#define CreateChildWindow(exsty,cls,cap,sty,l,t,w,h,prnt,id) CreateWindowEx \
(exsty,cls,cap,WS_CHILD|sty,l*WindowScaling(),t*WindowScaling(),w*WindowScaling(),h*WindowScaling(),prnt,(HMENU)id,hInstance,0)

double WindowScaling(void);
void CreateChildWindows(HWND hwndMainWindow, HINSTANCE hInstance);
HWND GetItem(int nIDDlgItem);
void Form_Unload(HWND hMainWnd);
void ScrollBar_Set(HWND hWndParent, HWND hwndScrollBar, UINT nScrollCode, int nPos);

//  EXE Name: main

