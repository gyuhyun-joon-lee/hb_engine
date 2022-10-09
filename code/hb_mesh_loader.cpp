/*
 * Written by Gyuhyun Lee
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

    // NOTE(joon) not supported or ignored
    obj_token_type_s,
    obj_token_type_off,
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
    
    // NOTE(joon) Instead of chekcing only a single character, check a string instead!
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

            case 's':
            {
                result.type = obj_token_type_s;
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


