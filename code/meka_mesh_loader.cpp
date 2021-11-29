/*
 * Written by Gyuhyun 'Joon' Lee
 * https://github.com/meka-lopo/
 */

#define Invalid_R32 10000.0f

struct find_closest_u32_result
{
    u32 number;
    u8 *advanced;
};

// NOTE : This function has no bound checking
internal find_closest_u32_result
find_closest_u32(u8 *c)
{
    find_closest_u32_result result = {};

    b32 found_number = false;
    while(1)
    {
        if(*c >= '0' && *c <= '9' )
        {
            found_number = true;

            result.number *= 10;
            result.number += *c-'0';
        }
        else
        {
            if(found_number)
            {
                result.advanced = c;
                break;
            }
        }

        c++;
    }
    
    return result;
}

#if 0
/*
    TODO(joon)
    - Cannot load more than u32 amount of points
    - Cannot load more than one mesh
    - Doesnt load any material and texture information
    - Doesnt check how many points are there in advance
    - Doesnt check how many buffers are specified inside the file
 */
// NOTE(joon) : Loads single mesh only gltf file, in a _very_ slow way.
internal raw_mesh
ReadSingleMeshOnlygltf(platform_api *platformApi, char *gltfFileName, char *binFileName)
{
    platform_read_file_result gltf = platformApi->ReadFile(gltfFileName);
    platform_read_file_result bin = platformApi->ReadFile(binFileName);

    raw_mesh result = {};

    if(gltf.memory && bin.memory)
    {
        char *c = (char *)gltf.memory;

        char *meshesCharPtr = 0;
        while(*c != '\0')
        {
            if(StringCompare(c, "meshes"))
            {
                meshesCharPtr = c;
                break;
            }

            c++;
        }

        c = (char *)gltf.memory;
        char *bufferViewCharPtr = 0;
        while(*c != '\0')
        {
            if(StringCompare(c, "bufferViews"))
            {
                bufferViewCharPtr = c;
                break;
            }

            c++;
        }

        assert(meshesCharPtr && bufferViewCharPtr);

        u32 positionBufferViewIndex = UINT_MAX;
        u32 normalBufferViewIndex = UINT_MAX;
        u32 textureCoordBufferViewIndex = UINT_MAX;
        u32 indexBufferViewIndex = UINT_MAX;

        u32 boxBracketCount = 0;
        //u32 curlyBracketCount = 0;
        c = (char *)meshesCharPtr;
        while(*c != '\0')
        {
            if(StringCompare(c, "\"NORMAL\""))
            {
                normalBufferViewIndex = FindClosestConsecutiveUInt32(c);
            }
            else if(StringCompare(c, "\"POSITION\""))
            {
                positionBufferViewIndex = FindClosestConsecutiveUInt32(c);
            }
            else if(StringCompare(c, "\"TEXCOORD_0\""))
            {
                textureCoordBufferViewIndex = FindClosestConsecutiveUInt32(c);
            }
            else if(StringCompare(c, "\"indices\""))
            {
                indexBufferViewIndex = FindClosestConsecutiveUInt32(c);
            }
            else if(*c == '[')
            {
                boxBracketCount++;
            }
            else if(*c == ']')
            {
                boxBracketCount--;
                if(boxBracketCount == 0)
                {
                    break;
                }
            }
            c++;
        }

        // TODO(joon) : Obviously, this is a hack.
        u32 bufferIndices[100] = {};
        u32 bufferIndexCount = 0;
        u32 byteLengths[100] = {};
        u32 byteLengthCount = 0;
        u32 byteOffsets[100] = {};
        u32 byteOffsetCount = 0;
        //u32 targets[100] = {};

        u32 curlBracketCount = 0;
        boxBracketCount = 0;
        c = (char *)bufferViewCharPtr;
        while(*c != '\0')
        {
            if(StringCompare(c, "\"buffer\""))
            {
                bufferIndices[bufferIndexCount++] = FindClosestConsecutiveUInt32(c);
            }
            else if(StringCompare(c, "\"byteLength\""))
            {
                byteLengths[byteLengthCount++] = FindClosestConsecutiveUInt32(c);
            }
            else if(StringCompare(c, "\"byteOffset\""))
            {
                byteOffsets[byteOffsetCount++] = FindClosestConsecutiveUInt32(c);
            }
            else if(StringCompare(c, "\"target\""))
            {
                // TODO(joon) : Should we care about this?
            }
            else if(*c == '[')
            {
                boxBracketCount++;
            }
            else if(*c == ']')
            {
                boxBracketCount--;
                if(boxBracketCount == 0)
                {
                    break;
                }
            }
            else if(*c == '{')
            {
                // one block of bufferview has started
                curlBracketCount++;
            }
            else if(*c == '}')
            {
                curlBracketCount--;
                assert(curlBracketCount == 0);

                // Some gltf files does not include the bytesOffset variable 
                // for the first member of bufferView to save memory
                if(byteOffsetCount != bufferIndexCount)
                {
                    assert(byteOffsetCount < bufferIndexCount);
                    byteOffsets[byteOffsetCount] = 0;
                    byteOffsetCount = bufferIndexCount;
                }
            }
            
            c++;
        }

        if(positionBufferViewIndex != UINT_MAX)
        {
            // TODO(joon) : We don't care about buffer index, as there is just one buffer
            assert(byteLengths[positionBufferViewIndex]%(sizeof(v3)) == 0);

            result.position_count = (byteLengths[positionBufferViewIndex]/(sizeof(v3)));
            result.positions = (v3 *)malloc(sizeof(v3)*result.position_count);

            r32 *memory = (r32 *)((u8 *)bin.memory + byteOffsets[positionBufferViewIndex]);
            for(u32 pIndex = 0;
                pIndex < result.position_count;
                pIndex++)
            {
                result.positions[pIndex].x = (r32)*memory;
                result.positions[pIndex].y = (r32)*(memory + 1);
                result.positions[pIndex].z = (r32)*(memory + 2);

                memory += 3;
            }
        }
        else
        {
            // TODO(joon) : vertex position is mandatory!
            assert(0);
        }

        if(normalBufferViewIndex != UINT_MAX)
        {
            assert(byteLengths[normalBufferViewIndex]%(sizeof(r32)*3) == 0);

            result.normalCount = (byteLengths[positionBufferViewIndex]/(sizeof(r32)*3));
            result.normals = (v3 *)malloc(sizeof(v3)*result.normalCount);

            r32 *memory = (r32 *)((u8 *)bin.memory + byteOffsets[normalBufferViewIndex]);
            for(u32 normalIndex = 0;
                normalIndex < result.normalCount;
                normalIndex++)
            {
                result.normals[normalIndex].x = *memory;
                result.normals[normalIndex].y = *(memory + 1);
                result.normals[normalIndex].z = *(memory + 2);

                memory += 3;
            }
        }
        else
        {
            // TODO(joon) : Generate normal vectors ourselves, if there are no normal information
            //assert(0);
        }

        if(textureCoordBufferViewIndex != UINT_MAX)
        {
            assert(byteLengths[textureCoordBufferViewIndex]%(sizeof(r32)*2) == 0);

            result.textureCoordCount = (byteLengths[textureCoordBufferViewIndex]/(sizeof(r32)*2));
            result.textureCoords = (v2 *)malloc(sizeof(v2)*result.textureCoordCount);

            r32 *memory = (r32 *)((u8 *)bin.memory + byteOffsets[normalBufferViewIndex]);
            for(u32 textureCoordIndex = 0;
                textureCoordIndex < result.textureCoordCount;
                textureCoordIndex++)
            {
                result.textureCoords[textureCoordIndex].x = *memory;
                result.textureCoords[textureCoordIndex].y = *(memory + 1);

                memory += 2;
            }
        }
        else
        {
            // TODO(joon) : Again, texture coord is not mandatory, so we should not assert here.
            //assert(0);
        }

        if(indexBufferViewIndex != UINT_MAX)
        {
            // TODO(joon) : Is all the indices inside gltf 16 bit or should we manually check that
            // by getting the total count from gltf?
            assert(byteLengths[indexBufferViewIndex]%(sizeof(u16)) == 0);

            result.index_count = byteLengths[indexBufferViewIndex]/sizeof(u16);
            assert(result.index_count%3 == 0); // no faces should be open
            result.indices = (u16 *)malloc(sizeof(u16)*result.index_count);

            u16 *memory = (u16 *)((u8 *)bin.memory + byteOffsets[indexBufferViewIndex]);
            for(u32 indexIndex = 0;
                indexIndex < result.index_count;
                indexIndex++)
            {
                result.indices[indexIndex] = *memory;

                memory += 1;
            }
        }
        else
        {
            assert(0);
        }
    }
    else
    {
        // TODO(joon) : this gltf file cannot be loaded!
        assert(0);
    }

    return result;
}
#endif

