#include "hb_obj.h"

// NOTE(gh) 'peek' function takes a Tokenizer as a value, therefore 
// not advancing the tokenizer
// 'eat' function takes a tokenizer pointer, and will advance the tokenizer
inline void
eat(Tokenizer *tokenizer, i32 a)
{
    tokenizer->at += a;
}

// NOTE(gh) this function has no bound checking
internal b32
string_compare(char *a, char *b)
{
    b32 result = true;

    while((*a != '\0') && 
        ((u8)*b != '\0'))
    {
        if(*a != (u8)*b)
        {
            result = false;
            break;
        }

        a++;
        b++;
    }

    return result;
}

// NOTE(gh) this function has no bound check
internal void
unsafe_string_append(char *dest, char *source, u32 source_size)
{
    while(*dest != '\0')
    {
        dest++;
    }

    while(source_size-- > 0)
    {
        *dest++ = *source++;
    }
}

// NOTE(gh) this function has no bound check
internal void
unsafe_string_append(char *dest, char *source)
{
    while(*dest != '\0')
    {
        dest++;
    }
    
    while(*source != '\0')
    {
        *dest++ = *source++;
    }
}

// NOTE(gh) THIS DOES NOT WORK IF THERE WAS A DIRECTORY WITH .
internal void
get_extension(char *dest, char *source)
{
    assert(*dest == 0);

    while(*source != '.')
    {
        source++;
    }

    source++;

    while(*source != '\0')
    {
        *dest++ = *source++;
    }
}

internal void
eat_until_whitespace(Tokenizer *tokenizer)
{
    while(tokenizer->at < tokenizer->one_past_end)
    {
        if(*tokenizer->at == '\n' ||
            *tokenizer->at == '\r' ||
           *tokenizer->at == ' ')
        {
            break;
        }

        tokenizer->at++;
    }
}

internal void
eat_until_newline(Tokenizer *tokenizer)
{
    while(tokenizer->at != tokenizer->one_past_end)
    {
        if(*tokenizer->at == '\n' ||
            *tokenizer->at == '\r')
        {
            break;
        }

        tokenizer->at++;
    }
}


internal void
eat_all_whitespaces(Tokenizer *tokenizer)
{
    while(tokenizer->at != tokenizer->one_past_end)
    {
        if(!(*tokenizer->at == '\n' ||
            *tokenizer->at == '\r' ||
           *tokenizer->at == ' '))
        {
            break;
        }

        tokenizer->at++;
    }
}

internal ParseNumericResult
eat_numeric(Tokenizer *tokenizer)
{
#if 0
    assert(*start == '0' || 
            *start == '1' || 
            *start == '2' || 
            *start == '3' || 
            *start == '4' || 
            *start == '5' || 
            *start == '6' || 
            *start == '7' || 
            *start == '8' || 
            *start == '9');
#endif

    ParseNumericResult result = {};

    b32 scientific_notation = false;
    f64 decimal_point_adjustment = 10.0f;
    f64 number = 0;
    while(tokenizer->at < tokenizer->one_past_end)
    {
        if(*tokenizer->at >= '0' && *tokenizer->at <= '9' )
        {
            number *= 10;
            number += *tokenizer->at-'0';
        }
        else if(*tokenizer->at == '.')
        {
            result.is_float = true;
        }
        else if(*tokenizer->at == 'e')
        {
            // -5.4335527188698052e-09
            scientific_notation = true;
            break;
        }
        else
        {
            break;
        }

        if(result.is_float)
        {
            decimal_point_adjustment *= 0.1f;
        }

        tokenizer->at++;
    }

    if(result.is_float)
    {
        result.value_f32 = (f32)(number*decimal_point_adjustment);
    }
    else
    {
        result.value_i32 = (i32)number;
    }

    if(scientific_notation)
    {
        assert(*tokenizer->at == 'e');
        eat(tokenizer, 1);

        b32 scientific_plus = false;
        if(*tokenizer->at == '+')
        {
            scientific_plus = true;
        }
        eat(tokenizer, 1);

        f32 scientific_value = 0;
        while(tokenizer->at < tokenizer->one_past_end)
        {
            if(*tokenizer->at >= '0' && *tokenizer->at <= '9' )
            {
                scientific_value *= 10;
                scientific_value += (*tokenizer->at-'0');
            }
            else
            {
                break;
            }

            eat(tokenizer, 1);
        }

        result.value_f32 *= powf(10.0f, (scientific_plus ? 1 : -1) * scientific_value);
    }
    
    return result;
}

