#include <windows.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include <iostream>


///////////////////////////////////////
//              Structs              //
///////////////////////////////////////

/** Represents a wall on the screen. */
struct Wall {
    POINT origin;
    char direction;
    int length;
    // setcursorpos depends on the customisation parameters of the screen
    // TODO: find this value automatically if possible
    double adjustmentRatio;
};

/** Represents a portal (which consists of two ends) on the screen. */
struct Portal {
    Wall first;
    Wall second;
};


///////////////////////////////////////
//              Consts               //
///////////////////////////////////////

/**+----------------+
 * | B1 >           | B1 = before first portal (">") or wall
 * |    > A1        | A1 = after first portal or wall
 * |    >     < B2  | B2 = before second portal ("<")
 * |       A2 <     | A2 = after second portal
 * |     O          | O = out
 * +----------------+ */
const int O  = 0x0;
const int B1 = 0x1;
const int A1 = 0x2;
const int A2 = 0x4; // == B1 << 2
const int B2 = 0x8; // == A1 << 2


///////////////////////////////////////
//              Globals              //
///////////////////////////////////////

/** Main hook. */
HHOOK hook;

/** The current and previous location of the cursor relative to a wall
    or a portal. */
// TODO: actually init with the correct values
int relativePos = O;
int prevRelativePos = O;

/** The absolute current and previous location of the cursor. */
// TODO: init with getCursorPos
POINT current = {0, 0};
POINT previous = {0, 0};

/** All walls on the screen. */
std::vector<Wall> walls;
/** The wall that was hit last. */
Wall latestHit;

/** All portals on the screen. */
std::vector<Portal> portals;
/** The portal that was crossed last. */
Portal latestCrossed;

/** Whether the latest interaction with an element was with a portal ('P')
    or a wall ('W') */
char latestInteraction;


///////////////////////////////////////
//              Funtions             //
///////////////////////////////////////

/** Inits the portals from the config file. */
bool readPortals() {
    // TODO: check for file existence
    std::ifstream inFile("conf/portals");
    Portal p;

    if (!inFile) {
        return false;
    }

    while(inFile
        >> p.first.direction
        >> p.first.origin.x  >> p.first.origin.y
		>> p.first.length  >> p.first.adjustmentRatio
        >> p.second.origin.x >> p.second.origin.y
		>> p.second.length >> p.second.adjustmentRatio
    ) {
        Portal newPortal = p;
		// currently mandatory
        newPortal.second.direction = newPortal.first.direction;
        portals.push_back(newPortal);
    }

    inFile.close();

    return true;
}

/** Inits the walls from the config file. */
bool readWalls() {
    // TODO: check for file existence
    std::ifstream inFile("conf/walls");
    Wall w;

    if (!inFile) {
        return false;
    }

    while(
		inFile >> w.direction
		>> w.origin.x >> w.origin.y
		>> w.length >> w.adjustmentRatio
	) {
        Wall newWall = w;
        walls.push_back(w);
    }

    inFile.close();

    return true;
}

/** Returns the position of a point relative to a wall. */
inline int positionRelativeToWall(const Wall& w, const POINT& pt) {
    int pos = O;

    if (w.direction == 'H') {
        if (pt.x >= w.origin.x && pt.x <= w.origin.x + w.length) {
            pos = pt.y < w.origin.y ? B1 : A1;
        }
    } else {
        if (pt.y >= w.origin.y && pt.y <= w.origin.y + w.length) {
            pos = pt.x < w.origin.x ? B1 : A1;
        }
    }

    return pos;
}

/** Returns the position of a point relative to a portal. */
inline int positionRelativeToPortal(const Portal& prtl, const POINT& pt) {
    int posToFirst = O, posToSecond = O;
    posToFirst = positionRelativeToWall(prtl.first, pt);
    posToSecond = positionRelativeToWall(prtl.second, pt) << 2;
    return posToFirst | posToSecond;
}

inline bool hasCrossedFirstPortal() {
    return (prevRelativePos & B1) && (relativePos & A1);
}

