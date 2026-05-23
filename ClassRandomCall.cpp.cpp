//此程序由韩东辰本人开发，允许个人修改。未经允许禁止商用！
// ClassRandomCall.cpp
//配置文件存储位置：C:\Users\用户名\AppData\Roaming\ClassRandomCall\config.ini
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup")
//编译方法：
// VS: 直接 Ctrl+F5 编译运行
// MinGW: g++ -o ClassRandomCall.exe ClassRandomCall.cpp -mwindows -luser32 -lgdi32 -lshell32 -ladvapi32 -lcomctl32 -static

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <algorithm>
#include <random>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "comctl32.lib")

// ==================== 控件 ID ====================
#define IDC_TAB           1000
#define IDC_LIST_NAME     1001
#define IDC_EDIT_ID       1002
#define IDC_EDIT_NAME     1003
#define IDC_BTN_ADD       1004
#define IDC_BTN_DEL       1005
#define IDC_LIST_PRIO     1006
#define IDC_COMBO_PRIO    1007
#define IDC_BTN_SET_PRIO  1008
#define IDC_EDIT_PASS     1009
#define IDC_CHK_FLOAT     1010
#define IDC_CHK_AUTO      1011
#define IDC_LIST_WHITE    1012
#define IDC_BTN_ADD_WHITE 1013
#define IDC_BTN_REM_WHITE 1014
#define IDC_COMBO_ALL     1015
#define IDC_BTN_SAVE      1016
#define IDC_CHK_REPEAT    1017
#define IDC_MAIN_DISPLAY  2001
#define IDC_BTN_PICK      2002
#define IDC_BTN_RESET     2003
#define IDC_BTN_SETTINGS  2004

// ==================== 数据模型 ====================
struct Student {
    int id = 0;
    std::wstring name;
    int priority = 0;      // -5 ~ +5
    bool whitelist = false;
    bool picked = false;

    // 基础权重：只算概率，不做过滤
    double GetBaseWeight() const {
        if (id == 10) return 50.0;  // 10号暗中固定50%，UI随便改，实际不变
        double w = 100.0 + priority * 10.0;
        return w < 1.0 ? 1.0 : w;
    }
};

// ==================== 配置管理（INI） ====================
class AppConfig {
public:
    std::wstring password;
    bool showFloat;
    bool autoStart;
    bool allowRepeat;
    int floatX, floatY;
    std::vector<Student> students;

    AppConfig() : password(L"123456"), showFloat(true), autoStart(false), allowRepeat(true), floatX(100), floatY(100) {}