internal void
numeric_obj_token_to_f32(ObjToken token, f32 *dest)
{
    if(token.type == obj_token_type_f32)
    {
        *dest = token.value_f32;
    }
    else if(token.type == obj_token_type_i32)
    {
        *dest = (f32)token.value_i32;
    }
    else
    {
        invalid_code_path;
    }
}



internal PlyToken
eat_ply_token(Tokenizer *tokenizer)
{
    PlyToken result = {};

    // NOTE(gh) eat all the unnecessary charcters(whilespace, newline...)
    eat_all_whitespaces(tokenizer);

    if(tokenizer->at < tokenizer->one_past_end)
    {
        if(string_compare((char *)tokenizer->at, "element"))
        {
            result.type = ply_token_type_element;
            eat_until_whitespace(tokenizer);
        }
        else if(string_compare((char *)tokenizer->at, "vertex"))
        {
            result.type = ply_token_type_vertex;
            eat_until_whitespace(tokenizer);
        }
        else if(string_compare((char *)tokenizer->at, "face"))
        {
            result.type = ply_token_type_face;
            eat_until_whitespace(tokenizer);
        }
        else if(string_compare((char *)tokenizer->at, "end_header"))
        {
            result.type = ply_token_type_end_header;
            eat_until_whitespace(tokenizer);
        }
        else if(string_compare((char *)tokenizer->at, "property"))
        {
            result.type = ply_token_type_property;
            eat_until_whitespace(tokenizer);
        }
  
        else
        {
            b32 hyphen_appeared = false;
            switch(*tokenizer->at)
            {
                case '-':
                {
                    hyphen_appeared = true;
                    tokenizer->at++;
                };
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
                    ParseNumericResult parse_numeric_result = eat_numeric(tokenizer);

                    if(parse_numeric_result.is_float)
                    {
                        result.type = ply_token_type_f32;
                        result.value_f32 = parse_numeric_result.value_f32;
                        result.is_float = true;

                        if(hyphen_appeared)
                        {
                            result.value_f32 *= -1.0f;
                            hyphen_appeared = false;
                        }
                    }
                    else
                    {
                        result.type = ply_token_type_i32;
                        result.value_i32 = parse_numeric_result.value_i32;
                        result.is_float = false;
                        if(hyphen_appeared)
                        {
                            result.value_i32 *= -1;
                            hyphen_appeared = false;
                        }
                    }
                }break;

                default:
                {
                }break;
            }

        }

    }

    return result;
}

internal PlyToken
eat_and_check_ply_token(Tokenizer *tokenizer, PlyTokenType expected_type)
{
    PlyToken result = eat_ply_token(tokenizer);
    assert(result.type == expected_type);

    return result;
}

internal PlyToken
peek_ply_token(Tokenizer tokenizer)
{
    PlyToken result = eat_ply_token(&tokenizer);

    return result;
}

internal ParsePlyHeaderResult
parse_ply_header(u8 *memory, u32 file_size)
{
    ParsePlyHeaderResult result = {};

    Tokenizer tokenizer = {};
    tokenizer.at = memory;
    tokenizer.one_past_end = memory + file_size;

    b32 end_header_appeared = false;
    while(tokenizer.at < tokenizer.one_past_end && !end_header_appeared)
    {
        PlyToken token = eat_ply_token(&tokenizer);

        switch(token.type)
        {
            case ply_token_type_element:
            {
                PlyToken element_name = eat_ply_token(&tokenizer);
                if(element_name.type == ply_token_type_vertex)
                {
                    PlyToken vertex_count = eat_and_check_ply_token(&tokenizer, ply_token_type_i32);
                    result.vertex_count = vertex_count.value_i32;
                }
                else if(element_name.type == ply_token_type_face)
                {
                    // It seems like this information is useless?
                }
                else
                {
                    invalid_code_path;
                }


            }break;

            case ply_token_type_property:
            {
                PlyToken type = eat_ply_token(&tokenizer); 
                PlyToken axis = eat_ply_token(&tokenizer); // x or y or z

                result.vertex_property_count++;
            }break;

            case ply_token_type_end_header:
            {
                end_header_appeared = true;
            }break;
        }
    }

    // TODO(gh) hack, probably not a bad idea (but what if .ply file only has two elements?)
    result.vertex_property_count--;

    // NOTE(gh) ply files do not specify how many indices are there, so we need to get them ourselvese
    for(u32 vertex_index = 0;
            vertex_index < result.vertex_count;
            ++vertex_index)
    {
        for(u32 property_index = 0;
                property_index < result.vertex_property_count;
                ++property_index)
        {
            PlyToken token = eat_ply_token(&tokenizer);

            assert(token.type == ply_token_type_f32 ||
                   token.type == ply_token_type_i32);

        }
    }

    while(tokenizer.at < tokenizer.one_past_end && 
            peek_ply_token(tokenizer).type != 0) // For eof
    {
        PlyToken index_count = eat_ply_token(&tokenizer);
        assert(index_count.type == ply_token_type_i32 &&
                index_count.value_i32 >= 3);

        result.index_count += (u32)(3 * (index_count.value_i32 - 2));

        eat_until_newline(&tokenizer);
    }
    // NOTE(gh) now get how many indices do we need

    return result;
}

