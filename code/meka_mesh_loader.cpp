/*
 * Written by Gyuhyun 'Joon' Lee
 * https://github.com/meka-lopo/
 */

#define Invalid_R32 10000.0f
#include <string.h> // memset, memcopy, memmove

internal u8 *
get_closest_carriage_return(u8 *start)
{
    u8 *result = start;
    while(1)
    {
        if(*result == '\n')
        {
            break;
        }

        result++;
    }

    return result;
}

struct parse_numeric_result
{
    b32 isFloat;
    union
    {
        i32 value_i32;
        r32 value_r32;
    };

    u32 advance;
};

internal parse_numeric_result
parse_numeric(u8 *start)
{
    parse_numeric_result result = {};

    r32 decimal_point_adjustment = 10.0f;
    b32 found_number = false;
    i32 number = 0;
    u8 *c = start;
    while(1)
    {
        if(*c >= '0' && *c <= '9' )
        {
            found_number = true;

            number *= 10;
            number += *c-'0';
        }
        else if(*c == '.')
        {
            result.isFloat = true;
        }
        else
        {
            if(found_number)
            {
                result.advance = c - start;
                break;
            }
        }

        if(result.isFloat)
        {
            decimal_point_adjustment *= 0.1f;
        }

        c++;
    }

    if(result.isFloat)
    {
        result.value_r32 = number*decimal_point_adjustment;
    }
    else
    {
        result.value_i32 = number;
    }
    
    return result;
}

char *
AdvancePointerUntil_v_(char *c)
{
    while(1)
    {
        if(*c == 'v' && *(c+1) == ' ')
        {
            break;
        }
        ++c;
    }

    return c;
}

internal b32
Is4x4MatrixInversable(r32 a11, r32 a12, r32 a13, r32 a14,
                       r32 a21, r32 a22, r32 a23, r32 a24, 
                       r32 a31, r32 a32, r32 a33, r32 a34, 
                       r32 a41, r32 a42, r32 a43, r32 a44)
{
    return (a11*a22*a33*a44 + a11*a23*a34*a42 + a11*a24*a32*a43 -
            a11*a24*a33*a42 - a11*a23*a32*a44 - a11*a22*a34*a43 - 
            a12*a21*a33*a44 - a13*a21*a34*a42 - a14*a21*a32*a43 +
            a14*a21*a33*a42 + a13*a21*a32*a44 + a12*a21*a34*a43 +
            a12*a23*a31*a44 + a13*a24*a31*a42 + a14*a22*a31*a43 -
            a14*a23*a31*a42 - a13*a22*a31*a44 - a12*a24*a31*a43 - 
            a12*a23*a34*a41 - a13*a24*a32*a41 - a14*a22*a33*a41 +
            a14*a23*a32*a41 + a13*a22*a34*a41 + a12*a24*a33*a41) != 0;

}

struct pair
{
    u16 index0;
    u16 index1;

    r32 cost;
    v3 newV;
};

internal b32
IsPairValid(pair *pair)
{
    b32 result = false;
    if(!(pair->index0 == U16_Max && pair->index1 == U16_Max))
    {
        result = true;
    }

    return result;
}

internal void 
UpdateQuadricMetricErrorCost(raw_mesh *loadedMesh, r32 *QPerEachVertex, pair *pair)
{
    assert(IsPairValid(pair));
    assert(pair->index0 != pair->index1);

    u32 QStride = 10;

    r32 *Qa = QPerEachVertex + QStride*pair->index0;
    r32 *Qb = QPerEachVertex + QStride*pair->index1;

    r32 QSum0 = Qa[0] + Qa[0];
    r32 QSum1 = Qa[1] + Qa[1];
    r32 QSum2 = Qa[2] + Qa[2];
    r32 QSum3 = Qa[3] + Qa[3];
    r32 QSum4 = Qa[4] + Qa[4];
    r32 QSum5 = Qa[5] + Qa[5];
    r32 QSum6 = Qa[6] + Qa[6];
    r32 QSum7 = Qa[7] + Qa[7];
    r32 QSum8 = Qa[8] + Qa[8];
    r32 QSum9 = Qa[9] + Qa[9];

    // TODO(joon) : Properly choose new v! For now, we are just using the center of the vector v0v1
    v3 newV = (loadedMesh->positions[pair->index0] + loadedMesh->positions[pair->index1])/2.0f;
    pair->newV = newV;

    // newVTranspose*(Q0+Q1)*newV
    pair->cost = newV.x*(QSum0*newV.x + QSum1*newV.y + QSum3*newV.z + QSum6) +
                newV.y*(QSum1*newV.x + QSum2*newV.y + QSum4*newV.z + QSum7) +
                newV.z*(QSum3*newV.x + QSum4*newV.y + QSum5*newV.z + QSum8) +
                1*(QSum6*newV.x + QSum7*newV.y + QSum8*newV.z + QSum9);
}


// Sort the pairs, starting from the least cost pair
internal void
SortPairByCost(pair *pairs, u32 pairCount)
{
// sort the pairs, as the pair with the least cost will be at the top of the pair array
    // not checking if the pair was a duplicate and erased or not, becasue that 
    // will be handled during the contraction
    for(u32 i = 0;
        i < pairCount;
        i++)
    {
        if(IsPairValid(pairs+i))
        {
            r32 minCost = FLT_MAX;
            u32 minIndex = i;

            for(u32 j = i + 1;
                j < pairCount;
                j++)
            {
                if(IsPairValid(pairs+j))
                {
                    if(minCost > pairs[j].cost)
                    {
                        minCost = pairs[j].cost;
                        minIndex = j;
                    }
                }
            }

            // Swap only if the ith pair's cost was not the minimum cost
            if(i != minIndex)
            {
                // TODO(joon) : Three copies..
                pair temp = pairs[i];
                pairs[i] = pairs[minIndex];
                pairs[minIndex] = temp;
            }
        }
    }
}

