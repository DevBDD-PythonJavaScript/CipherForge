#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <map>
#include <vector>
#include <random>
#include <fstream>

#pragma comment(lib, "comctl32.lib")

#ifndef EM_SETBKGNDCOLOR
#define EM_SETBKGNDCOLOR (ECM_FIRST + 0x0004)
#endif
#ifndef ECM_FIRST
#define ECM_FIRST 0x1500
#endif

class CipherEngine {
private:
    unsigned long long seed;
    std::map<wchar_t, std::wstring> encryptMap;
    std::map<std::wstring, wchar_t> decryptMap;
    std::wstring alphabet;
    std::wstring codeChars;

    void BuildAlphabet() {
        alphabet.clear();
        for (wchar_t c = L'А'; c <= L'Я'; ++c) alphabet += c;
        alphabet += L'Ё';
        for (wchar_t c = L'а'; c <= L'я'; ++c) alphabet += c;
        alphabet += L'ё';
        for (wchar_t c = L'A'; c <= L'Z'; ++c) alphabet += c;
        for (wchar_t c = L'a'; c <= L'z'; ++c) alphabet += c;
        for (wchar_t c = L'0'; c <= L'9'; ++c) alphabet += c;
        alphabet += L" !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

        codeChars.clear();
        for (wchar_t c = L'A'; c <= L'Z'; ++c) codeChars += c;
        for (wchar_t c = L'a'; c <= L'z'; ++c) codeChars += c;
        for (wchar_t c = L'0'; c <= L'9'; ++c) codeChars += c;
        codeChars += L"!@#$%^&*()_+-=[]{}|;:,.<>?/";
    }

    void GenerateMapping() {
        encryptMap.clear();
        decryptMap.clear();

        std::mt19937_64 rng(seed);
        std::uniform_int_distribution<int> lenDist(1, 3);
        std::uniform_int_distribution<int> charDist(0, (int)codeChars.length() - 1);
        std::vector<std::wstring> usedCodes;

        for (size_t i = 0; i < alphabet.length(); ++i) {
            wchar_t ch = alphabet[i];
            std::wstring code;
            bool unique = false;
            int attempts = 0;

            while (!unique && attempts < 1000) {
                int len = lenDist(rng);
                code.clear();
                for (int j = 0; j < len; ++j) {
                    code += codeChars[charDist(rng)];
                }
                unique = true;
                for (const auto& used : usedCodes) {
                    if (used == code) { unique = false; break; }
                }
                attempts++;
            }

            usedCodes.push_back(code);
            encryptMap[ch] = code;
            decryptMap[code] = ch;
        }
    }

public:
    CipherEngine() : seed(0) {
        BuildAlphabet();
        SetSeed(12345);
    }

    bool SetSeed(unsigned long long newSeed) {
        seed = newSeed;
        GenerateMapping();
        return true;
    }

    unsigned long long GenerateRandomSeed() {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<unsigned long long> dis(1, 999999999999999999ULL);
        return dis(gen);
    }

    std::wstring Encrypt(const std::wstring& text) {
        std::wstring result;
        for (wchar_t ch : text) {
            auto it = encryptMap.find(ch);
            result += (it != encryptMap.end()) ? it->second : std::wstring(1, ch);
        }
        return result;
    }

    std::wstring Decrypt(const std::wstring& text) {
        std::wstring result;
        size_t i = 0;
        while (i < text.length()) {
            bool found = false;
            for (int len = 3; len >= 1 && !found; --len) {
                if (i + len <= text.length()) {
                    std::wstring code = text.substr(i, len);
                    auto it = decryptMap.find(code);
                    if (it != decryptMap.end()) {
                        result += it->second;
                        i += len;
                        found = true;
                    }
                }
            }
            if (!found) { result += text[i]; ++i; }
        }
        return result;
    }

    std::wstring GetSeedString() { return std::to_wstring(seed); }
    unsigned long long GetSeed() { return seed; }

    bool SaveSeed(const std::wstring& filename) {
        std::wofstream file;
        file.open(filename.c_str());
        if (!file.is_open()) return false;
        file << seed;
        file.close();
        return true;
    }

    bool LoadSeed(const std::wstring& filename) {
        std::wifstream file;
        file.open(filename.c_str());
        if (!file.is_open()) return false;
        file >> seed;
        file.close();
        GenerateMapping();
        return true;
    }
};

CipherEngine g_cipher;
HWND hMainWnd, hSeedEdit, hInputEdit, hOutputEdit, hStatusBar, hSeedDisplay;
bool g_darkTheme = false;
HBRUSH g_darkBrush = NULL;
HBRUSH g_lightBrush = NULL;
COLORREF g_darkBg = RGB(30, 30, 30);
COLORREF g_darkText = RGB(240, 240, 240);
COLORREF g_lightBg = RGB(255, 255, 255);
COLORREF g_lightText = RGB(0, 0, 0);

void ApplyTheme(HWND hWnd) {
    SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND,
        (LONG_PTR)(g_darkTheme ? g_darkBrush : g_lightBrush));
    InvalidateRect(hWnd, NULL, TRUE);
}

