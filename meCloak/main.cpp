#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>
#include <map>
#include <vector>
#include <random>
#include <fstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

#ifndef EM_SETBKGNDCOLOR
#define EM_SETBKGNDCOLOR (ECM_FIRST + 0x0004)
#endif
#ifndef ECM_FIRST
#define ECM_FIRST 0x1500
#endif

class CipherEngine {
private:
    unsigned long long seed;
    std::map<wchar_t, std::wstring> textEncMap;
    std::map<std::wstring, wchar_t> textDecMap;
    std::wstring alphabet;
    std::wstring codeChars;

    std::string ws2s(const std::wstring& wstr) {
        std::string result;
        for (wchar_t c : wstr) result += (char)c;
        return result;
    }

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

    void BuildTextTable() {
        textEncMap.clear();
        textDecMap.clear();
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
                for (int j = 0; j < len; ++j) code += codeChars[charDist(rng)];
                unique = true;
                for (const auto& used : usedCodes) {
                    if (used == code) { unique = false; break; }
                }
                attempts++;
            }
            usedCodes.push_back(code);
            textEncMap[ch] = code;
            textDecMap[code] = ch;
        }
    }

    class GammaGenerator {
        std::mt19937_64 rng;
    public:
        GammaGenerator(unsigned long long s) : rng(s) {}
        unsigned char Next() { return (unsigned char)(rng() & 0xFF); }
    };

public:
    CipherEngine() : seed(0) { BuildAlphabet(); SetSeed(12345); }

    void SetSeed(unsigned long long newSeed) { seed = newSeed; BuildTextTable(); }

    unsigned long long GenerateRandomSeed() {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        return std::uniform_int_distribution<unsigned long long>(1, 999999999999999999ULL)(gen);
    }

    std::wstring EncryptText(const std::wstring& text) {
        std::wstring result;
        for (wchar_t ch : text) {
            auto it = textEncMap.find(ch);
            result += (it != textEncMap.end()) ? it->second : std::wstring(1, ch);
        }
        return result;
    }

    std::wstring DecryptText(const std::wstring& text) {
        std::wstring result;
        size_t i = 0;
        while (i < text.length()) {
            bool found = false;
            for (int len = 3; len >= 1 && !found; --len) {
                if (i + len <= text.length()) {
                    std::wstring code = text.substr(i, len);
                    auto it = textDecMap.find(code);
                    if (it != textDecMap.end()) { result += it->second; i += len; found = true; }
                }
            }
            if (!found) { result += text[i]; ++i; }
        }
        return result;
    }

    bool EncryptFile(const std::wstring& inputPath, const std::wstring& outputPath) {
        std::string inPath = ws2s(inputPath);
        std::string outPath = ws2s(outputPath);
        std::ifstream in(inPath.c_str(), std::ios::binary);
        if (!in) return false;
        std::ofstream out(outPath.c_str(), std::ios::binary);
        if (!out) return false;
        out.write((char*)&seed, sizeof(seed));
        GammaGenerator gamma(seed);
        unsigned long long counter = 0;
        char byte;
        while (in.get(byte)) {
            unsigned char keyByte = gamma.Next() ^ (unsigned char)(counter & 0xFF);
            out.put((char)((unsigned char)byte ^ keyByte));
            counter++;
        }
        return true;
    }

    bool DecryptFile(const std::wstring& inputPath, const std::wstring& outputPath, unsigned long long currentSeed = 0) {
        std::string inPath = ws2s(inputPath);
        std::string outPath = ws2s(outputPath);
        std::ifstream in(inPath.c_str(), std::ios::binary);
        if (!in) return false;
        unsigned long long fileSeed;
        in.read((char*)&fileSeed, sizeof(fileSeed));
        if (currentSeed != 0 && fileSeed != currentSeed) return false;
        std::ofstream out(outPath.c_str(), std::ios::binary);
        if (!out) return false;
        GammaGenerator gamma(fileSeed);
        unsigned long long counter = 0;
        char byte;
        while (in.get(byte)) {
            unsigned char keyByte = gamma.Next() ^ (unsigned char)(counter & 0xFF);
            out.put((char)((unsigned char)byte ^ keyByte));
            counter++;
        }
        return true;
    }

    std::wstring GetSeedString() { return std::to_wstring(seed); }
    unsigned long long GetSeed() { return seed; }

    int GetStrength() {
        std::string s = std::to_string(seed);
        int score = std::min(100, (int)s.length() * 10);
        for (char c : s) if (isdigit(c)) score += 2;
        if (s.length() > 10) score += 20;
        return std::min(100, score);
    }
};