b32 
FloatEpsilonCompare(r32 a, r32 b)
{
    b32 result = false;
    // NOTE(joon) : A bit high for an epsilon value, but shouldn't matter as we only use this function
    // to compare three FLT_MAX values
    r32 epsilon = 0.01f;

    if(a >= b - epsilon && a < b + epsilon)
    {
        result =true;
    }

    return result;
}

// TODO(joon) : Also add some rdtsc value and begincyclecount so that we can measure how much time this takes!
/*
    NOTE(joon) : Mesh optimizer using quadric error metrics by Garland & Heckbert
    - This assumes that the mesh you are providing is counter-clockwise(needed to calculate the plane equation for Q)
*/
internal void
OptimizeMeshGH(raw_mesh *loadedMesh, 
                r32 optimizeRatio) // newFaceCount = mesh->faceCount * optimizeRatio
{
    assert(optimizeRatio > 0.0f && optimizeRatio < 1.0f);
    /*
        NOTE(joon): Each Q will contain these values, in this order
        Q00// a*a
        Q10// a*b
        Q11// b*b
        Q20// a*c
        Q21// b*c
        Q22// c*c
        Q30// a*d
        Q31// b*d
        Q32// c*d
        Q33// d*d
    */   
    // TODO(joon) : this might be bigger than u32_max
    u32 totalQCount = 10*loadedMesh->position_count;
    r32 *QPerEachVertex = (r32 *)malloc(sizeof(r32)*totalQCount);
    memset (QPerEachVertex, 0, sizeof(r32)*totalQCount);
    u32 QStride = 10; // Because each Q will be containing 10 floats

    // TODO(joon) : What is the maximum number of pairs, in worst case scenario?
    u32 maxPairCount = 15*loadedMesh->index_count;
    pair *pairs = (pair *)malloc(sizeof(pair)*maxPairCount);
    u32 pairCount = 0;

    for(u32 indexIndex = 0;
        indexIndex < loadedMesh->index_count;
        indexIndex += 3)
    {
        u16 ia = loadedMesh->indices[indexIndex+0];
        u16 ib = loadedMesh->indices[indexIndex+1];
        u16 ic = loadedMesh->indices[indexIndex+2];

        v3 va = loadedMesh->positions[ia];
        v3 vb = loadedMesh->positions[ib];
        v3 vc = loadedMesh->positions[ic];

        v3 c = normalize(cross(va-vb, vc-vb));
        // d for plane equation
        r32 d = c.x*va.x + c.y*va.y + c.z*va.z;

        /*
            cost for a vertex = (vertex transpose)*(for all planes that intercept at that vertex, Sum of all Kp) * vertex
            while Kp = 
            a^2 ab ac ad
            ab b^2 bc bd
            ac bc c^2 cd
            ad bd cd d^2
            and because Kp is symmetrical, we can only store 10 floating points
         */
        r32 Kp00 = c.x*c.x; // a*a
        r32 Kp10 = c.x*c.y; // a*b
        r32 Kp11 = c.y*c.y; // b*b
        r32 Kp20 = c.x*c.z; // a*c
        r32 Kp21 = c.y*c.z; // b*c
        r32 Kp22 = c.z*c.z; // c*c
        r32 Kp30 = c.x*d; // a*d
        r32 Kp31 = c.y*d; // b*d
        r32 Kp32 = c.z*d; // c*d
        r32 Kp33 = d*d; // d*d

        r32 *Qa = QPerEachVertex + QStride*ia;
        r32 *Qb = QPerEachVertex + QStride*ib;
        r32 *Qc = QPerEachVertex + QStride*ic;

        Qa[0] += Kp00;
        Qb[0] += Kp00;
        Qc[0] += Kp00;

        Qa[1] += Kp10;
        Qb[1] += Kp10;
        Qc[1] += Kp10;

        Qa[2] += Kp11;
        Qb[2] += Kp11;
        Qc[2] += Kp11;

        Qa[3] += Kp20;
        Qb[3] += Kp20;
        Qc[3] += Kp20;

        Qa[4] += Kp21;
        Qb[4] += Kp21;
        Qc[4] += Kp21;

        Qa[5] += Kp22;
        Qb[5] += Kp22;
        Qc[5] += Kp22;

        Qa[6] += Kp30;
        Qb[6] += Kp30;
        Qc[6] += Kp30;

        Qa[7] += Kp31;
        Qb[7] += Kp31;
        Qc[7] += Kp31;

        Qa[8] += Kp32;
        Qb[8] += Kp32;
        Qc[8] += Kp32;

        Qa[9] += Kp33;
        Qb[9] += Kp33;
        Qc[9] += Kp33;
        // NOTE(joon) : Because these three vertices are already forming the edges,
        // add them to the pair
        pair *newPair = pairs + pairCount++;
        newPair->index0 = ia;
        newPair->index1 = ib;

        newPair = pairs + pairCount++;
        newPair->index0 = ib;
        newPair->index1 = ic;

        newPair = pairs + pairCount++;
        newPair->index0 = ic;
        newPair->index1 = ia;
    }

#if 0
    // Select pairs that are close enough to each other
    // TODO(joon) : This value need to be carefully calibrated
    r32 distanceThresholdSquare = 0.005f;
    for(u32 startingVertexIndex = 0;
        startingVertexIndex < loadedMesh->vertexCount;
        startingVertexIndex++)
    {
        for(u32 testVertexIndex = startingVertexIndex+1;
            testVertexIndex < loadedMesh->vertexCount;
            testVertexIndex++)
        {
            // test distance
            r32 lengthSquare = LengthSquare(loadedMesh->positions[startingVertexIndex] - loadedMesh->positions[testVertexIndex]);
            if(lengthSquare <= distanceThresholdSquare)
            {
                assert(pairCount < maxPairCount);
                pair *pair = pairs + pairCount++;
                pair->index0 = (u16)startingVertexIndex;
                pair->index1 = (u16)testVertexIndex; 
            }
        }
    }
#endif

    // Remove all the duplicate pairs, by marking both the indices to uint_max
    for(u32 i = 0;
        i  < pairCount;
        ++i)
    {
        for(u32 j = i+1;
            j < pairCount;
            ++j)
        {
            if((pairs[i].index0 == pairs[j].index0 && pairs[i].index1 == pairs[j].index1) ||
               (pairs[i].index0 == pairs[j].index1 && pairs[i].index1 == pairs[j].index0))
            {
                pairs[j].index0 = U16_Max;
                pairs[j].index1 = U16_Max;
            }
        }
    }

    // update the costs of all the pairs 
    for(u32 pairIndex = 0;
        pairIndex < pairCount;
        pairIndex++)
    {
        pair *pair = pairs + pairIndex;
        if(IsPairValid(pair))
        {
            UpdateQuadricMetricErrorCost(loadedMesh, QPerEachVertex, pair);
        }
#if 0
        if(Is4x4MatrixInversable(Qa[1]+Qb[1], Qa[1]+Qb[2], Qa[1]+Qb[3], Qa[1]+Qb[4], 
                                Qa[1]+Qb[2], Qa[2]+Qb[2], Qa[2]+Qb[3], Qa[2]+Qb[4], 
                                Qa[1]+Qb[23], Qa[33]+Qb[4], Qa[5]+Qb[5], Qa[8]+Qb[8], 
                                Qa[6]+Qb[6], Qa[7]+Qb[7], Qa[8]+Qb[8], Qa[9]+Qb[9]))
        {
            newPair->cost = 
        }
        else
        {
        }
#endif
    }

    SortPairByCost(pairs, pairCount);

    u32 faceToContractCount = (u32)((loadedMesh->index_count/3)*(1.0f - optimizeRatio));
    u32 contractedFaceCount = 0; 

    u32 newVertexCount = loadedMesh->position_count; // Start from the original vertex count, and decrement by 1 for each contraction
    // Contract the pairs,
    while(contractedFaceCount < faceToContractCount)
    {
        pair *minCostPair = 0;
        r32 minCost = FLT_MAX;
        for(u32 pairIndex = 0;
            pairIndex < pairCount;
            pairIndex++)
        {
            pair *testPair = pairs + pairIndex;
            if(IsPairValid(testPair))
            {
                if(minCost > testPair->cost)
                {
                    minCost = testPair->cost;
                    minCostPair = testPair;
                }
            }
        }
        assert(minCostPair);
        
        /*
            1. Get rid of the least cost pair
            2. update the costs of the pairs that were using the contracted vertex
            3. Re-sort
        */
        //loadedMesh->positions[minCostPair->index0] = minCostPair->newV;

        // invalidate the vertex
        loadedMesh->positions[minCostPair->index1].x = Invalid_R32;
        loadedMesh->positions[minCostPair->index1].y = Invalid_R32;
        loadedMesh->positions[minCostPair->index1].z = Invalid_R32;

        newVertexCount--;

        // update the faces that were using the index that was contracted
        for(u32 indexIndex = 0;
            indexIndex < loadedMesh->index_count;
            indexIndex += 3)
        {
            u32 *i0 = loadedMesh->indices + indexIndex;
            u32 *i1 = loadedMesh->indices + indexIndex + 1;
            u32 *i2 = loadedMesh->indices + indexIndex + 2;

            if(*i0 == minCostPair->index1)
            {
                *i0 = minCostPair->index0;
            }
            else if(*i1 == minCostPair->index1)
            {
                *i1 = minCostPair->index0;
            }
            else if(*i2 == minCostPair->index1)
            {
                *i2 = minCostPair->index0;
            }

            if(!(*i0 == U16_Max && *i1 == U16_Max && *i2 == U16_Max))
            {
                // If more than two out of three indices are the same, it means the face should be contracdted
                if(*i0 == *i1 || *i0 == *i2 || *i1 == *i2)
                {
                    *i0 = U16_Max;
                    *i1 = U16_Max;
                    *i2 = U16_Max;

                    contractedFaceCount++;
                    if(contractedFaceCount == faceToContractCount)
                    {
                        break;
                    }
                }
            }
        }

        u16 originalPairIndex0 = minCostPair->index0;
        u16 originalPairIndex1 = minCostPair->index1;

        // update all the pairs that were using index1
        for(u32 p = 0;
            p < pairCount;
            p++)
        {
            if(p == 626)
            {
                int breakhere = 0;
            }
            if(IsPairValid(pairs + p))
            {
                if(pairs[p].index0 == originalPairIndex1) 
                {
                    pairs[p].index0 = originalPairIndex0;
                }
                if(pairs[p].index1 == originalPairIndex1)
                {
                    pairs[p].index1 = originalPairIndex0;
                }

                if(pairs[p].index0 != pairs[p].index1)
                {
                    // Update the cost
                    UpdateQuadricMetricErrorCost(loadedMesh, QPerEachVertex, pairs + p);
                }
                else
                {
                    pairs[p].index0 = U16_Max;
                    pairs[p].index1 = U16_Max;
                }
            }
        }


        // invalidate the pair
        minCostPair->index0 = U16_Max;
        minCostPair->index1 = U16_Max;

    }
    
    u32 newIndexCount = loadedMesh->index_count - faceToContractCount*3;
    u32 *newIndices = (u32 *)malloc(sizeof(u32)*newIndexCount);
    u32 newIndexIndex = 0;
    for(u32 indexIndex = 0;
        indexIndex < loadedMesh->index_count;
        indexIndex += 3)
    {
        u16 i0 = loadedMesh->indices[indexIndex+0];
        u16 i1 = loadedMesh->indices[indexIndex+1];
        u16 i2 = loadedMesh->indices[indexIndex+2];

        if(!(i0 == U16_Max && i1 == U16_Max && i2 == U16_Max))
        {
            newIndices[newIndexIndex] = i0;
            newIndices[newIndexIndex + 1] = i1; 
            newIndices[newIndexIndex + 2] = i2; 

            newIndexIndex += 3;
        }
    }

    // construct the new mesh that uses updated vertices and indices
    // TODO(joon) : Don't use malloc, use custom allocator?
    v3 *newVertices = (v3 *)malloc(sizeof(v3)*newVertexCount);
    u32 newVertexIndex = 0;
    for(u32 vertexIndex = 0;
        vertexIndex < loadedMesh->position_count;
        ++vertexIndex)
    {
        v3 *vertex = loadedMesh->positions + vertexIndex;

        if(!(FloatEpsilonCompare(vertex->x, Invalid_R32) && FloatEpsilonCompare(vertex->y, Invalid_R32) && 
            FloatEpsilonCompare(vertex->z, Invalid_R32)))
        {
            newVertices[newVertexIndex++] = *vertex;
        }
        else
        {
            for(u32 indexIndex = 0;
                indexIndex < newIndexCount;
                ++indexIndex)
            {
                // Because the vertex was contracted, we need to _push down_ indices that were pointing at the vertices
                // that had bigger indices
                if(newIndices[indexIndex] > vertexIndex)
                {
                    newIndices[indexIndex]--;
                }
            }
        }
    }

    free(pairs);
    free(loadedMesh->positions);
    free(loadedMesh->indices);

    loadedMesh->positions = newVertices;
    loadedMesh->position_count = newVertexCount;
    loadedMesh->indices = newIndices;
    loadedMesh->index_count = newIndexCount;
}

