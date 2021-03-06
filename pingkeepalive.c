/*
 * PING Keep Alive
 *   a lean and mean background IPv4 pinger
 *
 * compile:
 *   call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
 *   rc /nologo /l 0x0409 /fo resource.res resource.rc
 *   cl /W4 /O2 pingkeepalive.c /link /subsystem:windows kernel32.lib user32.lib gdi32.lib shell32.lib iphlpapi.lib ws2_32.lib resource.res /out:pingkeepalive.exe
 *
 */

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <shellapi.h>
#include "resource.h"

#define MAX_LOADSTRING 64
#define MIN_WINDOW_WIDTH 360
#define MIN_WINDOW_HEIGHT 220

static HINSTANCE hInst = 0; 
static UINT WM_TASKBAR = 0;
static HWND g_hWnd = 0, g_hWnd_tb1 = 0, g_hWnd_tb2 = 0;
static unsigned int g_pingCount = 0;
static HMENU Hmenu;
static NOTIFYICONDATA notifyIconData;
static TCHAR szTitle[MAX_LOADSTRING];     // title bar text
static TCHAR szClassName[MAX_LOADSTRING]; // the main window class name
static BOOL g_pingTimerEngaged = FALSE;

/* function prototypess */
LRESULT CALLBACK WindowProcedure(HWND, UINT, WPARAM, LPARAM);
static void InitNotifyIconData(HWND);
static int ping(HWND, TCHAR *);

int WINAPI WinMain(HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);    // silence the  warning C4100: unreferenced formal parameter
    UNREFERENCED_PARAMETER(lpszArgument);
    hInst = hThisInstance;

    LoadString(hThisInstance, IDS_MYAPP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hThisInstance, IDC_MYAPP, szClassName, MAX_LOADSTRING);
    if (FindWindow(szClassName, szTitle))
    {
        MessageBox(NULL, TEXT("Previous instance alredy running!"), szTitle, MB_OK);
        return 0;
    }

    WM_TASKBAR = RegisterWindowMessage("TaskbarCreated");

    MSG msg;
    WNDCLASSEX wincl;                         // a struct for the windowclass to be registered with RegisterClassEx()  */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      // our 'WindowProc callback function' that will get called when OS has any msg for us
    wincl.style = CS_DBLCLKS;                 // do send us double-click message/event to WindowProcedure
    wincl.cbSize = sizeof(WNDCLASSEX);
    wincl.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ICO1));
    wincl.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ICO1));
    wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
    wincl.lpszMenuName = MAKEINTRESOURCE(IDC_MYAPP);
    wincl.cbClsExtra = 0;                     // number of bytes to allocate windowclass struct
    wincl.cbWndExtra = 0;
    wincl.hbrBackground = (HBRUSH)(CreateSolidBrush(RGB(255, 255, 255)));
    if (!RegisterClassEx (&wincl))            // register the windowclass struct for CreateWindowEx() use
        return 0;

    HWND hWnd = CreateWindowEx(0, szClassName, szTitle, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT,
               HWND_DESKTOP,        /* the window is a child-window to desktop, HWND_DESKTOP is NULL */
               NULL, hThisInstance, NULL);
    if (!hWnd)
    {
        MessageBox(NULL, "Can't create a window!", TEXT("Warning!"), MB_ICONERROR | MB_OK | MB_TOPMOST);
        return 1;
    }
    g_hWnd = hWnd;
    //CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("Foo"), TEXT("bar"), WS_CHILD | WS_VISIBLE | WS_BORDER, x, y, w, h, hWnd, NULL, NULL, NULL);

    /* create GUI elements */
    static HWND hWnd_lbl1, hWnd_tb1, hWnd_tb2, hWnd_btn1;  // declared as static so won't get lost once WM_CREATE returns control to OS
    int x, w, y, h;
    y = 10; h = 20;
    x = 10; w = 50;                           // create a label
    hWnd_lbl1 = CreateWindow(TEXT("static"), "ST_U", WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, w, h, hWnd, (HMENU)(501), (HINSTANCE) GetWindowLong(hWnd, GWLP_HINSTANCE), NULL);
    SetWindowText(hWnd_lbl1, "IP:");
    x += w; w = 60 + 70;                      // create a textbox
    g_hWnd_tb1 = hWnd_tb1 = CreateWindow(TEXT("edit"), "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_LEFT | WS_BORDER, x, y, w, h, hWnd, (HMENU)(502), (HINSTANCE)GetWindowLong(hWnd, GWLP_HINSTANCE), NULL);
    SetWindowText(hWnd_tb1, "192.168.1.1");
    x += w + 10; w = 50;                      // create a button
    hWnd_btn1 = CreateWindow(TEXT("BUTTON"), TEXT("PING"), WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, x, y, w, h, hWnd, NULL, (HINSTANCE)GetWindowLong(hWnd, GWLP_HINSTANCE), NULL);

    x = 10; y += h + 10; w = 330; h = 100;    // create a textbox for output from ping()
    g_hWnd_tb2 = hWnd_tb2 = CreateWindow(TEXT("edit"), "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_TABSTOP | ES_LEFT | WS_BORDER, x, y, w, h, hWnd, (HMENU)(503), (HINSTANCE)GetWindowLong(hWnd, GWLP_HINSTANCE), NULL);
    SetWindowText(hWnd_tb1, "192.168.1.1");


    if (!hWnd_lbl1 || !hWnd_tb1 || !hWnd_tb2 || !hWnd_btn1)
    {
        MessageBox(NULL, "Failed to create GUI elements", TEXT("Warning!"), MB_ICONERROR | MB_OK | MB_TOPMOST);
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    /* main message loop. It will run until GetMessage() returns 0 */
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);               // translate virtual-key messages into character messages
        DispatchMessage(&msg);                // send message to WindowProcedure
    }

    UnregisterClass(szClassName, hThisInstance);
    return (int) msg.wParam;
}