CipherEngine g_cipher;
HWND hMainWnd, hSeedEdit, hInputEdit, hOutputEdit, hStatusBar, hStrengthBar;
bool g_darkTheme = true;
HBRUSH g_darkBrush = NULL, g_lightBrush = NULL;
COLORREF g_darkBg = RGB(25, 25, 35);
COLORREF g_darkText = RGB(0, 255, 150);
COLORREF g_lightBg = RGB(240, 240, 245);
COLORREF g_lightText = RGB(20, 20, 30);
HFONT g_font = NULL, g_titleFont = NULL;

void UpdateStrength() {
    int strength = g_cipher.GetStrength();
    SendMessage(hStrengthBar, PBM_SETPOS, strength, 0);
    COLORREF color;
    if (strength < 30) color = RGB(255, 60, 60);
    else if (strength < 65) color = RGB(255, 180, 0);
    else color = RGB(0, 230, 100);
    SendMessage(hStrengthBar, PBM_SETBARCOLOR, 0, color);
    std::wstring status;
    if (strength > 80) status = L"Strong protection";
    else if (strength > 40) status = L"Medium protection";
    else status = L"Weak - use longer seed";
    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)status.c_str());
}

void ApplyTheme(HWND hWnd) {
    SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)(g_darkTheme ? g_darkBrush : g_lightBrush));
    InvalidateRect(hWnd, NULL, TRUE);
}

void SetEditTheme(HWND hEdit) {
    SendMessage(hEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)(g_darkTheme ? g_darkBg : g_lightBg));
    InvalidateRect(hEdit, NULL, TRUE);
}

void CopyToClipboard(const std::wstring& text) {
    if (text.empty() || !OpenClipboard(hMainWnd)) return;
    EmptyClipboard();
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, (text.length() + 1) * sizeof(wchar_t));
    if (hGlobal) {
        wchar_t* p = (wchar_t*)GlobalLock(hGlobal);
        if (p) { wcscpy_s(p, text.length() + 1, text.c_str()); GlobalUnlock(hGlobal); }
        SetClipboardData(CF_UNICODETEXT, hGlobal);
    }
    CloseClipboard();
}

std::wstring GetDroppedFile(HDROP hDrop) {
    wchar_t path[MAX_PATH];
    DragQueryFile(hDrop, 0, path, MAX_PATH);
    return std::wstring(path);
}

std::wstring SaveFileDialog(const std::wstring& defaultName) {
    wchar_t path[MAX_PATH] = {0};
    wcscpy_s(path, defaultName.c_str());
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = L"Encrypted Files (*.enc)\0*.enc\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (GetSaveFileName(&ofn)) return std::wstring(path);
    return L"";
}

std::wstring OpenFileDialog() {
    wchar_t path[MAX_PATH] = {0};
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0Encrypted Files (*.enc)\0*.enc\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileName(&ofn)) return std::wstring(path);
    return L"";
}