void SetEditTheme(HWND hEdit) {
    SendMessage(hEdit, EM_SETBKGNDCOLOR, 0,
        (LPARAM)(g_darkTheme ? g_darkBg : g_lightBg));
    InvalidateRect(hEdit, NULL, TRUE);
}

void UpdateStatusBar(const std::wstring& text) {
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)text.c_str());
}

void CopyToClipboard(HWND hWnd, const std::wstring& text) {
    if (text.empty()) return;
    if (OpenClipboard(hWnd)) {
        EmptyClipboard();
        size_t size = (text.length() + 1) * sizeof(wchar_t);
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
        if (hGlobal) {
            wchar_t* pGlobal = (wchar_t*)GlobalLock(hGlobal);
            if (pGlobal) {
                wcscpy_s(pGlobal, text.length() + 1, text.c_str());
                GlobalUnlock(hGlobal);
                SetClipboardData(CF_UNICODETEXT, hGlobal);
            }
        }
        CloseClipboard();
    }
}

void UpdateSeedDisplay() {
    std::wstring seedStr = L"Seed: " + g_cipher.GetSeedString();
    SetWindowText(hSeedDisplay, seedStr.c_str());
}

void OnApplySeed() {
    wchar_t buffer[256];
    GetWindowText(hSeedEdit, buffer, 256);
    std::wstring seedStr(buffer);
    try {
        unsigned long long seed = std::stoull(seedStr);
        g_cipher.SetSeed(seed);
        UpdateSeedDisplay();
        UpdateStatusBar(L"Seed applied: " + seedStr);
    } catch (...) {
        MessageBox(hMainWnd, L"Invalid seed format", L"Error", MB_ICONERROR);
    }
}

void OnRandomSeed() {
    unsigned long long seed = g_cipher.GenerateRandomSeed();
    std::wstring seedStr = std::to_wstring(seed);
    SetWindowText(hSeedEdit, seedStr.c_str());
    g_cipher.SetSeed(seed);
    UpdateSeedDisplay();
    UpdateStatusBar(L"Random seed: " + seedStr);
}

void OnEncrypt() {
    wchar_t buffer[4096];
    GetWindowText(hInputEdit, buffer, 4096);
    std::wstring input(buffer);
    if (input.empty()) {
        UpdateStatusBar(L"Enter text to encrypt");
        return;
    }
    SetWindowText(hOutputEdit, g_cipher.Encrypt(input).c_str());
    UpdateStatusBar(L"Encrypted");
}

void OnDecrypt() {
    wchar_t buffer[4096];
    GetWindowText(hInputEdit, buffer, 4096);
    std::wstring input(buffer);
    if (input.empty()) {
        UpdateStatusBar(L"Enter text to decrypt");
        return;
    }
    SetWindowText(hOutputEdit, g_cipher.Decrypt(input).c_str());
    UpdateStatusBar(L"Decrypted");
}

void OnCopyResult() {
    wchar_t buffer[4096];
    GetWindowText(hOutputEdit, buffer, 4096);
    std::wstring text(buffer);
    if (text.empty()) { UpdateStatusBar(L"Nothing to copy"); return; }
    CopyToClipboard(hMainWnd, text);
    UpdateStatusBar(L"Copied to clipboard");
}

void OnCopySeed() {
    std::wstring seedStr = g_cipher.GetSeedString();
    CopyToClipboard(hMainWnd, seedStr);
    UpdateStatusBar(L"Seed copied: " + seedStr);
}

void OnSaveSeed() {
    OPENFILENAME ofn = {0};
    wchar_t filename[MAX_PATH] = L"seed.txt";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    if (GetSaveFileName(&ofn)) {
        if (g_cipher.SaveSeed(filename)) {
            UpdateStatusBar(L"Seed saved: " + std::wstring(filename));
        } else {
            MessageBox(hMainWnd, L"Failed to save file", L"Error", MB_ICONERROR);
        }
    }
}

void OnLoadSeed() {
    OPENFILENAME ofn = {0};
    wchar_t filename[MAX_PATH] = L"";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;
    if (GetOpenFileName(&ofn)) {
        if (g_cipher.LoadSeed(filename)) {
            SetWindowText(hSeedEdit, g_cipher.GetSeedString().c_str());
            UpdateSeedDisplay();
            UpdateStatusBar(L"Seed loaded: " + std::wstring(filename));
        } else {
            MessageBox(hMainWnd, L"Failed to load file", L"Error", MB_ICONERROR);
        }
    }
}

