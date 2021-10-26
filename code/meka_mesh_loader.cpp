/*
 * Written by Gyuhyun 'Joon' Lee
 * https://github.com/meka-lopo/
 */
struct loaded_vertex
{
    v3 p;
    v3 normal;
    v2 textureCoord;
};

struct load_mesh_file_result
{
    v3 *vertices;
    u32 vertexCount;

    v3 *normals;
    u32 normalCount;

    v2 *textureCoords;
    u32 textureCoordCount;

    u16 *indices;
    u32 indexCount;
};

// NOTE : This function has no bound checking
internal u32
FindClosestConsecutiveUInt32(char *c)
{
    b32 foundFirstNumber = false;

    u32 number = 0;
    while(!foundFirstNumber ||
        ((*c >= '0' && *c <= '9')))
    {
        if(*c >= '0' && *c <= '9' )
        {
            u32 numericChar = *c-48;
            number *= 10;
            number += numericChar;

            foundFirstNumber = true;
        }
        else if(*c == '.')
        {
            // This function only expectes unsigned integer value
            Assert(0)
        }

        c++;
    }

    return number;
}

/*
 * TODO(joon)
 * - Cannot load more than u32 amount of points
 * - Cannot load more than one mesh
 * - Doesnt load any material and texture information
 * - Doesnt check how many points are there in advance
 * - Doesnt check how many buffers are specified inside the file
 */
// NOTE(joon) : Loads single mesh only gltf file, in a _very_ slow way.
internal load_mesh_file_result
ReadSingleMeshOnlygltf(platform_api *platformApi, char *gltfFileName, char *binFileName)
{
    platform_read_file_result gltf = platformApi->ReadFile(gltfFileName);
    platform_read_file_result bin = platformApi->ReadFile(binFileName);

    load_mesh_file_result result = {};

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

        Assert(meshesCharPtr && bufferViewCharPtr);

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
                Assert(curlBracketCount == 0);

                // Some gltf files does not include the bytesOffset variable 
                // for the first member of bufferView to save memory
                if(byteOffsetCount != bufferIndexCount)
                {
                    Assert(byteOffsetCount < bufferIndexCount);
                    byteOffsets[byteOffsetCount] = 0;
                    byteOffsetCount = bufferIndexCount;
                }
            }
            
            c++;
        }

        if(positionBufferViewIndex != UINT_MAX)
        {
            // TODO(joon) : We don't care about buffer index, as there is just one buffer
            Assert(byteLengths[positionBufferViewIndex]%(sizeof(v3)) == 0);

            result.vertexCount = (byteLengths[positionBufferViewIndex]/(sizeof(v3)));
            result.vertices = (v3 *)malloc(sizeof(v3)*result.vertexCount);

            r32 *memory = (r32 *)((u8 *)bin.memory + byteOffsets[positionBufferViewIndex]);
            for(u32 pIndex = 0;
                pIndex < result.vertexCount;
                pIndex++)
            {
                result.vertices[pIndex].x = (r32)*memory;
                result.vertices[pIndex].y = (r32)*(memory + 1);
                result.vertices[pIndex].z = (r32)*(memory + 2);

                memory += 3;
            }
        }
        else
        {
            // TODO(joon) : vertex position is mandatory!
            Assert(0);
        }

        if(normalBufferViewIndex != UINT_MAX)
        {
            Assert(byteLengths[normalBufferViewIndex]%(sizeof(r32)*3) == 0);

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
            //Assert(0);
        }

        if(textureCoordBufferViewIndex != UINT_MAX)
        {
            Assert(byteLengths[textureCoordBufferViewIndex]%(sizeof(r32)*2) == 0);

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
            //Assert(0);
        }

        if(indexBufferViewIndex != UINT_MAX)
        {
            // TODO(joon) : Is all the indices inside gltf 16 bit or should we manually check that
            // by getting the total count from gltf?
            Assert(byteLengths[indexBufferViewIndex]%(sizeof(u16)) == 0);

            result.indexCount = byteLengths[indexBufferViewIndex]/sizeof(u16);
            Assert(result.indexCount%3 == 0); // no faces should be open
            result.indices = (u16 *)malloc(sizeof(u16)*result.indexCount);

            u16 *memory = (u16 *)((u8 *)bin.memory + byteOffsets[indexBufferViewIndex]);
            for(u32 indexIndex = 0;
                indexIndex < result.indexCount;
                indexIndex++)
            {
                result.indices[indexIndex] = *memory;

                memory += 1;
            }
        }
        else
        {
            Assert(0);
        }
    }
    else
    {
        // TODO(joon) : this gltf file cannot be loaded!
        Assert(0);
    }

    return result;
}

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
                Assert(0);
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


