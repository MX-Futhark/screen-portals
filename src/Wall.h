#ifndef WALL_H
#define WALL_H

/** Represents a wall on the screen. */
struct Wall {
    POINT origin;
    char direction;
    int length;
    // setcursorpos depends on the customisation parameters of the screen
    // TODO: find this value automatically if possible
    double adjustmentRatio;
};

#endif
