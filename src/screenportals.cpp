#include <windows.h>
#include <stdio.h>
#include <iostream>

#include "ScreensState.cpp"

/** Main hook. */
HHOOK hook;

/** Current state of the screen */
ScreensState* st;

/** Callback of the mouse hook. */
LRESULT CALLBACK HookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
    MSLLHOOKSTRUCT msStruct;
    if (nCode >= 0) {
        if (wParam == WM_MOUSEMOVE) {
            msStruct = *((MSLLHOOKSTRUCT*)lParam);
            st->setCurrent(msStruct.pt);
            printf("new pos: %d %d\n", msStruct.pt.x, msStruct.pt.y);
            st->setLatestInteraction('W');

            st->checkPortalCrossed();
            if (st->getLatestInteraction() != 'P') {
                st->checkWallHit();
            }
        }
    }

    return CallNextHookEx(hook, nCode, wParam, lParam);
}


///////////////////////////////////////
//                Main               //
///////////////////////////////////////

int main(int argc, char** argv) {
    printf("start\n");
    st = new ScreensState();

    if (!st->isInitSuccess()) {
        MessageBox(NULL, "Failed to read config files.", "Error", MB_ICONERROR);
        return 1;
    }

    if (!(hook = SetWindowsHookEx(WH_MOUSE_LL, HookCallback, NULL, 0))) {
        MessageBox(NULL, "Failed to set mouse hook.", "Error", MB_ICONERROR);
        return 1;
    }

    if (argc == 1) { // TODO: clean this up
        HWND hWnd = GetConsoleWindow();
        ShowWindow(hWnd, SW_HIDE);
    }

    MSG msg;
    POINT current;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if(msg.hwnd == NULL) {
            if(msg.message == WM_USER) {
                std::cout << st->getLatestInteraction() << std::endl;
                if (st->getLatestInteraction() == 'P') {
                    st->crossPortalUpdate();
                } else {
                    st->hitWallUpdate();
                }
                current = st->getCurrent();
                // TODO: make this more reliable.
                SetCursorPos((int)current.x, (int)current.y);
            }
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    UnhookWindowsHookEx(hook);

    return 0;
}
