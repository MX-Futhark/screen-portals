#ifndef SCREENS_STATE_H
#define SCREENS_STATE_H

#include <windows.h>
#include <vector>

#include "Wall.h"
#include "Portal.h"
#include "consts.h"

class ScreensState {

public:

    ScreensState::ScreensState();
    bool isInitSuccess();
    char getLatestInteraction();
    void setLatestInteraction(char newInteraction);
    POINT getCurrent();
    void setCurrent(POINT newCurrent);

    /** Updates the coordinates of the current point after the cursor crosses a portal. */
    void crossPortalUpdate();

    /** Updates the coordinates of the current point after the cursor hits a wall. */
    void hitWallUpdate();

    /** Updates globals and takes action if a portal is crossed. */
    void checkPortalCrossed();

    /** Updates globals and takes action if a wall is hit. */
    void checkWallHit();

private:

    // Whether initialization was a success or not
    int initSuccess = true;

    /** The current and previous location of the cursor relative to a wall
        or a portal. */
    // TODO: actually init these with the correct values
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


    /** Inits the portals from the config file. */
    bool readPortals();

    /** Inits the walls from the config file. */
    bool readWalls();

    /** Checks if the first end of a portal pair was crossed */
    bool hasCrossedFirstPortal();

    /** Computes a coordinate of the cursor after a portal was crossed. */
    LONG getPostPortalCursorCoord(int curCoord, int coord1, int coord2);

    /** Returns the position of a point relative to a portal. */
    // TODO: put this in the Portal class
    static int positionRelativeToPortal(const Portal& prtl, const POINT& pt);

    /** Returns the position of a point relative to this wall. */
    // TODO: put this in the Wall class
    static int positionRelativeToWall(const Wall& w, const POINT& pt);
};


#endif
