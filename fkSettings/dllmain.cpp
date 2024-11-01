﻿typedef struct IUnknown IUnknown;

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
#include "CDButton.h"

#pragma comment(lib,"user32.lib") 
#pragma comment(lib,"libs\\libMinHook.x86.lib")

#define BUTTON_IDENTIFIER 1

std::string lang;
CButton advancedOptionsBtn;
LPCTSTR advancedOptionsLabel;

CWnd* tcpAddressDropdown;

bool leftAlign = false;

bool IPXEnabled = false;

bool createAdvancedOptions = false;
bool overrideAddressBook = false;

double GetDpiScaleFactor(HWND hwnd)
{
    HDC hdc = GetDC(hwnd);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwnd, hdc);
    return dpi / 96.0; // 96 is the default DPI
}

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

//The pointer to the original window message processing function
WNDPROC ogVideoOptWndProc = nullptr;

WNDPROC ogNetworkPlayWndProc = nullptr;
WNDPROC ogAddressBookWndProc = nullptr;
WNDPROC ogIPXBtnWndProc = nullptr;
WNDPROC ogTCPBtnWndProc = nullptr;

CWnd* hint;

//Process the TCP button's incoming messages
LRESULT CALLBACK TCPBtnWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId = LOWORD(wParam);
    int wmEvent = HIWORD(wParam);

    switch (message)
    {
    case WM_LBUTTONDOWN:
    {
        LRESULT result = CallWindowProc(ogTCPBtnWndProc, hWnd, message, wParam, lParam);
        IPXEnabled = false;

        if (tcpAddressDropdown != NULL)
            tcpAddressDropdown->ShowWindow(true);
        return result;
    }
    break;
    default:
        return CallWindowProc(ogTCPBtnWndProc, hWnd, message, wParam, lParam);
    }
    return 0;
}

//Process the IPX button's incoming messages
LRESULT CALLBACK IPXBtnWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId = LOWORD(wParam);
    int wmEvent = HIWORD(wParam);

    switch (message)
    {
    case WM_LBUTTONDOWN:
    {
        LRESULT result = CallWindowProc(ogIPXBtnWndProc, hWnd, message, wParam, lParam);
        IPXEnabled = true;

        if (tcpAddressDropdown != NULL)
            tcpAddressDropdown->ShowWindow(false);

        return result;
    }
    break;
    default:
        return CallWindowProc(ogIPXBtnWndProc, hWnd, message, wParam, lParam);
    }
    return 0;
}

//Process the Address book button's incoming messages
LRESULT CALLBACK AddressBookBtnWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId = LOWORD(wParam);
    int wmEvent = HIWORD(wParam);

    switch (message)
    {
    case WM_ENABLE:
    {
        //Prevent the frontend from disabling the address book button
        LRESULT result = CallWindowProc(ogAddressBookWndProc, hWnd, message, wParam, lParam);

        if(wmId == 0)
            EnableWindow(hWnd, true);

        return result;
    }
    break;
    default:
        return CallWindowProc(ogAddressBookWndProc, hWnd, message, wParam, lParam);
    }
    return 0;
}

//Process the Network Play tab's incoming messages
LRESULT CALLBACK NetworkPlayWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId = LOWORD(wParam);
    int wmEvent = HIWORD(wParam);

    switch (message)
    {
    case WM_COMMAND:
    {
        if (wmEvent == BN_CLICKED)
        {
            // Handle button click
            HWND hButtonClicked = (HWND)lParam;

            auto data = GetWindowLongPtr(hButtonClicked, GWLP_USERDATA);

            //Address Book btn
            if (wmId == 1243) {
                if (IPXEnabled) 
                {
                    //Run the custom IPX address book
                    bool exists = std::filesystem::exists("ipxaddress.exe");

                    if (exists)
                    {
                        STARTUPINFOA si;
                        PROCESS_INFORMATION pi;

                        ZeroMemory(&si, sizeof(si));
                        si.cb = sizeof(si);
                        ZeroMemory(&pi, sizeof(pi));

                        CreateProcessA("ipxaddress.exe", NULL, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);

                        CloseHandle(pi.hProcess);
                        CloseHandle(pi.hThread);
                    }
                }
                else
                    return CallWindowProc(ogNetworkPlayWndProc, hWnd, message, wParam, lParam);
            }
            else //Something else has been clicked, let the frontend handle it
                return CallWindowProc(ogNetworkPlayWndProc, hWnd, message, wParam, lParam);
        }
    }
    break;
    default:
        return CallWindowProc(ogNetworkPlayWndProc, hWnd, message, wParam, lParam);
    }
    return 0;
}

//Process the Video options tab's incoming messages
LRESULT CALLBACK VideoOptWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
            if (data == 1)
                HandleButtonClick(hButtonClicked);
            else //Something else has been clicked, let the frontend handle it
                return CallWindowProc(ogVideoOptWndProc, hWnd, message, wParam, lParam);
        }
    }
    break;
    default:
        return CallWindowProc(ogVideoOptWndProc, hWnd, message, wParam, lParam);
    }
    return 0;
}