enum obj_token_type
{
    obj_token_type_v,
    obj_token_type_vn,
    obj_token_type_vt,
    obj_token_type_f,
    obj_token_type_i32,
    obj_token_type_r32,
    obj_token_type_slash,
    obj_token_type_hyphen,
    obj_token_type_newline, // unfortunately, this is the only way to parse the obj files

    obj_token_type_eof,
};

struct obj_token
{
    obj_token_type type;

    union
    {
        i32 value_i32;
        r32 value_r32;
    };
};

struct obj_tokenizer
{
    u8 *at;
    u8 *end;
};

internal void
eat_all_white_spaces(obj_tokenizer *tokenizer)
{
    while((//*tokenizer->at == '\n' ||
          *tokenizer->at == '\r' || 
          *tokenizer->at == ' '||
          *tokenizer->at == '#' ) &&
          tokenizer->at <= tokenizer->end)
    {
        if(*tokenizer->at == '#')
        {
            while(*tokenizer->at != '\n')
            {
                tokenizer->at++;
            }
        }
        else
        {
            tokenizer->at++;
        }
    }
}

internal b32
find_string(obj_tokenizer *tokenizer, char *string_to_find, u32 string_size)
{
    b32 result = true;

    eat_all_white_spaces(tokenizer);

    u8 *c = tokenizer->at;
    if(c + string_size <= tokenizer->end)
    {
        while(string_size--)
        {
            if(*c++ != *string_to_find++)
            {
                result = false;
                break;
            }
        }
    }
    else
    {
        result = false;
    }
    

    return result;
}

