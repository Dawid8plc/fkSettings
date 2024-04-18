typedef struct IUnknown IUnknown;

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#include <filesystem>

#ifdef _X86_
extern "C" { int _afxForceUSRDLL; }
#else
extern "C" { int __afxForceUSRDLL; }
#endif

#include "include/MinHook.h"
#include "Hooks.h"

#include <sstream>
#include <fstream>

#pragma comment(lib,"user32.lib") 
#pragma comment(lib,"libs\\libMinHook.x86.lib")

#define BUTTON_IDENTIFIER 1

std::string lang;
CButton advancedOptionsBtn;
LPCTSTR advancedOptionsLabel;

//Run the settings app if it exists
void HandleButtonClick(HWND hWnd)
{
    bool exists = std::filesystem::exists("settings.exe");

    if (exists)
    {
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        CreateProcessA("settings.exe", NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}


WNDPROC g_pOldWndProc = nullptr;

LRESULT CALLBACK ParentWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);
        if (wmEvent == BN_CLICKED)
        {
            // Handle button click
            HWND hButtonClicked = (HWND)lParam;

            auto data = GetWindowLongPtr(hButtonClicked, GWLP_USERDATA);

            //Advanced Options button
            if(data == 1)
                HandleButtonClick(hButtonClicked);
            else //Something else has been clicked, let the frontend handle it
                return CallWindowProc(g_pOldWndProc, hWnd, message, wParam, lParam);
        }
    }
    break;
    default:
        return CallWindowProc(g_pOldWndProc, hWnd, message, wParam, lParam);
    }
    return 0;
}

typedef HWND(WINAPI* CreateDialogIndirectParamAType)(HINSTANCE hInstance, LPCDLGTEMPLATEA lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
CreateDialogIndirectParamAType pCreateDialogIndirectParamA = nullptr; //original function pointer after hook
CreateDialogIndirectParamAType pCreateDialogIndirectParamATarget; //original function pointer BEFORE hook do not call this!
HWND WINAPI detourCreateDialogIndirectParamA(HINSTANCE hInstance, LPCDLGTEMPLATEA lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
    auto returnVal = pCreateDialogIndirectParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);

    if (returnVal != NULL && !(advancedOptionsBtn.GetSafeHwnd() && ::IsWindow(advancedOptionsBtn.GetSafeHwnd()))) {
        CWnd* pWnd = CWnd::FromHandle(returnVal);

        CString title;
        pWnd->GetWindowTextW(title);

        if (title == L"Video options" || title == L"Настройки видео" || title == L"Opcje grafiki" 
            || title == L"Opzioni video" || title == L"Options vidéo" || title == L"Video alternativ"
            || title == L"Video opties" || title == L"Opções de vídeo" || title == L"Video-Optionen"
            || title == L"Opciones de vídeo" || title == L"Opciones Video")
        {
            int xPreview = 280;
            int yPreview = 23;

            int previewWidth = 315;

            HDC hDC = GetDC(NULL);
            RECT r = { 0, 0, 0, 0 };
            DrawText(hDC, advancedOptionsLabel, _tcslen(advancedOptionsLabel), &r, DT_CALCRECT | DT_NOPREFIX | DT_SINGLELINE);
            long textWidth = abs(r.right - r.left);

            int buttonWidth = textWidth + 8;

            int buttonHeight = 25;

            RECT rect;
            rect.left = xPreview + (previewWidth / 2) - (buttonWidth / 2);
            rect.top = yPreview - (buttonHeight / 2);
            rect.right = rect.left + buttonWidth;
            rect.bottom = rect.top + buttonHeight;

            advancedOptionsBtn.Create(advancedOptionsLabel, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_LEFT, rect, pWnd, BUTTON_IDENTIFIER);

            SetWindowLongPtr(advancedOptionsBtn.m_hWnd, GWLP_USERDATA, BUTTON_IDENTIFIER);

            g_pOldWndProc = (WNDPROC)SetWindowLongPtr(returnVal, GWLP_WNDPROC, (LONG_PTR)ParentWndProc);
        }
    }

    return returnVal;
}

void AssignLabels() 
{
    if (lang == "en")
    {
        advancedOptionsLabel = _TEXT("Advanced options");
    }
    else if (lang == "pl")
    {
        advancedOptionsLabel = _TEXT("Zaawansowane opcje");
    }
    else if (lang == "de")
    {
        advancedOptionsLabel = _TEXT("Erweiterte Einstellungen");
    }
    else if (lang == "es" || lang == "es-419")
    {
        advancedOptionsLabel = _TEXT("Opciones avanzadas");
    }
    else if (lang == "fr")
    {
        advancedOptionsLabel = _TEXT("Options avancées");
    }
    else if (lang == "it") 
    {
        advancedOptionsLabel = _TEXT("Impostazioni avanzate");
    }
    else if (lang == "nl") 
    {
        advancedOptionsLabel = _TEXT("Uitgebreide instellingen");
    }
    else if (lang == "pt" || lang == "pt-br")
    {
        advancedOptionsLabel = _TEXT("Opções Avançadas");
    }
    else if (lang == "ru")
    {
        advancedOptionsLabel = _TEXT("Расширенные настройки");
    }
    else if (lang == "sv")
    {
        advancedOptionsLabel = _TEXT("Avancerade inställningar");
    }else
    {
        advancedOptionsLabel = _TEXT("Advanced options");
    }
}

void shutdown() {

    MH_Uninitialize();
}

bool Initialized = false;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        bool exists = std::filesystem::exists("settings.exe");

        if (exists) 
        {
            MH_STATUS status = MH_Initialize();

            if (status != MH_OK)
            {
                std::string sStatus = MH_StatusToString(status);
                shutdown();
                return 0;
            }

            if (MH_CreateHookApiEx(L"user32", "CreateDialogIndirectParamA", &detourCreateDialogIndirectParamA, reinterpret_cast<void**>(&pCreateDialogIndirectParamA), reinterpret_cast<void**>(&pCreateDialogIndirectParamATarget)) != MH_OK) {
                shutdown();
                return 1;
            }

            if (MH_EnableHook(reinterpret_cast<void**>(pCreateDialogIndirectParamATarget)) != MH_OK) {
                shutdown();
                return 1;
            }

            Initialized = true;

            if (std::filesystem::exists("language.txt")) 
            {
                std::ifstream t("language.txt");
                std::stringstream buffer;
                buffer << t.rdbuf();
                lang = buffer.str();
            }
            else {
                lang = "en";
            }

            AssignLabels();
        }
    }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        if(Initialized && lpReserved)
            shutdown();
        break;
    }
    return TRUE;
}