void OnToggleTheme() {
    g_darkTheme = !g_darkTheme;
    ApplyTheme(hMainWnd);
    HWND controls[] = {hSeedEdit, hInputEdit, hOutputEdit};
    for (HWND hCtrl : controls) SetEditTheme(hCtrl);
    InvalidateRect(hSeedDisplay, NULL, TRUE);
    UpdateStatusBar(g_darkTheme ? L"Dark theme" : L"Light theme");
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            int y = 10;

            CreateWindow(L"STATIC", L"Seed:", WS_CHILD | WS_VISIBLE,
                10, y, 50, 20, hWnd, NULL, NULL, NULL);

            hSeedEdit = CreateWindow(L"EDIT", L"12345",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
                60, y, 180, 22, hWnd, NULL, NULL, NULL);

            CreateWindow(L"BUTTON", L"Apply", WS_CHILD | WS_VISIBLE,
                250, y, 60, 22, hWnd, (HMENU)1001, NULL, NULL);

            CreateWindow(L"BUTTON", L"Random", WS_CHILD | WS_VISIBLE,
                320, y, 60, 22, hWnd, (HMENU)1002, NULL, NULL);

            y += 30;

            hSeedDisplay = CreateWindow(L"STATIC", L"Seed: 12345",
                WS_CHILD | WS_VISIBLE, 10, y, 300, 20, hWnd, NULL, NULL, NULL);

            y += 30;

            CreateWindow(L"STATIC", L"Input:", WS_CHILD | WS_VISIBLE,
                10, y, 100, 20, hWnd, NULL, NULL, NULL);

            y += 20;
            hInputEdit = CreateWindow(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL,
                10, y, 580, 80, hWnd, NULL, NULL, NULL);

            y += 90;

            CreateWindow(L"BUTTON", L"Encrypt", WS_CHILD | WS_VISIBLE,
                10, y, 100, 30, hWnd, (HMENU)1004, NULL, NULL);

            CreateWindow(L"BUTTON", L"Decrypt", WS_CHILD | WS_VISIBLE,
                120, y, 100, 30, hWnd, (HMENU)1005, NULL, NULL);

            CreateWindow(L"BUTTON", L"Copy Result", WS_CHILD | WS_VISIBLE,
                230, y, 100, 30, hWnd, (HMENU)1006, NULL, NULL);

            CreateWindow(L"BUTTON", L"Copy Seed", WS_CHILD | WS_VISIBLE,
                340, y, 100, 30, hWnd, (HMENU)1007, NULL, NULL);

            y += 40;

            CreateWindow(L"BUTTON", L"Save Seed", WS_CHILD | WS_VISIBLE,
                10, y, 100, 30, hWnd, (HMENU)1008, NULL, NULL);

            CreateWindow(L"BUTTON", L"Load Seed", WS_CHILD | WS_VISIBLE,
                120, y, 100, 30, hWnd, (HMENU)1009, NULL, NULL);

            y += 40;

            CreateWindow(L"STATIC", L"Output:", WS_CHILD | WS_VISIBLE,
                10, y, 100, 20, hWnd, NULL, NULL, NULL);

            y += 20;
            hOutputEdit = CreateWindow(L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                10, y, 580, 80, hWnd, NULL, NULL, NULL);

            hStatusBar = CreateWindow(STATUSCLASSNAME, L"Ready",
                WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                0, 0, 0, 0, hWnd, NULL, NULL, NULL);

            ApplyTheme(hWnd);
            SetEditTheme(hSeedEdit);
            SetEditTheme(hInputEdit);
            SetEditTheme(hOutputEdit);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case 1001: OnApplySeed(); break;
                case 1002: OnRandomSeed(); break;
                case 1004: OnEncrypt(); break;
                case 1005: OnDecrypt(); break;
                case 1006: OnCopyResult(); break;
                case 1007: OnCopySeed(); break;
                case 1008: OnSaveSeed(); break;
                case 1009: OnLoadSeed(); break;
            }
            break;

        case WM_KEYDOWN:
            if (wParam == 'D' && GetKeyState(VK_MENU) & 0x8000) OnToggleTheme();
            break;

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            if (g_darkTheme) {
                SetTextColor(hdc, g_darkText);
                SetBkColor(hdc, g_darkBg);
                return (LRESULT)g_darkBrush;
            } else {
                SetTextColor(hdc, g_lightText);
                SetBkColor(hdc, g_lightBg);
                return (LRESULT)g_lightBrush;
            }
        }

        case WM_SIZE: {
            RECT rc;
            GetClientRect(hWnd, &rc);
            SendMessage(hStatusBar, WM_SIZE, 0, 0);
            int w = rc.right - 20;
            if (w > 0) {
                SetWindowPos(hInputEdit, NULL, 0, 0, w, 80, SWP_NOMOVE | SWP_NOZORDER);
                SetWindowPos(hOutputEdit, NULL, 0, 0, w, 80, SWP_NOMOVE | SWP_NOZORDER);
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icex = {sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES};
    InitCommonControlsEx(&icex);

    g_darkBrush = CreateSolidBrush(g_darkBg);
    g_lightBrush = CreateSolidBrush(g_lightBg);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = g_lightBrush;
    wc.lpszClassName = L"CipherForge";
    RegisterClass(&wc);

    hMainWnd = CreateWindow(L"CipherForge", L"CipherForge v2.0",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 620, 580,
        NULL, NULL, hInstance, NULL);

    if (!hMainWnd) return 1;

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DeleteObject(g_darkBrush);
    DeleteObject(g_lightBrush);
    return (int)msg.wParam;
}