// NOTE(gh) minimal ply parser, that only parses vertices for now. 
internal void
parse_ply(u8 *memory, u32 file_size, ParsePlyHeaderResult header, f32 *vertices, u32 *indices)
{
    Tokenizer tokenizer = {};
    tokenizer.at = memory;
    tokenizer.one_past_end = memory + file_size;

    while(tokenizer.at < tokenizer.one_past_end)
    {
        PlyToken token = eat_ply_token(&tokenizer);

        if(token.type == ply_token_type_end_header)
        {
            break;
        }
    }

    u32 vertex_index = 0;
    for(u32 i = 0;
            i < header.vertex_count;
            ++i)
    {
        for(u32 vertex_property_index = 0;
                vertex_property_index < header.vertex_property_count;
                ++vertex_property_index)
        {
            PlyToken token = eat_ply_token(&tokenizer);
            assert(token.type == ply_token_type_f32 || 
                    token.type == ply_token_type_i32);

            if(token.is_float)
            {
                vertices[vertex_index] = token.value_f32;
            }
            else
            {
                vertices[vertex_index] = (f32)token.value_i32;
            }

            vertex_index++;
        }

        eat_until_newline(&tokenizer);
    }

    u32 index_index = 0;
    // NOTE(gh) this assumes that the indices will always appear at the last
    while(tokenizer.at < tokenizer.one_past_end && 
            peek_ply_token(tokenizer).type != 0) // for eof
    {
        PlyToken index_count = eat_ply_token(&tokenizer);
        assert(index_count.type == ply_token_type_i32 &&
                index_count.value_i32 >= 3);

        PlyToken index_0 = eat_ply_token(&tokenizer); // will be used as a first index to the strip for this line
        PlyToken index_1 = eat_ply_token(&tokenizer);
        PlyToken index_2 = eat_ply_token(&tokenizer);

        assert(index_0.type == ply_token_type_i32 && 
               index_1.type == ply_token_type_i32 &&
               index_2.type == ply_token_type_i32);

        indices[index_index++] = index_0.value_i32;
        indices[index_index++] = index_1.value_i32;
        indices[index_index++] = index_2.value_i32;

        // starting from 1, as we already parsed the first strip
        for(u32 strip_index = 1;
                strip_index < index_count.value_i32 - 2;
                ++strip_index)
        {
            u32 second_index = indices[index_index-1];
            
            indices[index_index++] = index_0.value_i32;
            indices[index_index++] = second_index;
            indices[index_index++] = eat_ply_token(&tokenizer).value_i32;
        }
    }

    assert(index_index == header.index_count);
}