// NOTE/Joon: This function is more like a general purpose token getter, with minimum erro checking.
// the error checking itself will happen inside the parsing loop, not here.
internal obj_token
peek_token(obj_tokenizer *tokenizer, b32 should_advance)
{
    obj_token result = {};

    eat_all_white_spaces(tokenizer);
    
    u32 advance = 0;
    if(tokenizer->at != tokenizer->end)
    {
        switch(*tokenizer->at)
        {
            case 'v':
            {
                if(*(tokenizer->at+1) == 't')
                {
                    result.type = obj_token_type_vt;
                    advance = 2;
                }
                else if(*(tokenizer->at+1) == 'n')
                {
                    result.type = obj_token_type_vn;
                    advance = 2;
                }
                else
                {
                    result.type = obj_token_type_v;
                    advance = 1;
                }
            }break;

            case 'f':
            {
                result.type = obj_token_type_f;
                advance = 1;
            }break;

            case '-':
            {
                result.type = obj_token_type_hyphen;
                advance = 1;
            }break;

            case '/':
            {
                result.type = obj_token_type_slash;
                advance = 1;
            }break;

            case '\n':
            {
                result.type = obj_token_type_newline;
                advance = 1;
            }break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                parse_numeric_result parse_result = parse_numeric(tokenizer->at);

                if(parse_result.isFloat)
                {
                    result.type = obj_token_type_r32;
                    result.value_r32 = parse_result.value_r32;
                }
                else
                {
                    result.type = obj_token_type_i32;
                    result.value_i32 = parse_result.value_i32;
                }

                advance = parse_result.advance;
            }break;

            case '#':
            {
                tokenizer->at = get_closest_carriage_return(tokenizer->at);
            }break;

            default:
            {
                assert(0);
            }break;
        }
    }
    else
    {
        result.type = obj_token_type_eof;
    }

    if(should_advance)
    {
        tokenizer->at += advance;
    }

    return result;
}

// Mostly used for v for vn
// TODO(Joon) : Also usuable for parsing two numeric values?
internal v3
peek_up_to_three_numeric_values(obj_tokenizer *tokenizer)
{
    v3 result = {};

    u32 numeric_token_count = 0;
    float numbers[3] = {};

    b32 is_minus = false;
    b32 newline_appeared = false;
    while(!newline_appeared)
    {
        obj_token token = peek_token(tokenizer, true);

        switch(token.type)
        {
            case obj_token_type_r32: 
            {
                numbers[numeric_token_count++] = (is_minus?-1:1)*token.value_r32;
                is_minus = false;
            }break;

            case obj_token_type_i32: 
            {
                numbers[numeric_token_count++] = (is_minus?-1:1)*token.value_r32;
                is_minus = false;
            }break;

            case obj_token_type_hyphen: 
            {
                if(is_minus)
                {
                    assert(0);
                }
                else
                {
                    is_minus = true;
                }
            }break;

            case obj_token_type_newline:
            {
                newline_appeared = true;
            }break;

            default:{assert(0);}break; // not allowed!
        }
        assert(numeric_token_count <= 3);
    }

    assert(numeric_token_count == 3);

    result.x = numbers[0];
    result.y = numbers[1];
    result.z = numbers[2];

    return result;
}

