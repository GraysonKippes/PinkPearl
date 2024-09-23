#ifndef BOX_H
#define BOX_H

#define boxWidth(box) _Generic((box), \
			BoxI: boxWidthI, \
			BoxF: boxWidthF, \
			BoxD: boxWidthD \
		)(box)

#define boxLength(box) _Generic((box), \
			BoxI: boxLengthI, \
			BoxF: boxLengthF, \
			BoxD: boxLengthD \
		)(box)

#define boxArea(box) _Generic((box), \
			BoxI: boxAreaI, \
			BoxF: boxAreaF, \
			BoxD: boxAreaD \
		)(box)

// Box struct of type signed integer.
typedef struct BoxI {
	int x1, y1;	// Point 1.
	int x2, y2;	// Point 2; must be greater than point 1.
} BoxI;

int boxWidthI(const BoxI box);

int boxLengthI(const BoxI box);

int boxAreaI(const BoxI box);

// Box struct of type single-precision floating-point number.
typedef struct BoxF {
	float x1, y1;	// Point 1.
	float x2, y2;	// Point 2; must be greater than point 1.
} BoxF;

float boxWidthF(const BoxF box);

float boxLengthF(const BoxF box);

float boxAreaF(const BoxF box);

// Box struct of type double-precision floating-point number.
typedef struct BoxD {
	double x1, y1;	// Point 1.
	double x2, y2;	// Point 2; must be greater than point 1.
} BoxD;

double boxWidthD(const BoxD box);

double boxLengthD(const BoxD box);

double boxAreaD(const BoxD box);

BoxF boxD2F(const BoxD box);

BoxD boxF2D(const BoxF box);

#endif // BOX_H