// NOTE/gh: This function is more like a general purpose token getter, with minimum erro checking.
// the error checking itself will happen inside the parsing loop, not here.
internal ObjToken
eat_obj_token(Tokenizer *tokenizer)
{
    ObjToken result = {};

    eat_all_whitespaces(tokenizer);
    
    if(tokenizer->at < tokenizer->one_past_end)
    {
        b32 hyphen_appeared = false;
        if(*tokenizer->at ==  '-')
        {
            // NOTE(gh) rare case where the case should go on, instead of breaking out
            hyphen_appeared = true;
            tokenizer->at++;
        }

        if(string_compare((char *)tokenizer->at, "v "))
        {
            result.type = obj_token_type_v;
            eat_until_whitespace(tokenizer);
        }
        else if(string_compare((char *)tokenizer->at, "vt "))
        {
            result.type = obj_token_type_vt;
            eat_until_whitespace(tokenizer);
        }
        else if(string_compare((char *)tokenizer->at, "vn "))
        {
            result.type = obj_token_type_vn;
            eat_until_whitespace(tokenizer);
        }
        else if(string_compare((char *)tokenizer->at, "f "))
        {
            result.type = obj_token_type_f;
            eat_until_whitespace(tokenizer);
        }
        else if(string_compare((char *)tokenizer->at, "mtllib "))
        {
            eat_until_newline(tokenizer);
        }
        else if(string_compare((char *)tokenizer->at, "o "))
        {
            eat_until_newline(tokenizer);
        }
        else if(*tokenizer->at == '/')
        {
            result.type = obj_token_type_slash;
            eat(tokenizer, 1);
        }
        else if(*tokenizer->at == '#')
        {
            eat_until_newline(tokenizer);
        }
        else if((*tokenizer->at >= '0') && 
            (*tokenizer->at <= '9'))
        {
            ParseNumericResult parse_result = eat_numeric(tokenizer);

            if(parse_result.is_float)
            {
                result.type = obj_token_type_f32;
                result.value_f32 = parse_result.value_f32;

                if(hyphen_appeared)
                {
                    result.value_f32 *= -1.0f;
                    hyphen_appeared = false;
                }
            }

            else
            {
                result.type = obj_token_type_i32;
                result.value_i32 = parse_result.value_i32;

                if(hyphen_appeared)
                {
                    result.value_i32 *= -1;
                    hyphen_appeared = false;
                }
            }

        }
    }

    return result;
}

internal ObjToken
peek_obj_token(Tokenizer tokenizer)
{
    ObjToken result = eat_obj_token(&tokenizer);

    return result;
}

// TODO(gh): parsing positions and vertex normals work just fine,
// but havent yet tested with the texture coords. Will do when I have a png loader :)
// pre_parse returns how many vertices / normals / indices the user needs to allocate.
internal PreParseObjResult
pre_parse_obj(u8 *file, size_t file_size)
{
    assert(file && file_size > 0);

    PreParseObjResult result = {};

    Tokenizer tokenizer = {};
    tokenizer.at = file;
    tokenizer.one_past_end = file + file_size;

    b32 v_appeared = false;
    b32 vn_appeared = false;
    b32 vt_appeared = false;

    while(tokenizer.at < tokenizer.one_past_end)
    {
        ObjToken token = eat_obj_token(&tokenizer);
        switch(token.type)
        {
            case obj_token_type_v:
            {
                result.position_count++;
                v_appeared = true;
            }break;
            case obj_token_type_vn:
            {
                result.normal_count++;
                vn_appeared = true;
            }break;
            case obj_token_type_vt:
            {
                result.texcoord_count++;
                vt_appeared = true;
            }break;

            case obj_token_type_f:
            {
                u32 number_count = 0;
                while(1)
                {
                    ObjToken t = peek_obj_token(tokenizer);

                    if(t.type == obj_token_type_i32)
                    {
                        number_count++;
                        eat_obj_token(&tokenizer); // actually advance
                    }
                    else if(t.type == obj_token_type_slash)
                    {
                        eat_obj_token(&tokenizer); // actually advance
                    }
                    else
                    {
                        break;
                    }
                }

                assert(v_appeared || vt_appeared || vn_appeared);

                // TODO(gh) What happens if the normal index and the position index were different?
                u32 strip_count = (number_count / (v_appeared + vt_appeared + vn_appeared));
                result.index_count += 3 * (strip_count - 2);
            }break;
        }
    }

    if(v_appeared)
    {
        if(!vt_appeared && !vn_appeared)
        {
            result.vertex_type = obj_vertex_type_v;
        }
        else if(!vt_appeared && vn_appeared)
        {
            result.vertex_type = obj_vertex_type_v_vn;
        }
        else if(vt_appeared && !vn_appeared)
        {
            result.vertex_type = obj_vertex_type_v_vt;
        }
        else if(vt_appeared && vn_appeared)
        {
            result.vertex_type = obj_vertex_type_v_vt_vn;
        }
    }
    else
    {
        invalid_code_path;
    }

    return result;
}