// TODO(joon): parsing positions and vertex normals work just fine,
// but havent yet tested with the texture coords. Will do when I have a png loader :)
internal raw_mesh
parse_obj_tokens(MemoryArena *memory_arena, u8 *file, size_t file_size)
{
    assert(file && file_size > 0);

    raw_mesh result ={};

    obj_tokenizer tokenizer = {};
    tokenizer.at = file;
    tokenizer.end = file+file_size;

    b32 is_parsing = true;

    u32 position_count = 0;
    u32 normal_count = 0;
    u32 texcoord_count = 0;

    u32 index_count = 0;
    u32 normal_index_count = 0;
    u32 texcoord_index_count = 0;
   
    // TODO(joon): Just checking one letter might not be safe
    // pre parse the amount of things(vertex, vertex normal, indices..) that we need?
    while(is_parsing)
    {
        obj_token token = peek_token(&tokenizer, true);

        switch(token.type)
        {
            case obj_token_type_v:
            {
                position_count++;
            }break;
            case obj_token_type_vn:
            {
                normal_count++;
            }break;
            case obj_token_type_vt:
            {
                texcoord_count++;
            }break;
            case obj_token_type_f:
            {
                b32 newline_appeared = false;

                // NOTE : property_indicator%3 == 0(v), 1(vt), 2(vn),
                u32 property_indicator = 0;
                u32 index_count_this_line = 0;
                while(!newline_appeared)
                {
                    token = peek_token(&tokenizer, true);

                    switch(token.type)
                    {
                        case obj_token_type_i32:
                        {
                            if(property_indicator == 0)
                            {
                                index_count_this_line++;
                            }
                            else if(property_indicator == 1)
                            {
                                texcoord_index_count++;
                            }
                            else
                            {
                                assert(property_indicator <=  2);
                                normal_index_count++;
                            }

                            obj_token peek = peek_token(&tokenizer, false);
                            if(peek.type == obj_token_type_i32)
                            {
                                property_indicator = 0;
                            }
                        }break;

                        case obj_token_type_slash:
                        {
                            property_indicator++;
                        }break;

                        case obj_token_type_newline:
                        {
                            newline_appeared = true;
                        }break;

                        default:{assert(0);}break; // not allowed!
                    }

                }

                index_count += 3*(index_count_this_line-2);
            }break;
            case obj_token_type_eof:
            {
                is_parsing = false;
            }break;
        }
    }

    result.position_count = position_count;
    result.normal_count = normal_count;
    result.texcoord_count = texcoord_count;
    result.index_count = index_count;
    result.normal_index_count = normal_index_count;
    
    // NOTE(joon) : position and indices are crucial, so we assert heree
    assert(result.position_count != 0);
    result.positions = (v3 *)push_array(memory_arena, v3, result.position_count);
    assert(result.index_count != 0);
    result.indices = (u32 *)push_array(memory_arena, u32, result.index_count);

    // NOTE(joon): properties below are not crucial, but we need to check the count of it 
    // because malloc will return the pointer whether the count is zero or not

    // Do not allow only one of them being NULL 
    assert((result.normal_count != 0 && result.normal_index_count != 0) ||
            (result.normal_count == 0 && result.normal_index_count == 0));
    if(result.normal_count != 0)
    {
        result.normals = (v3 *)push_array(memory_arena, v3, result.normal_count);
    }
    if(result.normal_index_count != 0)
    {
        result.normal_indices = (u32 *)push_array(memory_arena, u32, result.normal_index_count);
    }

    // Do not allow only one of them being NULL 
    assert((result.texcoord_count != 0 && result.texcoord_index_count != 0) ||
            (result.texcoord_count == 0 && result.texcoord_index_count == 0));
    if(result.texcoord_count != 0)
    {
        result.texcoords = (v2 *)push_array(memory_arena, v2, result.texcoord_count);
    }
    if(result.texcoord_index_count != 0)
    {
        result.texcoord_indices = (u32 *)push_array(memory_arena, u32, result.normal_index_count);
    }

    // Reset the counts to use them below
    position_count = 0;
    normal_count = 0;
    texcoord_count = 0;

    index_count = 0;
    normal_index_count = 0;
    texcoord_index_count = 0;

    // Reset the tokenizer to the start of file
    tokenizer.at = file;
    is_parsing = true;
    while(is_parsing)
    {
        obj_token token = peek_token(&tokenizer, true);

        switch(token.type)
        {
            case obj_token_type_v:
            {
                // feed up to three numeric tokens(including minus)
                result.positions[position_count++] = peek_up_to_three_numeric_values(&tokenizer);
            }break;
            case obj_token_type_vn:
            {
                result.normals[normal_count++] = peek_up_to_three_numeric_values(&tokenizer);
            }break;
            case obj_token_type_vt:
            {
                // feed up to two numeric tokens(not including minus)
            }break;
            case obj_token_type_f:
            {
                b32 newline_appeared = false;

                // NOTE : property_indicator%3 == 0(v), 1(vt), 2(vn),
                u32 property_indicator = 0;
                u32 first_index = 0;
                u32 index_count_this_line = 0;
                while(!newline_appeared)
                {
                    token = peek_token(&tokenizer, true);

                    switch(token.type)
                    {
                        case obj_token_type_i32:
                        {
                            if(property_indicator == 0)
                            {
                                if(index_count_this_line < 3)
                                {
                                    if(index_count_this_line == 0)
                                    {
                                        first_index = token.value_i32 - 1;
                                    }
                                    result.indices[index_count++] = token.value_i32 - 1;

                                    index_count_this_line++;
                                }
                                else
                                {
                                    u32 previous_index = result.indices[index_count-1];

                                    result.indices[index_count++] = first_index;
                                    result.indices[index_count++] = previous_index;
                                    result.indices[index_count++] = token.value_i32 - 1;
                                }
                            }
                            else if(property_indicator == 1)
                            {
                                result.texcoord_indices[texcoord_index_count++] = token.value_i32 - 1;
                            }
                            else
                            {
                                assert(property_indicator <=  2);
                                result.normal_indices[normal_index_count++] = token.value_i32 - 1;
                            }

                            obj_token peek = peek_token(&tokenizer, false);
                            if(peek.type == obj_token_type_i32)
                            {
                                property_indicator = 0;
                            }
                        }break;

                        case obj_token_type_slash:
                        {
                            property_indicator++;
                        }break;

                        case obj_token_type_newline:
                        {
                            newline_appeared = true;
                        }break;

                        default:{assert(0);}break; // not allowed!
                    }
                }

            }break;
            case obj_token_type_eof:
            {
                is_parsing = false;
            }break;
        }
    }

    assert(index_count == result.index_count);
    assert(normal_index_count == result.normal_index_count);
    assert(texcoord_index_count == result.texcoord_index_count);

    return result;
}
 