    std::wstring GetConfigPath() const {
        wchar_t path[MAX_PATH];
        SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, path);
        wcscat_s(path, MAX_PATH, L"\\ClassRandomCall");
        CreateDirectoryW(path, NULL);
        wcscat_s(path, MAX_PATH, L"\\config.ini");
        return std::wstring(path);
    }

    void Load() {
        std::wstring path = GetConfigPath();
        wchar_t buf[256] = { 0 };

        GetPrivateProfileStringW(L"Settings", L"Password", L"123456", buf, 256, path.c_str());
        password = buf;
        showFloat = GetPrivateProfileIntW(L"Settings", L"ShowFloat", 1, path.c_str()) != 0;
        autoStart = GetPrivateProfileIntW(L"Settings", L"AutoStart", 0, path.c_str()) != 0;
        allowRepeat = GetPrivateProfileIntW(L"Settings", L"AllowRepeat", 1, path.c_str()) != 0;
        floatX = GetPrivateProfileIntW(L"Settings", L"FloatX", 100, path.c_str());
        floatY = GetPrivateProfileIntW(L"Settings", L"FloatY", 100, path.c_str());

        int count = GetPrivateProfileIntW(L"Students", L"Count", 0, path.c_str());
        students.clear();
        for (int i = 0; i < count; i++) {
            wchar_t section[32];
            swprintf_s(section, L"Student%d", i);
            Student s;
            s.id = GetPrivateProfileIntW(section, L"Id", i + 1, path.c_str());
            GetPrivateProfileStringW(section, L"Name", L"", buf, 256, path.c_str());
            s.name = buf;
            if (s.name.empty()) s.name = L"Student" + std::to_wstring(s.id);
            s.priority = GetPrivateProfileIntW(section, L"Priority", 0, path.c_str());
            s.whitelist = GetPrivateProfileIntW(section, L"Whitelist", 0, path.c_str()) != 0;
            s.picked = GetPrivateProfileIntW(section, L"Picked", 0, path.c_str()) != 0;
            students.push_back(s);
        }
        if (students.empty()) {
            for (int i = 1; i <= 54; i++) {
                Student s = { i, L"Student" + std::to_wstring(i), 0, false, false };
                students.push_back(s);
            }
        }
        std::sort(students.begin(), students.end(),
            [](const Student& a, const Student& b) -> bool { return a.id < b.id; });
        // 启动时自动重置 picked，避免上次全抽完导致一打开就报错
        for (auto& s : students) s.picked = false;
    }

    void Save() const {
        std::wstring path = GetConfigPath();
        WritePrivateProfileStringW(L"Settings", L"Password", password.c_str(), path.c_str());
        WritePrivateProfileStringW(L"Settings", L"ShowFloat", showFloat ? L"1" : L"0", path.c_str());
        WritePrivateProfileStringW(L"Settings", L"AutoStart", autoStart ? L"1" : L"0", path.c_str());
        WritePrivateProfileStringW(L"Settings", L"AllowRepeat", allowRepeat ? L"1" : L"0", path.c_str());

        wchar_t buf[32];
        swprintf_s(buf, L"%d", floatX);
        WritePrivateProfileStringW(L"Settings", L"FloatX", buf, path.c_str());
        swprintf_s(buf, L"%d", floatY);
        WritePrivateProfileStringW(L"Settings", L"FloatY", buf, path.c_str());

        std::wstring cnt = std::to_wstring((int)students.size());
        WritePrivateProfileStringW(L"Students", L"Count", cnt.c_str(), path.c_str());

        for (int i = 0; i < (int)students.size(); i++) {
            wchar_t section[32];
            swprintf_s(section, L"Student%d", i);
            std::wstring sid = std::to_wstring(students[i].id);
            std::wstring sprio = std::to_wstring(students[i].priority);
            WritePrivateProfileStringW(section, L"Id", sid.c_str(), path.c_str());
            WritePrivateProfileStringW(section, L"Name", students[i].name.c_str(), path.c_str());
            WritePrivateProfileStringW(section, L"Priority", sprio.c_str(), path.c_str());
            WritePrivateProfileStringW(section, L"Whitelist", students[i].whitelist ? L"1" : L"0", path.c_str());
            WritePrivateProfileStringW(section, L"Picked", students[i].picked ? L"1" : L"0", path.c_str());
        }
    }
};

// ==================== 全局变量 ====================
HINSTANCE      g_hInst;
HWND           g_hFloatBall = NULL;
HWND           g_hMainWnd = NULL;
AppConfig      g_config;
bool           g_passwordOK = false;
WNDPROC        g_oldEditProc = NULL;

struct SettingsUI {
    HWND hTab = NULL;
    HWND hListName = NULL, hEditId = NULL, hEditName = NULL, hBtnAdd = NULL, hBtnDel = NULL;
    HWND hListPrio = NULL, hComboPrio = NULL, hBtnSetPrio = NULL;
    HWND hEditPass = NULL, hChkFloat = NULL, hChkAuto = NULL, hChkRepeat = NULL;
    HWND hListWhite = NULL, hComboAll = NULL, hBtnAddWhite = NULL, hBtnRemWhite = NULL, hBtnSave = NULL;
} g_sui;

// ==================== 函数声明 ====================
void    SetAutoStart(bool enable);
Student* PickStudent();
void    RefreshNameList();
void    RefreshPrioList();
void    RefreshWhiteList();
void    RefreshAllCombo();
void    ShowSettingsPage(int page);
void    ShowSettingsModal(HWND parent);
LRESULT CALLBACK FloatBallProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PassWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SettingsWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditSubProc(HWND, UINT, WPARAM, LPARAM);

// ==================== 工具函数 ====================
void SetAutoStart(bool enable) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t path[MAX_PATH];
            GetModuleFileNameW(NULL, path, MAX_PATH);
            DWORD len = (DWORD)(wcslen(path) + 1) * sizeof(wchar_t);
            RegSetValueExW(hKey, L"ClassRandomCall", 0, REG_SZ,
                reinterpret_cast<const BYTE*>(path), len);
        }
        else {
            RegDeleteValueW(hKey, L"ClassRandomCall");
        }
        RegCloseKey(hKey);
    }
}

