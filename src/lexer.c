#include "dc.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static size_t
float_prefix_len (const char *s)
{
    const char *p = s;
    if (*p == '+' || *p == '-') p++;

    const char *int_start = p;
    while (*p && isdigit ((unsigned char)*p)) p++;
    int digits_before = (int)(p - int_start);

    int digits_after = 0;
    if (*p == '.')
    {
        p++;
        const char *frac_start = p;
        while (isdigit ((unsigned char)*p)) p++;
        digits_after = (int)(p - frac_start);
    }

    if (digits_before + digits_after == 0) return 0;

    if (*p == 'e' || *p == 'E')
    {
        const char *exp_pos = p;
        const char *q = p + 1;
        if (*q == '+' || *q == '-') q++;
        const char *exp_start = q;
        while (isdigit ((unsigned char)*q)) q++;
        if (q == exp_start) { return (size_t)(exp_pos - s); }
        else
        {
            p = q;
        }
    }

    return (size_t)(p - s);
}

static bool
lex_number (lexer *l, token *t)
{
    size_t matched_len = float_prefix_len (l->src + l->idx);
    if (matched_len == 0) return false;

    assert (matched_len + l->idx <= l->len);

    char *acc = malloc (matched_len + 1);
    if (!acc) return false;
    memcpy (acc, l->src + l->idx, matched_len);
    acc[matched_len] = '\0';

    mpf_init (t->as.num);
    if (mpf_set_str (t->as.num, acc, 10) != 0)
    {
        mpf_clear (t->as.num);
        free (acc);
        return false;
    }

    free (acc);

    l->idx += matched_len;

    t->kind = TOKEN_NUM;
    return true;
}

token
token_next (lexer *l)
{
    if (l->idx == l->len) return (token){ .kind = TOKEN_EOF };
    while (l->src[l->idx] && isspace (l->src[l->idx])) l->idx += 1;

    if (l->src[l->idx] == '\0') return (token){ .kind = TOKEN_EOF };

    char c = l->src[l->idx];
    token r;

    if (lex_number (l, &r))
        return r;
    else if (c == '[')
    {
        size_t start = l->idx + 1;
        size_t i = start;
        while (i < l->len && l->src[i] != ']') i++;

        r.kind = TOKEN_STR;
        r.as.str.ptr = l->src + start;
        r.as.str.len = (i > start) ? (i - start) : 0;

        l->idx = (i < l->len && l->src[i] == ']') ? (i + 1) : i;
        return r;
    }
    else
    {
        r.kind = TOKEN_CMD;
        r.as.cmd = c;
        l->idx += 1;
        return r;
    }
}

void
token_clear (token *t)
{
    switch (t->kind)
    {
    case TOKEN_NUM: mpf_clear (t->as.num); break;
    default: t->kind = TOKEN_EOF;
    }
}

int
reg_next (lexer *l)
{
    if (l->idx == l->len) return -1;
    while (l->src[l->idx] && isspace (l->src[l->idx])) l->idx += 1;
    return l->src[l->idx++];
}