#if 0
#if MEKA_ARM
#include <arm_neon.h>
#elif MEKA_X86_64
#include <immintrin.h>
#endif

// TODO/joon : we can make this faster by multi threading the parsing
internal void
parse_obj_fast(u8 *file, size_t file_size)
{
    assert(file);

    struct mach_timebase_info mach_time_info;
    mach_timebase_info(&mach_time_info);
    r32 nano_seconds_per_tick = ((r32)mach_time_info.numer/(r32)mach_time_info.denom);

    u64 begin_time = mach_absolute_time();

    // NOTE/joon: adding 15 to round up, as each char chunk is 16bytes
    u32 char_chunk_count = (file_size + 15)/16;

    uint8x16_t v_128 = vdupq_n_u8('v');
    uint8x16_t t_128 = vdupq_n_u8('t');
    uint8x16_t f_128 = vdupq_n_u8('f');
    uint8x16_t n_128 = vdupq_n_u8('n');
    uint8x16_t carriage_128 = vdupq_n_u8('\n');

    while(file_size > 15)
    {
        //uint8x16_t c_128 = vld1q_u8(file);
        uint8x16_t c_128 = vdupq_n_u8(0xff);
        uint8x16_t v_compare_result = vceqq_u8(c_128, v_128);
        uint8x16_t t_compare_result = vceqq_u8(c_128, t_128);
        uint8x16_t f_compare_result = vceqq_u8(c_128, f_128);
        uint8x16_t n_compare_result = vceqq_u8(c_128, n_128);

        uint8x16_t result_0 = vshrq_n_u8(c_128, 7);
        uint16x8_t result_1 = vreinterpretq_u16_u8(result_0);
        uint32x4_t result_2 = vsraq_n_u16(result_1, result_1, 7);
        uint64x2_t result_3 = vreinterpretq_u64_u32(vsraq_n_u32(result_2, result_2, 14));
        uint8x16_t paired64 = vreinterpretq_u8_u64(vsraq_n_u64(result_3, result_3, 28));

        u8 low = vgetq_lane_u8(paired64, 0);
        u8 high = vgetq_lane_u8(paired64, 8);

        file_size -= 16;
    }

    u64 end_time = mach_absolute_time();
    u64 TimeDifferenceInNanoSecond = (end_time-begin_time)*nano_seconds_per_tick;

    int a = 1;
}

enum obj_lexicon_type
{
    obj_lexicon_type_v,
    obj_lexicon_type_vn,
    obj_lexicon_type_vt,
    obj_lexicon_type_f,
};

struct obj_lexicon
{
    // TODO/Joon: Is it smarter to store the indices?
    obj_lexicon_type type;
    u8 *start;
    u8 *end;

    u32 index; // NOTE/joon : for each property(vertex, vertex normal...), what index it is
};
internal v3
parse_obj_line_with_v_or_vn(u8 *start, u8 *end)
{
    v3 result = {};
    u8 *c = start;

    r32 p[3] = {};
    u32 p_index = 0;
    while(c < end)
    {
        // TODO/joon: for now, we will just assume that all numbers start with a number, not just .
        if((*c >= '0' && *c <= '9') ||
            *c == '-')
        {
            r32 sign = 1.0f;
            if(*c == '-')
            {
                sign = -1.0f;
                c++;
            }
            
            // NOTE/joon : for example, if the number was 12.9442, decimal_point_p is 4
            b32 decimal_point_appeared = false;
            u32 decimal_point_p = 0;
            u32 number = 0;
            while(*c != ' ' && c < end)
            {
                if(decimal_point_appeared)
                {
                    decimal_point_p++;
                }

                if(*c >= '0' && *c <= '9')
                {
                    number *= 10;
                    number += *c - '0';
                }
                else if(*c == '.')
                {
                    decimal_point_appeared = true;
                }
                else
                {
                    // TODO/joon: file is corrupted!
                    assert(0);
                }

                c++;
            }

            if(p_index < array_count(p))
            {
                if(decimal_point_appeared && decimal_point_p == 0)
                {
                    // TODO/joon: file is corrupted!
                    assert(0);
                }

                // TODO/joon : just use a macro instead of this method which has an issue with floating point precision?
                r32 decimal_adjustment = 1.0f;
                for(u32 decimal_point_index = 0;
                        decimal_point_index < decimal_point_p;
                        ++decimal_point_index)
                {
                    decimal_adjustment *= 0.1f;
                }

                p[p_index++] = sign*number*decimal_adjustment;
            }
        }

        c++;
    }

    result.x = p[0];
    result.y = p[1];
    result.z = p[2];

    return result;
}


