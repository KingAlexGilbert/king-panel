#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#include <windows.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <setupapi.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdarg.h>
#include <stdint.h>

// Always terminate truncated labels, including unusually long driver names.
static void safeFormat(WCHAR *out, size_t capacity, const WCHAR *format, ...) {
    if (!capacity) return;
    out[0] = 0;
    va_list args;
    va_start(args, format);
    vswprintf(out, capacity, format, args);
    va_end(args);
    out[capacity - 1] = 0;
}

#define APP L"King Panel"
#define TRAY_MSG (WM_APP + 1)
#define ID_EXIT 60000
#define ID_KEEP 60001
#define ID_REVERT 60002
#define ID_HDR 60003
#define ID_SCALE_FIRST 61000
#define ID_SCALE_LAST 61999

typedef struct { WCHAR device[32]; DEVMODEW mode; } Choice;
// Menu selections only need these five fields; full DEVMODE is kept for rollback.
typedef struct {
    WCHAR device[32];
    DWORD width, height, frequency, bits, flags;
} MenuChoice;
static MenuChoice *choices;
static size_t count, capacity;
static HWND owner, confirmation, countdown;
static NOTIFYICONDATAW tray;
static UINT taskbarCreated;
// Stable notification-area identity so Windows can remember King Panel tray preferences
// across upgrades and installation-path changes. Windows still decides initial visibility.
static const GUID kingPanelTrayGuid = {
    0x2b39554d, 0x54a0, 0x4b67, {0xa7, 0x27, 0x52, 0x42, 0x77, 0x16, 0x9d, 0xe1}
};
static BOOL pending, menuOpen, menuInvalidated;
static Choice previous, requested;
static ULONGLONG deadline;
static HFONT font;

// HDR control uses Windows Display Configuration (CCD) and is resolved against
// the current Windows primary display each time. Newer Windows 11 builds expose
// HDR separately from wide-color-gamut state; older builds use Advanced Color.
#define KP_INFO_GET_ADVANCED_COLOR ((DISPLAYCONFIG_DEVICE_INFO_TYPE)9)
#define KP_INFO_SET_ADVANCED_COLOR ((DISPLAYCONFIG_DEVICE_INFO_TYPE)10)
#define KP_INFO_GET_ADVANCED_COLOR_2 ((DISPLAYCONFIG_DEVICE_INFO_TYPE)15)
#define KP_INFO_SET_HDR_STATE ((DISPLAYCONFIG_DEVICE_INFO_TYPE)16)
#define KP_ADVANCED_COLOR_MODE_HDR 2u

typedef struct {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    UINT32 value;
    UINT32 colorEncoding;
    UINT32 bitsPerColorChannel;
} KpGetAdvancedColorInfo;

typedef struct {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    UINT32 value;
    UINT32 colorEncoding;
    UINT32 bitsPerColorChannel;
    UINT32 activeColorMode;
} KpGetAdvancedColorInfo2;

typedef struct {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    UINT32 value;
} KpSetColorState;

typedef struct {
    LUID adapterId;
    UINT32 targetId;
    BOOL enabled;
    BOOL useNewApi;
} HdrTarget;

// Windows does not publish a supported per-monitor DPI scaling setter. The
// Settings app uses source-level DisplayConfig device-info packets; the -3/-4
// packet types below are intentionally isolated here so King Panel can fail
// safely if a future Windows version changes them.
#define KP_INFO_GET_DPI_SCALE ((DISPLAYCONFIG_DEVICE_INFO_TYPE)-3)
#define KP_INFO_SET_DPI_SCALE ((DISPLAYCONFIG_DEVICE_INFO_TYPE)-4)

typedef struct {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    INT32 minScaleRel;
    INT32 curScaleRel;
    INT32 maxScaleRel;
} KpGetDpiScale;

typedef struct {
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    INT32 scaleRel;
} KpSetDpiScale;

typedef struct {
    UINT32 minimum;
    UINT32 current;
    UINT32 recommended;
    UINT32 maximum;
} ScaleInfo;

typedef struct {
    LUID adapterId;
    UINT32 sourceId;
    UINT32 percent;
} ScaleChoice;

static ScaleChoice *scaleChoices;
static size_t scaleCount, scaleCapacity;
static const UINT32 scaleValues[] = {
    100, 125, 150, 175, 200, 225, 250, 300, 350, 400, 450, 500
};

