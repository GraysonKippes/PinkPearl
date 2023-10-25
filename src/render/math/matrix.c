#include "Matrix.hpp"
#include <cmath>

namespace render
{
const Matrix4F identityMatrix = {
	1.0F, 0.0F, 0.0F, 0.0F,
	0.0F, 1.0F, 0.0F, 0.0F,
	0.0F, 0.0F, 1.0F, 0.0F,
	0.0F, 0.0F, 0.0F, 1.0F
};

Matrix4F makeTranslationMatrix(float x, float y)
{
	return {
		1.0F, 0.0F, 0.0F, 0.0F,
		0.0F, 1.0F, 0.0F, 0.0F,
		0.0F, 0.0F, 1.0F, 0.0F,
		x,    -y,   0.0F, 1.0F
	};
}

Matrix4F makeScalingMatrix(float x, float y)
{
	return {
		x,    0.0F, 0.0F, 0.0F,
		0.0F, y,    0.0F, 0.0F,
		0.0F, 0.0F, 1.0F, 0.0F,
		0.0F, 0.0F, 0.0F, 1.0F
	};
}

Matrix4F makeRotationMatrix(float r)
{
	return {
		std::cos(r), -std::sin(r), 0.0F, 0.0F,
		std::sin(r), std::cos(r), 0.0F, 0.0F,
		0.0F, 0.0F, 1.0F, 0.0F,
		0.0F, 0.0F, 0.0F, 1.0F
	};
}

// 2D orthographic projection matrix, does not include far-near scaling or translation.
Matrix4F makeOrthographicProjectionMatrix(float top, float bottom, float right, float left)
{
	return {
		(2.0F / (right - left)),	0.0F,						0.0F,	-((right + left) / (right - left)),
		0.0F,						(2.0F / (top - bottom)),	0.0F,	-((top + bottom) / (top - bottom)),
		0.0F,						0.0F,						1.0F,	0.0F,
		0.0F,						0.0F,						0.0F,	1.0F
	};
}

Matrix4F makeOrthographicProjectionMatrix(float width, float height)
{
	return makeOrthographicProjectionMatrix((height / 2.0F), -(height / 2.0F), (width / 2.0F), -(width / 2.0F));
}

Matrix4F operator*(const Matrix4F &a, const Matrix4F &b)
{
	// Row 1
	float f0 = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
	float f1 = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
	float f2 = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
	float f3 = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

	// Row 2
	float f4 = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
	float f5 = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
	float f6 = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
	float f7 = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

	// Row 3
	float f8 = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
	float f9 = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
	float f10 = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
	float f11 = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

	// Row 4
	float f12 = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
	float f13 = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
	float f14 = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
	float f15 = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];

	return {
		f0, f1, f2, f3,
		f4, f5, f6, f7,
		f8, f9, f10, f11,
		f12, f13, f14, f15
	};
}

Matrix4F &operator*=(Matrix4F &a, const Matrix4F &b)
{
	Matrix4F c = a;

	// Row 1
	a[0] = c[0] * b[0] + c[1] * b[4] + c[2] * b[8] + c[3] * b[12];
	a[1] = c[0] * b[1] + c[1] * b[5] + c[2] * b[9] + c[3] * b[13];
	a[2] = c[0] * b[2] + c[1] * b[6] + c[2] * b[10] + c[3] * b[14];
	a[3] = c[0] * b[3] + c[1] * b[7] + c[2] * b[11] + c[3] * b[15];

	// Row 2
	a[4] = c[4] * b[0] + c[5] * b[4] + c[6] * b[8] + c[7] * b[12];
	a[5] = c[4] * b[1] + c[5] * b[5] + c[6] * b[9] + c[7] * b[13];
	a[6] = c[4] * b[2] + c[5] * b[6] + c[6] * b[10] + c[7] * b[14];
	a[7] = c[4] * b[3] + c[5] * b[7] + c[6] * b[11] + c[7] * b[15];

	// Row 3
	a[8] = c[8] * b[0] + c[9] * b[4] + c[10] * b[8] + c[11] * b[12];
	a[9] = c[8] * b[1] + c[9] * b[5] + c[10] * b[9] + c[11] * b[13];
	a[10] = c[8] * b[2] + c[9] * b[6] + c[10] * b[10] + c[11] * b[14];
	a[11] = c[8] * b[3] + c[9] * b[7] + c[10] * b[11] + c[11] * b[15];

	// Row 4
	a[12] = c[12] * b[0] + c[13] * b[4] + c[14] * b[8] + c[15] * b[12];
	a[13] = c[12] * b[1] + c[13] * b[5] + c[14] * b[9] + c[15] * b[13];
	a[14] = c[12] * b[2] + c[13] * b[6] + c[14] * b[10] + c[15] * b[14];
	a[15] = c[12] * b[3] + c[13] * b[7] + c[14] * b[11] + c[15] * b[15];

	return a;
}
}