/* this function is called by the Windows function DispatchMessage() */
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static TCHAR ipaddr[16] = { 0 };

    if (message == WM_TASKBAR && !IsWindowVisible(hWnd)) {
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    }

    switch (message) {
    case WM_ACTIVATE:
        break;
    case WM_CREATE:                           // WM_CREATE message gets sent during CreateWindowEx(), WARNING! global hWnd is still 0 during this time!
        ShowWindow(hWnd, SW_HIDE);            // but local hWnd carries the valid handle
        Hmenu = CreatePopupMenu();
        InitNotifyIconData(hWnd);             // initialize the NOTIFYICONDATA structure only once
        AppendMenu(Hmenu, MF_STRING, ID_TRAY_ABOUT, TEXT("About"));
        AppendMenu(Hmenu, MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));
        Shell_NotifyIcon(NIM_ADD, &notifyIconData);  // send message to systemtray to add an icon
        break;
    case WM_SYSCOMMAND:
		switch (wParam & 0xFFF0) {
        case SC_MINIMIZE:
        case SC_CLOSE:  
            ShowWindow(hWnd, SW_HIDE);
            return 0;
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case BN_PUSHED:
        case BN_CLICKED:                      // initiate ping
            if (g_pingTimerEngaged) {
                KillTimer(hWnd, IDT_MYPING);  // stop the pinging it is running and user pressed PING button again
                g_pingTimerEngaged = FALSE;
                return 0;
            }
            SendMessage(g_hWnd_tb1, WM_GETTEXT, (WPARAM)15, (LPARAM)&ipaddr);
            //PostMessage(hWnd, WM_MYPING, 0, (LPARAM)&ipaddr);
            SetTimer(hWnd, IDT_MYPING, 1500, (TIMERPROC) NULL);  // timer fires every 1.5sec
            g_pingTimerEngaged = TRUE;
            break;
        case IDM_CLEAR:
            SendMessage(g_hWnd_tb2, WM_SETTEXT, 0, (LPARAM) "output cleared.\r\n");
            return 0;
        case ID_TRAY_ABOUT:
        case IDM_ABOUT:
            TCHAR aboutBuf[100];
            LoadString(hInst, IDS_MYAPP_ABOUT, aboutBuf, 100);
            MessageBox(hWnd, aboutBuf, TEXT("About"), MB_ICONINFORMATION | MB_OK);
            return 0;
        case ID_TRAY_EXIT:
        case IDM_EXIT:
            PostMessage(hWnd, WM_DESTROY, 0, 0);
            return 0;
        }
        break;

    case WM_GETMINMAXINFO:                    // message received when size or position of the window is about to change
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = MIN_WINDOW_WIDTH;
        lpMMI->ptMinTrackSize.y = MIN_WINDOW_HEIGHT;
        break;

    case WM_MYPING:
        ping(hWnd, (TCHAR *) lParam);
        break;

    case WM_TIMER:
        switch (wParam) {
        case IDT_MYPING:
            PostMessage(hWnd, WM_MYPING, 0, (LPARAM)&ipaddr);
            return 0;
        }
        break;

    case WM_MYSYSICON:                        // our user defined WM_MYSYSICON message
    {
        switch (wParam)
        {
        case ID_TRAY_APP_ICON:
            SetForegroundWindow(hWnd);
            break;
        }

        switch (lParam) {
        case WM_LBUTTONUP:
            ShowWindow(hWnd, SW_SHOW);
            break;
        case WM_RBUTTONDOWN:
            POINT curPoint;
            GetCursorPos(&curPoint);          // get current mouse position
            SetForegroundWindow(hWnd);
            // TrackPopupMenu blocks the app until TrackPopupMenu returns (e.g. user makes the choice)
            UINT cmd = TrackPopupMenu(Hmenu, TPM_RETURNCMD | TPM_LEFTALIGN| TPM_NONOTIFY, curPoint.x, curPoint.y, 0, hWnd, NULL);
            //SendMessage(hWnd, WM_NULL, 0, 0); // send benign message to window to make sure the menu goes away
            SendMessage(hWnd, WM_COMMAND, cmd, 0);
            break;
        }
    }
    break;

    case WM_NCHITTEST:                        // among things, allows the app to be dragged with Ctrl+leftclick
        UINT uHitTest = DefWindowProc(hWnd, WM_NCHITTEST, wParam, lParam);
        if (uHitTest == HTCLIENT)
            return HTCAPTION;
        else
            return uHitTest;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