void ProcessFile(const std::wstring& path, bool forceDecrypt = false) {
    bool isEnc = (path.length() > 4 && path.substr(path.length() - 4) == L".enc");
    if (isEnc || forceDecrypt) {
        std::wstring outPath = path;
        if (outPath.length() > 4 && outPath.substr(outPath.length() - 4) == L".enc")
            outPath = outPath.substr(0, outPath.length() - 4);
        else outPath += L".decrypted";
        std::wstring savePath = SaveFileDialog(outPath);
        if (savePath.empty()) return;
        if (g_cipher.DecryptFile(path, savePath, g_cipher.GetSeed())) {
            MessageBox(hMainWnd, (L"File decrypted!\nSaved: " + savePath).c_str(), L"Success", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBox(hMainWnd, L"Decryption failed! Wrong seed or corrupted file.", L"Error", MB_ICONERROR);
        }
    } else {
        std::wstring outPath = SaveFileDialog(path + L".enc");
        if (outPath.empty()) return;
        if (g_cipher.EncryptFile(path, outPath)) {
            MessageBox(hMainWnd, (L"File encrypted!\nSaved: " + outPath).c_str(), L"Success", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBox(hMainWnd, L"Encryption failed!", L"Error", MB_ICONERROR);
        }
    }
}

void OnEncrypt() {
    wchar_t buffer[8192];
    GetWindowText(hInputEdit, buffer, 8192);
    std::wstring input(buffer);
    if (input.empty()) { SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Type text or select file"); return; }
    SetWindowText(hOutputEdit, g_cipher.EncryptText(input).c_str());
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Text encrypted");
}

void OnDecrypt() {
    wchar_t buffer[8192];
    GetWindowText(hInputEdit, buffer, 8192);
    std::wstring input(buffer);
    if (input.empty()) return;
    SetWindowText(hOutputEdit, g_cipher.DecryptText(input).c_str());
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Text decrypted");
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_font = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
            g_titleFont = CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, FF_MODERN, L"Segoe UI");

            int y = 8;
            HWND hTitle = CreateWindow(L"STATIC", L"meCloak", WS_CHILD | WS_VISIBLE, 10, y, 580, 28, hWnd, NULL, NULL, NULL);
            SendMessage(hTitle, WM_SETFONT, (WPARAM)g_titleFont, TRUE);
            y += 32;

            CreateWindow(L"STATIC", L"Seed:", WS_CHILD | WS_VISIBLE, 10, y, 45, 22, hWnd, NULL, NULL, NULL);
            hSeedEdit = CreateWindow(L"EDIT", L"12345", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 58, y, 140, 24, hWnd, NULL, NULL, NULL);
            SendMessage(hSeedEdit, WM_SETFONT, (WPARAM)g_font, TRUE);
            CreateWindow(L"BUTTON", L"Apply", WS_CHILD | WS_VISIBLE, 208, y, 55, 24, hWnd, (HMENU)1001, NULL, NULL);
            CreateWindow(L"BUTTON", L"Random", WS_CHILD | WS_VISIBLE, 270, y, 55, 24, hWnd, (HMENU)1002, NULL, NULL);
            hStrengthBar = CreateWindow(PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE, 335, y, 245, 20, hWnd, NULL, NULL, NULL);
            SendMessage(hStrengthBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            UpdateStrength();
            y += 32;

            CreateWindow(L"STATIC", L"File:", WS_CHILD | WS_VISIBLE, 10, y, 35, 22, hWnd, NULL, NULL, NULL);
            CreateWindow(L"BUTTON", L"Select File...", WS_CHILD | WS_VISIBLE, 50, y, 95, 24, hWnd, (HMENU)1010, NULL, NULL);
            CreateWindow(L"STATIC", L"or drag & drop files here", WS_CHILD | WS_VISIBLE, 155, y, 200, 22, hWnd, NULL, NULL, NULL);
            y += 30;

            CreateWindow(L"STATIC", L"Text Input:", WS_CHILD | WS_VISIBLE, 10, y, 80, 20, hWnd, NULL, NULL, NULL);
            y += 22;
            hInputEdit = CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL, 10, y, 580, 100, hWnd, NULL, NULL, NULL);
            SendMessage(hInputEdit, WM_SETFONT, (WPARAM)g_font, TRUE);
            y += 108;

            CreateWindow(L"BUTTON", L"Encrypt (Ctrl+E)", WS_CHILD | WS_VISIBLE, 10, y, 135, 32, hWnd, (HMENU)1004, NULL, NULL);
            CreateWindow(L"BUTTON", L"Decrypt (Ctrl+D)", WS_CHILD | WS_VISIBLE, 155, y, 135, 32, hWnd, (HMENU)1005, NULL, NULL);
            CreateWindow(L"BUTTON", L"Copy Output", WS_CHILD | WS_VISIBLE, 300, y, 100, 32, hWnd, (HMENU)1006, NULL, NULL);
            CreateWindow(L"BUTTON", L"Copy Seed", WS_CHILD | WS_VISIBLE, 410, y, 100, 32, hWnd, (HMENU)1007, NULL, NULL);
            CreateWindow(L"BUTTON", L"Theme", WS_CHILD | WS_VISIBLE, 520, y, 70, 32, hWnd, (HMENU)1008, NULL, NULL);
            y += 40;

            CreateWindow(L"STATIC", L"Output:", WS_CHILD | WS_VISIBLE, 10, y, 80, 20, hWnd, NULL, NULL, NULL);
            y += 22;
            hOutputEdit = CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 10, y, 580, 130, hWnd, NULL, NULL, NULL);
            SendMessage(hOutputEdit, WM_SETFONT, (WPARAM)g_font, TRUE);

            hStatusBar = CreateWindow(STATUSCLASSNAME, L"", WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hWnd, NULL, NULL, NULL);
            int widths[] = {350, -1};
            SendMessage(hStatusBar, SB_SETPARTS, 2, (LPARAM)widths);
            SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Ready - Drag files, select file, or type text");

            ApplyTheme(hWnd);
            SetEditTheme(hSeedEdit);
            SetEditTheme(hInputEdit);
            SetEditTheme(hOutputEdit);
            DragAcceptFiles(hWnd, TRUE);
            SetTimer(hWnd, 1, 500, NULL);
            break;
        }

        case WM_TIMER: {
            wchar_t buf[256];
            GetWindowText(hSeedEdit, buf, 256);
            try { g_cipher.SetSeed(std::stoull(buf)); UpdateStrength(); } catch (...) {}
            break;
        }

        case WM_DROPFILES: {
            std::wstring path = GetDroppedFile((HDROP)wParam);
            DragFinish((HDROP)wParam);
            ProcessFile(path);
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case 1001: {
                    wchar_t buf[256];
                    GetWindowText(hSeedEdit, buf, 256);
                    try { g_cipher.SetSeed(std::stoull(buf)); UpdateStrength(); }
                    catch (...) { MessageBox(hWnd, L"Invalid seed!", L"Error", MB_ICONERROR); }
                    break;
                }
                case 1002: {
                    unsigned long long s = g_cipher.GenerateRandomSeed();
                    SetWindowText(hSeedEdit, std::to_wstring(s).c_str());
                    g_cipher.SetSeed(s);
                    UpdateStrength();
                    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Random seed generated");
                    break;
                }
                case 1004: OnEncrypt(); break;
                case 1005: OnDecrypt(); break;
                case 1006: {
                    wchar_t buf[8192];
                    GetWindowText(hOutputEdit, buf, 8192);
                    CopyToClipboard(buf);
                    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Output copied to clipboard");
                    break;
                }
                case 1007:
                    CopyToClipboard(g_cipher.GetSeedString());
                    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Seed copied to clipboard");
                    break;
                case 1008:
                    g_darkTheme = !g_darkTheme;
                    ApplyTheme(hWnd);
                    SetEditTheme(hSeedEdit);
                    SetEditTheme(hInputEdit);
                    SetEditTheme(hOutputEdit);
                    InvalidateRect(hWnd, NULL, TRUE);
                    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)(g_darkTheme ? L"Dark theme" : L"Light theme"));
                    break;
                case 1010: {
                    std::wstring path = OpenFileDialog();
                    if (!path.empty()) ProcessFile(path);
                    break;
                }
            }
            break;

        case WM_KEYDOWN:
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                if (wParam == 'E') OnEncrypt();
                else if (wParam == 'D') OnDecrypt();
            }
            break;

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, g_darkTheme ? g_darkText : g_lightText);
            SetBkColor(hdc, g_darkTheme ? g_darkBg : g_lightBg);
            return (LRESULT)(g_darkTheme ? g_darkBrush : g_lightBrush);
        }

        case WM_SIZE: {
            RECT rc;
            GetClientRect(hWnd, &rc);
            SendMessage(hStatusBar, WM_SIZE, 0, 0);
            int w = rc.right - 20;
            if (w > 0) {
                SetWindowPos(hInputEdit, NULL, 0, 0, w, 100, SWP_NOMOVE | SWP_NOZORDER);
                SetWindowPos(hOutputEdit, NULL, 0, 0, w, 130, SWP_NOMOVE | SWP_NOZORDER);
            }
            break;
        }

        case WM_DESTROY:
            DeleteObject(g_font);
            DeleteObject(g_titleFont);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icex = {sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_BAR_CLASSES | ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&icex);
    g_darkBrush = CreateSolidBrush(g_darkBg);
    g_lightBrush = CreateSolidBrush(g_lightBg);

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = g_darkBrush;
    wc.lpszClassName = L"meCloak";
    RegisterClass(&wc);

    hMainWnd = CreateWindow(L"meCloak", L"meCloak - File & Text Encryption",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 620, 700,
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
