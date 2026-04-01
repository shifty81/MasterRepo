#pragma once
#include "Core/Math/Vector.h"
#include <cmath>
#include <algorithm>

namespace NF {

/// @brief Column-major 4×4 floating-point matrix.
///
/// Storage: M[col][row] — i.e. each column occupies contiguous memory,
/// matching the convention expected by OpenGL and most GPU APIs.
struct Matrix4x4 {
    float M[4][4]{};

    constexpr Matrix4x4() noexcept = default;

    // -----------------------------------------------------------------------
    // Factory functions
    // -----------------------------------------------------------------------

    /// @brief Return the identity matrix (ones on the main diagonal).
    [[nodiscard]] static constexpr Matrix4x4 Identity() noexcept {
        Matrix4x4 m{};
        m.M[0][0] = 1.f;
        m.M[1][1] = 1.f;
        m.M[2][2] = 1.f;
        m.M[3][3] = 1.f;
        return m;
    }

    // -----------------------------------------------------------------------
    // Arithmetic
    // -----------------------------------------------------------------------

    /// @brief Matrix multiplication (this * other).
    [[nodiscard]] constexpr Matrix4x4 Multiply(const Matrix4x4& o) const noexcept {
        Matrix4x4 r{};
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
                for (int k = 0; k < 4; ++k)
                    r.M[col][row] += M[k][row] * o.M[col][k];
        return r;
    }

    [[nodiscard]] constexpr Matrix4x4 operator*(const Matrix4x4& o) const noexcept {
        return Multiply(o);
    }

    /// @brief Transform a column vector: result = M * v.
    [[nodiscard]] constexpr Vector4 operator*(const Vector4& v) const noexcept {
        return {
            M[0][0] * v.X + M[1][0] * v.Y + M[2][0] * v.Z + M[3][0] * v.W,
            M[0][1] * v.X + M[1][1] * v.Y + M[2][1] * v.Z + M[3][1] * v.W,
            M[0][2] * v.X + M[1][2] * v.Y + M[2][2] * v.Z + M[3][2] * v.W,
            M[0][3] * v.X + M[1][3] * v.Y + M[2][3] * v.Z + M[3][3] * v.W
        };
    }

    // -----------------------------------------------------------------------
    // Linear-algebra operations
    // -----------------------------------------------------------------------

    /// @brief Return the transpose of this matrix.
    [[nodiscard]] constexpr Matrix4x4 Transpose() const noexcept {
        Matrix4x4 r{};
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
                r.M[col][row] = M[row][col];
        return r;
    }

    /// @brief Compute the matrix inverse via Gauss-Jordan elimination with
    ///        partial pivoting.
    /// @return The inverse, or Identity() if the matrix is singular.
    [[nodiscard]] Matrix4x4 Inverse() const noexcept {
        // Work in row-major order for elimination: aug[row][col] = M[col][row].
        // The right half of each row is initialised to the identity.
        float aug[4][8]{};
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r) {
                aug[r][c]     = M[c][r];
                aug[r][4 + c] = (r == c) ? 1.f : 0.f;
            }

        for (int pivot = 0; pivot < 4; ++pivot) {
            // Partial pivoting: find the row with the largest absolute value in
            // the current column.
            int   maxRow = pivot;
            float maxVal = std::abs(aug[pivot][pivot]);
            for (int r = pivot + 1; r < 4; ++r) {
                const float v = std::abs(aug[r][pivot]);
                if (v > maxVal) { maxVal = v; maxRow = r; }
            }

            if (maxVal < 1e-8f)
                return Identity(); // Singular matrix

            if (maxRow != pivot)
                for (int col = 0; col < 8; ++col)
                    std::swap(aug[pivot][col], aug[maxRow][col]);

            // Scale pivot row so that the diagonal element becomes 1
            const float invPiv = 1.f / aug[pivot][pivot];
            for (int col = pivot; col < 8; ++col)
                aug[pivot][col] *= invPiv;

            // Eliminate the current column in all other rows
            for (int r = 0; r < 4; ++r) {
                if (r == pivot) continue;
                const float factor = aug[r][pivot];
                for (int col = 0; col < 8; ++col)
                    aug[r][col] -= factor * aug[pivot][col];
            }
        }

        // Extract the right half back into a column-major result matrix
        Matrix4x4 result{};
        for (int c = 0; c < 4; ++c)
            for (int r = 0; r < 4; ++r)
                result.M[c][r] = aug[r][4 + c];
        return result;
    }
};

} // namespace NF
