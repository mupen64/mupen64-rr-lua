#ifndef _3DMATH_H
#define _3DMATH_H

static void mat_cpy(float m0[4][4], float m1[4][4])
{
    std::memcpy(m0, m1, 16 * sizeof(float));
}

static void mat_mul(float m0[4][4], float m1[4][4])
{
    float result[4][4]{};

    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            for (int k = 0; k < 4; ++k)
            {
                result[row][col] += m1[row][k] * m0[k][col];
            }
        }
    }

    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            m0[row][col] = result[row][col];
        }
    }
}

static void vert_transform(float vtx[4], float mtx[4][4])
{
    float original_vtx[4];

    std::memcpy(original_vtx, vtx, 4 * sizeof(float));

    vtx[0] = original_vtx[0] * mtx[0][0] + original_vtx[1] * mtx[1][0] + original_vtx[2] * mtx[2][0] + mtx[3][0];
    vtx[1] = original_vtx[0] * mtx[0][1] + original_vtx[1] * mtx[1][1] + original_vtx[2] * mtx[2][1] + mtx[3][1];
    vtx[2] = original_vtx[0] * mtx[0][2] + original_vtx[1] * mtx[1][2] + original_vtx[2] * mtx[2][2] + mtx[3][2];
    vtx[3] = original_vtx[0] * mtx[0][3] + original_vtx[1] * mtx[1][3] + original_vtx[2] * mtx[2][3] + mtx[3][3];
}

static void vec_transform(float vec[3], float mtx[4][4])
{
    const float temp_x = vec[0];
    const float temp_y = vec[1];
    const float temp_z = vec[2];

    vec[0] = temp_x * mtx[0][0] + temp_y * mtx[1][0] + temp_z * mtx[2][0];
    vec[1] = temp_x * mtx[0][1] + temp_y * mtx[1][1] + temp_z * mtx[2][1];
    vec[2] = temp_x * mtx[0][2] + temp_y * mtx[1][2] + temp_z * mtx[2][2];
}

static void vec_normalize(float v[3])
{
    const float v0 = v[0];
    const float v1 = v[1];
    const float v2 = v[2];

    const float length_squared = v0 * v0 + v1 * v1 + v2 * v2;

    if (length_squared != 0.0f)
    {
        const float inverse_length = 1.0f / std::sqrt(length_squared);
        v[0] = v0 * inverse_length;
        v[1] = v1 * inverse_length;
        v[2] = v2 * inverse_length;
    }
}

static float dot_product(float v0[3], float v1[3])
{
    return v0[0] * v1[0] + v0[1] * v1[1] + v0[2] * v1[2];
}

#endif