internal void
parse_obj(PreParseObjResult *pre_parse, u8 *file, u32 file_size, 
        v3 *positions, v3 *normals, v2 *texcoords, u32 *indices)
{
    assert(file && file_size > 0);
    assert(pre_parse->vertex_type == obj_vertex_type_v || 
           pre_parse->vertex_type == obj_vertex_type_v_vn);

    Tokenizer tokenizer = {};
    tokenizer.at = file;
    tokenizer.one_past_end = file + file_size;

    u32 position_index = 0;
    u32 normal_index = 0;
    u32 index_index = 0;
    while(tokenizer.at < tokenizer.one_past_end)
    {
        ObjToken token = eat_obj_token(&tokenizer);
        switch(token.type)
        {
            case obj_token_type_v:
            {
                ObjToken p0 = eat_obj_token(&tokenizer);
                ObjToken p1 = eat_obj_token(&tokenizer);
                ObjToken p2 = eat_obj_token(&tokenizer);

                v3 *position = positions + position_index++;

                numeric_obj_token_to_f32(p0, &position->x);
                numeric_obj_token_to_f32(p1, &position->y);
                numeric_obj_token_to_f32(p2, &position->z);
            }break;

            case obj_token_type_vn:
            {
                ObjToken n0 = eat_obj_token(&tokenizer);
                ObjToken n1 = eat_obj_token(&tokenizer);
                ObjToken n2 = eat_obj_token(&tokenizer);

                v3 *normal = normals + normal_index++;

                numeric_obj_token_to_f32(n0, &normal->x);
                numeric_obj_token_to_f32(n1, &normal->y);
                numeric_obj_token_to_f32(n2, &normal->z);
            }break;

            case obj_token_type_vt:
            {
                // TODO(gh): add support for vt
            }break;

            case obj_token_type_f:
            {
                switch(pre_parse->vertex_type)
                {
                    case obj_vertex_type_v:
                    {
                        ObjToken i0 = eat_obj_token(&tokenizer);
                        ObjToken i1 = eat_obj_token(&tokenizer);
                        ObjToken i2 = eat_obj_token(&tokenizer);
                        indices[index_index++] = i0.value_i32;
                        indices[index_index++] = i1.value_i32;
                        indices[index_index++] = i2.value_i32;

                        while(1)
                        {
                            ObjToken t = peek_obj_token(tokenizer);
                            if(t.type == obj_token_type_i32)
                            {
                                indices[index_index++] = i0.value_i32;
                                indices[index_index] = indices[index_index-2];
                                index_index++;
                                indices[index_index++] = t.value_i32;

                                eat_obj_token(&tokenizer);
                            }
                            else
                            {
                                break;
                            }
                        }
                    }break;
                    case obj_vertex_type_v_vn:
                    {
                        ObjToken i0 = eat_obj_token(&tokenizer);
                        ObjToken s0 = eat_obj_token(&tokenizer);
                        ObjToken s1 = eat_obj_token(&tokenizer);
                        ObjToken vni0 = eat_obj_token(&tokenizer);

                        ObjToken i1 = eat_obj_token(&tokenizer);
                        ObjToken s2 = eat_obj_token(&tokenizer);
                        ObjToken s3 = eat_obj_token(&tokenizer);
                        ObjToken vni1 = eat_obj_token(&tokenizer);

                        ObjToken i2 = eat_obj_token(&tokenizer);
                        ObjToken s4 = eat_obj_token(&tokenizer);
                        ObjToken s5 = eat_obj_token(&tokenizer);
                        ObjToken vni2 = eat_obj_token(&tokenizer);

                        indices[index_index++] = i0.value_i32;
                        indices[index_index++] = i1.value_i32;
                        indices[index_index++] = i2.value_i32;

                        //f 304808//304808 304802//304802 304823//304823 

                        u32 number_count;
                        while(1)
                        {
                            ObjToken t = peek_obj_token(tokenizer);
                            if(t.type == obj_token_type_i32)
                            {
                                if((number_count & 2) == 0)
                                {
                                    indices[index_index++] = i0.value_i32;
                                    indices[index_index] = indices[index_index-2];
                                    index_index++;
                                    indices[index_index++] = t.value_i32;
                                }

                                number_count++;

                                eat_obj_token(&tokenizer);
                            }
                            else if(t.type == obj_token_type_slash)
                            {
                                eat_obj_token(&tokenizer);
                            }
                            else
                            {
                                break;
                            }
                        }

                    }break;

                    // TODO(gh) implement these!
                    case obj_vertex_type_v_vt:
                    {
                    }break;
                    case obj_vertex_type_v_vt_vn:
                    {
                    }break;
                }
            }break;
        }
    }
}












