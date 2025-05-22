#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <wlanapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define ID_TREEVIEW 1001
#define ID_REFRESH_BUTTON 1002
#define ID_SAVE_BUTTON 1003
#define ID_TIMER 1004
#define ID_SIGNAL_GRAPH 1005
#define ID_STATUS_BAR 1006

#define WINDOW_WIDTH 1400
#define WINDOW_HEIGHT 900
#define GRAPH_WIDTH 500
#define GRAPH_HEIGHT 350
#define PANEL_MARGIN 20
#define CONTROL_SPACING 15
#define HEADER_HEIGHT 80
#define SIDEBAR_WIDTH 550

typedef struct {
    char ssid[256];
    char bssid[64];
    int signal_strength;
    char security[64];
    int channel;
    int frequency;
    BOOL is_connected;
} WiFiNetwork;

typedef struct {
    WiFiNetwork* networks;
    int count;
    int capacity;
} WiFiNetworkList;

extern void InitializeWiFiScanner(void);
extern int ScanWiFiNetworks(WiFiNetworkList* list);
extern void CleanupWiFiScanner(void);
extern void CleanupNetworkList(WiFiNetworkList* list);

extern void DrawSignalGraph(HDC hdc, RECT rect, WiFiNetworkList* networks);extern void DrawModernButton(HDC hdc, RECT rect, const char* text, BOOL pressed, BOOL enabled);extern void ApplyModernStyling(HWND hwnd);extern void CleanupGUICache(void);