/** Updates globals and take action if a portal is crossed. */
void checkPortalCrossed() {
    std::vector<Portal>::iterator it;
    for(it = portals.begin(); it < portals.end(); ++it) {
        prevRelativePos = positionRelativeToPortal(*it, previous);
        relativePos = positionRelativeToPortal(*it, current);
        std::cout << prevRelativePos << " " << relativePos << std::endl;
        // FIXME: this doesn't take into account the case where
		// an intermediary position crosses a portal
        if (
			hasCrossedFirstPortal() 
			|| (prevRelativePos & B2) && (relativePos & A2)
		) {
            latestCrossed = *it;
            latestInteraction = 'P';
            PostThreadMessage(GetCurrentThreadId(), WM_USER, 0, 0);
            break;
        }
    }
}

/** Updates globals and take action if a wall is hit. */
void checkWallHit() {
    std::vector<Wall>::iterator it;
    for(it = walls.begin(); it < walls.end(); ++it) {
        prevRelativePos = positionRelativeToWall(*it, previous);
        relativePos = positionRelativeToWall(*it, current);
        std::cout << prevRelativePos << " " << relativePos << std::endl;
        // FIXME: this doesn't take into account the case where
		// an intermediary position hits a wall
        if (
			(prevRelativePos & B1) && (relativePos & A1) 
			|| (prevRelativePos & A1) && (relativePos & B1)
		) {
            latestHit = *it;
            PostThreadMessage(GetCurrentThreadId(), WM_USER, 0, 0);
            break;
        }
    }
}

/** Callback of the mouse hook. */
LRESULT CALLBACK HookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
    MSLLHOOKSTRUCT msStruct;
    if (nCode >= 0) {
        if (wParam == WM_MOUSEMOVE) {
            msStruct = *((MSLLHOOKSTRUCT*)lParam);
            previous = current;
            current = msStruct.pt;
            printf("new pos: %d %d\n", current.x, current.y);
            latestInteraction = 'W';

            checkPortalCrossed();
            if (latestInteraction != 'P') {
                checkWallHit();
            }
        }
    }

    return CallNextHookEx(hook, nCode, wParam, lParam);
}

/** Computes a coordinate of the cursor after a portal was crossed. */
inline LONG getPostPortalCursorCoord(int curCoord, int coord1, int coord2) {
    return hasCrossedFirstPortal()
        ? (LONG)(
			(
				coord2
				+ (curCoord - coord1)
				* latestCrossed.second.length
				/ latestCrossed.first.length
			)
			/ latestCrossed.second.adjustmentRatio
		)
        : (LONG)(
			(
				coord1
				+ (curCoord - coord2)
				* latestCrossed.first.length
				/ latestCrossed.second.length
			)
			/ latestCrossed.first.adjustmentRatio
		);
}

/** Updates the coordinates of a point after the cursor crosses a portal. */
void crossPortalResult(POINT& pt) {
    if (latestCrossed.first.direction == 'H') {
        pt.x = getPostPortalCursorCoord(
			previous.x,
			latestCrossed.first.origin.x,
			latestCrossed.second.origin.x
		);
        pt.y += (hasCrossedFirstPortal() ? 1 : -1)
			* (latestCrossed.second.origin.y - latestCrossed.first.origin.y);
    } else {
        pt.y = getPostPortalCursorCoord(
			previous.y,
			latestCrossed.first.origin.y,
			latestCrossed.second.origin.y
		);
        pt.x += (hasCrossedFirstPortal() ? 1 : -1)
			* (latestCrossed.second.origin.x - latestCrossed.first.origin.x);
    }
    std::cout << "post portal pos (" << pt.x << ", " << pt.y
		<< ")" << std::endl;
}

/** Updates the coordinates of a point after the cursor hits a wall. */
void hitWallResult(POINT& pt) {
    if (latestHit.direction == 'H') {
        pt.y = previous.y;
    } else {
        pt.x = previous.x;
    }
    pt.x /= latestHit.adjustmentRatio;
    pt.y /= latestHit.adjustmentRatio;
    std::cout << "post wall pos (" << pt.x << ", " << pt.y
		<< ")" << std::endl;
}


///////////////////////////////////////
//                Main               //
///////////////////////////////////////

int main(int argc, char** argv) {
    int initSuccess = true;
    printf("start\n");
    initSuccess = initSuccess && readPortals();
    initSuccess = initSuccess && readWalls();

    if (!initSuccess) {
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
    while (GetMessage(&msg, NULL, 0, 0)) {
        if(msg.hwnd == NULL) {
            if(msg.message == WM_USER) {
                std::cout << latestInteraction << std::endl;
                if (latestInteraction == 'P') {
                    crossPortalResult(current);
                } else {
                    hitWallResult(current);
                }
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
