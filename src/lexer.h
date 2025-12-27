#ifndef LEXER_H
#define LEXER_H

#include <gmp.h>
#include <stdlib.h>

typedef mpf_t dc_num_t;
typedef struct
{
    const char *ptr;
    size_t len;
} dc_strv_t;

typedef struct
{
    enum
    {
        TOKEN_NUM,
        TOKEN_STR,
        TOKEN_CMD,
        TOKEN_EOF
    } kind;
    union
    {
        dc_num_t num;
        dc_strv_t str;
        int cmd;
    } as;
} token;

typedef struct
{
    const char *src;
    size_t idx, len;
} lexer;

token token_next (lexer *l);
void token_clear (token *t);

#endif
