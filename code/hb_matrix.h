#ifndef HB_MATRIX_H
#define HB_MATRIX_H

inline m3x9d
operator *(const m3x9d &a, const m9x9d &b)
{
    m3x9d result = {};

    for(u32 r = 0;
            r < 3;
            ++r)
    {
        for(u32 c = 0;
                c < 9;
                ++c)
        {
            result.e[r][c] = a.e[r][0]*b.e[0][c] +
                             a.e[r][1]*b.e[1][c] + 
                             a.e[r][2]*b.e[2][c] + 
                             a.e[r][3]*b.e[3][c] + 
                             a.e[r][4]*b.e[4][c] + 
                             a.e[r][5]*b.e[5][c] + 
                             a.e[r][6]*b.e[6][c] + 
                             a.e[r][7]*b.e[7][c] + 
                             a.e[r][8]*b.e[8][c];
        }
    }

    return result;
}

inline m3x9d
operator +(const m3x9d &a, const m3x9d &b)
{
    m3x9d result = {};
    for(u32 i = 0;
            i < 3;
            ++i)
    {
        result.rows[i] = a.rows[i] + b.rows[i];
    }

    return result;
}

inline m3x9d
operator *(f64 value, const m3x9d &m)
{
    m3x9d result = m;
    for(u32 i = 0;
            i < 9;
            ++i)
    {
        result.rows[i] *= value;
    }

    return result;
}

inline v3d
operator *(m3x9d m, v9d v)
{
    v3d result = {};

    result.x = dot(m.rows[0], v);
    result.y = dot(m.rows[1], v);
    result.z = dot(m.rows[2], v);

    return result;
}

#endif