// TODO/Joon : not super important, but measure the time just for fun
// Naive method, which is stepping character by character(1 byte)
internal raw_mesh
parse_obj_slow(memory_arena *memory_arena, memory_arena *transient_memory_arena, u8 *file, size_t file_size)
{
    assert(file && file_size != 0);

    parse_obj_fast(file, file_size);

    //TODO/Joon: considering that each lexicon represents each significant line, we can make this number to be equal to \n counts
    // TODO/Joon: This is just a arbitrary number
    u32 lexicon_max_count = 0x00000fffff;
    temp_memory lexicon_temp_memory = start_temp_memory(transient_memory_arena, sizeof(obj_lexicon)*lexicon_max_count);

    u32 v_line_count = 0;
    u32 vn_line_count = 0;
    u32 vt_line_count = 0;
    u32 f_line_count = 0;

    u32 lexicon_count = 0;
    obj_lexicon *obj_lexicons = (obj_lexicon *)lexicon_temp_memory.base;

    u8 *c = file;
    u8 *file_end = file+file_size;
    while(c < file_end)
    {
        switch(*c)
        {
            case 'v':
            {
                obj_lexicon *lexicon = obj_lexicons + lexicon_count++;

                if(*(c+1) == 't')
                {
                    // vt
                    lexicon->type = obj_lexicon_type_vt;
                    lexicon->index = vt_line_count++;
                }
                else if(*(c+1) == 'n')
                {
                    // vn
                    lexicon->type = obj_lexicon_type_vn;
                    lexicon->index = vn_line_count++;
                }
                else
                {
                    // v
                    lexicon->type = obj_lexicon_type_v;
                    lexicon->index = v_line_count++;
                }

                u8 *lexicon_end = get_closest_carriage_return(c);
                lexicon->start = c;
                lexicon->end = lexicon_end;
                c = lexicon_end;
            }break;
            case 'f':
            {
                u8 *lexicon_end = get_closest_carriage_return(c);

                obj_lexicon *lexicon = obj_lexicons + lexicon_count++;
                lexicon->type = obj_lexicon_type_f;
                lexicon->start = c;
                lexicon->end = lexicon_end;
                lexicon->index = f_line_count++;

                c = lexicon_end;
            }break;

            case '#':
            {
                // NOTE/joon : skip the comments
                c = get_closest_carriage_return(c);
            }break;
        }

        assert(lexicon_count < lexicon_max_count);
        c++;
    }

    u32 stride = (v_line_count != 0? 1:0) + (vn_line_count != 0? 1:0) + (vt_line_count != 0? 1:0);

    // NOTE/joon:obj files do allow the normal count and the position count to be different,
    // but we are not going to, as very few models actually has that kind of format(maybe some very simple models such as quad and cube)
    assert(vn_line_count == 0 || (vn_line_count == v_line_count));

    u32 index_count = 0;
    // NOTE/joon: One huge problem with the obj file is that we dont' know how many faces there are in advance,
    // so we cannot pre-allocate the memory for the indices->which forces us to pre-step into the lexicons and find out how many 
    // indices are there in total
    for(u32 lexicon_index = 0;
            lexicon_index < lexicon_count;
            ++lexicon_index)
    {
        obj_lexicon *lexicon = obj_lexicons + lexicon_index;
        
        u32 number_count = 0;
        if(lexicon->type == obj_lexicon_type_f)
        {
            b32 did_number_appeared = false;

            u8 *c = lexicon->start;
            while(c <= lexicon->end)
            {
                if(*c >= '0' && *c <= '9')
                {
                    did_number_appeared = true;
                }
                else
                {
                    if(did_number_appeared)
                    {
                        did_number_appeared = false;
                        number_count++;
                    }
                }

                c++;
            }

            if(stride)
            {
                assert(number_count % stride == 0);

                number_count /= stride;
            }

            index_count += 3*((number_count) - 2);
        }
    }

    raw_mesh mesh = {};
    mesh.index_count = index_count;
    mesh.indices = push_array(memory_arena, u32, sizeof(u32)*index_count);

    mesh.normal_index_count = index_count;
    mesh.normal_indices = push_array(memory_arena, u32, sizeof(u32)*me);

    mesh.texcoord_index_count = index_count;
    mesh.vt_indices = push_array(memory_arena, u32, sizeof(u32)*index_count);

    mesh.positions = push_array(memory_arena, v3, sizeof(v3)*v_line_count);
    if(vn_line_count)
    {
        mesh.normals = push_array(memory_arena, v3, sizeof(v3)*vn_line_count);
    }

    u32 mesh_index_count = 0;
    u32 mesh_normal_index_count = 0;
    u32 mesh_texcoord_index_count = 0;
    for(u32 lexicon_index = 0;
            lexicon_index < lexicon_count;
            ++lexicon_index)
    {
        obj_lexicon *lexicon = obj_lexicons + lexicon_index;
        switch(lexicon->type)
        {
            case obj_lexicon_type_v:
            {
                mesh.positions[mesh.position_count++] = parse_obj_line_with_v_or_vn(lexicon->start, lexicon->end);
            }break;
            case obj_lexicon_type_vn:
            {
                mesh.normals[mesh.normal_count++] = parse_obj_line_with_v_or_vn(lexicon->start, lexicon->end);
            }break;
            case obj_lexicon_type_vt:
            {
            }break;
            case obj_lexicon_type_f:
            {
                // NOTE/joon : obj file represents face in the order of v/vt/vn 
                u32 numbers[100] = {};
                u32 number_count = 0;

                u8 *c = lexicon->start;

                while(c < lexicon->end)
                {
                    parse_numeric_result closest_number = parse_numeric(c);

                    numbers[number_count++] = closest_number.value_i32;
                    c += closest_number.advance;
                }

                // add first, self, and self+1 to the index
                // for exampe if the indices were  A B C D E, we should add ABC, ACD, ADE to the index buffer
                // note that this loop is per v/vt/vn
                for(u32 i = stride;
                        i+stride < number_count;
                        i += stride)
                {
                    mesh.indices[mesh_index_count++] = numbers[0]-1;
                    mesh.indices[mesh_index_count++] = numbers[i]-1;
                    mesh.indices[mesh_index_count++] = numbers[i + stride]-1;
                }

                if(vn_line_count != 0 && vt_line_count == 0)
                {
                    u32 offset = 1;
                    for(u32 i = stride;
                            i+stride < number_count;
                            i += stride)
                    {
                        mesh.vn_indices[mesh_normal_index_count++] = numbers[0+offset]-1;
                        mesh.vn_indices[mesh_normal_index_count++] = numbers[i+offset]-1;
                        mesh.vn_indices[mesh_normal_index_count++] = numbers[i+stride+offset]-1;
                    }
                }
                else if(vn_line_count == 0 && vt_line_count != 0)
                {
                    u32 offset = 1;
                    for(u32 i = stride;
                            i+stride < number_count;
                            i += stride)
                    {
                        mesh.vt_indices[mesh_texcoord_index_count++] = numbers[0+offset]-1;
                        mesh.vt_indices[mesh_texcoord_index_count++] = numbers[i+offset]-1;
                        mesh.vt_indices[mesh_texcoord_index_count++] = numbers[i+stride+offset]-1;
                    }
                }
                else
                {
                    u32 offset = 1;
                    for(u32 i = stride;
                            i+stride < number_count;
                            i += stride)
                    {
                        mesh.vt_indices[mesh_texcoord_index_count++] = numbers[0+offset]-1;
                        mesh.vt_indices[mesh_texcoord_index_count++] = numbers[i+offset]-1;
                        mesh.vt_indices[mesh_texcoord_index_count++] = numbers[i+stride+offset]-1;
                    }

                    offset = 2;
                    for(u32 i = stride;
                            i+stride < number_count;
                            i += stride)
                    {
                        mesh.vn_indices[mesh_normal_index_count++] = numbers[0+offset]-1;
                        mesh.vn_indices[mesh_normal_index_count++] = numbers[i+offset]-1;
                        mesh.vn_indices[mesh_normal_index_count++] = numbers[i+stride+offset]-1;
                    }
                }
            }break;
        }
    }

    assert(mesh_index_count == mesh.index_count);


    end_temp_memory(&lexicon_temp_memory);

    return mesh;
}
#endif

