#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <math.h>

#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "gdi32.lib")

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

static HBRUSH CreateGradientBrush(HDC hdc, RECT rect, COLORREF color1, COLORREF color2, BOOL vertical) {    HDC hdcMem = CreateCompatibleDC(hdc);    if (!hdcMem) return NULL;        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);    if (!hBitmap) {        DeleteDC(hdcMem);        return NULL;    }        HBITMAP hOldBitmap = SelectObject(hdcMem, hBitmap);        TRIVERTEX vertices[2];    vertices[0].x = 0;    vertices[0].y = 0;    vertices[0].Red = GetRValue(color1) << 8;    vertices[0].Green = GetGValue(color1) << 8;    vertices[0].Blue = GetBValue(color1) << 8;    vertices[0].Alpha = 0x0000;        vertices[1].x = rect.right - rect.left;    vertices[1].y = rect.bottom - rect.top;    vertices[1].Red = GetRValue(color2) << 8;    vertices[1].Green = GetGValue(color2) << 8;    vertices[1].Blue = GetBValue(color2) << 8;    vertices[1].Alpha = 0x0000;        GRADIENT_RECT gradientRect;    gradientRect.UpperLeft = 0;    gradientRect.LowerRight = 1;        GradientFill(hdcMem, vertices, 2, &gradientRect, 1,                 vertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);        HBRUSH hBrush = CreatePatternBrush(hBitmap);        SelectObject(hdcMem, hOldBitmap);    DeleteObject(hBitmap);    DeleteDC(hdcMem);        return hBrush;}

static void DrawRoundedRect(HDC hdc, RECT rect, int radius, HBRUSH hBrush, HPEN hPen) {
    HPEN hOldPen = SelectObject(hdc, hPen);
    HBRUSH hOldBrush = SelectObject(hdc, hBrush);
    
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
}

static COLORREF BlendColors(COLORREF color1, COLORREF color2, float ratio) {
    BYTE r1 = GetRValue(color1), g1 = GetGValue(color1), b1 = GetBValue(color1);
    BYTE r2 = GetRValue(color2), g2 = GetGValue(color2), b2 = GetBValue(color2);
    
    BYTE r = (BYTE)(r1 * (1.0f - ratio) + r2 * ratio);
    BYTE g = (BYTE)(g1 * (1.0f - ratio) + g2 * ratio);
    BYTE b = (BYTE)(b1 * (1.0f - ratio) + b2 * ratio);
    
    return RGB(r, g, b);
}

