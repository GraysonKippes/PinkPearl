#pragma once

#include <array>

namespace render
{
// 4x4 matrix of floats.
typedef std::array<float, 16> Matrix4F;

// Pre-built identity matrix, which does not change anything it is multiplied with.
extern const Matrix4F identityMatrix;

// Makes a 2D translation matrix, where x goes from left to right, and y goes from bottom to top.
Matrix4F makeTranslationMatrix(float x, float y);

// Makes a 2D scaling matrix.
Matrix4F makeScalingMatrix(float x, float y);

// Makes a 2D rotation matrix, where r is the angle from the x-axis counter-clockwise, in radians.
Matrix4F makeRotationMatrix(float r);

// 2D orthographic projection matrix; does not include far-near scaling or translation.
Matrix4F makeOrthographicProjectionMatrix(float top, float bottom, float right, float left);

Matrix4F makeOrthographicProjectionMatrix(float width, float height);

Matrix4F operator*(const Matrix4F &multiplicand, const Matrix4F &multiplier);

Matrix4F &operator*=(Matrix4F &multiplicand, const Matrix4F &multiplier);

inline double lerp(double start, double end, double delta) { return start + delta * (end - start); };
}