#define decimalPointP1 0.1f
#define decimalPointP2 0.01f
#define decimalPointP3 0.001f
#define decimalPointP4 0.0001f
#define decimalPointP5 0.00001f
#define decimalPointP6 0.000001f

internal r32
ParseCharIntoR32(char *buffer, u32 start, u32 end, u32 decimalPointP, b32 isNegativeValue)
{
    i32 decimal = 0;

    for(u32 index = start;
        index < end;
        ++index)
    {
        char c = buffer[index]; 
        switch(c)
        {
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '0':
            {
                i32 number = c - 48;
                decimal = 10*decimal;
                decimal += number;
            }
        }
    }

    if(isNegativeValue)
    {
        decimal *= -1;
    }

    r32 result = (r32)decimal;
    // TODO(joon) : This is busted if we have a number without decimal point
    if(end - decimalPointP - 1 > 0)
    {
        float decimalPointR32 = 0.0f;
        switch(end - decimalPointP - 1)
        {
            case 1:
            {
                decimalPointR32 = decimalPointP1;
            }break;
            case 2:
            {
                decimalPointR32 = decimalPointP2;
            }break;
            case 3:
            {
                decimalPointR32 = decimalPointP3;
            }break;
            case 4:
            {
                decimalPointR32 = decimalPointP4;
            }break;
            case 5:
            {
                decimalPointR32 = decimalPointP5;
            }break;
            case 6:
            {
                decimalPointR32 = decimalPointP6;
            }break;

            default:
            {
                // TODO(joon): doesn't support more than 6 deicmal points, as it's out of floating point precision
                assert(0);
            }break;
        }

        result *= decimalPointR32;
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

/*

static void ParseLines(example_terminal *Terminal, source_buffer_range Range, cursor_state *Cursor)
{
     TODO(casey): Currently, if the commit of line data _straddles_ a control code boundary
       this code does not properly _stop_ the processing cursor.  This can cause an edge case
       where a VT code that splits a line _doesn't_ split the line as it should.  To fix this
       the ending code just needs to check to see if the reason it couldn't parse an escape
       code in "AtEscape" was that it ran out of characters, and if so, don't advance the parser
       past that point.

    // load 8bit character into the simd storage, which means there will be 16 characters per __m128i
    __m128i Carriage = _mm_set1_epi8('\n');
    __m128i Escape = _mm_set1_epi8('\x1b');
    __m128i Complex = _mm_set1_epi8(0x80);

    size_t SplitLineAtCount = 4096;
    size_t LastP = Range.AbsoluteP;
    while(Range.Count)
    {
        __m128i ContainsComplex = _mm_setzero_si128();
        size_t Count = Range.Count;
        if(Count > SplitLineAtCount) Count = SplitLineAtCount;
        char *Data = Range.Data;
        while(Count >= 16)
        {
            // load 16 characters
            __m128i Batch = _mm_loadu_si128((__m128i *)Data);

            // test if the character has certain patterns.
            __m128i TestC = _mm_cmpeq_epi8(Batch, Carriage);
            __m128i TestE = _mm_cmpeq_epi8(Batch, Escape);
            __m128i TestX = _mm_and_si128(Batch, Complex);
            __m128i Test = _mm_or_si128(TestC, TestE);
            int Check = _mm_movemask_epi8(Test);
            if(Check)
            {
                int Advance = _tzcnt_u32(Check);
                __m128i MaskX = _mm_loadu_si128((__m128i *)(OverhangMask + 16 - Advance));
                TestX = _mm_and_si128(MaskX, TestX);
                ContainsComplex = _mm_or_si128(ContainsComplex, TestX);
                Count -= Advance;
                Data += Advance;
                break;
            }

            ContainsComplex = _mm_or_si128(ContainsComplex, TestX);
            Count -= 16;
            Data += 16;
        }

        Range = ConsumeCount(Range, Data - Range.Data);

        Terminal->Lines[Terminal->CurrentLineIndex].ContainsComplexChars |=
            _mm_movemask_epi8(ContainsComplex);

        if(AtEscape(&Range))
        {
            size_t FeedAt = Range.AbsoluteP;
            if(ParseEscape(Terminal, &Range, Cursor))
            {
                LineFeed(Terminal, FeedAt, FeedAt, Cursor->Props);
            }
        }
        else
        {
            char Token = GetToken(&Range);
            if(Token == '\n')
            {
                LineFeed(Terminal, Range.AbsoluteP, Range.AbsoluteP, Cursor->Props);
            }
            else if(Token < 0) // TODO(casey): Not sure what is a "combining char" here, really, but this is a rough test
            {
                Terminal->Lines[Terminal->CurrentLineIndex].ContainsComplexChars = 1;
            }
        }

        UpdateLineEnd(Terminal, Range.AbsoluteP);
        if(GetLineLength(&Terminal->Lines[Terminal->CurrentLineIndex]) > SplitLineAtCount)
        {
            LineFeed(Terminal, Range.AbsoluteP, Range.AbsoluteP, Cursor->Props);
        }
    }
}
*/

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
    if(!(pair->index0 == U16_MAX && pair->index1 == U16_MAX))
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
    assert(optimizeRatio > 0.0f && optimizeRatio < 1.0f)
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

        v3 cross = normalize(Cross(va-vb, vc-vb));
        // d for plane equation
        r32 d = cross.x*va.x + cross.y*va.y + cross.z*va.z;

        /*
            cost for a vertex = (vertex transpose)*(for all planes that intercept at that vertex, Sum of all Kp) * vertex
            while Kp = 
            a^2 ab ac ad
            ab b^2 bc bd
            ac bc c^2 cd
            ad bd cd d^2
            and because Kp is symmetrical, we can only store 10 floating points
         */
        r32 Kp00 = cross.x*cross.x; // a*a
        r32 Kp10 = cross.x*cross.y; // a*b
        r32 Kp11 = cross.y*cross.y; // b*b
        r32 Kp20 = cross.x*cross.z; // a*c
        r32 Kp21 = cross.y*cross.z; // b*c
        r32 Kp22 = cross.z*cross.z; // c*c
        r32 Kp30 = cross.x*d; // a*d
        r32 Kp31 = cross.y*d; // b*d
        r32 Kp32 = cross.z*d; // c*d
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
                pairs[j].index0 = U16_MAX;
                pairs[j].index1 = U16_MAX;
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

            if(!(*i0 == U16_MAX && *i1 == U16_MAX && *i2 == U16_MAX))
            {
                // If more than two out of three indices are the same, it means the face should be contracdted
                if(*i0 == *i1 || *i0 == *i2 || *i1 == *i2)
                {
                    *i0 = U16_MAX;
                    *i1 = U16_MAX;
                    *i2 = U16_MAX;

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
                    pairs[p].index0 = U16_MAX;
                    pairs[p].index1 = U16_MAX;
                }
            }
        }


        // invalidate the pair
        minCostPair->index0 = U16_MAX;
        minCostPair->index1 = U16_MAX;

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

        if(!(i0 == U16_MAX && i1 == U16_MAX && i2 == U16_MAX))
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

#if MEKA_ARM
#include <arm_neon.h>
#elif MEKA_X86_64
#include <immintrin.h>
#endif

// TODO/joon : we can make this faster by multi threading the parsing
internal void
parse_obj_into_lexicon(u8 *file, size_t file_size)
{
    assert(file);

    // NOTE/joon: adding 15 to round up, as each char chunk is 16bytes
    u32 char_chunk_count = (file_size + 15)/16;

    uint8x16_t v_128 = vdupq_n_u8('v');
    uint8x16_t t_128 = vdupq_n_u8('t');
    uint8x16_t f_128 = vdupq_n_u8('f');
    uint8x16_t n_128 = vdupq_n_u8('n');

    while(file_size > 0)
    {
        if(file_size >= 16)
        {
            uint8x16_t c_128 = vld1q_u8(file);
            uint8x16_t v_compare_result = vceqq_u8(c_128, v_128);
            uint8x16_t t_compare_result = vceqq_u8(c_128, t_128);
            uint8x16_t f_compare_result = vceqq_u8(c_128, f_128);
            uint8x16_t n_compare_result = vceqq_u8(c_128, n_128);

            file_size -= 16;
        }
        else
        {
        }
    }
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
parse_obj_slow(memory_arena *permanent_memory_arena, memory_arena *transient_memory_arena, u8 *file, size_t file_size)
{
    assert(file && file_size != 0);

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
    mesh.indices = push_array(permanent_memory_arena, u32, sizeof(u32)*index_count);

    mesh.vn_index_count = index_count;
    mesh.vn_indices = push_array(permanent_memory_arena, u32, sizeof(u32)*index_count);

    mesh.vt_index_count = index_count;
    mesh.vt_indices = push_array(permanent_memory_arena, u32, sizeof(u32)*index_count);

    mesh.positions = push_array(permanent_memory_arena, v3, sizeof(v3)*v_line_count);
    if(vn_line_count)
    {
        mesh.normals = push_array(permanent_memory_arena, v3, sizeof(v3)*vn_line_count);
    }

    u32 mesh_index_count = 0;
    u32 mesh_vn_index_count = 0;
    u32 mesh_vt_index_count = 0;
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
                    find_closest_u32_result closest_u32 = find_closest_u32(c);

                    numbers[number_count++] = closest_u32.number;
                    c = closest_u32.advanced;
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
                        mesh.vn_indices[mesh_vn_index_count++] = numbers[0+offset]-1;
                        mesh.vn_indices[mesh_vn_index_count++] = numbers[i+offset]-1;
                        mesh.vn_indices[mesh_vn_index_count++] = numbers[i+stride+offset]-1;
                    }
                }
                else if(vn_line_count == 0 && vt_line_count != 0)
                {
                    u32 offset = 1;
                    for(u32 i = stride;
                            i+stride < number_count;
                            i += stride)
                    {
                        mesh.vt_indices[mesh_vt_index_count++] = numbers[0+offset]-1;
                        mesh.vt_indices[mesh_vt_index_count++] = numbers[i+offset]-1;
                        mesh.vt_indices[mesh_vt_index_count++] = numbers[i+stride+offset]-1;
                    }
                }
                else
                {
                    u32 offset = 1;
                    for(u32 i = stride;
                            i+stride < number_count;
                            i += stride)
                    {
                        mesh.vt_indices[mesh_vt_index_count++] = numbers[0+offset]-1;
                        mesh.vt_indices[mesh_vt_index_count++] = numbers[i+offset]-1;
                        mesh.vt_indices[mesh_vt_index_count++] = numbers[i+stride+offset]-1;
                    }

                    offset = 2;
                    for(u32 i = stride;
                            i+stride < number_count;
                            i += stride)
                    {
                        mesh.vn_indices[mesh_vn_index_count++] = numbers[0+offset]-1;
                        mesh.vn_indices[mesh_vn_index_count++] = numbers[i+offset]-1;
                        mesh.vn_indices[mesh_vn_index_count++] = numbers[i+stride+offset]-1;
                    }
                }
            }break;
        }
    }

    assert(mesh_index_count == mesh.index_count);


    end_temp_memory(&lexicon_temp_memory);

    return mesh;
}
