static void DrawSignalBar(HDC hdc, RECT rect, int signalStrength, BOOL isConnected, const char* label) {
    int barHeight = rect.bottom - rect.top - 30;
    int barWidth = (rect.right - rect.left - 10) / 5;
    
    COLORREF strongColor = isConnected ? RGB(0, 200, 0) : RGB(0, 150, 255);
    COLORREF mediumColor = RGB(255, 200, 0);
    COLORREF weakColor = RGB(255, 100, 100);
    COLORREF emptyColor = RGB(60, 60, 60);
    
    int signalBars = 0;
    if (signalStrength > -30) signalBars = 5;
    else if (signalStrength > -50) signalBars = 4;
    else if (signalStrength > -60) signalBars = 3;
    else if (signalStrength > -70) signalBars = 2;
    else if (signalStrength > -80) signalBars = 1;
    
    for (int i = 0; i < 5; i++) {
        RECT barRect;
        barRect.left = rect.left + 5 + i * (barWidth + 2);
        barRect.right = barRect.left + barWidth;
        barRect.bottom = rect.bottom - 20;
        barRect.top = barRect.bottom - ((barHeight * (i + 1)) / 5);
        
        COLORREF barColor;
        if (i < signalBars) {
            if (signalBars >= 4) barColor = strongColor;
            else if (signalBars >= 2) barColor = mediumColor;
            else barColor = weakColor;
        } else {
            barColor = emptyColor;
        }
        
        HBRUSH hBrush = CreateSolidBrush(barColor);
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
        
        DrawRoundedRect(hdc, barRect, 3, hBrush, hPen);
        
        DeleteObject(hBrush);
        DeleteObject(hPen);
    }
    
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);
    
        HFONT hFont = CreateFontA(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,                             DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,                             CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");    HFONT hOldFont = SelectObject(hdc, hFont);        RECT textRect = {rect.left, rect.bottom - 18, rect.right, rect.bottom};    DrawTextA(hdc, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

void DrawSignalGraph(HDC hdc, RECT rect, WiFiNetworkList* networks) {    int margin = 25;    int titleHeight = 45;    int legendHeight = 35;        HBRUSH hBackBrush = CreateGradientBrush(hdc, rect, RGB(18, 20, 24), RGB(28, 32, 38), TRUE);    FillRect(hdc, &rect, hBackBrush);    DeleteObject(hBackBrush);        HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(65, 70, 80));    HPEN hOldPen = SelectObject(hdc, hBorderPen);    HBRUSH hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 12, 12);    SelectObject(hdc, hOldBrush);    SelectObject(hdc, hOldPen);    DeleteObject(hBorderPen);        HFONT hTitleFont = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,                                  DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,                                  CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");    HFONT hOldFont = SelectObject(hdc, hTitleFont);        SetTextColor(hdc, RGB(240, 245, 250));    SetBkMode(hdc, TRANSPARENT);        RECT titleRect = {rect.left + margin, rect.top + 15, rect.right - margin, rect.top + titleHeight};    DrawTextA(hdc, "📊 Network Signal Analysis", -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);        SelectObject(hdc, hOldFont);    DeleteObject(hTitleFont);        if (!networks || networks->count == 0) {        HFONT hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,                                 DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,                                 CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");        hOldFont = SelectObject(hdc, hFont);                SetTextColor(hdc, RGB(150, 155, 165));        RECT noDataRect = {rect.left + margin, rect.top + titleHeight + 20,                           rect.right - margin, rect.bottom - margin};        DrawTextA(hdc, "🔍 No WiFi networks detected\n\nClick 'Refresh Networks' to scan for available networks",                  -1, &noDataRect, DT_CENTER | DT_VCENTER | DT_WORDBREAK);                SelectObject(hdc, hOldFont);        DeleteObject(hFont);        return;    }        RECT chartRect = {rect.left + margin, rect.top + titleHeight + 20,                      rect.right - margin, rect.bottom - legendHeight - margin};        int chartWidth = chartRect.right - chartRect.left;    int chartHeight = chartRect.bottom - chartRect.top;    int maxNetworks = min(6, networks->count);        if (maxNetworks > 0) {        HPEN hGridPen = CreatePen(PS_DOT, 1, RGB(45, 50, 60));        hOldPen = SelectObject(hdc, hGridPen);                for (int i = 1; i <= 4; i++) {            int y = chartRect.top + (chartHeight * i) / 5;            MoveToEx(hdc, chartRect.left, y, NULL);            LineTo(hdc, chartRect.right, y);        }                SelectObject(hdc, hOldPen);        DeleteObject(hGridPen);                HFONT hLabelFont = CreateFontA(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,                                      DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,                                      CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");        hOldFont = SelectObject(hdc, hLabelFont);                SetTextColor(hdc, RGB(120, 125, 135));                const char* strengthLabels[] = {"Excellent", "Good", "Fair", "Weak", "Poor"};        for (int i = 0; i <= 4; i++) {            int y = chartRect.top + (chartHeight * i) / 5;            RECT labelRect = {rect.left + 5, y - 8, chartRect.left - 5, y + 8};            DrawTextA(hdc, strengthLabels[i], -1, &labelRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);        }                SelectObject(hdc, hOldFont);        DeleteObject(hLabelFont);                int barWidth = (chartWidth - (maxNetworks + 1) * 12) / maxNetworks;        int maxBarWidth = 65;        if (barWidth > maxBarWidth) barWidth = maxBarWidth;                for (int i = 0; i < maxNetworks; i++) {            WiFiNetwork* net = &networks->networks[i];                        int x = chartRect.left + 12 + i * (barWidth + 12);                        int signalPercent = (net->signal_strength + 100) * 100 / 70;            if (signalPercent < 0) signalPercent = 5;            if (signalPercent > 100) signalPercent = 100;                        int barHeight = (chartHeight * signalPercent) / 100;                        RECT barRect;            barRect.left = x;            barRect.right = x + barWidth;            barRect.bottom = chartRect.bottom;            barRect.top = chartRect.bottom - barHeight;                        COLORREF barColor;            if (net->is_connected) {                barColor = RGB(40, 200, 80);            } else if (net->signal_strength > -50) {                barColor = RGB(30, 150, 255);            } else if (net->signal_strength > -65) {                barColor = RGB(255, 165, 0);            } else if (net->signal_strength > -80) {                barColor = RGB(255, 95, 95);            } else {                barColor = RGB(180, 60, 60);            }                        HBRUSH hBarBrush = CreateGradientBrush(hdc, barRect,                                                   BlendColors(barColor, RGB(255, 255, 255), 0.2f),                                                  BlendColors(barColor, RGB(0, 0, 0), 0.15f), TRUE);            HPEN hBarPen = CreatePen(PS_SOLID, 2, BlendColors(barColor, RGB(0, 0, 0), 0.3f));                        if (hBarBrush && hBarPen) {                DrawRoundedRect(hdc, barRect, 6, hBarBrush, hBarPen);                                RECT shadowRect = {barRect.left + 2, barRect.top + 2, barRect.right + 2, barRect.bottom + 2};                HBRUSH hShadowBrush = CreateSolidBrush(RGB(0, 0, 0));                if (hShadowBrush) {                    HRGN hClipRgn = CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);                    SelectClipRgn(hdc, hClipRgn);                                        SelectObject(hdc, hShadowBrush);                    SelectObject(hdc, GetStockObject(NULL_PEN));                    RoundRect(hdc, shadowRect.left, shadowRect.top, shadowRect.right, shadowRect.bottom, 6, 6);                                        SelectClipRgn(hdc, NULL);                    DeleteObject(hClipRgn);                    DeleteObject(hShadowBrush);                }                                DrawRoundedRect(hdc, barRect, 6, hBarBrush, hBarPen);            }                        if (hBarBrush) DeleteObject(hBarBrush);            if (hBarPen) DeleteObject(hBarPen);                        HFONT hNetworkFont = CreateFontA(9, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,                                           DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,                                           CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");            hOldFont = SelectObject(hdc, hNetworkFont);                        SetTextColor(hdc, RGB(220, 225, 235));                        char label[32];            if (strlen(net->ssid) > 0) {                strncpy_s(label, sizeof(label), net->ssid, 10);                if (strlen(net->ssid) > 10) {                    strcpy_s(label + 10, sizeof(label) - 10, "...");                }            } else {                strcpy_s(label, sizeof(label), "Hidden");            }                        RECT nameRect = {x - 8, chartRect.bottom + 8, x + barWidth + 8, chartRect.bottom + 25};            DrawTextA(hdc, label, -1, &nameRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);                        char strengthText[16];            sprintf_s(strengthText, sizeof(strengthText), "%d dBm", net->signal_strength);                        SetTextColor(hdc, RGB(160, 170, 185));            RECT strengthRect = {x - 8, chartRect.bottom + 25, x + barWidth + 8, chartRect.bottom + 40};            DrawTextA(hdc, strengthText, -1, &strengthRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);                        if (net->is_connected) {                SetTextColor(hdc, RGB(255, 255, 255));                RECT connRect = {barRect.left + 2, barRect.top + 8, barRect.right - 2, barRect.top + 20};                HBRUSH hConnBrush = CreateSolidBrush(RGB(40, 200, 80));                FillRect(hdc, &connRect, hConnBrush);                DeleteObject(hConnBrush);                DrawTextA(hdc, "●", -1, &connRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);            }                        SelectObject(hdc, hOldFont);            DeleteObject(hNetworkFont);        }                RECT legendRect = {rect.left + margin, rect.bottom - legendHeight,                           rect.right - margin, rect.bottom - 10};                HFONT hLegendFont = CreateFontA(10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,                                       DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,                                       CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");        hOldFont = SelectObject(hdc, hLegendFont);                SetTextColor(hdc, RGB(140, 150, 165));                char legendText[128];        sprintf_s(legendText, sizeof(legendText),                  "📶 Displaying top %d networks • Green = Connected • Blue = Excellent • Orange = Good • Red = Weak",                  maxNetworks);        DrawTextA(hdc, legendText, -1, &legendRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);                SelectObject(hdc, hOldFont);        DeleteObject(hLegendFont);    }}

void DrawModernButton(HDC hdc, RECT rect, const char* text, BOOL pressed, BOOL enabled) {    if (!hdc || !text) return;        COLORREF baseColor, shadowColor;        if (strstr(text, "🔄") != NULL) {        baseColor = enabled ? RGB(45, 125, 70) : RGB(85, 85, 85);    } else if (strstr(text, "💾") != NULL) {        baseColor = enabled ? RGB(70, 110, 180) : RGB(85, 85, 85);    } else {        baseColor = enabled ? RGB(0, 122, 204) : RGB(85, 85, 85);    }        shadowColor = RGB(0, 0, 0);        RECT shadowRect = rect;    OffsetRect(&shadowRect, 2, 2);        HBRUSH hShadowBrush = CreateSolidBrush(shadowColor);    if (hShadowBrush) {        HRGN hClipRgn = CreateRectRgn(rect.left - 5, rect.top - 5, rect.right + 5, rect.bottom + 5);        SelectClipRgn(hdc, hClipRgn);                HPEN hShadowPen = CreatePen(PS_SOLID, 1, shadowColor);        HPEN hOldPen = SelectObject(hdc, hShadowPen);        HBRUSH hOldBrush = SelectObject(hdc, hShadowBrush);                RoundRect(hdc, shadowRect.left, shadowRect.top, shadowRect.right, shadowRect.bottom, 12, 12);                SelectObject(hdc, hOldBrush);        SelectObject(hdc, hOldPen);        DeleteObject(hShadowPen);        DeleteObject(hShadowBrush);                SelectClipRgn(hdc, NULL);        DeleteObject(hClipRgn);    }        COLORREF topColor, bottomColor, borderColor, textColor;        if (pressed && enabled) {        topColor = BlendColors(baseColor, RGB(0, 0, 0), 0.25f);        bottomColor = BlendColors(baseColor, RGB(0, 0, 0), 0.1f);        borderColor = BlendColors(baseColor, RGB(0, 0, 0), 0.4f);        OffsetRect(&rect, 1, 1);    } else if (enabled) {        topColor = BlendColors(baseColor, RGB(255, 255, 255), 0.15f);        bottomColor = BlendColors(baseColor, RGB(0, 0, 0), 0.15f);        borderColor = BlendColors(baseColor, RGB(0, 0, 0), 0.25f);    } else {        topColor = bottomColor = baseColor;        borderColor = RGB(120, 120, 120);    }        textColor = enabled ? RGB(255, 255, 255) : RGB(160, 160, 160);        HBRUSH hBrush = CreateGradientBrush(hdc, rect, topColor, bottomColor, TRUE);    HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);        if (hBrush && hPen) {        DrawRoundedRect(hdc, rect, 12, hBrush, hPen);                if (enabled && !pressed) {            RECT glowRect = {rect.left + 2, rect.top + 2, rect.right - 2, rect.top + 4};            HBRUSH hGlowBrush = CreateSolidBrush(BlendColors(topColor, RGB(255, 255, 255), 0.6f));            if (hGlowBrush) {                HPEN hGlowPen = CreatePen(PS_SOLID, 1, BlendColors(topColor, RGB(255, 255, 255), 0.6f));                HPEN hOldGlowPen = SelectObject(hdc, hGlowPen);                HBRUSH hOldGlowBrush = SelectObject(hdc, hGlowBrush);                                RoundRect(hdc, glowRect.left, glowRect.top, glowRect.right, glowRect.bottom, 8, 8);                                SelectObject(hdc, hOldGlowBrush);                SelectObject(hdc, hOldGlowPen);                DeleteObject(hGlowPen);                DeleteObject(hGlowBrush);            }        }    }        if (hBrush) DeleteObject(hBrush);    if (hPen) DeleteObject(hPen);        SetTextColor(hdc, textColor);    SetBkMode(hdc, TRANSPARENT);        HFONT hFont = CreateFontA(13, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,                             DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,                             CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");    if (hFont) {        HFONT hOldFont = SelectObject(hdc, hFont);                RECT textRect = rect;        InflateRect(&textRect, -8, -2);                DrawTextA(hdc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);                SelectObject(hdc, hOldFont);        DeleteObject(hFont);    }}

void ApplyModernStyling(HWND hwnd) {
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, 250, LWA_ALPHA);
    
    RECT rect;
    GetWindowRect(hwnd, &rect);
    
    HRGN hRgn = CreateRoundRectRgn(0, 0, rect.right - rect.left, rect.bottom - rect.top, 20, 20);
    SetWindowRgn(hwnd, hRgn, TRUE);
    
    HDC hdc = GetDC(hwnd);
    if (hdc) {
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        
        HBRUSH hHeaderBrush = CreateGradientBrush(hdc, clientRect, RGB(40, 40, 40), RGB(20, 20, 20), TRUE);
        
        RECT headerRect = {0, 0, clientRect.right, 50};
        FillRect(hdc, &headerRect, hHeaderBrush);
        
        DeleteObject(hHeaderBrush);
        
        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        
                HFONT hTitleFont = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,                                      DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,                                      CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");        HFONT hOldFont = SelectObject(hdc, hTitleFont);                RECT titleRect = {20, 10, clientRect.right - 20, 40};        DrawTextA(hdc, "Professional WiFi Scanner", -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        
        HPEN hAccentPen = CreatePen(PS_SOLID, 3, RGB(0, 120, 215));
        HPEN hOldPen = SelectObject(hdc, hAccentPen);
        
        MoveToEx(hdc, 20, 45, NULL);
        LineTo(hdc, clientRect.right - 20, 45);
        
        SelectObject(hdc, hOldPen);
        DeleteObject(hAccentPen);
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hTitleFont);
        
        ReleaseDC(hwnd, hdc);
    }
} 