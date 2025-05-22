#define WIN32_LEAN_AND_MEAN#include <windows.h>#include <commctrl.h>#include <stdio.h>#include <math.h>#include "colors.h"

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



// Function declarations
static void InitializeGUICache(void);
void CleanupGUICache(void);
static HBRUSH CreateModernGradient(HDC hdc, RECT rect, COLORREF color1, COLORREF color2, BOOL vertical);
static void DrawModernCard(HDC hdc, RECT rect, COLORREF backgroundColor, int cornerRadius);
static void DrawGlowEffect(HDC hdc, RECT rect, COLORREF glowColor, int intensity);
static COLORREF BlendColors(COLORREF color1, COLORREF color2, float ratio);

// Enhanced resource cache
static HBRUSH g_cachedBrushes[10] = {0};
static HFONT g_cachedFonts[5] = {0};
static HPEN g_cachedPens[5] = {0};

static void InitializeGUICache(void) {
    // Create modern font family with proper sizes
    g_cachedFonts[0] = CreateFontA(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");  // Title
    
    g_cachedFonts[1] = CreateFontA(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");  // Headers
    
    g_cachedFonts[2] = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");  // Body
    
    g_cachedFonts[3] = CreateFontA(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");  // Small
    
    g_cachedFonts[4] = CreateFontA(18, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, VARIABLE_PITCH, "Segoe UI");  // Button
    
        // Create modern brushes    g_cachedBrushes[0] = CreateSolidBrush(APP_COLOR_BACKGROUND);
    g_cachedBrushes[1] = CreateSolidBrush(COLOR_PANEL);
    g_cachedBrushes[2] = CreateSolidBrush(COLOR_ACCENT);
    g_cachedBrushes[3] = CreateSolidBrush(COLOR_SUCCESS);
    g_cachedBrushes[4] = CreateSolidBrush(COLOR_WARNING);
    g_cachedBrushes[5] = CreateSolidBrush(COLOR_ERROR);
    
    // Create modern pens
    g_cachedPens[0] = CreatePen(PS_SOLID, 1, COLOR_BORDER);
    g_cachedPens[1] = CreatePen(PS_SOLID, 2, COLOR_ACCENT);
    g_cachedPens[2] = CreatePen(PS_SOLID, 1, COLOR_SUCCESS);
    g_cachedPens[3] = CreatePen(PS_SOLID, 1, COLOR_WARNING);
    g_cachedPens[4] = CreatePen(PS_SOLID, 1, COLOR_ERROR);
}

void CleanupGUICache(void) {
    for (int i = 0; i < 5; i++) {
        if (g_cachedFonts[i]) {
            DeleteObject(g_cachedFonts[i]);
            g_cachedFonts[i] = NULL;
        }
    }
    
    for (int i = 0; i < 10; i++) {
        if (g_cachedBrushes[i]) {
            DeleteObject(g_cachedBrushes[i]);
            g_cachedBrushes[i] = NULL;
        }
    }
    
    for (int i = 0; i < 5; i++) {
        if (g_cachedPens[i]) {
            DeleteObject(g_cachedPens[i]);
            g_cachedPens[i] = NULL;
        }
    }
}

static HBRUSH CreateModernGradient(HDC hdc, RECT rect, COLORREF color1, COLORREF color2, BOOL vertical) {
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    if (width <= 0 || height <= 0) return NULL;
    
    HDC hdcMem = CreateCompatibleDC(hdc);
    if (!hdcMem) return NULL;
    
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
    if (!hBitmap) {
        DeleteDC(hdcMem);
        return NULL;
    }
    
    HBITMAP hOldBitmap = SelectObject(hdcMem, hBitmap);
    
    TRIVERTEX vertices[2];
    vertices[0].x = 0;
    vertices[0].y = 0;
    vertices[0].Red = GetRValue(color1) << 8;
    vertices[0].Green = GetGValue(color1) << 8;
    vertices[0].Blue = GetBValue(color1) << 8;
    vertices[0].Alpha = 0x0000;
    
    vertices[1].x = width;
    vertices[1].y = height;
    vertices[1].Red = GetRValue(color2) << 8;
    vertices[1].Green = GetGValue(color2) << 8;
    vertices[1].Blue = GetBValue(color2) << 8;
    vertices[1].Alpha = 0x0000;
    
    GRADIENT_RECT gradientRect = {0, 1};
    
    GradientFill(hdcMem, vertices, 2, &gradientRect, 1,
                 vertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);
    
    HBRUSH hBrush = CreatePatternBrush(hBitmap);
    
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    
    return hBrush;
}

static void DrawModernCard(HDC hdc, RECT rect, COLORREF backgroundColor, int cornerRadius) {
    HBRUSH hBrush = CreateSolidBrush(backgroundColor);
    HPEN hPen = CreatePen(PS_SOLID, 1, COLOR_BORDER);
    
    HPEN hOldPen = SelectObject(hdc, hPen);
    HBRUSH hOldBrush = SelectObject(hdc, hBrush);
    
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, cornerRadius, cornerRadius);
    
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

static COLORREF BlendColors(COLORREF color1, COLORREF color2, float ratio) {
    BYTE r1 = GetRValue(color1), g1 = GetGValue(color1), b1 = GetBValue(color1);
    BYTE r2 = GetRValue(color2), g2 = GetGValue(color2), b2 = GetBValue(color2);
    
    BYTE r = (BYTE)(r1 * (1.0f - ratio) + r2 * ratio);
    BYTE g = (BYTE)(g1 * (1.0f - ratio) + g2 * ratio);
    BYTE b = (BYTE)(b1 * (1.0f - ratio) + b2 * ratio);
    
    return RGB(r, g, b);
}

void DrawSignalGraph(HDC hdc, RECT rect, WiFiNetworkList* networks) {
    InitializeGUICache();
    
        // Modern dark background with subtle gradient    HBRUSH hBackBrush = CreateModernGradient(hdc, rect, APP_COLOR_BACKGROUND, COLOR_PANEL, TRUE);
    FillRect(hdc, &rect, hBackBrush);
    DeleteObject(hBackBrush);
    
    // Modern card-style border
    DrawModernCard(hdc, rect, COLOR_PANEL, 16);
    
    // Enhanced spacing and layout
    int margin = 32;
    int titleHeight = 60;
    int legendHeight = 50;
    
    // Beautiful title with large text
    HFONT hOldFont = SelectObject(hdc, g_cachedFonts[0]); // 24px bold
    SetTextColor(hdc, COLOR_TEXT_PRIMARY);
    SetBkMode(hdc, TRANSPARENT);
    
    RECT titleRect = {rect.left + margin, rect.top + 20, rect.right - margin, rect.top + titleHeight};
    DrawTextA(hdc, "📊 Network Signal Analyzer", -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    // Modern accent line under title
    HPEN hAccentPen = CreatePen(PS_SOLID, 3, COLOR_ACCENT);
    HPEN hOldPen = SelectObject(hdc, hAccentPen);
    MoveToEx(hdc, titleRect.left, titleRect.bottom + 8, NULL);
    LineTo(hdc, titleRect.left + 200, titleRect.bottom + 8);
    SelectObject(hdc, hOldPen);
    DeleteObject(hAccentPen);
    
    if (!networks || networks->count == 0) {
        SelectObject(hdc, g_cachedFonts[1]); // 16px semibold
        SetTextColor(hdc, COLOR_TEXT_SECONDARY);
        
        RECT noDataRect = {rect.left + margin, rect.top + titleHeight + 40,
                           rect.right - margin, rect.bottom - margin};
        DrawTextA(hdc, "🔍 No WiFi networks detected\n\nClick 'Refresh Networks' to discover available networks in your area",
                  -1, &noDataRect, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
        
        SelectObject(hdc, hOldFont);
        return;
    }
    
    RECT chartRect = {rect.left + margin, rect.top + titleHeight + 30,
                      rect.right - margin, rect.bottom - legendHeight - margin};
    
    int chartWidth = chartRect.right - chartRect.left;
    int chartHeight = chartRect.bottom - chartRect.top;
    int maxNetworks = min(6, networks->count);
    
    if (maxNetworks > 0) {
        // Modern grid lines
        HPEN hGridPen = CreatePen(PS_DOT, 1, RGB(45, 50, 60));
        hOldPen = SelectObject(hdc, hGridPen);
        
        for (int i = 1; i <= 4; i++) {
            int y = chartRect.top + (chartHeight * i) / 5;
            MoveToEx(hdc, chartRect.left, y, NULL);
            LineTo(hdc, chartRect.right, y);
        }
        
        SelectObject(hdc, hOldPen);
        DeleteObject(hGridPen);
        
        // Enhanced strength labels
        SelectObject(hdc, g_cachedFonts[2]); // 14px normal
        SetTextColor(hdc, COLOR_TEXT_SECONDARY);
        
        const char* strengthLabels[] = {"Excellent", "Good", "Fair", "Weak", "Poor"};
        for (int i = 0; i <= 4; i++) {
            int y = chartRect.top + (chartHeight * i) / 5;
            RECT labelRect = {rect.left + 5, y - 10, chartRect.left - 10, y + 10};
            DrawTextA(hdc, strengthLabels[i], -1, &labelRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        }
        
        // Enhanced network bars
        int barWidth = (chartWidth - (maxNetworks + 1) * 20) / maxNetworks;
        int maxBarWidth = 80;
        if (barWidth > maxBarWidth) barWidth = maxBarWidth;
        
        for (int i = 0; i < maxNetworks; i++) {
            WiFiNetwork* net = &networks->networks[i];
            
            int x = chartRect.left + 20 + i * (barWidth + 20);
            
            int signalPercent = (net->signal_strength + 100) * 100 / 70;
            if (signalPercent < 0) signalPercent = 5;
            if (signalPercent > 100) signalPercent = 100;
            
            int barHeight = (chartHeight * signalPercent) / 100;
            
            RECT barRect;
            barRect.left = x;
            barRect.right = x + barWidth;
            barRect.bottom = chartRect.bottom;
            barRect.top = chartRect.bottom - barHeight;
            
            // Modern color scheme for signal strength
            COLORREF barColor;
            if (net->is_connected) {
                barColor = COLOR_SUCCESS;
            } else if (net->signal_strength > -50) {
                barColor = COLOR_ACCENT;
            } else if (net->signal_strength > -65) {
                barColor = COLOR_WARNING;
            } else {
                barColor = COLOR_ERROR;
            }
            
            // Beautiful gradient bars with modern styling
            HBRUSH hBarBrush = CreateModernGradient(hdc, barRect,
                                                   BlendColors(barColor, RGB(255, 255, 255), 0.1f),
                                                   BlendColors(barColor, RGB(0, 0, 0), 0.2f), TRUE);
            
            // Modern rounded bars
            HPEN hBarPen = CreatePen(PS_SOLID, 2, BlendColors(barColor, RGB(255, 255, 255), 0.3f));
            HPEN hOldBarPen = SelectObject(hdc, hBarPen);
            HBRUSH hOldBarBrush = SelectObject(hdc, hBarBrush);
            
            RoundRect(hdc, barRect.left, barRect.top, barRect.right, barRect.bottom, 12, 12);
            
            SelectObject(hdc, hOldBarBrush);
            SelectObject(hdc, hOldBarPen);
            DeleteObject(hBarBrush);
            DeleteObject(hBarPen);
            
            // Enhanced network labels with larger text
            SelectObject(hdc, g_cachedFonts[2]); // 14px
            SetTextColor(hdc, COLOR_TEXT_PRIMARY);
            
            char label[32];
            if (strlen(net->ssid) > 0) {
                strncpy_s(label, sizeof(label), net->ssid, 12);
                if (strlen(net->ssid) > 12) {
                    strcpy_s(label + 12, sizeof(label) - 12, "...");
                }
            } else {
                strcpy_s(label, sizeof(label), "Hidden");
            }
            
            RECT nameRect = {x - 15, chartRect.bottom + 15, x + barWidth + 15, chartRect.bottom + 35};
            DrawTextA(hdc, label, -1, &nameRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            // Signal strength text with better visibility
            char strengthText[32];
            sprintf_s(strengthText, sizeof(strengthText), "%d dBm", net->signal_strength);
            
            SetTextColor(hdc, COLOR_TEXT_SECONDARY);
            RECT strengthRect = {x - 15, chartRect.bottom + 35, x + barWidth + 15, chartRect.bottom + 52};
            DrawTextA(hdc, strengthText, -1, &strengthRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            // Connection indicator
            if (net->is_connected) {
                SetTextColor(hdc, COLOR_SUCCESS);
                RECT connRect = {barRect.left, barRect.top - 25, barRect.right, barRect.top - 5};
                DrawTextA(hdc, "● CONNECTED", -1, &connRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
        }
        
        // Enhanced legend with larger text
        RECT legendRect = {rect.left + margin, rect.bottom - legendHeight + 10,
                           rect.right - margin, rect.bottom - 10};
        
        SelectObject(hdc, g_cachedFonts[2]); // 14px
        SetTextColor(hdc, COLOR_TEXT_SECONDARY);
        
        char legendText[256];
        sprintf_s(legendText, sizeof(legendText),
                  "📶 Showing %d strongest networks  •  🟢 Connected  •  🔵 Excellent  •  🟡 Good  •  🔴 Weak",
                  maxNetworks);
        DrawTextA(hdc, legendText, -1, &legendRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    
    SelectObject(hdc, hOldFont);
}

void DrawModernButton(HDC hdc, RECT rect, const char* text, BOOL pressed, BOOL enabled) {
    if (!hdc || !text) return;
    
    InitializeGUICache();
    
    // Modern button colors
    COLORREF baseColor;
    if (strstr(text, "🔄") != NULL) {
        baseColor = enabled ? COLOR_SUCCESS : RGB(75, 85, 99);
    } else if (strstr(text, "💾") != NULL) {
        baseColor = enabled ? COLOR_ACCENT : RGB(75, 85, 99);
    } else {
        baseColor = enabled ? COLOR_ACCENT : RGB(75, 85, 99);
    }
    
    // Enhanced button styling
    COLORREF topColor, bottomColor, borderColor, textColor;
    
    if (pressed && enabled) {
        topColor = BlendColors(baseColor, RGB(0, 0, 0), 0.3f);
        bottomColor = BlendColors(baseColor, RGB(0, 0, 0), 0.1f);
        borderColor = BlendColors(baseColor, RGB(0, 0, 0), 0.4f);
        OffsetRect(&rect, 2, 2);
    } else if (enabled) {
        topColor = BlendColors(baseColor, RGB(255, 255, 255), 0.2f);
        bottomColor = BlendColors(baseColor, RGB(0, 0, 0), 0.1f);
        borderColor = BlendColors(baseColor, RGB(255, 255, 255), 0.3f);
    } else {
        topColor = bottomColor = RGB(75, 85, 99);
        borderColor = RGB(107, 114, 128);
    }
    
    textColor = enabled ? COLOR_TEXT_PRIMARY : RGB(156, 163, 175);
    
    // Modern gradient button
    HBRUSH hBrush = CreateModernGradient(hdc, rect, topColor, bottomColor, TRUE);
    HPEN hPen = CreatePen(PS_SOLID, 2, borderColor);
    
    if (hBrush && hPen) {
        HPEN hOldPen = SelectObject(hdc, hPen);
        HBRUSH hOldBrush = SelectObject(hdc, hBrush);
        
        RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 16, 16);
        
        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        
        // Subtle inner highlight
        if (enabled && !pressed) {
            RECT glowRect = {rect.left + 3, rect.top + 3, rect.right - 3, rect.top + 8};
            HBRUSH hGlowBrush = CreateSolidBrush(BlendColors(topColor, RGB(255, 255, 255), 0.4f));
            if (hGlowBrush) {
                HPEN hGlowPen = CreatePen(PS_SOLID, 1, BlendColors(topColor, RGB(255, 255, 255), 0.4f));
                HPEN hOldGlowPen = SelectObject(hdc, hGlowPen);
                HBRUSH hOldGlowBrush = SelectObject(hdc, hGlowBrush);
                
                RoundRect(hdc, glowRect.left, glowRect.top, glowRect.right, glowRect.bottom, 12, 12);
                
                SelectObject(hdc, hOldGlowBrush);
                SelectObject(hdc, hOldGlowPen);
                DeleteObject(hGlowPen);
                DeleteObject(hGlowBrush);
            }
        }
    }
    
    if (hBrush) DeleteObject(hBrush);
    if (hPen) DeleteObject(hPen);
    
    // Enhanced button text
    SetTextColor(hdc, textColor);
    SetBkMode(hdc, TRANSPARENT);
    
    HFONT hOldFont = SelectObject(hdc, g_cachedFonts[4]); // 18px medium
    
    RECT textRect = rect;
    InflateRect(&textRect, -12, -4);
    
    DrawTextA(hdc, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, hOldFont);
}

void ApplyModernStyling(HWND hwnd) {
    InitializeGUICache();
    
    // Modern window styling
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, 245, LWA_ALPHA);
    
    RECT rect;
    GetWindowRect(hwnd, &rect);
    
    // Modern rounded corners
    HRGN hRgn = CreateRoundRectRgn(0, 0, rect.right - rect.left, rect.bottom - rect.top, 24, 24);
    SetWindowRgn(hwnd, hRgn, TRUE);
    
    HDC hdc = GetDC(hwnd);
    if (hdc) {
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        
        // Beautiful header gradient
        HBRUSH hHeaderBrush = CreateModernGradient(hdc, clientRect, 
                                                  RGB(37, 99, 235), RGB(29, 78, 216), FALSE);
        
        RECT headerRect = {0, 0, clientRect.right, 70};
        FillRect(hdc, &headerRect, hHeaderBrush);
        DeleteObject(hHeaderBrush);
        
        // Enhanced title
        SetTextColor(hdc, COLOR_TEXT_PRIMARY);
        SetBkMode(hdc, TRANSPARENT);
        
        HFONT hOldFont = SelectObject(hdc, g_cachedFonts[0]); // 24px bold
        
        RECT titleRect = {30, 20, clientRect.right - 30, 50};
        DrawTextA(hdc, "🌐 Professional WiFi Scanner", -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        
        // Modern accent elements
        HPEN hAccentPen = CreatePen(PS_SOLID, 4, RGB(139, 200, 255));
        HPEN hOldPen = SelectObject(hdc, hAccentPen);
        
        MoveToEx(hdc, 30, 60, NULL);
        LineTo(hdc, clientRect.right - 30, 60);
        
        SelectObject(hdc, hOldPen);
        DeleteObject(hAccentPen);
        
        SelectObject(hdc, hOldFont);
        ReleaseDC(hwnd, hdc);
    }
} 