// NOTE(joon) : Really slow method, no multithreading, no SIMD
void
ReadOBJFileLineByLine(platform_api *platformApi, char* fileName)
{
    platform_read_file_result file = platformApi->ReadFile(fileName);

    if(file.memory)
    {
        char *vStartChar = AdvancePointerUntil_v_((char *)file.memory);

        char *c = vStartChar;
        u32 vertexPCharCount = 0;
        while(1)
        {
            if((*c == 'v' && *(c+1) == 'n') ||
                (*c == 'v' && *(c+1) == 't') ||
                (*c == 'f'))
            {
                break;
            }

            ++vertexPCharCount;
            ++c;
        }


        // TODO(joon) : complete hack!
        r32 *pXYZs = (r32 *)malloc(sizeof(r32)*5000*3);
        u32 pXYZIndex = 0;

        char *startingNumberChar = 0;
        u32 decimalPointP = 0;
        u32 numberStartIndex = 0;
        b32 numberInputStarted = false;
        b32 isNegativeValue = false;
        for(u32 charIndex = 0;
            charIndex < vertexPCharCount;
            ++charIndex)
        {
            char *character = vStartChar + charIndex;
            if(*character == '0' ||
                *character == '1' ||
                *character == '2' ||
                *character == '3' ||
                *character == '4' ||
                *character == '5' ||
                *character == '6' ||
                *character == '8' ||
                *character == '8' ||
                *character == '9' ||
                *character == '9')
            {
                if(!numberInputStarted)
                {
                    numberStartIndex = charIndex;
                    numberInputStarted = true;
                }
            }
            else if(*character == ' ' || 
                    *character == '\r' ||
                    *character == '\n')
            {
                if(numberInputStarted)
                {
                    pXYZs[pXYZIndex++] = ParseCharIntoR32(vStartChar,
                                                        numberStartIndex,
                                                        charIndex, 
                                                        decimalPointP, isNegativeValue);

                    isNegativeValue = false;
                    numberInputStarted = false;
                }
            }
            else if(*character == '.')
            {
                decimalPointP = charIndex;
            }
            else if(*character == '-')
            {
                isNegativeValue = true;
            }
        }

        int a = 1;
#if 0
        else if(line == "vn ")
        {
        }
        else if(line == "vt ")
        {
        }
#endif
    }
    else
    {
        // NOTE(joon) : obj file not loadable
        Assert(0);
    }
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

// 
// TODO(joon) : Also add some rdtsc value and begincyclecount so that we can measure how much time this takes!
/*
 * NOTE(joon) : Mesh optimizer using quadric error metrics by Garland & Heckbert
 * - This assumes that the mesh you are providing is counter-clockwise(needed to calculate the plane equation for Q)
 * */
internal void
OptimizeMeshGH(load_mesh_file_result *loadedMesh)
{
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

    r32 *QPerEachVertex = (r32 *)malloc(sizeof(r32)*10*loadedMesh->vertexCount);
    u32 QStride = 10; // Because each Q will be containing 10 floats

    // TODO(joon) : What is the maximum number of pairs, in worst case scenario?
    pair *pairs = (pair *)malloc(sizeof(pair)*2*loadedMesh->indexCount);
    u32 pairCount = 0;

    for(u32 indexIndex = 0;
        indexIndex < loadedMesh->indexCount;
        indexIndex += 3)
    {
        u16 ia = loadedMesh->indices[indexIndex+0];
        u16 ib = loadedMesh->indices[indexIndex+1];
        u16 ic = loadedMesh->indices[indexIndex+2];

        v3 va = loadedMesh->vertices[ia];
        v3 vb = loadedMesh->vertices[ib];
        v3 vc = loadedMesh->vertices[ic];


        v3 cross = Cross(va-vb, vc-vb);
        // d for plane equation
        r32 d = cross.x*va.x + cross.y*va.y + cross.z*va.z;
        // NOTE(joon) : Because Q is symmetrical, we only need to store 10 r32s
        r32 Q00 = cross.x*cross.x; // a*a
        r32 Q10 = cross.x*cross.y; // a*b
        r32 Q11 = cross.y*cross.y; // b*b
        r32 Q20 = cross.x*cross.z; // a*c
        r32 Q21 = cross.y*cross.z; // b*c
        r32 Q22 = cross.z*cross.z; // c*c
        r32 Q30 = cross.x*d; // a*d
        r32 Q31 = cross.y*d; // b*d
        r32 Q32 = cross.z*d; // c*d
        r32 Q33 = d*d; // d*d

        r32 *Qa = QPerEachVertex + QStride*ia;
        r32 *Qb = QPerEachVertex + QStride*ib;
        r32 *Qc = QPerEachVertex + QStride*ic;
        Qa[0] += Q00;
        Qb[0] += Q00;
        Qc[0] += Q00;

        Qa[1] += Q10;
        Qb[1] += Q10;
        Qc[1] += Q10;

        Qa[2] += Q11;
        Qb[2] += Q11;
        Qc[2] += Q11;

        Qa[3] += Q20;
        Qb[3] += Q20;
        Qc[3] += Q20;

        Qa[4] += Q21;
        Qb[4] += Q21;
        Qc[4] += Q21;

        Qa[5] += Q22;
        Qb[5] += Q22;
        Qc[5] += Q22;

        Qa[6] += Q30;
        Qb[6] += Q30;
        Qc[6] += Q30;

        Qa[7] += Q31;
        Qb[7] += Q31;
        Qc[7] += Q31;

        Qa[8] += Q32;
        Qb[8] += Q32;
        Qc[8] += Q32;

        Qa[9] += Q33;
        Qb[9] += Q33;
        Qc[9] += Q33;

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
    r32 distanceThresholdSquare = 0.3f;
    for(u32 startingVertexIndex = 0;
        startingVertexIndex < loadedMesh->vertexCount;
        startingVertexIndex++)
    {
        for(u32 testVertexIndex = startingVertexIndex+1;
            testVertexIndex < loadedMesh->vertexCount;
            testVertexIndex++)
        {
            b32 isAlreadyAnEdge = false;
            // NOTE(joon) : Check if there's already an edge formed by two vertices
            for(u32 pairIndex = 0;
                pairIndex < pairCount;
                pairIndex++)
            {
                pair *pair = pairs + pairIndex;
                if((pair->index0 == startingVertexIndex && pair->index1 == testVertexIndex) ||
                    (pair->index0 == testVertexIndex && pair->index1 == startingVertexIndex))
                {
                    isAlreadyAnEdge = true;
                    break;
                }
            }

            if(!isAlreadyAnEdge)
            {
                // test distance
                if(LengthSquare(loadedMesh->vertices[startingVertexIndex] - loadedMesh->vertices[testVertexIndex]) <= distanceThresholdSquare)
                {
                    pair *pair = pairs + pairCount++;
                    pair->index0 = startingVertexIndex;
                    pair->index1 = testVertexIndex; 
                }
            }
        }
    }
#endif

    // update the costs of all the pairs 
    for(u32 pairIndex = 0;
        pairIndex < pairCount;
        pairIndex++)
    {
        pair *pair = pairs + pairIndex;

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
        v3 newV = (loadedMesh->vertices[pair->index0] + loadedMesh->vertices[pair->index1])/2.0f;
        pair->newV = newV;
        // cost = newVTranspose*(Q0+Q1)*newV
        pair->cost = newV.x*(QSum0*newV.x + QSum1*newV.y + QSum3*newV.z + QSum6) +
                        newV.y*(QSum1*newV.x + QSum2*newV.y + QSum4*newV.z + QSum7) +
                        newV.z*(QSum3*newV.x + QSum4*newV.y + QSum5*newV.z + QSum8) +
                        1*(QSum6*newV.x + QSum7*newV.y + QSum8*newV.z + QSum9);
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

    // sort the pairs, as the pair with the least cost will be at the top of the pair array
    for(u32 i = 0;
        i < pairCount;
        i++)
    {
        for(u32 j = i + 1;
            j < pairCount;
            j++)
        {
            if(pairs[i].cost > pairs[j].cost)
            {
                // TODO(joon) : three copies...
                pair temp = pairs[i];
                pairs[i] = pairs[j];
                pairs[j] = temp;
            }
        }
    }

    u32 newVertexCount = loadedMesh->vertexCount;
    // Contract the pairs,
    // TODO(joon) : and update the costs of the pairs that were using index0(or index1?)
    for(u32 pairIndex = 0;
        pairIndex < pairCount;
        pairIndex++)
    {
        pair *pair = pairs + pairIndex;
        if(pair->index0 != pair->index1)
        {
            // Always update the vertex value of the first index of the pair,
            // and make the second index point to the first index
            loadedMesh->vertices[pair->index0] = pair->newV;

            // invalidate the vertex
            loadedMesh->vertices[pair->index1].x = FLT_MAX;
            loadedMesh->vertices[pair->index1].y = FLT_MAX;
            loadedMesh->vertices[pair->index1].z = FLT_MAX;

            // update all the pairs that were using index1
            // don't need to check the pairs that were already contracted
            for(u32 p = pairIndex + 1;
                p < pairCount;
                p++)
            {
                if(pairs[p].index0 == pair->index1) 
                {
                    pairs[p].index0 = pair->index0;
                }
                if(pairs[p].index1 == pair->index1)
                {
                    pairs[p].index1 = pair->index0;
                }
            }

            // update the index
            for(u32 indexIndex = 0;
                indexIndex < loadedMesh->indexCount;
                indexIndex++)
            {
                if(loadedMesh->indices[indexIndex] == pair->index1)
                {
                    // NOTE(joon) : If there was a face that should be contracted(i.e face with index0, index1 and some other index)
                    // this will make the face to be index0, index0, some other index -> and will be invalidated when we 
                    // validate the indexes
                    loadedMesh->indices[indexIndex] = pair->index0;
                }
            }

            newVertexCount--;

            // NOTE(joon) : This basically is the LOD we want
            if(newVertexCount < loadedMesh->vertexCount/1.1f)
            {
                break;
            }
        }
    }
    
    // construct the new mesh that uses updated vertices and indices
    v3 *newVertices = (v3 *)malloc(sizeof(v3)*newVertexCount);
    u32 nvi = 0;
    for(u32 vertexIndex = 0;
        vertexIndex < loadedMesh->vertexCount;
        vertexIndex++)
    {
        v3 vertex = loadedMesh->vertices[vertexIndex];
        if(vertex.x == FLT_MAX && vertex.y == FLT_MAX && vertex.z == FLT_MAX)
        {
            for(u32 indexIndex = 0;
                indexIndex < loadedMesh->indexCount;
                indexIndex++)
            {
                // reposition the indexes that have higher index value than the deleted vertex index
                if(loadedMesh->indices[indexIndex] > vertexIndex)
                {
                    loadedMesh->indices[indexIndex]--;
                }
            }
        }
        else
        {
            Assert(nvi < newVertexCount);
            newVertices[nvi++] = vertex;
        }
    }
    Assert(nvi == newVertexCount);

    // TODO(joon) : Count is a bit hard to know in advance... so this is hard coded for now.
    u16 *newIndices = (u16 *)malloc(sizeof(u16)*loadedMesh->indexCount);
    u32 newIndexCount = 0;

    for(u32 indexIndex = 0;
        indexIndex < loadedMesh->indexCount;
        indexIndex += 3)
    {
        u16 i0 = loadedMesh->indices[indexIndex+0];
        u16 i1 = loadedMesh->indices[indexIndex+1];
        u16 i2 = loadedMesh->indices[indexIndex+2];

        // NOTE(joon) : If two indices out of 3 indices are the same, it means the face is contracted
        if(!(i0 == i1 || i1 == i2 || i0 == i2))
        {
            newIndices[newIndexCount++] = i0;
            newIndices[newIndexCount++] = i1;
            newIndices[newIndexCount++] = i2;

            Assert(newIndexCount < loadedMesh->indexCount);

        }
    }

    free(loadedMesh->vertices);
    free(loadedMesh->indices);
    loadedMesh->vertices = newVertices;
    loadedMesh->vertexCount = newVertexCount;
    loadedMesh->indices = newIndices;
    loadedMesh->indexCount = newIndexCount;
}


















