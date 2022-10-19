#include "hb_obj.h"

internal void
eat_all_white_spaces(ObjTokenizer *tokenizer)
{
    // TODO(gyuhyun) Because certain OS uses \n\r as a carriage return,
    // we cannot break this loop when the character is \n
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

// TODO(gyuhyun) This is somewhat special compared to the general peek token,
// which might make people confused :(
internal void
peek_string_token(ObjTokenizer *tokenizer, ObjStringToken *token)
{
    eat_all_white_spaces(tokenizer);

    u8 *c = tokenizer->at;
    while(!(*tokenizer->at == '\n' ||
          *tokenizer->at == '\r') &&
          tokenizer->at < tokenizer->end)
    {
        token->string[token->string_size++] = *tokenizer->at;
        tokenizer->at++;
    }
}

// NOTE(gyuhyun) This function is more like a general purpose token getter, with minimum erro checking.
// the error checking itself will happen inside the parsing loop, not here.
internal ObjToken
peek_token(ObjTokenizer *tokenizer, b32 should_advance)
{
    ObjToken result = {};

    eat_all_white_spaces(tokenizer);
    
    u32 advance = 0;
    if(tokenizer->at < tokenizer->end)
    {
        switch(*tokenizer->at)
        {
            case 'v':
            {
                if(*(tokenizer->at+1) == 't')
                {
                    result.type = ObjTokenType_vt;
                    advance = 2;
                }
                else if(*(tokenizer->at+1) == 'n')
                {
                    result.type = ObjTokenType_vn;
                    advance = 2;
                }
                else
                {
                    result.type = ObjTokenType_v;
                    advance = 1;
                }
            }break;

            case 'f':
            {
                result.type = ObjTokenType_f;
                advance = 1;
            }break;

            case 's':
            {
                result.type = ObjTokenType_s;
                advance = 1;
            }break;

            case '-':
            {
                result.type = ObjTokenType_hyphen;
                advance = 1;
            }break;

            case '/':
            {
                result.type = ObjTokenType_slash;
                advance = 1;
            }break;

            case 'o':
            {
                if(*tokenizer->at == 'o' &&
                    *(tokenizer->at+1) == ' ')
                {
                    result.type = ObjTokenType_o;
                    tokenizer->at = get_closest_carriage_return(tokenizer->at, tokenizer->end);
                }
                else if(*tokenizer->at == 'o' &&
                        *(tokenizer->at+1) == 'f' &&
                        *(tokenizer->at+2) == 'f')
                {
                    result.type = ObjTokenType_off;
                    tokenizer->at = get_closest_carriage_return(tokenizer->at, tokenizer->end);
                }
            }break;

            case 'm':
            {
                if(*tokenizer->at == 'm' &&
                    *(tokenizer->at+1) == 't' && 
                    *(tokenizer->at+2) == 'l' && 
                    *(tokenizer->at+3) == 'l' && 
                    *(tokenizer->at+4) == 'i' && 
                    *(tokenizer->at+5) == 'b')
                {
                    result.type = ObjTokenType_mtllib;
                    advance = 6;
                }
            }break;

            case 'u':
            {
                if(*tokenizer->at == 'u' &&
                    *(tokenizer->at+1) == 's' &&
                    *(tokenizer->at+2) == 'e' &&
                    *(tokenizer->at+3) == 'm' &&
                    *(tokenizer->at+4) == 't' &&
                    *(tokenizer->at+5) == 'l')
                {
                    result.type = ObjTokenType_usemtl;
                    advance = 6;
                }
            }break;

            case 'g':
            {
                // NOTE(gyuhyun) g means 'group', which we don't care for now
                tokenizer->at = get_closest_carriage_return(tokenizer->at, tokenizer->end);
            }

            case '\n':
            {
                result.type = ObjTokenType_newline;
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
                ParseNumericResult parse_result = parse_numeric(tokenizer->at);

                if(parse_result.isFloat)
                {
                    result.type = ObjTokenType_f32;
                    result.value_f32 = parse_result.value_f32;
                }
                else
                {
                    result.type = ObjTokenType_i32;
                    result.value_i32 = parse_result.value_i32;
                }

                if(tokenizer->_previous_token_type == ObjTokenType_hyphen)
                {
                    if(parse_result.isFloat)
                    {
                        result.value_f32 *= -1.0f;
                    }
                    else
                    {
                        result.value_i32 *= -1;
                    }
                }

                advance = parse_result.advance;
            }break;

            case '#':
            {
                tokenizer->at = get_closest_carriage_return(tokenizer->at, tokenizer->end);
            }break;
        }
    }

    if(should_advance)
    {
        tokenizer->at += advance;
    }

    tokenizer->_previous_token_type = result.type;

    return result;
}

internal ObjPreParseResult
pre_parse_obj(u8 *file, size_t file_size)
{
    ObjPreParseResult result = {};

    ObjTokenizer tokenizer = {};
    tokenizer.at = file;
    tokenizer.end = file+file_size;

    b32 is_parsing = true;
   
    while(tokenizer.at < tokenizer.end)
    {
        ObjToken token = peek_token(&tokenizer, true);

        switch(token.type)
        {
            case ObjTokenType_v:
            {
                result.total_position_count++;
            }break;

            case ObjTokenType_vn:
            {
                result.total_normal_count++;
            }break;

            case ObjTokenType_vt:
            {
                result.total_texcoord_count++;
            }break;

            case ObjTokenType_o:
            {
                result.total_object_count++;
            }break;

            case ObjTokenType_mtllib:
            {
                tokenizer.at = get_closest_carriage_return(tokenizer.at, tokenizer.end);
            }break;

            case ObjTokenType_usemtl:
            {
                result.total_mtl_info_count++;
                tokenizer.at = get_closest_carriage_return(tokenizer.at, tokenizer.end);
            }break;

            case ObjTokenType_f:
            {
                b32 newline_appeared = false;

                // NOTE(gyuhyun) f v/vt/vn v/vt/vn ... 
                u32 property_indicator = 0;
                // NOTE(gyuhyun) Math here is : actual_index_count = 3 * (index_count_for_this_line - 2);
                u32 position_index_count_for_this_line = 0;
                u32 normal_index_count_for_this_line = 0;
                u32 texcoord_index_count_for_this_line = 0;
                while(!newline_appeared)
                {
                    // TODO(gyuhyun) The 'safer' way to do this would be
                    // 1. peek token without advancing
                    // 2. advance the tokenizer at the end of the switch statement
                    // This way, we can cover the case where there was two 'f' in a single line
                    token = peek_token(&tokenizer, true);

                    switch(token.type)
                    {
                        case ObjTokenType_i32:
                        {
                            if(property_indicator == 0)
                            {
                                position_index_count_for_this_line++;
                            }
                            else if(property_indicator == 1)
                            {
                                texcoord_index_count_for_this_line++;
                            }
                            else
                            {
                                normal_index_count_for_this_line++;

                                // finished parsing a single vertex, reset the property indicator
                                // to parse the next vertex
                                property_indicator = 0;
                            }
                        }break;

                        case ObjTokenType_slash:
                        {
                            property_indicator++;
                        }break;

                        case ObjTokenType_newline:
                        {
                            newline_appeared = true;
                        }break;

                        default:
                        {
                            // NOTE(gyuhyun) face line should not contain any other letters
                            invalid_code_path;
                        }break;
                    }
                }

                result.total_position_index_count += 3 * (position_index_count_for_this_line - 2);
                result.total_texcoord_index_count += 3 * (texcoord_index_count_for_this_line - 2);
                result.total_normal_index_count += 3 * (normal_index_count_for_this_line - 2);
            }break;
        }
    }

    // obj file should have at least the position data
    assert(result.total_position_count && result.total_position_index_count);

    // NOTE(gyuhyun) It might be possible for an obj file to contain only one object,
    // which means there was no line starting with 'o'
    if(result.total_object_count == 0)
    {
        result.total_object_count = 1;
    }
    return result;
}

internal void
peek_and_store_floating_numbers(ObjTokenizer *tokenizer, u32 desired_number_count, f32 *n)
{
    u32 numeric_number_count = 0;

    b32 is_parsing = true;
    while(is_parsing)
    {
        ObjToken token = peek_token(tokenizer, true);

        switch(token.type)
        {
            case ObjTokenType_f32: 
            {
                *n = token.value_f32;
                numeric_number_count++;
            }break;

            case ObjTokenType_i32: 
            {
                *n = token.value_i32;
                numeric_number_count++;
            }break;

            case ObjTokenType_hyphen:
            {
                // Do nothing, will be handled by the peek_token
            }break;

            default:
            {
                if(numeric_number_count == desired_number_count)
                {
                    is_parsing = false;
                }
                else
                {
                    // Other token appeared before parsing three numeric tokens 
                    invalid_code_path;
                }
            }break; 
        }
    }
}

internal ObjParseMemory
allocate_obj_parse_memory(ObjPreParseResult *pre_parse_result)
{
    ObjParseMemory result = {};

    result.loaded_objects = (LoadedObjObject *)malloc(sizeof(LoadedObjObject) * pre_parse_result->total_object_count);

    if(pre_parse_result->total_position_count)
    {
        result.positions = (v3 *)malloc(sizeof(v3) * pre_parse_result->total_position_count);
    }

    if(pre_parse_result->total_normal_count)
    {
        result.normals = (v3 *)malloc(sizeof(v3) * pre_parse_result->total_normal_count);
    }

    if(pre_parse_result->total_texcoord_count)
    {
        result.texcoords = (v2 *)malloc(sizeof(v2) * pre_parse_result->total_texcoord_count);
    }

    if(pre_parse_result->total_position_index_count)
    {
        result.position_indices = (u32 *)malloc(sizeof(u32) * pre_parse_result->total_position_index_count);
    }

    if(pre_parse_result->total_normal_index_count)
    {
        result.normal_indices = (u32 *)malloc(sizeof(u32) * pre_parse_result->total_normal_index_count);
    }

    if(pre_parse_result->total_texcoord_index_count)
    {
        result.texcoord_indices = (u32 *)malloc(sizeof(u32) * pre_parse_result->total_texcoord_index_count);
    }

    return result;
}

internal void
parse_obj(ObjParseMemory *memory, ObjPreParseResult *pre_parse_result, u8 *file, u32 file_size)
{
    // NOTE(gyuhyun) Validations
    assert(memory->positions && memory->position_indices);
    if(pre_parse_result->total_normal_count)
    {
        assert(memory->normals);
    }
    if(pre_parse_result->total_texcoord_count)
    {
        assert(memory->texcoords);
    }
    if(pre_parse_result->total_normal_index_count)
    {
        assert(memory->normal_indices);
    }
    if(pre_parse_result->total_texcoord_index_count)
    {
        assert(memory->texcoord_indices);
    }

    ObjTokenizer tokenizer = {};
    tokenizer.at = file;
    tokenizer.end = file+file_size;

    u32 used_mtl_info_count = 0;
    u32 used_position_count = 0;
    u32 used_normal_count = 0;
    u32 used_texcoord_count = 0;

    u32 used_position_index_count = 0;
    u32 used_normal_index_count = 0;
    u32 used_texcoord_index_count = 0;

    for(u32 object_index = 0;
            object_index < pre_parse_result->total_object_count && tokenizer.at != tokenizer.end;
            )
    {
        LoadedObjObject *object = memory->loaded_objects + object_index;

        // NOTE(gyuhyun) Initialize the pointers
        object->positions = memory->positions + used_position_count;
        object->normals = memory->normals + used_normal_count;
        object->texcoords = memory->texcoords + used_texcoord_count;

        object->position_indices = memory->position_indices + used_position_index_count;
        object->normal_indices = memory->normal_indices + used_normal_index_count;
        object->texcoord_indices = memory->texcoord_indices + used_texcoord_index_count;

        b32 is_parsing_this_object = true;
        while(tokenizer.at < tokenizer.end && is_parsing_this_object)
        {
            ObjToken token = peek_token(&tokenizer, true);

            switch(token.type)
            {
                case ObjTokenType_o:
                {
                    object_index++;
                }break;

                case ObjTokenType_v:
                {
                    // feed up to three numeric tokens(including minus)
                    peek_and_store_floating_numbers(&tokenizer, 3, object->positions[object->position_count++].e);
                }break;

                case ObjTokenType_vn:
                {
                     peek_and_store_floating_numbers(&tokenizer, 3, object->normals[object->normal_count++].e);
                }break;

                case ObjTokenType_vt:
                {
                    // TODO(joon) feed up to two numeric tokens(not including minus)
                     peek_and_store_floating_numbers(&tokenizer, 2, object->texcoords[object->texcoord_count++].e);
                }break;

                case ObjTokenType_f:
                {
                    b32 newline_appeared = false;

                    // NOTE : property_indicator%3 == 0(v), 1(vt), 2(vn),
                    u32 property_indicator = 0;

                    // TODO(gyuhyun) This is just a copy paste of what we were doing before...
                    // which works fine for now, but we might wanna find a cleanr way to do this later
                    u32 first_position_index = 0;
                    u32 position_index_count_this_line = 0;

                    u32 first_texcoord_index = 0;
                    u32 texcoord_index_count_this_line = 0;

                    u32 first_normal_index = 0;
                    u32 normal_index_count_this_line = 0;
                    
                    while(!newline_appeared)
                    {
                        token = peek_token(&tokenizer, true);

                        switch(token.type)
                        {
                            case ObjTokenType_i32:
                            {
                                u32 index_to_store = token.value_i32 - 1;

                                if(property_indicator == 0)
                                {
                                    if(position_index_count_this_line < 3)
                                    {
                                        if(position_index_count_this_line == 0)
                                        {
                                            first_position_index = index_to_store;
                                        }
                                        object->position_indices[object->position_index_count++] = index_to_store;

                                        position_index_count_this_line++;
                                    }
                                    else
                                    {
                                        u32 previous_index = object->position_indices[object->position_index_count-1];

                                        object->position_indices[object->position_index_count++] = first_position_index;
                                        object->position_indices[object->position_index_count++] = previous_index;
                                        object->position_indices[object->position_index_count++] = index_to_store;
                                    }
                                }
                                else if(property_indicator == 1)
                                {
                                    if(texcoord_index_count_this_line < 3)
                                    {
                                        if(texcoord_index_count_this_line == 0)
                                        {
                                            first_texcoord_index = index_to_store;
                                        }
                                        object->texcoord_indices[object->texcoord_index_count++] = index_to_store;

                                        texcoord_index_count_this_line++;
                                    }
                                    else
                                    {
                                        u32 previous_index = object->texcoord_indices[object->texcoord_index_count-1];

                                        object->texcoord_indices[object->texcoord_index_count++] = first_texcoord_index;
                                        object->texcoord_indices[object->texcoord_index_count++] = previous_index;
                                        object->texcoord_indices[object->texcoord_index_count++] = index_to_store;
                                    }
                                }
                                else
                                {
                                    if(normal_index_count_this_line < 3)
                                    {
                                        if(normal_index_count_this_line == 0)
                                        {
                                            first_normal_index = index_to_store;
                                        }
                                        object->normal_indices[object->normal_index_count++] = index_to_store;

                                        normal_index_count_this_line++;
                                    }
                                    else
                                    {
                                        u32 previous_index = object->normal_indices[object->normal_index_count-1];

                                        object->normal_indices[object->normal_index_count++] = first_normal_index;
                                        object->normal_indices[object->normal_index_count++] = previous_index;
                                        object->normal_indices[object->normal_index_count++] = index_to_store;
                                    }

                                    property_indicator = 0;
                                }
                            }break;

                            case ObjTokenType_slash:
                            {
                                property_indicator++;
                            }break;

                            case ObjTokenType_newline:
                            {
                                newline_appeared = true;
                            }break;

                            default:
                            {
                                invalid_code_path;
                            }break; 
                        }
                    }
                }break;

                case ObjTokenType_mtllib:
                {
                    ObjStringToken string_token = {};
                    peek_string_token(&tokenizer, &string_token);
                }break;

                case ObjTokenType_usemtl:
                {
                    if(!object->mtl_infos)
                    {
                        object->mtl_infos = memory->mtl_infos + used_mtl_info_count;
                    }

                    ObjMtlInfo *mtl_info = object->mtl_infos + object->mtl_info_count++;

                    ObjStringToken string_token = {};
                    peek_string_token(&tokenizer, &string_token);

                    // TODO(gyuhyun) We first need to do the pre-parsing & parsing routine for the .mtl files,
                    // and then populate the object's mtl infos 
                }break;

                case ObjTokenType_o:
                {
                    is_parsing_this_object = false;
                }break;

                case ObjTokenType_null:
                {
                    if(tokenizer.at != tokenizer.end)
                    {
                        invalid_code_path;
                    }
                }break;
            }
        }

        // NOTE(gyuhyun) advance how much we used inside the memory pool
        used_position_count += object->position_count;
        used_normal_count += object->normal_count;
        used_texcoord_count += object->texcoord_count;

        used_position_index_count += object->position_index_count;
        used_normal_index_count += object->normal_index_count;
        used_texcoord_index_count += object->texcoord_index_count;

        used_mtl_info_count += object->mtl_info_count;
    }

    assert(used_position_count == pre_parse_result->total_position_count);
    assert(used_normal_count == pre_parse_result->total_normal_count);
    assert(used_texcoord_count == pre_parse_result->total_texcoord_count);
    assert(used_position_index_count == pre_parse_result->total_position_index_count);
    assert(used_normal_index_count == pre_parse_result->total_normal_index_count);
    assert(used_texcoord_index_count == pre_parse_result->total_texcoord_index_count);
}

internal void
eat_all_white_spaces(MtlTokenizer *tokenizer)
{
    while((*tokenizer->at == '\n' ||
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

inline u32
string_size(const char *string)
{
    u32 result = 0;
    while(*string != '\0')
    {
        string++;
        result++;
    }

    return result;
}

internal MtlToken
peek_token(MtlTokenizer *tokenizer)
{
    MtlToken result = {};

    eat_all_white_spaces(tokenizer);
    
    if(tokenizer->at < tokenizer->end)
    {
        // newmtl
        if(*tokenizer->at == 'n' &&
            *(tokenizer->at + 1) == 'e' &&
            *(tokenizer->at + 2) == 'w' &&
            *(tokenizer->at + 3) == 'm' &&
            *(tokenizer->at + 4) == 't' &&
            *(tokenizer->at + 5) == 'l')
        {
            result.type = MtlTokenType_newmtl;
            tokenizer->at += string_size("newmtl");
        }

        else if(*tokenizer->at == 'K' &&
                *(tokenizer->at + 1) == 'd')
        {
            result.type = MtlTokenType_Kd;
            tokenizer->at += string_size("Kd");
        }

        else if(*tokenizer->at == 'K' &&
                *(tokenizer->at + 1) == 'e')
        {
            result.type = MtlTokenType_Ke;
            tokenizer->at += string_size("Ke");
        }

        else if(*tokenizer->at == 'N' &&
                *(tokenizer->at + 1) == 's')
        {
            result.type = MtlTokenType_Ns;
            tokenizer->at += string_size("Ns");
        }

        else if(*tokenizer->at == 'd')
        {
            result.type = MtlTokenType_d;
            tokenizer->at += string_size("d");
        }

        else if(*tokenizer->at == 'i' &&
                *(tokenizer->at + 1) == 'l' &&
                *(tokenizer->at + 2) == 'l' &&
                *(tokenizer->at + 3) == 'u' &&
                *(tokenizer->at + 4) == 'm')
        {
            result.type = MtlTokenType_illum;
            tokenizer->at += string_size("illum");
        }

        else if(*tokenizer->at == 'K' &&
                *(tokenizer->at + 1) == 'a')
        {
            result.type = MtlTokenType_Ka;
            tokenizer->at += string_size("Ka");
        }

        else if(*tokenizer->at == 'K' &&
                *(tokenizer->at + 1) == 's')
        {
            result.type = MtlTokenType_Ks;
            tokenizer->at += string_size("Ks");
        }

        else if(*tokenizer->at == 'N' &&
                *(tokenizer->at + 1) == 'i')
        {
            result.type = MtlTokenType_Ni;
            tokenizer->at += string_size("Ni");
        }

        else if(*tokenizer->at == 'm' &&
                *(tokenizer->at + 1) == 'a' &&
                *(tokenizer->at + 2) == 'p' &&
                *(tokenizer->at + 3) == '_' &&
                *(tokenizer->at + 4) == 'K' &&
                *(tokenizer->at + 5) == 'd')
        {
            result.type = MtlTokenType_map_Kd;
            tokenizer->at += string_size("map_Kd");
            tokenizer->at = get_closest_carriage_return(tokenizer->at, tokenizer->end);
        }

        else if(*tokenizer->at == '0' || 
                *tokenizer->at == '1' || 
                *tokenizer->at == '2' || 
                *tokenizer->at == '3' || 
                *tokenizer->at == '4' || 
                *tokenizer->at == '5' || 
                *tokenizer->at == '6' || 
                *tokenizer->at == '7' || 
                *tokenizer->at == '8' || 
                *tokenizer->at == '9')
        {
            ParseNumericResult parse_numeric_result = parse_numeric(tokenizer->at);

            if(parse_numeric_result.isFloat)
            {
                result.type = MtlTokenType_f32;
                result.value_f32 = parse_numeric_result.value_f32;
            }
            else
            {
                result.type = MtlTokenType_i32;
                result.value_i32 = parse_numeric_result.value_i32;
            }

            if(tokenizer->_previous_token_type == MtlTokenType_hyphen)
            {
                if(parse_numeric_result.isFloat)
                {
                    result.value_f32 *= -1.0f;
                }
                else
                {
                    result.value_i32 *= -1;
                }
            }

            tokenizer->at += parse_numeric_result.advance;
        }

        else
        {
            invalid_code_path;
        }
    }

    tokenizer->_previous_token_type = result.type;

    return result;
}


internal MtlPreParseResult
pre_parse_mtl(u8 *file, size_t file_size)
{
    MtlPreParseResult result = {};

    MtlTokenizer tokenizer = {};
    tokenizer.at = file;
    tokenizer.end = file+file_size;

    while(tokenizer.at < tokenizer.end)
    {
        MtlToken token = peek_token(&tokenizer);

        switch(token.type)
        {
            case MtlTokenType_newmtl:
            {
                result.total_mtl_count++;
                tokenizer.at = get_closest_carriage_return(tokenizer.at, tokenizer.end);
            }break;
        }
    }

    return result;
}












