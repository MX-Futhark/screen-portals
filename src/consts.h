#ifndef CONSTS_H
#define CONSTS_H

/**+----------------+
 * | B1 >           | B1 = before first portal (">") or wall
 * |    > A1        | A1 = after first portal or wall
 * |    >     < B2  | B2 = before second portal ("<")
 * |       A2 <     | A2 = after second portal
 * |     O          | O = out
 * +----------------+ */
#define O  0x0
#define B1 0x1
#define A1 0x2
#define A2 0x4 // == B1 << 2
#define B2 0x8 // == A1 << 2

#endif