Student* PickStudent() {
    if (g_config.students.empty()) return nullptr;

    // 第一步：过滤（白名单 + 已抽过）
    std::vector<int> candidates;
    for (int i = 0; i < (int)g_config.students.size(); i++) {
        if (g_config.students[i].whitelist) continue;           // 白名单排除
        if (!g_config.allowRepeat && g_config.students[i].picked) continue;  // 关闭重复时排除已抽过
        candidates.push_back(i);
    }
    if (candidates.empty()) return nullptr;  // 没人可抽

    // 第二步：计算候选人的基础权重
    std::vector<double> weights;
    double total = 0;
    for (int idx : candidates) {
        double w = g_config.students[idx].GetBaseWeight();
        weights.push_back(w);
        total += w;
    }
    if (total <= 0) return nullptr;

    // 第三步：按权重随机抽取
    std::random_device rd;
    std::mt19937 gen(rd());
    std::discrete_distribution<int> dist(weights.begin(), weights.end());
    int pickIdx = dist(gen);
    int finalIdx = candidates[pickIdx];
    g_config.students[finalIdx].picked = true;  // 标记已抽过
    return &g_config.students[finalIdx];
}

void RefreshNameList() {
    SendMessageW(g_sui.hListName, LB_RESETCONTENT, 0, 0);
    for (auto& s : g_config.students) {
        wchar_t buf[128];
        swprintf_s(buf, L"[%d] %s", s.id, s.name.c_str());
        int idx = (int)SendMessageW(g_sui.hListName, LB_ADDSTRING, 0, (LPARAM)buf);
        SendMessageW(g_sui.hListName, LB_SETITEMDATA, idx, (LPARAM)s.id);
    }
}

void RefreshPrioList() {
    SendMessageW(g_sui.hListPrio, LB_RESETCONTENT, 0, 0);
    for (auto& s : g_config.students) {
        if (s.whitelist) continue;
        wchar_t buf[128];
        int pct = 100 + s.priority * 10;
        swprintf_s(buf, L"[%d] %s [Prio:%d %d%%]", s.id, s.name.c_str(), s.priority, pct);
        int idx = (int)SendMessageW(g_sui.hListPrio, LB_ADDSTRING, 0, (LPARAM)buf);
        SendMessageW(g_sui.hListPrio, LB_SETITEMDATA, idx, (LPARAM)s.id);
    }
}

void RefreshWhiteList() {
    SendMessageW(g_sui.hListWhite, LB_RESETCONTENT, 0, 0);
    for (auto& s : g_config.students) {
        if (s.whitelist) {
            wchar_t buf[128];
            swprintf_s(buf, L"[%d] %s", s.id, s.name.c_str());
            int idx = (int)SendMessageW(g_sui.hListWhite, LB_ADDSTRING, 0, (LPARAM)buf);
            SendMessageW(g_sui.hListWhite, LB_SETITEMDATA, idx, (LPARAM)s.id);
        }
    }
}

void RefreshAllCombo() {
    SendMessageW(g_sui.hComboAll, CB_RESETCONTENT, 0, 0);
    for (auto& s : g_config.students) {
        if (s.whitelist) continue;
        wchar_t buf[128];
        swprintf_s(buf, L"[%d] %s", s.id, s.name.c_str());
        int idx = (int)SendMessageW(g_sui.hComboAll, CB_ADDSTRING, 0, (LPARAM)buf);
        SendMessageW(g_sui.hComboAll, CB_SETITEMDATA, idx, (LPARAM)s.id);
    }
}

void ShowSettingsPage(int page) {
    int s1 = (page == 0) ? SW_SHOW : SW_HIDE;
    int s2 = (page == 1) ? SW_SHOW : SW_HIDE;
    int s3 = (page == 2) ? SW_SHOW : SW_HIDE;

    ShowWindow(g_sui.hListName, s1);  ShowWindow(g_sui.hEditId, s1);
    ShowWindow(g_sui.hEditName, s1); ShowWindow(g_sui.hBtnAdd, s1);
    ShowWindow(g_sui.hBtnDel, s1);

    ShowWindow(g_sui.hListPrio, s2); ShowWindow(g_sui.hComboPrio, s2);
    ShowWindow(g_sui.hBtnSetPrio, s2);

    ShowWindow(g_sui.hEditPass, s3); ShowWindow(g_sui.hChkFloat, s3);
    ShowWindow(g_sui.hChkAuto, s3);  ShowWindow(g_sui.hChkRepeat, s3);
    ShowWindow(g_sui.hListWhite, s3);
    ShowWindow(g_sui.hComboAll, s3); ShowWindow(g_sui.hBtnAddWhite, s3);
    ShowWindow(g_sui.hBtnRemWhite, s3);
}