//Function that hooks to the CreateDialogIndirectParamA method
typedef HWND(WINAPI* CreateDialogIndirectParamAType)(HINSTANCE hInstance, LPCDLGTEMPLATEA lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
CreateDialogIndirectParamAType pCreateDialogIndirectParamA = nullptr; //original function pointer after hook
CreateDialogIndirectParamAType pCreateDialogIndirectParamATarget; //original function pointer BEFORE hook do not call this!
HWND WINAPI detourCreateDialogIndirectParamA(HINSTANCE hInstance, LPCDLGTEMPLATEA lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
    auto returnVal = pCreateDialogIndirectParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);

    //Custom hints are unused for now
    //if (hint == NULL && hWndParent != NULL)
    //{
    //    CWnd* parent = CWnd::FromHandle(hWndParent);

    //    if (parent->GetParent())
    //    {
    //        hint = parent->GetParent()->GetDlgItem(1003);
    //        //advancedOptionsBtn.hintObject = hint;
    //    }
    //}

    if (returnVal != NULL) {
        CWnd* pWnd = CWnd::FromHandle(returnVal);

        CString title;
        pWnd->GetWindowTextW(title);

        if (createAdvancedOptions && !(advancedOptionsBtn.GetSafeHwnd() && ::IsWindow(advancedOptionsBtn.GetSafeHwnd()))) {
            //Video options advanced button
            if (title == L"Video options" || title == L"Настройки видео" || title == L"Opcje grafiki"
                || title == L"Opzioni video" || title == L"Options vidéo" || title == L"Video alternativ"
                || title == L"Video opties" || title == L"Opções de vídeo" || title == L"Video-Optionen"
                || title == L"Opciones de vídeo" || title == L"Opciones Video")
            {
                //Get preview rectangle
                CRect previewRect;
                CWnd* previewWnd = pWnd->GetDlgItem(1258);
                previewWnd->GetWindowRect(&previewRect);
                pWnd->ScreenToClient(&previewRect);
                //

                //Calculate text size, 2002 is one of the checkboxes in Video options menu.
                CWnd* comboBox = pWnd->GetDlgItem(2002);
                CDC* comboDC = comboBox->GetDC();

                CFont* font = comboBox->GetFont();

                CFont* old = comboDC->SelectObject(font);

                CSize textSize = comboDC->GetTextExtent(advancedOptionsLabel);

                comboDC->SelectObject(old);

                int buttonWidth = textSize.cx;
                //

                if (leftAlign)
                    buttonWidth += 8;
                else
                    buttonWidth += 13;

                int buttonHeight = 25;

                RECT btnRect;
                btnRect.left = previewRect.left + (previewRect.Width() / 2) - (buttonWidth / 2);
                btnRect.top = (previewRect.top / 2) - (buttonHeight / 2);
                btnRect.right = btnRect.left + buttonWidth;
                btnRect.bottom = btnRect.top + buttonHeight;

                if (leftAlign)
                    advancedOptionsBtn.Create(advancedOptionsLabel, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_LEFT, btnRect, pWnd, BUTTON_IDENTIFIER);
                else
                    advancedOptionsBtn.Create(advancedOptionsLabel, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, btnRect, pWnd, BUTTON_IDENTIFIER);

                advancedOptionsBtn.SetFont(font);

                SetWindowLongPtr(advancedOptionsBtn.m_hWnd, GWLP_USERDATA, BUTTON_IDENTIFIER);

                ogVideoOptWndProc = (WNDPROC)SetWindowLongPtr(returnVal, GWLP_WNDPROC, (LONG_PTR)VideoOptWndProc);
            }
        }

        //IPX address book
        if (overrideAddressBook)
        {
            if (title == L"Network play" || title == L"Netzwerk-Spiel" || title == L"Juego en Red" || title == L"Partida en red" || title == L"Jeu sur réseau"
                || title == L"Gioco in rete" || title == L"Spelen in netwerk" || title == L"Gra w sieci" || title == L"Jogo de rede" || title == L"Сетевая игра"
                || title == L"Nätverk spel")
            {
                //Get original controls
                CWnd* orgAddressBook = pWnd->GetDlgItem(1243);

                CWnd* ipxBtn = pWnd->GetDlgItem(4003);
                CWnd* tcpBtn = pWnd->GetDlgItem(4004);

                tcpAddressDropdown = pWnd->GetDlgItem(1242);

                //Override original window processing methods
                ogNetworkPlayWndProc = (WNDPROC)SetWindowLongPtr(returnVal, GWLP_WNDPROC, (LONG_PTR)NetworkPlayWndProc);

                HWND addressBookHWND = orgAddressBook->GetSafeHwnd();
                ogAddressBookWndProc = (WNDPROC)SetWindowLongPtr(addressBookHWND, GWLP_WNDPROC, (LONG_PTR)AddressBookBtnWndProc);

                HWND ipxHWND = ipxBtn->GetSafeHwnd();
                ogIPXBtnWndProc = (WNDPROC)SetWindowLongPtr(ipxHWND, GWLP_WNDPROC, (LONG_PTR)IPXBtnWndProc);

                HWND tcpHWND = tcpBtn->GetSafeHwnd();
                ogTCPBtnWndProc = (WNDPROC)SetWindowLongPtr(tcpHWND, GWLP_WNDPROC, (LONG_PTR)TCPBtnWndProc);

                //Check if TCP or IPX is selected upon entering and reenable the IPX button if its disabled
                IPXEnabled = !IsWindowEnabled(addressBookHWND);

                if (IPXEnabled) 
                {
                    EnableWindow(addressBookHWND, true);

                    if (tcpAddressDropdown != NULL)
                        tcpAddressDropdown->ShowWindow(false);
                }
            }
        }
    }

    return returnVal;
}

void AssignLabels() 
{
    if (lang == "en")
    {
        advancedOptionsLabel = _TEXT("Advanced options");
        //advancedOptionsBtn.hintText = _TEXT("\nChange advanced graphic settings such as resolution or the renderer");
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
        leftAlign = true;
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
        createAdvancedOptions = std::filesystem::exists("settings.exe");
        overrideAddressBook = std::filesystem::exists("ipxaddress.exe");

        if (createAdvancedOptions || overrideAddressBook)
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