// Native menu behavior with application-drawn dark colors; no private UXTheme APIs.
typedef struct MenuLabel {
    struct MenuLabel *next;
    WCHAR text[256];
    BOOL separator, submenu;
} MenuLabel;
static MenuLabel *labels;
static BOOL dark, highContrast;
static HBRUSH darkBrush;
static HFONT menuFont;
static int dpi = 96;
static int px(int value) { return MulDiv(value, dpi, 96); }
// Opt into per-monitor-v2 while constructing/showing menus so custom menu
// geometry uses the target monitor DPI. Confirmation windows retain their DPI
// behavior. All APIs are resolved from the already-loaded system user32 module.
typedef HANDLE (WINAPI *SetThreadDpiContextFn)(HANDLE);
typedef UINT (WINAPI *GetWindowDpiFn)(HWND);
typedef BOOL (WINAPI *SystemMetricsForDpiFn)(UINT, UINT, PVOID, UINT, UINT);
static SetThreadDpiContextFn setThreadDpiContext;
static GetWindowDpiFn getWindowDpi;
static SystemMetricsForDpiFn systemMetricsForDpi;
static HANDLE beginMenuDpi(POINT pt) {
    HANDLE oldContext = NULL;
    if (setThreadDpiContext) oldContext = setThreadDpiContext((HANDLE)(INT_PTR)-4);
    if (!oldContext) return NULL; // Older Windows retains system-DPI behavior.
    // An invisible, short-lived window supplies the actual DPI at the tray menu.
    // The owner window is system-aware and cannot supply per-monitor DPI itself.
    HWND probe = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, L"STATIC", L"",
        WS_POPUP, pt.x, pt.y, 1, 1, NULL, NULL, GetModuleHandleW(NULL), NULL);
    UINT targetDpi = probe && getWindowDpi ? getWindowDpi(probe) : 0;
    if (probe) DestroyWindow(probe);
    NONCLIENTMETRICSW metrics = {0}; metrics.cbSize = sizeof(metrics);
    if (!targetDpi || !systemMetricsForDpi ||
        !systemMetricsForDpi(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0, targetDpi)) {
        setThreadDpiContext(oldContext);
        return NULL;
    }
    HFONT replacement = CreateFontIndirectW(&metrics.lfMenuFont);
    if (!replacement) { setThreadDpiContext(oldContext); return NULL; }
    if (menuFont != font) DeleteObject(menuFont);
    menuFont = replacement;
    dpi = (int)targetDpi;
    return oldContext;
}
static void updateTheme(void) {
    DWORD light = 1, bytes = sizeof(light);
    HIGHCONTRASTW hc = {0}; hc.cbSize = sizeof(hc);
    SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0);
    RegGetValueW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"AppsUseLightTheme", RRF_RT_REG_DWORD, NULL, &light, &bytes);
    highContrast = (hc.dwFlags & HCF_HIGHCONTRASTON) != 0;
    dark = !light && !highContrast;
    if (confirmation) {
        DwmSetWindowAttribute(confirmation, 20, &dark, sizeof(dark));
        RedrawWindow(confirmation, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
    }
}
static void releaseLabels(void) {
    while (labels) { MenuLabel *next = labels->next; free(labels); labels = next; }
}
static void themeMenu(HMENU menu) {
    if (highContrast) return;
    MENUINFO info = {0}; info.cbSize = sizeof(info);
    info.fMask = MIM_BACKGROUND; info.hbrBack = dark ? darkBrush : GetSysColorBrush(COLOR_MENU);
    SetMenuInfo(menu, &info);
    for (int i = 0; i < GetMenuItemCount(menu); ++i) {
        MenuLabel *label = calloc(1, sizeof(*label));
        if (!label) continue;
        MENUITEMINFOW item = {0}; item.cbSize = sizeof(item);
        item.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_SUBMENU;
        item.dwTypeData = label->text; item.cch = 256;
        if (!GetMenuItemInfoW(menu, (UINT)i, TRUE, &item)) { free(label); continue; }
        label->separator = (item.fType & MFT_SEPARATOR) != 0;
        label->submenu = item.hSubMenu != NULL;
        if (item.hSubMenu) themeMenu(item.hSubMenu);
        item.fMask = MIIM_FTYPE | MIIM_DATA;
        item.fType |= MFT_OWNERDRAW;
        item.dwItemData = (ULONG_PTR)label;
        if (!SetMenuItemInfoW(menu, (UINT)i, TRUE, &item)) { free(label); continue; }
        label->next = labels; labels = label;
    }
}
static BOOL measureMenu(MEASUREITEMSTRUCT *m) {
    if (m->CtlType != ODT_MENU || !m->itemData) return FALSE;
    MenuLabel *label = (MenuLabel *)m->itemData;
    HDC dc = GetDC(owner);
    HGDIOBJ old = SelectObject(dc, menuFont);
    SIZE size = {0}; GetTextExtentPoint32W(dc, label->text, lstrlenW(label->text), &size);
    SelectObject(dc, old); ReleaseDC(owner, dc);
    m->itemWidth = (UINT)(size.cx + px(64));
    m->itemHeight = label->separator ? (UINT)px(9) : (UINT)(size.cy + px(12));
    return TRUE;
}
static BOOL drawMenu(DRAWITEMSTRUCT *d) {
    if (d->CtlType != ODT_MENU || !d->itemData) return FALSE;
    MenuLabel *label = (MenuLabel *)d->itemData;
    int saved = SaveDC(d->hDC);
    BOOL selected = (d->itemState & ODS_SELECTED) != 0;
    BOOL disabled = (d->itemState & (ODS_DISABLED | ODS_GRAYED)) != 0;
    COLORREF background = dark ? (selected && !disabled ? RGB(55,55,55) : RGB(24,24,24)) :
        GetSysColor(selected && !disabled ? COLOR_HIGHLIGHT : COLOR_MENU);
    COLORREF foreground = dark ? (disabled ? RGB(145,145,145) : RGB(240,240,240)) :
        GetSysColor(disabled ? COLOR_GRAYTEXT : selected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT);
    SetDCBrushColor(d->hDC, background);
    FillRect(d->hDC, &d->rcItem, (HBRUSH)GetStockObject(DC_BRUSH));
    if (label->separator) {
        RECT line = d->rcItem; line.left += px(10); line.right -= px(10);
        line.top = (line.top+line.bottom)/2; line.bottom = line.top+1;
        SetDCBrushColor(d->hDC, dark ? RGB(65,65,65) : GetSysColor(COLOR_3DSHADOW));
        FillRect(d->hDC, &line, (HBRUSH)GetStockObject(DC_BRUSH));
    } else {
        SetBkMode(d->hDC, TRANSPARENT);
        SetTextColor(d->hDC, foreground);
        SelectObject(d->hDC, menuFont);
        RECT text = d->rcItem; text.left += px(30); text.right -= px(26);
        DrawTextW(d->hDC, label->text, -1, &text, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
        if (d->itemState & ODS_CHECKED) {
            RECT check = d->rcItem; check.left += px(7); check.right = check.left+px(20);
            DrawTextW(d->hDC, L"\x2713", 1, &check, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
        }
        if (label->submenu) {
            // Filled geometry, not a font glyph or Windows' small stock bitmap.
            // At 300% this triangle is 24 x 36 physical pixels.
            int right = d->rcItem.right - px(10);
            int middle = (d->rcItem.top + d->rcItem.bottom) / 2;
            POINT arrow[3] = {{right-px(8), middle-px(6)},
                              {right, middle}, {right-px(8), middle+px(6)}};
            SelectObject(d->hDC, GetStockObject(NULL_PEN));
            SelectObject(d->hDC, GetStockObject(DC_BRUSH));
            SetDCBrushColor(d->hDC, foreground);
            Polygon(d->hDC, arrow, 3);
        }
    }
    RestoreDC(d->hDC, saved);
    if (label->submenu) {
        // Windows paints its stock arrow after WM_DRAWITEM returns. Exclude
        // this row's arrow gutter from that subsequent pass to avoid two arrows.
        // Keep this exclusion after RestoreDC; restoring it would undo suppression.
        ExcludeClipRect(d->hDC, d->rcItem.right-px(26), d->rcItem.top,
                        d->rcItem.right+px(32), d->rcItem.bottom);
    }
    return TRUE;
}
static BOOL drawButton(DRAWITEMSTRUCT *d) {
    if (d->CtlType != ODT_BUTTON) return FALSE;
    int saved = SaveDC(d->hDC);
    RECT r = d->rcItem;
    if (dark) {
        SetDCBrushColor(d->hDC, (d->itemState & ODS_SELECTED) ? RGB(75,75,75) : RGB(48,48,48));
        FillRect(d->hDC, &r, (HBRUSH)GetStockObject(DC_BRUSH));
        SetDCBrushColor(d->hDC, RGB(100,100,100));
        FrameRect(d->hDC, &r, (HBRUSH)GetStockObject(DC_BRUSH));
    } else DrawFrameControl(d->hDC, &r, DFC_BUTTON, DFCS_BUTTONPUSH |
                           ((d->itemState & ODS_SELECTED) ? DFCS_PUSHED : 0));
    WCHAR text[64]; GetWindowTextW(d->hwndItem, text, 64);
    SelectObject(d->hDC, font); SetBkMode(d->hDC, TRANSPARENT);
    SetTextColor(d->hDC, dark ? RGB(240,240,240) : GetSysColor(COLOR_BTNTEXT));
    DrawTextW(d->hDC, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if ((d->itemState & ODS_FOCUS) && !(d->itemState & ODS_NOFOCUSRECT)) {
        InflateRect(&r, -4, -4); DrawFocusRect(d->hDC, &r);
    }
    RestoreDC(d->hDC, saved); return TRUE;
}

static void error(const WCHAR *what, LONG code) {
    WCHAR msg[512];
    safeFormat(msg, 512, L"%ls\n\nWindows display status: %ld", what, code);
    MessageBoxW(owner, msg, APP, MB_OK | MB_ICONERROR);
}
static BOOL current(const WCHAR *device, DEVMODEW *mode) {
    ZeroMemory(mode, sizeof(*mode)); mode->dmSize = sizeof(*mode);
    return EnumDisplaySettingsExW(device, ENUM_CURRENT_SETTINGS, mode, 0);
}

// Resolve the same user-facing monitor title Windows exposes for the PnP monitor
// device (the name shown under Device Manager > Monitors). EnumDisplayDevices with
// EDD_GET_DEVICE_INTERFACE_NAME gives the monitor interface path, which Microsoft
// documents as the bridge between the GDI display and its SetupAPI device.
static BOOL setupMonitorProperty(HDEVINFO infoSet, SP_DEVINFO_DATA *deviceInfo,
                                 DWORD property, WCHAR *out, DWORD outChars) {
    DWORD type = 0, required = 0;
    if (!out || !outChars) return FALSE;
    out[0] = L'\0';
    if (!SetupDiGetDeviceRegistryPropertyW(infoSet, deviceInfo, property, &type,
            (PBYTE)out, outChars * sizeof(WCHAR), &required)) return FALSE;
    out[outChars - 1] = L'\0';
    return (type == REG_SZ || type == REG_EXPAND_SZ) && out[0] != L'\0';
}

static BOOL deviceManagerMonitorName(const WCHAR *gdiDeviceName, WCHAR *out, DWORD outChars) {
    if (!out || !outChars) return FALSE;
    out[0] = L'\0';

    DISPLAY_DEVICEW monitor = {0};
    monitor.cb = sizeof(monitor);
    if (!EnumDisplayDevicesW(gdiDeviceName, 0, &monitor, EDD_GET_DEVICE_INTERFACE_NAME) ||
        !monitor.DeviceID[0]) return FALSE;

    HDEVINFO infoSet = SetupDiCreateDeviceInfoList(NULL, NULL);
    if (infoSet == INVALID_HANDLE_VALUE) return FALSE;

    BOOL found = FALSE;
    SP_DEVICE_INTERFACE_DATA interfaceData = {0};
    interfaceData.cbSize = sizeof(interfaceData);
    if (SetupDiOpenDeviceInterfaceW(infoSet, monitor.DeviceID, 0, &interfaceData)) {
        SP_DEVINFO_DATA deviceInfo = {0};
        deviceInfo.cbSize = sizeof(deviceInfo);
        if (SetupDiEnumDeviceInfo(infoSet, 0, &deviceInfo)) {
            // DEVPKEY_NAME, which Microsoft recommends for UI display, resolves to
            // FriendlyName when present and DeviceDesc otherwise. Querying these two
            // legacy SPDRP properties in that order produces the same name without
            // depending on newer property-key headers.
            found = setupMonitorProperty(infoSet, &deviceInfo, SPDRP_FRIENDLYNAME, out, outChars);
            if (!found)
                found = setupMonitorProperty(infoSet, &deviceInfo, SPDRP_DEVICEDESC, out, outChars);
        }
        SetupDiDeleteDeviceInterfaceData(infoSet, &interfaceData);
    }
    SetupDiDestroyDeviceInfoList(infoSet);
    return found;
}
static BOOL sameMode(const DEVMODEW *a, const DEVMODEW *b) {
    return a->dmPelsWidth == b->dmPelsWidth && a->dmPelsHeight == b->dmPelsHeight &&
        a->dmDisplayFrequency == b->dmDisplayFrequency && a->dmBitsPerPel == b->dmBitsPerPel &&
        a->dmDisplayFlags == b->dmDisplayFlags;
}
static int compare(const void *aa, const void *bb) {
    const DEVMODEW *a = aa, *b = bb;
#define CMP(field) if (a->field != b->field) return a->field < b->field ? -1 : 1
    CMP(dmPelsWidth); CMP(dmPelsHeight); CMP(dmDisplayFrequency); CMP(dmDisplayFlags); CMP(dmBitsPerPel);
#undef CMP
    return 0;
}
static UINT addChoice(const WCHAR *device, const DEVMODEW *mode) {
    if (count >= ID_EXIT - 1) return 0;
    if (count == capacity) {
        size_t n = capacity ? capacity * 2 : 128;
        MenuChoice *p = realloc(choices, n * sizeof(*p));
        if (!p) return 0;
        choices = p; capacity = n;
    }
    lstrcpynW(choices[count].device, device, 32);
    choices[count].width = mode->dmPelsWidth;
    choices[count].height = mode->dmPelsHeight;
    choices[count].frequency = mode->dmDisplayFrequency;
    choices[count].bits = mode->dmBitsPerPel;
    choices[count].flags = mode->dmDisplayFlags;
    return (UINT)++count;
}
static void addTray(void) {
    if (Shell_NotifyIconW(NIM_ADD, &tray)) {
        tray.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &tray);
    }
}
static BOOL findPrimaryDevice(WCHAR deviceName[CCHDEVICENAME]) {
    for (DWORD d = 0; d < 256; ++d) {
        DISPLAY_DEVICEW device = {0}; device.cb = sizeof(device);
        if (!EnumDisplayDevicesW(NULL, d, &device, 0)) break;
        if (!(device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) ||
            !(device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) ||
            (device.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)) continue;
        lstrcpynW(deviceName, device.DeviceName, CCHDEVICENAME);
        return TRUE;
    }
    return FALSE;
}
static DISPLAYCONFIG_PATH_INFO *activePaths(UINT32 *countOut) {
    *countOut = 0;
    for (int attempt = 0; attempt < 3; ++attempt) {
        UINT32 pc = 0, mc = 0;
        if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pc, &mc) != ERROR_SUCCESS ||
            !pc || pc > 4096 || mc > 16384) return NULL;
        const UINT32 pathCapacity = pc, modeCapacity = mc;
        DISPLAYCONFIG_PATH_INFO *paths = calloc(pc, sizeof(*paths));
        DISPLAYCONFIG_MODE_INFO *modes = calloc(mc ? mc : 1, sizeof(*modes));
        if (!paths || !modes) { free(paths); free(modes); return NULL; }
        LONG status = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pc, paths, &mc, modes, NULL);
        free(modes);
        if (status == ERROR_SUCCESS && pc <= pathCapacity && mc <= modeCapacity) {
            *countOut = pc;
            return paths;
        }
        free(paths);
        if (status != ERROR_INSUFFICIENT_BUFFER) break;
    }
    return NULL;
}
static BOOL displaySourceForDevice(const WCHAR *deviceName, LUID *adapterId, UINT32 *sourceId) {
    {
        UINT32 pc = 0;
        DISPLAYCONFIG_PATH_INFO *paths = activePaths(&pc);
        if (!paths) return FALSE;

        BOOL found = FALSE;
        for (UINT32 i = 0; i < pc; ++i) {
            DISPLAYCONFIG_SOURCE_DEVICE_NAME source = {0};
            source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
            source.header.size = sizeof(source);
            source.header.adapterId = paths[i].sourceInfo.adapterId;
            source.header.id = paths[i].sourceInfo.id;
            if (DisplayConfigGetDeviceInfo(&source.header) == ERROR_SUCCESS &&
                lstrcmpiW(source.viewGdiDeviceName, deviceName) == 0) {
                *adapterId = paths[i].sourceInfo.adapterId;
                *sourceId = paths[i].sourceInfo.id;
                found = TRUE;
                break;
            }
        }
        free(paths);
        return found;
    }
    return FALSE;
}
static BOOL primaryScaleSource(LUID *adapterId, UINT32 *sourceId) {
    WCHAR primary[CCHDEVICENAME];
    return findPrimaryDevice(primary) && displaySourceForDevice(primary, adapterId, sourceId);
}
static int scaleValueIndex(UINT32 percent) {
    for (size_t i = 0; i < sizeof(scaleValues)/sizeof(scaleValues[0]); ++i)
        if (scaleValues[i] == percent) return (int)i;
    return -1;
}
static BOOL getScaleInfo(LUID adapterId, UINT32 sourceId, ScaleInfo *info, LONG *statusOut) {
    if (sizeof(KpGetDpiScale) != 32) { if (statusOut) *statusOut = ERROR_NOT_SUPPORTED; return FALSE; }
    KpGetDpiScale request = {0};
    request.header.type = KP_INFO_GET_DPI_SCALE;
    request.header.size = sizeof(request);
    request.header.adapterId = adapterId;
    request.header.id = sourceId;
    LONG status = DisplayConfigGetDeviceInfo(&request.header);
    if (statusOut) *statusOut = status;
    if (status != ERROR_SUCCESS) return FALSE;

    const int valueCount = (int)(sizeof(scaleValues)/sizeof(scaleValues[0]));
    int64_t recommendedIndex = -(int64_t)request.minScaleRel;
    int64_t currentIndex = recommendedIndex + request.curScaleRel;
    int64_t maximumIndex = recommendedIndex + request.maxScaleRel;
    if (recommendedIndex < 0 || recommendedIndex >= valueCount ||
        maximumIndex < recommendedIndex || maximumIndex >= valueCount ||
        currentIndex < 0 || currentIndex > maximumIndex) {
        if (statusOut) *statusOut = ERROR_INVALID_DATA;
        return FALSE;
    }

    info->minimum = scaleValues[0];
    info->current = scaleValues[currentIndex];
    info->recommended = scaleValues[recommendedIndex];
    info->maximum = scaleValues[maximumIndex];
    return TRUE;
}
static BOOL setScale(LUID adapterId, UINT32 sourceId, UINT32 percent, LONG *statusOut) {
    ScaleInfo info = {0};
    LONG status = ERROR_INVALID_PARAMETER;
    if (!getScaleInfo(adapterId, sourceId, &info, &status)) {
        if (statusOut) *statusOut = status;
        return FALSE;
    }
    int targetIndex = scaleValueIndex(percent);
    int recommendedIndex = scaleValueIndex(info.recommended);
    if (targetIndex < 0 || recommendedIndex < 0 || percent < info.minimum || percent > info.maximum) {
        if (statusOut) *statusOut = ERROR_INVALID_PARAMETER;
        return FALSE;
    }

    if (sizeof(KpSetDpiScale) != 24) { if (statusOut) *statusOut = ERROR_NOT_SUPPORTED; return FALSE; }
    KpSetDpiScale request = {0};
    request.header.type = KP_INFO_SET_DPI_SCALE;
    request.header.size = sizeof(request);
    request.header.adapterId = adapterId;
    request.header.id = sourceId;
    request.scaleRel = (INT32)(targetIndex - recommendedIndex);
    status = DisplayConfigSetDeviceInfo(&request.header);
    if (statusOut) *statusOut = status;
    return status == ERROR_SUCCESS;
}
static UINT addScaleChoice(LUID adapterId, UINT32 sourceId, UINT32 percent) {
    if (scaleCount >= (size_t)(ID_SCALE_LAST - ID_SCALE_FIRST + 1)) return 0;
    if (scaleCount == scaleCapacity) {
        size_t next = scaleCapacity ? scaleCapacity * 2 : 64;
        ScaleChoice *p = realloc(scaleChoices, next * sizeof(*p));
        if (!p) return 0;
        scaleChoices = p; scaleCapacity = next;
    }
    scaleChoices[scaleCount].adapterId = adapterId;
    scaleChoices[scaleCount].sourceId = sourceId;
    scaleChoices[scaleCount].percent = percent;
    return ID_SCALE_FIRST + (UINT)scaleCount++;
}
static HMENU buildScalingMenu(LUID adapterId, UINT32 sourceId, WCHAR label[64]) {
    ScaleInfo info = {0};
    if (!getScaleInfo(adapterId, sourceId, &info, NULL)) return NULL;
    HMENU menu = CreatePopupMenu();
    if (!menu) return NULL;
    for (size_t i = 0; i < sizeof(scaleValues)/sizeof(scaleValues[0]); ++i) {
        UINT32 percent = scaleValues[i];
        if (percent < info.minimum || percent > info.maximum) continue;
        UINT id = addScaleChoice(adapterId, sourceId, percent);
        if (!id) { DestroyMenu(menu); return NULL; }
        WCHAR item[32];
        safeFormat(item, 32, L"%u%%", (unsigned)percent);
        AppendMenuW(menu, MF_STRING | (percent == info.current ? MF_CHECKED : 0), id, item);
    }
    if (!GetMenuItemCount(menu)) { DestroyMenu(menu); return NULL; }
    safeFormat(label, 64, L"Scaling - %u%%", (unsigned)info.current);
    return menu;
}