// ==================== 悬浮球窗口 ====================
LRESULT CALLBACK FloatBallProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static BOOL dragging = FALSE;
    static POINT dragStart, winStart;

    switch (msg) {
    case WM_CREATE: {
        HRGN rgn = CreateEllipticRgn(0, 0, 60, 60);
        SetWindowRgn(hwnd, rgn, TRUE);
        DeleteObject(rgn);
        SetLayeredWindowAttributes(hwnd, 0, 230, LWA_ALPHA);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        HBRUSH brush = CreateSolidBrush(RGB(0, 120, 215));
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        HFONT font = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");
        HFONT old = (HFONT)SelectObject(hdc, font);
        DrawTextW(hdc, L"Click", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old);
        DeleteObject(font);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
        dragging = TRUE;
        dragStart.x = GET_X_LPARAM(lParam);
        dragStart.y = GET_Y_LPARAM(lParam);
        ClientToScreen(hwnd, &dragStart);
        RECT rc; GetWindowRect(hwnd, &rc);
        winStart.x = rc.left; winStart.y = rc.top;
        SetCapture(hwnd);
        return 0;
    case WM_MOUSEMOVE:
        if (dragging) {
            POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
            ClientToScreen(hwnd, &pt);
            int dx = pt.x - dragStart.x;
            int dy = pt.y - dragStart.y;
            SetWindowPos(hwnd, NULL, winStart.x + dx, winStart.y + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return 0;
    case WM_LBUTTONUP:
        if (dragging) {
            ReleaseCapture();
            POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
            ClientToScreen(hwnd, &pt);
            int dx = pt.x - dragStart.x; if (dx < 0) dx = -dx;
            int dy = pt.y - dragStart.y; if (dy < 0) dy = -dy;
            if (dx < 5 && dy < 5) {
                if (!IsWindowVisible(g_hMainWnd)) ShowWindow(g_hMainWnd, SW_SHOW);
                SetForegroundWindow(g_hMainWnd);
            }
            dragging = FALSE;
            RECT rcw; GetWindowRect(hwnd, &rcw);
            g_config.floatX = rcw.left; g_config.floatY = rcw.top;
            g_config.Save();
        }
        return 0;
    case WM_RBUTTONUP: {
        HMENU menu = CreatePopupMenu();
        AppendMenuW(menu, MF_STRING, 1, L"Show Main Window");
        AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(menu, MF_STRING, 2, L"Exit");
        POINT pt; GetCursorPos(&pt);
        int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
        DestroyMenu(menu);
        if (cmd == 1) {
            if (!IsWindowVisible(g_hMainWnd)) ShowWindow(g_hMainWnd, SW_SHOW);
            SetForegroundWindow(g_hMainWnd);
        }
        else if (cmd == 2) {
            PostQuitMessage(0);
        }
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ==================== 主窗口 ====================
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HWND hDisp = CreateWindowW(L"STATIC", L"Click [Pick] to start",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            50, 40, 300, 70, hwnd, (HMENU)IDC_MAIN_DISPLAY, g_hInst, NULL);
        HFONT bigFont = CreateFontW(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft YaHei");
        SendMessageW(hDisp, WM_SETFONT, (WPARAM)bigFont, TRUE);

        CreateWindowW(L"BUTTON", L"Pick", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            70, 140, 100, 40, hwnd, (HMENU)IDC_BTN_PICK, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"Reset", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            210, 140, 100, 40, hwnd, (HMENU)IDC_BTN_RESET, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"Settings", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            140, 200, 100, 35, hwnd, (HMENU)IDC_BTN_SETTINGS, g_hInst, NULL);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_PICK: {
            Student* s = PickStudent();
            if (s) {
                wchar_t buf[256];
                swprintf_s(buf, L"[%d] %s", s->id, s->name.c_str());
                SetDlgItemTextW(hwnd, IDC_MAIN_DISPLAY, buf);
                g_config.Save();
            }
            else {
                SetDlgItemTextW(hwnd, IDC_MAIN_DISPLAY, L"All picked or whitelisted");
            }
            break;
        }
        case IDC_BTN_RESET:
            for (auto& s : g_config.students) s.picked = false;
            SetDlgItemTextW(hwnd, IDC_MAIN_DISPLAY, L"Reset. Click Pick to start");
            break;
        case IDC_BTN_SETTINGS:
            ShowSettingsModal(hwnd);
            break;
        }
        return 0;
    case WM_CLOSE:
        if (MessageBoxW(hwnd, L"Minimize to tray?\n\nYes = Hide to tray\nNo = Exit program",
            L"Confirm", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            ShowWindow(hwnd, SW_HIDE);
        }
        else {
            PostQuitMessage(0);
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ==================== 密码输入子类化 ====================
LRESULT CALLBACK EditSubProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
        SendMessageW(GetParent(hwnd), WM_COMMAND, IDOK, 0);
        return 0;
    }
    if (msg == WM_KEYDOWN && wParam == VK_ESCAPE) {
        SendMessageW(GetParent(hwnd), WM_COMMAND, IDCANCEL, 0);
        return 0;
    }
    return CallWindowProcW(g_oldEditProc, hwnd, msg, wParam, lParam);
}

// ==================== 密码验证窗口 ====================
LRESULT CALLBACK PassWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit;
    switch (msg) {
    case WM_CREATE:
        CreateWindowW(L"STATIC", L"Enter admin password:", WS_CHILD | WS_VISIBLE,
            20, 20, 200, 20, hwnd, NULL, g_hInst, NULL);
        hEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD,
            20, 50, 210, 24, hwnd, (HMENU)100, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            30, 90, 85, 28, hwnd, (HMENU)IDOK, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE,
            145, 90, 85, 28, hwnd, (HMENU)IDCANCEL, g_hInst, NULL);
        g_oldEditProc = (WNDPROC)SetWindowLongPtrW(hEdit, GWLP_WNDPROC, (LONG_PTR)EditSubProc);
        SetFocus(hEdit);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK && HIWORD(wParam) == BN_CLICKED) {
            wchar_t buf[64] = { 0 };
            GetWindowTextW(hEdit, buf, 64);
            if (std::wstring(buf) == g_config.password) {
                g_passwordOK = true;
                DestroyWindow(hwnd);
            }
            else {
                MessageBoxW(hwnd, L"Wrong password", L"Hint", MB_OK | MB_ICONWARNING);
                SetWindowTextW(hEdit, L"");
                SetFocus(hEdit);
            }
        }
        else if (LOWORD(wParam) == IDCANCEL && HIWORD(wParam) == BN_CLICKED) {
            g_passwordOK = false;
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_CLOSE:
        g_passwordOK = false;
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ==================== 设置窗口（三页 Tab） ====================
LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_sui.hTab = CreateWindowW(WC_TABCONTROLW, L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            10, 10, 500, 30, hwnd, (HMENU)IDC_TAB, g_hInst, NULL);
        TCITEM tie = { 0 }; tie.mask = TCIF_TEXT;
        tie.pszText = const_cast<wchar_t*>(L"List"); TabCtrl_InsertItem(g_sui.hTab, 0, &tie);
        tie.pszText = const_cast<wchar_t*>(L"Priority"); TabCtrl_InsertItem(g_sui.hTab, 1, &tie);
        tie.pszText = const_cast<wchar_t*>(L"Advanced"); TabCtrl_InsertItem(g_sui.hTab, 2, &tie);

        // ---- Page 1: List ----
        g_sui.hListName = CreateWindowW(L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | WS_BORDER,
            20, 50, 220, 300, hwnd, (HMENU)IDC_LIST_NAME, g_hInst, NULL);
        CreateWindowW(L"STATIC", L"ID:", WS_CHILD | WS_VISIBLE, 260, 60, 50, 20, hwnd, NULL, g_hInst, NULL);
        g_sui.hEditId = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
            330, 60, 100, 22, hwnd, (HMENU)IDC_EDIT_ID, g_hInst, NULL);
        CreateWindowW(L"STATIC", L"Name:", WS_CHILD | WS_VISIBLE, 260, 95, 50, 20, hwnd, NULL, g_hInst, NULL);
        g_sui.hEditName = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            330, 95, 150, 22, hwnd, (HMENU)IDC_EDIT_NAME, g_hInst, NULL);
        g_sui.hBtnAdd = CreateWindowW(L"BUTTON", L"Add/Edit",
            WS_CHILD | WS_VISIBLE, 260, 140, 100, 30, hwnd, (HMENU)IDC_BTN_ADD, g_hInst, NULL);
        g_sui.hBtnDel = CreateWindowW(L"BUTTON", L"Delete",
            WS_CHILD | WS_VISIBLE, 380, 140, 100, 30, hwnd, (HMENU)IDC_BTN_DEL, g_hInst, NULL);

        // ---- Page 2: Priority ----
        g_sui.hListPrio = CreateWindowW(L"LISTBOX", L"",
            WS_CHILD | WS_VSCROLL | LBS_NOTIFY | WS_BORDER,
            20, 50, 300, 300, hwnd, (HMENU)IDC_LIST_PRIO, g_hInst, NULL);
        CreateWindowW(L"STATIC", L"Adjust priority (+/-10% per level)", WS_CHILD,
            340, 60, 180, 20, hwnd, NULL, g_hInst, NULL);
        g_sui.hComboPrio = CreateWindowW(L"COMBOBOX", L"",
            WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
            340, 90, 150, 250, hwnd, (HMENU)IDC_COMBO_PRIO, g_hInst, NULL);
        for (int i = -5; i <= 5; i++) {
            wchar_t buf[32];
            swprintf_s(buf, L"%+d (%d%%)", i, 100 + i * 10);
            int idx = (int)SendMessageW(g_sui.hComboPrio, CB_ADDSTRING, 0, (LPARAM)buf);
            SendMessageW(g_sui.hComboPrio, CB_SETITEMDATA, idx, (LPARAM)i);
        }
        SendMessageW(g_sui.hComboPrio, CB_SETCURSEL, 5, 0);
        g_sui.hBtnSetPrio = CreateWindowW(L"BUTTON", L"Apply", WS_CHILD,
            340, 140, 100, 30, hwnd, (HMENU)IDC_BTN_SET_PRIO, g_hInst, NULL);

        // ---- Page 3: Advanced ----
        CreateWindowW(L"STATIC", L"Admin password:", WS_CHILD,
            30, 60, 80, 20, hwnd, NULL, g_hInst, NULL);
        g_sui.hEditPass = CreateWindowW(L"EDIT", L"",
            WS_CHILD | WS_BORDER | ES_PASSWORD,
            120, 60, 150, 22, hwnd, (HMENU)IDC_EDIT_PASS, g_hInst, NULL);
        g_sui.hChkFloat = CreateWindowW(L"BUTTON", L"Show float ball",
            WS_CHILD | BS_AUTOCHECKBOX,
            30, 100, 150, 20, hwnd, (HMENU)IDC_CHK_FLOAT, g_hInst, NULL);
        g_sui.hChkAuto = CreateWindowW(L"BUTTON", L"Auto start",
            WS_CHILD | BS_AUTOCHECKBOX,
            30, 130, 150, 20, hwnd, (HMENU)IDC_CHK_AUTO, g_hInst, NULL);
        g_sui.hChkRepeat = CreateWindowW(L"BUTTON", L"Allow repeat pick",
            WS_CHILD | BS_AUTOCHECKBOX,
            30, 155, 200, 20, hwnd, (HMENU)IDC_CHK_REPEAT, g_hInst, NULL);
        CreateWindowW(L"STATIC", L"Whitelist (never picked):", WS_CHILD,
            30, 170, 200, 20, hwnd, NULL, g_hInst, NULL);
        g_sui.hListWhite = CreateWindowW(L"LISTBOX", L"",
            WS_CHILD | WS_VSCROLL | WS_BORDER,
            30, 195, 200, 130, hwnd, (HMENU)IDC_LIST_WHITE, g_hInst, NULL);
        CreateWindowW(L"STATIC", L"Add student:", WS_CHILD,
            250, 195, 100, 20, hwnd, NULL, g_hInst, NULL);
        g_sui.hComboAll = CreateWindowW(L"COMBOBOX", L"",
            WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
            250, 220, 160, 200, hwnd, (HMENU)IDC_COMBO_ALL, g_hInst, NULL);
        g_sui.hBtnAddWhite = CreateWindowW(L"BUTTON", L"Add", WS_CHILD,
            250, 260, 80, 30, hwnd, (HMENU)IDC_BTN_ADD_WHITE, g_hInst, NULL);
        g_sui.hBtnRemWhite = CreateWindowW(L"BUTTON", L"Remove", WS_CHILD,
            340, 260, 80, 30, hwnd, (HMENU)IDC_BTN_REM_WHITE, g_hInst, NULL);

        g_sui.hBtnSave = CreateWindowW(L"BUTTON", L"Save & Close",
            WS_CHILD | WS_VISIBLE,
            380, 340, 120, 35, hwnd, (HMENU)IDC_BTN_SAVE, g_hInst, NULL);

        RefreshNameList();
        RefreshPrioList();
        RefreshWhiteList();
        RefreshAllCombo();
        SetWindowTextW(g_sui.hEditPass, g_config.password.c_str());
        CheckDlgButton(hwnd, IDC_CHK_FLOAT, g_config.showFloat ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_CHK_AUTO, g_config.autoStart ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwnd, IDC_CHK_REPEAT, g_config.allowRepeat ? BST_CHECKED : BST_UNCHECKED);

        ShowSettingsPage(0);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_LIST_NAME:
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                int idx = (int)SendMessageW(g_sui.hListName, LB_GETCURSEL, 0, 0);
                if (idx >= 0) {
                    int id = (int)SendMessageW(g_sui.hListName, LB_GETITEMDATA, idx, 0);
                    for (auto& s : g_config.students) {
                        if (s.id == id) {
                            SetWindowTextW(g_sui.hEditId, std::to_wstring(s.id).c_str());
                            SetWindowTextW(g_sui.hEditName, s.name.c_str());
                            break;
                        }
                    }
                }
            }
            break;
        case IDC_LIST_PRIO:
            if (HIWORD(wParam) == LBN_SELCHANGE) {
                int idx = (int)SendMessageW(g_sui.hListPrio, LB_GETCURSEL, 0, 0);
                if (idx >= 0) {
                    int id = (int)SendMessageW(g_sui.hListPrio, LB_GETITEMDATA, idx, 0);
                    for (auto& s : g_config.students) {
                        if (s.id == id) {
                            SendMessageW(g_sui.hComboPrio, CB_SETCURSEL, (WPARAM)(s.priority + 5), 0);
                            break;
                        }
                    }
                }
            }
            break;
        case IDC_BTN_ADD: {
            wchar_t idBuf[16] = { 0 }, nameBuf[64] = { 0 };
            GetWindowTextW(g_sui.hEditId, idBuf, 16);
            GetWindowTextW(g_sui.hEditName, nameBuf, 64);
            int id = _wtoi(idBuf);
            if (id > 0 && wcslen(nameBuf) > 0) {
                bool found = false;
                for (auto& s : g_config.students) {
                    if (s.id == id) { s.name = nameBuf; found = true; break; }
                }
                if (!found) {
                    Student ns = { id, nameBuf, 0, false, false };
                    g_config.students.push_back(ns);
                    std::sort(g_config.students.begin(), g_config.students.end(),
                        [](const Student& a, const Student& b) -> bool { return a.id < b.id; });
                }
                RefreshNameList(); RefreshPrioList(); RefreshAllCombo();
            }
            break;
        }
        case IDC_BTN_DEL: {
            int idx = (int)SendMessageW(g_sui.hListName, LB_GETCURSEL, 0, 0);
            if (idx >= 0) {
                int id = (int)SendMessageW(g_sui.hListName, LB_GETITEMDATA, idx, 0);
                g_config.students.erase(std::remove_if(g_config.students.begin(), g_config.students.end(),
                    [id](const Student& s) -> bool { return s.id == id; }), g_config.students.end());
                RefreshNameList(); RefreshPrioList(); RefreshWhiteList(); RefreshAllCombo();
                SetWindowTextW(g_sui.hEditId, L""); SetWindowTextW(g_sui.hEditName, L"");
            }
            break;
        }
        case IDC_BTN_SET_PRIO: {
            int idx = (int)SendMessageW(g_sui.hListPrio, LB_GETCURSEL, 0, 0);
            if (idx >= 0) {
                int id = (int)SendMessageW(g_sui.hListPrio, LB_GETITEMDATA, idx, 0);
                int comboIdx = (int)SendMessageW(g_sui.hComboPrio, CB_GETCURSEL, 0, 0);
                int prio = (int)SendMessageW(g_sui.hComboPrio, CB_GETITEMDATA, comboIdx, 0);
                for (auto& s : g_config.students) { if (s.id == id) { s.priority = prio; break; } }
                RefreshPrioList();
            }
            break;
        }
        case IDC_BTN_ADD_WHITE: {
            int comboIdx = (int)SendMessageW(g_sui.hComboAll, CB_GETCURSEL, 0, 0);
            if (comboIdx >= 0) {
                int id = (int)SendMessageW(g_sui.hComboAll, CB_GETITEMDATA, comboIdx, 0);
                for (auto& s : g_config.students) { if (s.id == id) { s.whitelist = true; break; } }
                RefreshWhiteList(); RefreshPrioList(); RefreshAllCombo();
            }
            break;
        }
        case IDC_BTN_REM_WHITE: {
            int idx = (int)SendMessageW(g_sui.hListWhite, LB_GETCURSEL, 0, 0);
            if (idx >= 0) {
                int id = (int)SendMessageW(g_sui.hListWhite, LB_GETITEMDATA, idx, 0);
                for (auto& s : g_config.students) { if (s.id == id) { s.whitelist = false; break; } }
                RefreshWhiteList(); RefreshPrioList(); RefreshAllCombo();
            }
            break;
        }
        case IDC_BTN_SAVE: {
            wchar_t passBuf[64] = { 0 };
            GetWindowTextW(g_sui.hEditPass, passBuf, 64);
            g_config.password = passBuf;
            g_config.showFloat = IsDlgButtonChecked(hwnd, IDC_CHK_FLOAT) == BST_CHECKED;
            g_config.autoStart = IsDlgButtonChecked(hwnd, IDC_CHK_AUTO) == BST_CHECKED;
            g_config.allowRepeat = IsDlgButtonChecked(hwnd, IDC_CHK_REPEAT) == BST_CHECKED;
            g_config.Save();
            SetAutoStart(g_config.autoStart);
            ShowWindow(g_hFloatBall, g_config.showFloat ? SW_SHOW : SW_HIDE);
            DestroyWindow(hwnd);
            break;
        }
        }
        return 0;
    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->idFrom == IDC_TAB && ((LPNMHDR)lParam)->code == TCN_SELCHANGE) {
            ShowSettingsPage(TabCtrl_GetCurSel(g_sui.hTab));
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ==================== 模态设置对话框 ====================
void ShowSettingsModal(HWND parent) {
    g_passwordOK = false;
    HWND hPass = CreateWindowExW(WS_EX_DLGMODALFRAME,
        L"PassWndClass", L"Password Required", WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 270, 170, parent, NULL, g_hInst, NULL);

    RECT rcP, rcD;
    GetWindowRect(parent, &rcP); GetWindowRect(hPass, &rcD);
    SetWindowPos(hPass, NULL,
        rcP.left + (rcP.right - rcP.left - (rcD.right - rcD.left)) / 2,
        rcP.top + (rcP.bottom - rcP.top - (rcD.bottom - rcD.top)) / 2,
        0, 0, SWP_NOSIZE | SWP_NOZORDER);

    EnableWindow(parent, FALSE);
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            SendMessageW(hPass, WM_CLOSE, 0, 0); continue;
        }
        TranslateMessage(&msg); DispatchMessageW(&msg);
        if (!IsWindow(hPass)) break;
    }
    EnableWindow(parent, TRUE); SetForegroundWindow(parent);
    if (!g_passwordOK) return;

    HWND hDlg = CreateWindowExW(WS_EX_DLGMODALFRAME,
        L"SettingsWndClass", L"Settings", WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 540, 420, parent, NULL, g_hInst, NULL);

    GetWindowRect(parent, &rcP); GetWindowRect(hDlg, &rcD);
    SetWindowPos(hDlg, NULL,
        rcP.left + (rcP.right - rcP.left - (rcD.right - rcD.left)) / 2,
        rcP.top + (rcP.bottom - rcP.top - (rcD.bottom - rcD.top)) / 2,
        0, 0, SWP_NOSIZE | SWP_NOZORDER);

    EnableWindow(parent, FALSE);
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            SendMessageW(hDlg, WM_CLOSE, 0, 0); continue;
        }
        TranslateMessage(&msg); DispatchMessageW(&msg);
        if (!IsWindow(hDlg)) break;
    }
    EnableWindow(parent, TRUE); SetForegroundWindow(parent);
}

// ==================== 入口 ====================
ATOM MyRegisterClass(const wchar_t* name, WNDPROC proc, HBRUSH brush) {
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = proc;
    wc.hInstance = g_hInst;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = brush ? brush : (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = name;
    return RegisterClassExW(&wc);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    g_hInst = hInstance;
    InitCommonControls();
    g_config.Load();
    SetAutoStart(g_config.autoStart);

    MyRegisterClass(L"FloatBallClass", FloatBallProc, (HBRUSH)GetStockObject(NULL_BRUSH));
    MyRegisterClass(L"MainWndClass", MainWndProc, NULL);
    MyRegisterClass(L"PassWndClass", PassWndProc, NULL);
    MyRegisterClass(L"SettingsWndClass", SettingsWndProc, NULL);

    g_hFloatBall = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED,
        L"FloatBallClass", L"", WS_POPUP | WS_VISIBLE,
        g_config.floatX, g_config.floatY, 60, 60, NULL, NULL, g_hInst, NULL);

    g_hMainWnd = CreateWindowW(L"MainWndClass", L"Random Pick",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 420, 300, NULL, NULL, g_hInst, NULL);

    if (!g_config.showFloat) ShowWindow(g_hFloatBall, SW_HIDE);
    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}