internal f32 *
load_merl_brdf(PlatformReadFileResult file)
{
    // NOTE(joon): merl brdf file = 3 ints at the start + 3(r, g, b) * 90 * 90 * 180 * sizeof(double), without any whitespace
    // So if we want to load r32 numbers instead of r64,
    // we need to make the parser ourselves
    u8 *memory = file.memory;

    i32 dim_0 = *(i32 *)(memory);
    i32 dim_1 = *(i32 *)(memory + sizeof(i32));
    i32 dim_2 = *(i32 *)(memory + 2*sizeof(i32));
    assert(dim_0 == 90 && dim_1 == 90 && dim_2 == 180);

    u32 brdf_table_element_count = 3 * 90 * 90 * 180;
    f32 *brdf_table = (f32 *)malloc(brdf_table_element_count * sizeof(f32));

    memory += 3*sizeof(i32);
    for(u32 element_index = 0;
            element_index < brdf_table_element_count;
            ++element_index)
    {
        brdf_table[element_index] = *((float *)memory);

        memory += sizeof(double);
    }

    return brdf_table;
}

struct load_vox_result
{
    // NOTE(joon) : in voxel
    i32 x_count; 
    i32 y_count;
    i32 z_count;

    i32 voxel_count;
    u8 *colorIDs;
    u8 *xs;
    u8 *ys;
    u8 *zs;

    u32 *palette;
};

internal load_vox_result
load_vox(u8 *file, u32 file_size)
{
    load_vox_result result = {};

    u8 *current = file;
    u8 *end = file + file_size;

    u32 *palette = 0;
    while(current != end)
    {
        if(*current == 'S' &&
            *(current + 1) == 'I' &&
            *(current + 2) == 'Z' &&
            *(current + 3) == 'E')
        {
            i32 *internal_current = (i32 *)(current + 4); 
            i32 chunk_content_size = *(internal_current++);
            i32 children_chunk_content_size = *(internal_current++);
            assert(children_chunk_content_size == 0);

            result.x_count = *(internal_current++);
            result.y_count = *(internal_current++);
            result.z_count = *(internal_current++);
        }
        else if(*current == 'X' &&
                *(current + 1) == 'Y' &&
                *(current + 2) == 'Z' &&
                *(current + 3) == 'I')
        {
            i32 *internal_current = (i32 *)(current + 4); 
            i32 chunk_content_size = *(internal_current++);
            i32 children_chunk_content_size = *(internal_current++);
            assert(children_chunk_content_size == 0);

            result.voxel_count = *(internal_current++);
            result.colorIDs = (u8 *)malloc(result.voxel_count);
            result.xs = (u8 *)malloc(result.voxel_count);
            result.ys = (u8 *)malloc(result.voxel_count);
            result.zs = (u8 *)malloc(result.voxel_count);
            for(u32 voxel_index = 0;
                    voxel_index < result.voxel_count;
                    ++voxel_index)
            {
                i32 xyzi = *internal_current++;

                result.colorIDs[voxel_index] = ((xyzi >> 24) & 0xff);
                result.xs[voxel_index] = ((xyzi >> 16) & 0xff);
                result.ys[voxel_index] = ((xyzi >> 8) & 0xff);
                result.zs[voxel_index] = ((xyzi >> 0) & 0xff);
            }
        }
        else if(*current == 'P' &&
                *(current + 1) == 'A' &&
                *(current + 2) == 'C' &&
                *(current + 3) == 'K')
        {
            invalid_code_path;
        }
        else if(*current == 'R' &&
                *(current + 1) == 'G' &&
                *(current + 2) == 'B' &&
                *(current + 3) == 'A')
        {
            u32 *internal_current = (u32 *)(current + 8); 
            palette = (u32 *)malloc(sizeof(u32) * 256);
            for(u32 i = 0; 
                    i < 256; 
                    i++) 
            {
                palette[i] = *(internal_current++); 
            }
        }

        current++;
    }

    if(palette)
    {
        result.palette = palette;
    }
    else
    {
        // NOTE(joon) use the default one, in case there is no palette provided
        u32 default_palette[256] = 
        {
            0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
            0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
            0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
            0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
            0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
            0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
            0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
            0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
            0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
            0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
            0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
            0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
            0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
            0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
            0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
            0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111
        };

        result.palette = (u32 *)malloc(sizeof(u32) * 256);
        memcpy(result.palette, default_palette, sizeof(u32)*array_count(default_palette));
    }

    return result;
}

internal void
free_loaded_vox(load_vox_result *loaded_vox)
{
    free(loaded_vox->xs);
    free(loaded_vox->ys);
    free(loaded_vox->zs);
}