HWND g_hMainWindow = NULL;
HWND g_hTreeView = NULL;
HWND g_hRefreshButton = NULL;
HWND g_hSaveButton = NULL;
HWND g_hStatusBar = NULL;
HWND g_hSignalGraph = NULL;
WiFiNetworkList g_NetworkList = {0};
HFONT g_hFont = NULL;
HBRUSH g_hBackgroundBrush = NULL;
BOOL g_IsScanning = FALSE;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GraphProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateControls(HWND hwnd);
void PopulateTreeView(void);
void RefreshNetworks(void);
void SaveNetworks(void);
void UpdateStatusBar(const char* message);
void ResizeControls(HWND hwnd);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    const char* className = "WiFiScannerWindow";
    WNDCLASSA wc = {0};
    
    InitCommonControls();
    InitializeWiFiScanner();
    
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;
    wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 30));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Failed to register window class", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    g_hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");
    
    g_hBackgroundBrush = CreateSolidBrush(RGB(30, 30, 30));
    
    g_hMainWindow = CreateWindowExA(
        WS_EX_APPWINDOW,
        className,
        "Professional WiFi Scanner v1.0",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );
    
    if (!g_hMainWindow) {
        MessageBoxA(NULL, "Failed to create window", "Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    
    ApplyModernStyling(g_hMainWindow);
    ShowWindow(g_hMainWindow, nCmdShow);
    UpdateWindow(g_hMainWindow);
    
    SetTimer(g_hMainWindow, ID_TIMER, 8000, NULL);
    RefreshNetworks();
    
    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
        KillTimer(g_hMainWindow, ID_TIMER);    CleanupNetworkList(&g_NetworkList);    CleanupWiFiScanner();    CleanupGUICache();        if (g_hFont) DeleteObject(g_hFont);    if (g_hBackgroundBrush) DeleteObject(g_hBackgroundBrush);
    
    return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            CreateControls(hwnd);
            break;
            
        case WM_SIZE:
            ResizeControls(hwnd);
            break;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_REFRESH_BUTTON:
                    if (!g_IsScanning) {
                        RefreshNetworks();
                    }
                    break;
                case ID_SAVE_BUTTON:
                    SaveNetworks();
                    break;
            }
            break;
            
        case WM_TIMER:
            if (wParam == ID_TIMER && !g_IsScanning) {
                RefreshNetworks();
            }
            break;
            
        case WM_DRAWITEM:
            {
                DRAWITEMSTRUCT* pDIS = (DRAWITEMSTRUCT*)lParam;
                if (pDIS->CtlType == ODT_BUTTON) {
                    BOOL pressed = (pDIS->itemState & ODS_SELECTED) != 0;
                    BOOL enabled = (pDIS->itemState & ODS_DISABLED) == 0;
                    
                    char buttonText[256];
                    GetWindowTextA(pDIS->hwndItem, buttonText, sizeof(buttonText));
                    
                    DrawModernButton(pDIS->hDC, pDIS->rcItem, buttonText, pressed, enabled);
                    return TRUE;
                }
            }
            break;
            
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
            SetTextColor((HDC)wParam, RGB(255, 255, 255));
            SetBkColor((HDC)wParam, RGB(30, 30, 30));
            return (LRESULT)g_hBackgroundBrush;
            
        case WM_ERASEBKGND:
            {
                RECT rect;
                GetClientRect(hwnd, &rect);
                FillRect((HDC)wParam, &rect, g_hBackgroundBrush);
                return 1;
            }
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void CreateControls(HWND hwnd) {    RECT clientRect;    GetClientRect(hwnd, &clientRect);        int contentWidth = clientRect.right - (PANEL_MARGIN * 2);    int contentHeight = clientRect.bottom - HEADER_HEIGHT - 60;    int rightPanelX = clientRect.right - SIDEBAR_WIDTH - PANEL_MARGIN;        g_hTreeView = CreateWindowExA(        WS_EX_CLIENTEDGE | WS_EX_STATICEDGE,        WC_TREEVIEWA,        "",        WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT |         TVS_FULLROWSELECT | TVS_SHOWSELALWAYS,        PANEL_MARGIN,         HEADER_HEIGHT + PANEL_MARGIN,        rightPanelX - PANEL_MARGIN - CONTROL_SPACING,         contentHeight - 40,        hwnd, (HMENU)ID_TREEVIEW, GetModuleHandle(NULL), NULL    );        const char* graphClass = "SignalGraphClass";    WNDCLASSA graphWc = {0};    graphWc.lpfnWndProc = GraphProc;    graphWc.hInstance = GetModuleHandle(NULL);    graphWc.lpszClassName = graphClass;    graphWc.hbrBackground = CreateSolidBrush(RGB(25, 25, 30));    graphWc.hCursor = LoadCursor(NULL, IDC_ARROW);    RegisterClassA(&graphWc);        g_hSignalGraph = CreateWindowExA(        WS_EX_CLIENTEDGE | WS_EX_STATICEDGE,        graphClass,        "",        WS_VISIBLE | WS_CHILD,        rightPanelX,         HEADER_HEIGHT + PANEL_MARGIN,        SIDEBAR_WIDTH - PANEL_MARGIN,         GRAPH_HEIGHT,        hwnd, (HMENU)ID_SIGNAL_GRAPH, GetModuleHandle(NULL), NULL    );        int buttonY = HEADER_HEIGHT + PANEL_MARGIN + GRAPH_HEIGHT + CONTROL_SPACING;    int buttonWidth = (SIDEBAR_WIDTH - PANEL_MARGIN - CONTROL_SPACING) / 2;        g_hRefreshButton = CreateWindowA(        "BUTTON",        "🔄 Refresh Networks",        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,        rightPanelX,        buttonY,        buttonWidth,         45,        hwnd, (HMENU)ID_REFRESH_BUTTON, GetModuleHandle(NULL), NULL    );        g_hSaveButton = CreateWindowA(        "BUTTON",        "💾 Export Results",        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,        rightPanelX + buttonWidth + CONTROL_SPACING,        buttonY,        buttonWidth,         45,        hwnd, (HMENU)ID_SAVE_BUTTON, GetModuleHandle(NULL), NULL    );        g_hStatusBar = CreateWindowA(        STATUSCLASSNAMEA,        "",        WS_VISIBLE | WS_CHILD | SBARS_SIZEGRIP,        0, 0, 0, 0,        hwnd, (HMENU)ID_STATUS_BAR, GetModuleHandle(NULL), NULL    );        HFONT headerFont = CreateFontA(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,                                  DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,                                  CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");        if (g_hFont) {        SendMessage(g_hTreeView, WM_SETFONT, (WPARAM)g_hFont, TRUE);        SendMessage(g_hRefreshButton, WM_SETFONT, (WPARAM)g_hFont, TRUE);        SendMessage(g_hSaveButton, WM_SETFONT, (WPARAM)g_hFont, TRUE);        SendMessage(g_hStatusBar, WM_SETFONT, (WPARAM)g_hFont, TRUE);    }        TreeView_SetBkColor(g_hTreeView, RGB(32, 35, 42));    TreeView_SetTextColor(g_hTreeView, RGB(240, 242, 245));    TreeView_SetLineColor(g_hTreeView, RGB(70, 75, 85));        if (headerFont) DeleteObject(headerFont);        UpdateStatusBar("🔍 Professional WiFi Scanner Ready - Click Refresh to discover networks");}

LRESULT CALLBACK GraphProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                RECT rect;
                GetClientRect(hwnd, &rect);
                
                DrawSignalGraph(hdc, rect, &g_NetworkList);
                
                EndPaint(hwnd, &ps);
                return 0;
            }
        case WM_ERASEBKGND:
            return 1;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

void PopulateTreeView(void) {
    TreeView_DeleteAllItems(g_hTreeView);
    
    if (g_NetworkList.count == 0) {
        TVINSERTSTRUCTA tvis = {0};
        tvis.hParent = TVI_ROOT;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = TVIF_TEXT;
                tvis.item.pszText = "No networks found";        SendMessageA(g_hTreeView, TVM_INSERTITEMA, 0, (LPARAM)&tvis);        return;
    }
    
    for (int i = 0; i < g_NetworkList.count; i++) {
        WiFiNetwork* net = &g_NetworkList.networks[i];
        
        TVINSERTSTRUCTA tvis = {0};
        tvis.hParent = TVI_ROOT;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = TVIF_TEXT | TVIF_CHILDREN;
        tvis.item.pszText = net->ssid[0] ? net->ssid : "<Hidden Network>";
        tvis.item.cChildren = 1;
        
        HTREEITEM hParent = (HTREEITEM)SendMessageA(g_hTreeView, TVM_INSERTITEMA, 0, (LPARAM)&tvis);
        
        char details[512];
        sprintf_s(details, sizeof(details), "Signal: %d dBm (%s)",
                 net->signal_strength,
                 net->signal_strength > -50 ? "Excellent" :
                 net->signal_strength > -60 ? "Good" :
                 net->signal_strength > -70 ? "Fair" : "Weak");
        
                tvis.hParent = hParent;        tvis.item.pszText = details;        tvis.item.cChildren = 0;        SendMessageA(g_hTreeView, TVM_INSERTITEMA, 0, (LPARAM)&tvis);
        
                sprintf_s(details, sizeof(details), "BSSID: %s", net->bssid);        tvis.item.pszText = details;        SendMessageA(g_hTreeView, TVM_INSERTITEMA, 0, (LPARAM)&tvis);
        
                sprintf_s(details, sizeof(details), "Security: %s", net->security);        tvis.item.pszText = details;        SendMessageA(g_hTreeView, TVM_INSERTITEMA, 0, (LPARAM)&tvis);
        
                sprintf_s(details, sizeof(details), "Channel: %d (%.1f GHz)",                 net->channel, net->frequency / 1000.0);        tvis.item.pszText = details;        SendMessageA(g_hTreeView, TVM_INSERTITEMA, 0, (LPARAM)&tvis);
        
                if (net->is_connected) {            tvis.item.pszText = "Status: CONNECTED";            SendMessageA(g_hTreeView, TVM_INSERTITEMA, 0, (LPARAM)&tvis);        }
    }
    
    InvalidateRect(g_hSignalGraph, NULL, TRUE);
}

void RefreshNetworks(void) {
    g_IsScanning = TRUE;
    UpdateStatusBar("Scanning for networks...");
    EnableWindow(g_hRefreshButton, FALSE);
    
    int result = ScanWiFiNetworks(&g_NetworkList);
    
    if (result == 0) {
        PopulateTreeView();
        char statusMsg[256];
        time_t rawtime;
        struct tm timeinfo;
        char timeStr[64];
        time(&rawtime);
        localtime_s(&timeinfo, &rawtime);
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        sprintf_s(statusMsg, sizeof(statusMsg), "Found %d networks - Last scan: %s",
                 g_NetworkList.count, timeStr);
        UpdateStatusBar(statusMsg);
    } else {
        UpdateStatusBar("Error: Failed to scan networks");
        MessageBoxA(g_hMainWindow, "Failed to scan WiFi networks", "Scan Error", MB_OK | MB_ICONWARNING);
    }
    
    EnableWindow(g_hRefreshButton, TRUE);
    g_IsScanning = FALSE;
}

void SaveNetworks(void) {
    if (g_NetworkList.count == 0) {
        MessageBoxA(g_hMainWindow, "No networks to save", "Export", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    OPENFILENAMEA ofn = {0};
    char fileName[MAX_PATH] = "wifi_scan_results.csv";
    
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hMainWindow;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = sizeof(fileName);
    ofn.lpstrFilter = "CSV Files\0*.csv\0Text Files\0*.txt\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = "csv";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    
    if (GetSaveFileNameA(&ofn)) {
        FILE* file;
        if (fopen_s(&file, fileName, "w") == 0) {
            fprintf(file, "SSID,BSSID,Signal Strength (dBm),Security,Channel,Frequency (MHz),Connected\n");
            
            for (int i = 0; i < g_NetworkList.count; i++) {
                WiFiNetwork* net = &g_NetworkList.networks[i];
                fprintf(file, "\"%s\",\"%s\",%d,\"%s\",%d,%d,%s\n",
                       net->ssid[0] ? net->ssid : "Hidden",
                       net->bssid,
                       net->signal_strength,
                       net->security,
                       net->channel,
                       net->frequency,
                       net->is_connected ? "Yes" : "No");
            }
            
            fclose(file);
            
            char successMsg[512];
            sprintf_s(successMsg, sizeof(successMsg), "Successfully exported %d networks to:\n%s",
                     g_NetworkList.count, fileName);
            MessageBoxA(g_hMainWindow, successMsg, "Export Complete", MB_OK | MB_ICONINFORMATION);
            UpdateStatusBar("Export completed successfully");
        } else {
            MessageBoxA(g_hMainWindow, "Failed to create file", "Export Error", MB_OK | MB_ICONERROR);
        }
    }
}

void UpdateStatusBar(const char* message) {
    if (g_hStatusBar) {
        SendMessageA(g_hStatusBar, SB_SETTEXTA, 0, (LPARAM)message);
    }
}

void ResizeControls(HWND hwnd) {    RECT clientRect;    GetClientRect(hwnd, &clientRect);        int contentWidth = clientRect.right - (PANEL_MARGIN * 2);    int contentHeight = clientRect.bottom - HEADER_HEIGHT - 60;    int rightPanelX = clientRect.right - SIDEBAR_WIDTH - PANEL_MARGIN;        if (g_hTreeView) {        SetWindowPos(g_hTreeView, NULL,                     PANEL_MARGIN,                     HEADER_HEIGHT + PANEL_MARGIN,                    rightPanelX - PANEL_MARGIN - CONTROL_SPACING,                     contentHeight - 40,                    SWP_NOZORDER);    }        if (g_hSignalGraph) {        SetWindowPos(g_hSignalGraph, NULL,                    rightPanelX,                     HEADER_HEIGHT + PANEL_MARGIN,                    SIDEBAR_WIDTH - PANEL_MARGIN,                     GRAPH_HEIGHT,                    SWP_NOZORDER);    }        int buttonY = HEADER_HEIGHT + PANEL_MARGIN + GRAPH_HEIGHT + CONTROL_SPACING;    int buttonWidth = (SIDEBAR_WIDTH - PANEL_MARGIN - CONTROL_SPACING) / 2;        if (g_hRefreshButton) {        SetWindowPos(g_hRefreshButton, NULL,                    rightPanelX,                    buttonY,                    buttonWidth,                     45,                    SWP_NOZORDER);    }        if (g_hSaveButton) {        SetWindowPos(g_hSaveButton, NULL,                    rightPanelX + buttonWidth + CONTROL_SPACING,                    buttonY,                    buttonWidth,                     45,                    SWP_NOZORDER);    }        if (g_hStatusBar) {        SendMessage(g_hStatusBar, WM_SIZE, 0, 0);    }}
