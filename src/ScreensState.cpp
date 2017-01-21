#include <fstream>
#include <iostream>

#include "ScreensState.h"

///////////////////////////////////////
//          Public members           //
///////////////////////////////////////

ScreensState::ScreensState() {
    initSuccess = readPortals();
    initSuccess = initSuccess && readWalls();
}

bool ScreensState::isInitSuccess() {
    return initSuccess;
}

char ScreensState::getLatestInteraction() {
    return latestInteraction;
}

void ScreensState::setLatestInteraction(char newInteraction) {
    latestInteraction = newInteraction == 'W' ? 'W' : 'P';
}

POINT ScreensState::getCurrent() {
    return current;
}

void ScreensState::setCurrent(POINT newCurrent) {
    previous = current;
    current = newCurrent;
}

void ScreensState::crossPortalUpdate() {
    if (latestCrossed.first.direction == 'H') {
        current.x = getPostPortalCursorCoord(
            previous.x,
            latestCrossed.first.origin.x,
            latestCrossed.second.origin.x
        );
        current.y += (hasCrossedFirstPortal() ? 1 : -1)
            * (latestCrossed.second.origin.y - latestCrossed.first.origin.y);
    } else {
        current.y = getPostPortalCursorCoord(
            previous.y,
            latestCrossed.first.origin.y,
            latestCrossed.second.origin.y
        );
        current.x += (hasCrossedFirstPortal() ? 1 : -1)
            * (latestCrossed.second.origin.x - latestCrossed.first.origin.x);
    }
    std::cout << "post portal pos (" << current.x << ", " << current.y
        << ")" << std::endl;
}

void ScreensState::hitWallUpdate() {
    if (latestHit.direction == 'H') {
        current.y = previous.y;
    } else {
        current.x = previous.x;
    }
    current.x /= latestHit.adjustmentRatio;
    current.y /= latestHit.adjustmentRatio;
    std::cout << "post wall pos (" << current.x << ", " << current.y
        << ")" << std::endl;
}

void ScreensState::checkPortalCrossed() {
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

void ScreensState::checkWallHit() {
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


///////////////////////////////////////
//          Private members          //
///////////////////////////////////////

bool ScreensState::readPortals() {
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

bool ScreensState::readWalls() {
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

bool ScreensState::hasCrossedFirstPortal() {
    return (prevRelativePos & B1) && (relativePos & A1);
}

LONG ScreensState::getPostPortalCursorCoord(int curCoord, int coord1, int coord2) {
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

int ScreensState::positionRelativeToPortal(const Portal& prtl, const POINT& pt) {
    int posToFirst = O, posToSecond = O;
    posToFirst = positionRelativeToWall(prtl.first, pt);
    posToSecond = positionRelativeToWall(prtl.second, pt) << 2;
    return posToFirst | posToSecond;
}

int ScreensState::positionRelativeToWall(const Wall& w, const POINT& pt) {
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