static void InitNotifyIconData(HWND hWnd)
{
    memset(&notifyIconData, 0, sizeof(NOTIFYICONDATA));

    notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    notifyIconData.hWnd = hWnd;
    notifyIconData.uID = ID_TRAY_APP_ICON;
    notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    notifyIconData.uCallbackMessage = WM_MYSYSICON;  // user defined message
    notifyIconData.hIcon = (HICON)LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ICO1)) ;
    strncpy(notifyIconData.szTip, szTitle, sizeof(szTitle));  // tooltip string
}

static int ping(HWND hWnd, TCHAR* ipaddr)
{
    HANDLE hIcmp;
    char *SendData = "ICMP SEND DATA";
    LPVOID ReplyBuffer;
    DWORD dwRetVal;
    DWORD buflen;
    PICMP_ECHO_REPLY pIcmpEchoReply;

    hIcmp = IcmpCreateFile();
    unsigned long ipaddr_ul = inet_addr(ipaddr);

    buflen = sizeof(ICMP_ECHO_REPLY) + strlen(SendData) + 1;
    ReplyBuffer = (VOID*) malloc(buflen);
    if (hIcmp == INVALID_HANDLE_VALUE || ipaddr_ul == INADDR_NONE || ReplyBuffer == NULL) {
        MessageBox(hWnd, TEXT("Failed to initiate ping"), TEXT("Info"), MB_ICONINFORMATION | MB_OK);
        return 1;
    }
    memset(ReplyBuffer, 0, buflen);
    pIcmpEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;

    static TCHAR buff[384] = { 0 };
    int outputTextboxLength = 0;
    dwRetVal = IcmpSendEcho2(hIcmp, NULL, NULL, NULL, ipaddr_ul, SendData, strlen(SendData), NULL, ReplyBuffer, buflen, 1000);
    if (dwRetVal != 0) {
        struct in_addr ReplyAddr;
        ReplyAddr.S_un.S_addr = pIcmpEchoReply->Address;
        TCHAR ipstatus[100] = { 0 };
        if (pIcmpEchoReply->Status != IP_SUCCESS) {  // GetIpErrorString() can only be called when there's no IP_SUCCESS
            GetIpErrorString(pIcmpEchoReply->Status, (PWSTR) &ipstatus, (PDWORD) sizeof(ipstatus)-1);
            sprintf(buff, "Status: %s\r\nMessage: %s\r\n", ipstatus, (char *) pIcmpEchoReply->Data);
        } else {
            sprintf(buff, "Reply from %s bytes=%d time=%dms\r\n", inet_ntoa(ReplyAddr), pIcmpEchoReply->DataSize, pIcmpEchoReply->RoundTripTime);
        }
        outputTextboxLength = GetWindowTextLength(g_hWnd_tb2);
        SendMessage(g_hWnd_tb2, EM_SETSEL, (WPARAM) outputTextboxLength, (LPARAM) outputTextboxLength);  // set selection to end of text
        SendMessage(g_hWnd_tb2, EM_REPLACESEL, 0, (LPARAM) &buff);
        //SendMessage(g_hWnd_tb2, WM_SETTEXT, 0, (LPARAM) &buff);    // overwrites the entire textbox each time
        //Sleep(1000 - pIcmpEchoReply->RoundTripTime);  // no need to sleep and stall UI anymore, the timer periodically fires
        //PostMessage(hWnd, WM_MYPING, 0, (LPARAM) ipaddr);
        g_pingCount++;
    } else {
        sprintf(buff, "Call to IcmpSendEcho() failed.\r\n"
            "Error: %ld\r\n", GetLastError());
        SendMessage(g_hWnd_tb2, WM_SETTEXT, 0, (LPARAM) &buff);
    }

    if ((outputTextboxLength + 100) > 30000)  // clear the output textbox once it nears 2^15 characters
        SendMessage(g_hWnd_tb2, WM_SETTEXT, 0, (LPARAM) "output cleared.\r\n");

    IcmpCloseHandle(hIcmp);
    free(ReplyBuffer);
    return 0;
}