static BOOL primaryHdrTarget(HdrTarget *target) {
    WCHAR primary[CCHDEVICENAME];
    if (!findPrimaryDevice(primary)) return FALSE;

    {
        UINT32 pc = 0;
        DISPLAYCONFIG_PATH_INFO *paths = activePaths(&pc);
        if (!paths) return FALSE;

        BOOL found = FALSE;
        for (UINT32 i = 0; i < pc && !found; ++i) {
            DISPLAYCONFIG_SOURCE_DEVICE_NAME source = {0};
            source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
            source.header.size = sizeof(source);
            source.header.adapterId = paths[i].sourceInfo.adapterId;
            source.header.id = paths[i].sourceInfo.id;
            if (DisplayConfigGetDeviceInfo(&source.header) != ERROR_SUCCESS ||
                lstrcmpiW(source.viewGdiDeviceName, primary) != 0) continue;

            KpGetAdvancedColorInfo2 info2 = {0};
            info2.header.type = KP_INFO_GET_ADVANCED_COLOR_2;
            info2.header.size = sizeof(info2);
            info2.header.adapterId = paths[i].targetInfo.adapterId;
            info2.header.id = paths[i].targetInfo.id;
            LONG newer = DisplayConfigGetDeviceInfo(&info2.header);
            if (newer == ERROR_SUCCESS) {
                // Bit 4 = HDR supported, bit 5 = HDR user enabled.
                if (info2.value & (1u << 4)) {
                    target->adapterId = paths[i].targetInfo.adapterId;
                    target->targetId = paths[i].targetInfo.id;
                    target->enabled = (info2.value & (1u << 5)) != 0 ||
                                      info2.activeColorMode == KP_ADVANCED_COLOR_MODE_HDR;
                    target->useNewApi = TRUE;
                    found = TRUE;
                }
                // On the split-color API, an unsupported target must not fall
                // back to legacy Advanced Color because WCG-only is not HDR.
                continue;
            }

            if (newer != ERROR_NOT_SUPPORTED && newer != ERROR_INVALID_PARAMETER) continue;

            KpGetAdvancedColorInfo legacy = {0};
            legacy.header.type = KP_INFO_GET_ADVANCED_COLOR;
            legacy.header.size = sizeof(legacy);
            legacy.header.adapterId = paths[i].targetInfo.adapterId;
            legacy.header.id = paths[i].targetInfo.id;
            if (DisplayConfigGetDeviceInfo(&legacy.header) == ERROR_SUCCESS && (legacy.value & 1u) && !(legacy.value & (1u << 3))) {
                target->adapterId = paths[i].targetInfo.adapterId;
                target->targetId = paths[i].targetInfo.id;
                target->enabled = (legacy.value & (1u << 1)) != 0;
                target->useNewApi = FALSE;
                found = TRUE;
            }
        }
        free(paths);
        return found;
    }
    return FALSE;
}
static BOOL setPrimaryHdr(const HdrTarget *target, BOOL enable, LONG *status) {
    KpSetColorState state = {0};
    state.header.type = target->useNewApi ? KP_INFO_SET_HDR_STATE : KP_INFO_SET_ADVANCED_COLOR;
    state.header.size = sizeof(state);
    state.header.adapterId = target->adapterId;
    state.header.id = target->targetId;
    state.value = enable ? 1u : 0u;
    *status = DisplayConfigSetDeviceInfo(&state.header);
    return *status == ERROR_SUCCESS;
}
static void togglePrimaryHdr(void) {
    HdrTarget target = {0};
    if (!primaryHdrTarget(&target)) {
        MessageBoxW(owner, L"HDR is not available on the current primary display.", APP,
                    MB_OK | MB_ICONINFORMATION);
        return;
    }
    LONG status = ERROR_SUCCESS;
    if (!setPrimaryHdr(&target, !target.enabled, &status))
        error(L"Windows could not change HDR on the current primary display.", status);
}
static void finish(BOOL keep) {
    if (!pending) return;
    pending = FALSE;
    HWND old = confirmation;
    confirmation = NULL;
    if (old) { KillTimer(old, 1); DestroyWindow(old); }
    LONG result;
    if (keep) {
        // Persist only after explicit confirmation. Preserve current desktop position.
        DEVMODEW now;
        if (!current(requested.device, &now) || !sameMode(&now, &requested.mode)) {
            result = ChangeDisplaySettingsExW(previous.device, &previous.mode, NULL, 0, NULL);
            error(L"The display changed before confirmation. The previous mode was requested again.", result);
            return;
        }
        result = ChangeDisplaySettingsExW(requested.device, &now, NULL, CDS_UPDATEREGISTRY, NULL);
        if (result != DISP_CHANGE_SUCCESSFUL) {
            LONG restore = ChangeDisplaySettingsExW(previous.device, &previous.mode, NULL, 0, NULL);
            error(L"Windows could not save the new mode.", result);
            if (restore != DISP_CHANGE_SUCCESSFUL) error(L"Windows could not restore the previous mode.", restore);
        }
    } else {
        result = ChangeDisplaySettingsExW(previous.device, &previous.mode, NULL, 0, NULL);
        if (result != DISP_CHANGE_SUCCESSFUL)
            error(L"Windows could not restore the previous mode. Open Windows Display Settings to recover.", result);
    }
}
static void updateCountdown(void) {
    ULONGLONG now = GetTickCount64();
    if (now >= deadline) { finish(FALSE); return; }
    WCHAR text[160];
    safeFormat(text, 160, L"Keep these display settings?\nReverting in %llu seconds.",
             (unsigned long long)((deadline - now + 999) / 1000));
    SetWindowTextW(countdown, text);
}
static LRESULT CALLBACK confirmProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_ERASEBKGND:
        if (dark) { RECT r; GetClientRect(w, &r); FillRect((HDC)wp, &r, darkBrush); return 1; }
        break;
    case WM_CTLCOLORSTATIC:
        if (dark) {
            SetTextColor((HDC)wp, RGB(240,240,240)); SetBkColor((HDC)wp, RGB(24,24,24));
            return (LRESULT)darkBrush;
        }
        break;
    case WM_DRAWITEM: if (drawButton((DRAWITEMSTRUCT *)lp)) return TRUE; break;
    case DM_GETDEFID: return MAKELRESULT(ID_REVERT, DC_HASDEFID);
    case WM_COMMAND:
        if (LOWORD(wp) == ID_KEEP) finish(TRUE);
        else if (LOWORD(wp) == ID_REVERT || LOWORD(wp) == IDCANCEL) finish(FALSE);
        return 0;
    case WM_TIMER: updateCountdown(); return 0;
    case WM_CLOSE: finish(FALSE); return 0;
    }
    return DefWindowProcW(w, msg, wp, lp);
}
static void applyChoice(Choice selected, BOOL quick) {
    if (pending) return;
    previous = selected;
    if (!current(selected.device, &previous.mode)) {
        error(L"This monitor is no longer available. Open the menu again.", -1); return;
    }
    if (quick && (previous.mode.dmPelsWidth != selected.mode.dmPelsWidth ||
                  previous.mode.dmPelsHeight != selected.mode.dmPelsHeight)) {
        MessageBoxW(owner, L"The resolution changed while the menu was open. Open the menu again.",
                    APP, MB_OK | MB_ICONINFORMATION); return;
    }
    if (sameMode(&previous.mode, &selected.mode)) return;
    // Start with the live mode to preserve position and orientation.
    requested = previous;
    requested.mode.dmPelsWidth = selected.mode.dmPelsWidth;
    requested.mode.dmPelsHeight = selected.mode.dmPelsHeight;
    requested.mode.dmDisplayFrequency = selected.mode.dmDisplayFrequency;
    requested.mode.dmDisplayFlags = selected.mode.dmDisplayFlags;
    requested.mode.dmBitsPerPel = selected.mode.dmBitsPerPel;
    requested.mode.dmFields |= DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY | DM_DISPLAYFLAGS | DM_BITSPERPEL;
    LONG result = ChangeDisplaySettingsExW(selected.device, &requested.mode, NULL, CDS_TEST, NULL);
    if (result != DISP_CHANGE_SUCCESSFUL) { error(L"Windows rejected this display mode.", result); return; }
    result = ChangeDisplaySettingsExW(selected.device, &requested.mode, NULL, 0, NULL);
    if (result != DISP_CHANGE_SUCCESSFUL) { error(L"Windows could not apply this display mode.", result); return; }
    pending = TRUE;
    deadline = GetTickCount64() + 15000;
    RECT area; SystemParametersInfoW(SPI_GETWORKAREA, 0, &area, 0);
    confirmation = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"DisplayTrayConfirm", APP,
        WS_CAPTION | WS_SYSMENU, area.left + (area.right-area.left-380)/2,
        area.top + (area.bottom-area.top-165)/2, 380, 165, owner, NULL, GetModuleHandleW(NULL), NULL);
    if (!confirmation) { finish(FALSE); return; }
    countdown = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 18, 15, 340, 48, confirmation, NULL, NULL, NULL);
    HWND keep = CreateWindowW(L"BUTTON", L"Keep", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        145, 80, 90, 28, confirmation, (HMENU)(INT_PTR)ID_KEEP, NULL, NULL);
    HWND revert = CreateWindowW(L"BUTTON", L"Revert", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        245, 80, 90, 28, confirmation, (HMENU)(INT_PTR)ID_REVERT, NULL, NULL);
    if (!countdown || !keep || !revert || !SetTimer(confirmation, 1, 250, NULL)) { finish(FALSE); return; }
    SendMessageW(countdown, WM_SETFONT, (WPARAM)font, TRUE);
    SendMessageW(keep, WM_SETFONT, (WPARAM)font, TRUE);
    SendMessageW(revert, WM_SETFONT, (WPARAM)font, TRUE);
    updateTheme();
    updateCountdown();
    ShowWindow(confirmation, SW_SHOW); SetForegroundWindow(confirmation); SetFocus(revert);
}
// Reposition only this thread's popup menus while our tray menu is open.
static HMENU positionedRoot;
static HWINEVENTHOOK menuHook;
static BOOL positioningPopup;
static BOOL parentRow(HMENU menu, HMENU child, RECT *row) {
    for (int i = 0; i < GetMenuItemCount(menu); ++i) {
        HMENU sub = GetSubMenu(menu, i);
        if (sub == child) return GetMenuItemRect(NULL, menu, (UINT)i, row);
        if (sub && parentRow(sub, child, row)) return TRUE;
    }
    return FALSE;
}
// Do not subclass the system menu: that changes its initialization/paint path.
// Wait for its normal popup-start event, then move and repaint it normally.
static void CALLBACK popupOpened(HWINEVENTHOOK hook, DWORD event, HWND w,
                                  LONG object, LONG child, DWORD thread, DWORD time) {
    (void)object; (void)child; (void)time;
    if (hook != menuHook || !positionedRoot || positioningPopup ||
        event != EVENT_SYSTEM_MENUPOPUPSTART || thread != GetCurrentThreadId() ||
        !IsWindow(w) || !IsWindowVisible(w)) return;
    MENUBARINFO info = {0}; info.cbSize = sizeof(info);
    RECT row, bounds;
    if (!GetMenuBarInfo(w, OBJID_CLIENT, 0, &info) || !info.hMenu ||
        info.hMenu == positionedRoot || !parentRow(positionedRoot, info.hMenu, &row) ||
        !GetWindowRect(w, &bounds)) return;
    MONITORINFO monitor = {0}; monitor.cbSize = sizeof(monitor);
    if (!GetMonitorInfoW(MonitorFromRect(&row, MONITOR_DEFAULTTONEAREST), &monitor)) return;
    int height = bounds.bottom-bounds.top;
    if (height <= 0) return;
    int y = row.bottom-height;
    if (y+height > monitor.rcWork.bottom) y = monitor.rcWork.bottom-height;
    if (y < monitor.rcWork.top) y = monitor.rcWork.top;
    positioningPopup = TRUE;
    // Retain the original menu brush and owner-drawn items when the popup moves.
    if (dark) {
        MENUINFO background = {0}; background.cbSize = sizeof(background);
        background.fMask = MIM_BACKGROUND; background.hbrBack = darkBrush;
        SetMenuInfo(info.hMenu, &background);
    }
    if (y != bounds.top)
        SetWindowPos(w, NULL, bounds.left, y, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
    RedrawWindow(w, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_UPDATENOW);
    positioningPopup = FALSE;
}
static void showMenu(BOOL quick) {
    if (pending) { SetForegroundWindow(confirmation); return; }
    if (menuOpen) return;
    POINT pt = {0}; GetCursorPos(&pt);
    int savedDpi = dpi;
    HFONT savedMenuFont = menuFont;
    // Keep the system-DPI font for the fallback path and later confirmation UI.
    menuFont = font;
    HANDLE oldDpiContext = beginMenuDpi(pt);
    if (!oldDpiContext) menuFont = savedMenuFont;
    updateTheme();
    menuOpen = TRUE;
    menuInvalidated = FALSE;
    HMENU root = CreatePopupMenu();
    if (!root) {
        if (oldDpiContext) {
            DeleteObject(menuFont); menuFont = savedMenuFont; dpi = savedDpi;
            setThreadDpiContext(oldDpiContext);
        }
        menuOpen = FALSE; return;
    }
    count = 0;
    scaleCount = 0;
    for (DWORD d = 0; d < 256; ++d) {
        DISPLAY_DEVICEW device = {0}; device.cb = sizeof(device);
        if (!EnumDisplayDevicesW(NULL, d, &device, 0)) break;
        if (!(device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) ||
            (device.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)) continue;
        DEVMODEW live;
        if (!current(device.DeviceName, &live)) continue;
        DEVMODEW *modes = NULL; size_t n = 0, cap = 0;
        for (DWORD i = 0; i < 16384; ++i) {
            DEVMODEW mode = {0}; mode.dmSize = sizeof(mode);
            if (!EnumDisplaySettingsExW(device.DeviceName, i, &mode, 0)) break;
            if (mode.dmBitsPerPel != 32 || mode.dmDisplayOrientation != live.dmDisplayOrientation) continue;
            if (quick && (mode.dmPelsWidth != live.dmPelsWidth || mode.dmPelsHeight != live.dmPelsHeight)) continue;
            if (n == cap) {
                size_t next = cap ? cap * 2 : 128;
                DEVMODEW *p = realloc(modes, next * sizeof(*p));
                if (!p) break;
                modes = p; cap = next;
            }
            modes[n++] = mode;
        }
        if (n > 1) qsort(modes, n, sizeof(*modes), compare);
        HMENU monitor = CreatePopupMenu(), resolution = NULL;
        if (!monitor) { free(modes); continue; }
        if (quick) resolution = monitor;
        DWORD width = 0, height = 0;
        WCHAR label[256];
        for (size_t i = 0; i < n; ++i) {
            DEVMODEW *m = &modes[i];
            if (i && sameMode(m, &modes[i-1])) continue;
            if (!quick && (!resolution || m->dmPelsWidth != width || m->dmPelsHeight != height)) {
                width = m->dmPelsWidth; height = m->dmPelsHeight;
                resolution = CreatePopupMenu();
                if (!resolution) break;
                safeFormat(label, 256, L"%lu x %lu", (unsigned long)width, (unsigned long)height);
                if (!AppendMenuW(monitor, MF_POPUP | ((width == live.dmPelsWidth && height == live.dmPelsHeight) ? MF_CHECKED : 0),
                            (UINT_PTR)resolution, label)) { DestroyMenu(resolution); break; }
            }
            UINT id = addChoice(device.DeviceName, m);
            if (!id) break;
            if (m->dmDisplayFrequency <= 1) lstrcpyW(label, L"Driver default");
            else safeFormat(label, 256, L"%lu Hz%ls", (unsigned long)m->dmDisplayFrequency,
                (m->dmDisplayFlags & DM_INTERLACED) ? L" (interlaced)" : L"");
            AppendMenuW(resolution, MF_STRING | (sameMode(m, &live) ? MF_CHECKED : 0), id, label);
        }
        free(modes);
        if (!GetMenuItemCount(monitor)) AppendMenuW(monitor, MF_GRAYED, 0, L"No display modes available");
        if (!quick) {
            LUID scaleAdapter = {0}; UINT32 scaleSource = 0;
            if (displaySourceForDevice(device.DeviceName, &scaleAdapter, &scaleSource)) {
                WCHAR scaleLabel[64];
                HMENU scaling = buildScalingMenu(scaleAdapter, scaleSource, scaleLabel);
                if (scaling) {
                    AppendMenuW(monitor, MF_SEPARATOR, 0, NULL);
                    if (!AppendMenuW(monitor, MF_POPUP, (UINT_PTR)scaling, scaleLabel))
                        DestroyMenu(scaling);
                }
            }
        }
        WCHAR monitorName[256] = L"";
        if (!deviceManagerMonitorName(device.DeviceName, monitorName, 256)) {
            DISPLAY_DEVICEW fallback = {0}; fallback.cb = sizeof(fallback);
            if (EnumDisplayDevicesW(device.DeviceName, 0, &fallback, 0) && fallback.DeviceString[0])
                lstrcpynW(monitorName, fallback.DeviceString, 256);
            else
                lstrcpynW(monitorName, L"Monitor", 256);
        }
        // Use the Windows display number in the label, keeping the device path for API calls.
        WCHAR displayLabel[64];
        const WCHAR *displayNumber = wcsstr(device.DeviceName, L"DISPLAY");
        if (displayNumber && displayNumber[7] >= L'0' && displayNumber[7] <= L'9')
            safeFormat(displayLabel, 64, L"Display %ls", displayNumber + 7);
        else
            safeFormat(displayLabel, 64, L"Display %lu", (unsigned long)d + 1);
        safeFormat(label, 256, L"%ls - %ls%ls", displayLabel, monitorName,
            (device.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) ? L" (primary)" : L"");
        if (quick) {
            size_t used = (size_t)lstrlenW(label);
            safeFormat(label+used, 256-used, L" - %lu x %lu", (unsigned long)live.dmPelsWidth, (unsigned long)live.dmPelsHeight);
        }
        if (!AppendMenuW(root, MF_POPUP, (UINT_PTR)monitor, label)) DestroyMenu(monitor);
    }
    if (!GetMenuItemCount(root)) AppendMenuW(root, MF_GRAYED, 0, L"No active monitors found");
    if (quick && GetMenuItemCount(root) == 1 && GetSubMenu(root, 0)) {
        HMENU only = GetSubMenu(root, 0);
        RemoveMenu(root, 0, MF_BYPOSITION); DestroyMenu(root); root = only;
    }
    if (!quick) {
        int controlsSeparator = GetMenuItemCount(root);
        AppendMenuW(root, MF_SEPARATOR, 0, NULL);
        BOOL addedControl = FALSE;

        LUID primaryAdapter = {0}; UINT32 primarySource = 0;
        if (primaryScaleSource(&primaryAdapter, &primarySource)) {
            WCHAR scaleLabel[64];
            HMENU scaling = buildScalingMenu(primaryAdapter, primarySource, scaleLabel);
            if (scaling) {
                if (AppendMenuW(root, MF_POPUP, (UINT_PTR)scaling, scaleLabel)) addedControl = TRUE;
                else DestroyMenu(scaling);
            }
        }

        HdrTarget hdr = {0};
        if (primaryHdrTarget(&hdr)) {
            AppendMenuW(root, MF_STRING | (hdr.enabled ? MF_CHECKED : 0), ID_HDR, L"HDR");
            addedControl = TRUE;
        }
        if (!addedControl) RemoveMenu(root, (UINT)controlsSeparator, MF_BYPOSITION);

        AppendMenuW(root, MF_SEPARATOR, 0, NULL);
        AppendMenuW(root, MF_STRING, ID_EXIT, L"Exit");
        AppendMenuW(root, MF_SEPARATOR, 0, NULL);
        AppendMenuW(root, MF_GRAYED, 0, L"King Panel - King Alex Gilbert");
    }
    themeMenu(root);
    SetForegroundWindow(owner);
    positionedRoot = root;
    menuHook = SetWinEventHook(EVENT_SYSTEM_MENUPOPUPSTART, EVENT_SYSTEM_MENUPOPUPSTART,
        NULL, popupOpened, GetCurrentProcessId(), GetCurrentThreadId(), WINEVENT_OUTOFCONTEXT);
    UINT id = TrackPopupMenu(root, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON |
                            TPM_BOTTOMALIGN | TPM_NOANIMATION, pt.x, pt.y, 0, owner, NULL);
    positionedRoot = NULL;
    if (menuHook) { UnhookWinEvent(menuHook); menuHook = NULL; }
    PostMessageW(owner, WM_NULL, 0, 0);
    DestroyMenu(root);
    releaseLabels();
    if (oldDpiContext) {
        DeleteObject(menuFont); menuFont = savedMenuFont; dpi = savedDpi;
        setThreadDpiContext(oldDpiContext);
    }
    if (menuInvalidated) id = 0;
    Choice selected = {0};
    ScaleChoice scaleSelected = {0}; BOOL hasScaleChoice = FALSE;
    if (id > 0 && id <= count) {
        const MenuChoice *choice = &choices[id-1];
        lstrcpynW(selected.device, choice->device, 32);
        selected.mode.dmPelsWidth = choice->width;
        selected.mode.dmPelsHeight = choice->height;
        selected.mode.dmDisplayFrequency = choice->frequency;
        selected.mode.dmBitsPerPel = choice->bits;
        selected.mode.dmDisplayFlags = choice->flags;
    }
    if (id >= ID_SCALE_FIRST && id <= ID_SCALE_LAST) {
        size_t scaleIndex = (size_t)(id - ID_SCALE_FIRST);
        if (scaleIndex < scaleCount) { scaleSelected = scaleChoices[scaleIndex]; hasScaleChoice = TRUE; }
    }
    free(choices); choices = NULL; count = capacity = 0;
    free(scaleChoices); scaleChoices = NULL; scaleCount = scaleCapacity = 0;
    menuOpen = FALSE;
    if (id == ID_EXIT) DestroyWindow(owner);
    else if (id == ID_HDR) togglePrimaryHdr();
    else if (hasScaleChoice) {
        LONG status = ERROR_SUCCESS;
        if (!setScale(scaleSelected.adapterId, scaleSelected.sourceId, scaleSelected.percent, &status))
            error(L"Windows could not change display scaling.", status);
    }
    else if (selected.device[0]) applyChoice(selected, quick);
}
static LRESULT CALLBACK windowProc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    if (taskbarCreated && msg == taskbarCreated) { addTray(); return 0; }
    switch (msg) {
    case TRAY_MSG: {
        // NOTIFYICON_VERSION_4 places the notification code in LOWORD(lParam).
        UINT event = LOWORD(lp);
        if (event == WM_LBUTTONUP || event == NIN_KEYSELECT) showMenu(TRUE);
        else if (event == WM_RBUTTONUP || event == WM_CONTEXTMENU) showMenu(FALSE);
        return 0;
    }
    case WM_MEASUREITEM: if (measureMenu((MEASUREITEMSTRUCT *)lp)) return TRUE; break;
    case WM_DRAWITEM: if (drawMenu((DRAWITEMSTRUCT *)lp)) return TRUE; break;
    case WM_DISPLAYCHANGE:
    case WM_DEVICECHANGE:
        if (menuOpen) { menuInvalidated = TRUE; EndMenu(); }
        return 0;
    case WM_SETTINGCHANGE:
    case WM_THEMECHANGED:
    case WM_SYSCOLORCHANGE:
        if (menuOpen) { menuInvalidated = TRUE; EndMenu(); }
        updateTheme(); return 0;
    case WM_QUERYENDSESSION: finish(FALSE); return TRUE;
    case WM_CLOSE: finish(FALSE); DestroyWindow(w); return 0;
    case WM_DESTROY: finish(FALSE); Shell_NotifyIconW(NIM_DELETE, &tray); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(w, msg, wp, lp);
}
int WINAPI WinMain(HINSTANCE instance, HINSTANCE unused, LPSTR command, int show) {
    (void)unused; (void)command; (void)show;
    HANDLE mutex = CreateMutexW(NULL, FALSE, L"Local\\DisplayTray-94BE521E");
    if (!mutex) return 1;
    if (GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(mutex); return 0; }
    SetProcessDPIAware();
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    setThreadDpiContext = (SetThreadDpiContextFn)(void *)GetProcAddress(user32, "SetThreadDpiAwarenessContext");
    getWindowDpi = (GetWindowDpiFn)(void *)GetProcAddress(user32, "GetDpiForWindow");
    systemMetricsForDpi = (SystemMetricsForDpiFn)(void *)GetProcAddress(user32, "SystemParametersInfoForDpi");
    font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HDC screen = GetDC(NULL); dpi = GetDeviceCaps(screen, LOGPIXELSX); ReleaseDC(NULL, screen);
    NONCLIENTMETRICSW metrics = {0}; metrics.cbSize = sizeof(metrics);
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0))
        menuFont = CreateFontIndirectW(&metrics.lfMenuFont);
    if (!menuFont) menuFont = font;
    darkBrush = CreateSolidBrush(RGB(24,24,24));
    updateTheme();
    WNDCLASSW wc = {0}; wc.hInstance = instance; wc.lpfnWndProc = windowProc; wc.lpszClassName = L"DisplayTrayOwner";
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
    if (!RegisterClassW(&wc)) { CloseHandle(mutex); return 1; }
    wc.lpfnWndProc = confirmProc; wc.lpszClassName = L"DisplayTrayConfirm";
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    if (!RegisterClassW(&wc)) { CloseHandle(mutex); return 1; }
    owner = CreateWindowExW(WS_EX_TOOLWINDOW, L"DisplayTrayOwner", APP, WS_POPUP, 0, 0, 0, 0, NULL, NULL, instance, NULL);
    if (!owner) { CloseHandle(mutex); return 1; }
    taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    tray.cbSize = sizeof(tray); tray.hWnd = owner; tray.uID = 1; tray.guidItem = kingPanelTrayGuid;
    tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_GUID; tray.uCallbackMessage = TRAY_MSG;
    tray.hIcon = (HICON)LoadImageW(instance, MAKEINTRESOURCEW(1), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED);
    lstrcpyW(tray.szTip, L"King Panel - left: refresh rate / right: display + scaling + HDR");
    if (!Shell_NotifyIconW(NIM_ADD, &tray)) {
        MessageBoxW(NULL, L"Could not add the tray icon. Try starting King Panel again.", APP, MB_OK | MB_ICONERROR);
        DestroyWindow(owner); CloseHandle(mutex); return 1;
    }
    tray.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &tray);
    MSG msg; BOOL result;
    while ((result = GetMessageW(&msg, NULL, 0, 0)) > 0) {
        if (confirmation && IsDialogMessageW(confirmation, &msg)) continue;
        TranslateMessage(&msg); DispatchMessageW(&msg);
    }
    if (result == -1) { finish(FALSE); Shell_NotifyIconW(NIM_DELETE, &tray); }
    if (menuFont != font) DeleteObject(menuFont);
    DeleteObject(darkBrush);
    CloseHandle(mutex);
    return result == -1 ? 1 : 0;
}

_Static_assert(sizeof(Choice) == 284, "rollback choice ABI");
_Static_assert(sizeof(MenuChoice) == 84, "compact choice ABI");
_Static_assert(sizeof(KpGetDpiScale) == 32, "DPI query ABI");
_Static_assert(sizeof(KpSetDpiScale) == 24, "DPI setter ABI");
_Static_assert(sizeof(KpGetAdvancedColorInfo2) == 36, "HDR query ABI");
_Static_assert(sizeof(KpSetColorState) == 24, "HDR setter ABI");
