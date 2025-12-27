#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <gmp.h>

#include "lexer.h"

typedef struct
{
    enum
    {
        CELL_NUM,
        CELL_STR,
    } type;
    union
    {
        dc_num_t num;
        char *str;
    } as;
} cell;

cell
cell_strv (dc_strv_t strv)
{
    return (cell){
        .type = CELL_STR,
        .as.str = strndup (strv.ptr, strv.len),
    };
}

cell
cell_string (const char *str)
{
    return (cell){
        .type = CELL_STR,
        .as.str = strdup (str),
    };
}

cell
cell_number (dc_num_t num)
{
    cell r = (cell){
        .type = CELL_NUM,
    };
    mpf_init_set (r.as.num, num);
    return r;
}

void
cell_disp (FILE *f, const cell *c)
{
    switch (c->type)
    {
    case CELL_STR: fprintf (f, "%s", c->as.str); break;
    case CELL_NUM: mpf_out_str (f, 10, 0, c->as.num); break;
    }
}

void
cell_clear (cell *c)
{
    if (c->type == CELL_STR)
    {
        free (c->as.str);
        c->as.str = NULL;
    }
    else if (c->type == CELL_NUM) { mpf_clear (c->as.num); }
    c->type = CELL_STR;
    c->as.str = NULL;
}

typedef struct
{
    cell *data;
    size_t top, cap;
} stack;
#define INITIAL_STACK_SIZE 64

void
push (stack *s, cell c)
{
    if (s->cap <= s->top)
    {
        if (s->cap == 0)
            s->cap = INITIAL_STACK_SIZE;
        else
            s->cap *= 2;

        s->data = realloc (s->data, s->cap * sizeof *s->data);
        assert (s->data != NULL);
    }

    s->data[s->top++] = c;
}

bool
popn (stack *s, cell *c, size_t n)
{
    if (s->top < n) return false;
    for (size_t i = 0; i < n; ++i) { c[i] = s->data[s->top - i - 1]; }
    s->top -= n;
    return true;
}

const cell *
top (stack *s)
{
    if (s->top == 0)
    {
        fprintf (stderr, "dc/top: stack is empty\n");
        return NULL;
    }

    return &s->data[s->top - 1];
}

void
stack_free (stack *s)
{
    for (size_t i = 0; i < s->top; ++i) cell_clear (&s->data[i]);
    free (s->data);
    s->data = NULL;
    s->top = s->cap = 0;
}

void
dump (const stack *s)
{
    printf ("[%lu]\n", s->top);
    for (size_t i = 0; i < s->top; ++i)
    {
        const cell *c = &s->data[i];
        printf ("<");
        switch (c->type)
        {
        case CELL_STR: printf ("STR"); break;
        case CELL_NUM: printf ("NUM"); break;
        }
        printf ("> ");
        cell_disp (stdout, c);
        printf ("\n");
    }
}

typedef struct
{
    stack dstack;
} execution_ctx;

typedef void (*command_cb) (execution_ctx *, void *);
static command_cb dispatch_table[256] = { 0 };

void
dispatch (execution_ctx *ctx, int cmd, void *userdata)
{
    if (dispatch_table[cmd] == NULL)
    {
        fprintf (stderr, "dc/dispatch(%c): command not implemented\n", cmd);
        return;
    }
    dispatch_table[cmd](ctx, userdata);
}

void
register_callback (int cmd, command_cb cb)
{
    dispatch_table[cmd] = cb;
}

void
dispatch_plus (execution_ctx *ctx, void *data)
{
    (void)data;

    cell c[2];
    if (!popn (&ctx->dstack, c, 2)) return;

    if (c[0].type != c[1].type)
    {
        fprintf (stderr, "dc/dispatch_plus: can't add string to a number\n");
        return;
    }

    switch (c[0].type)
    {
    case CELL_NUM: {
        mpf_t r;
        mpf_init (r);

        mpf_add (r, c[0].as.num, c[1].as.num);

        cell_clear (c);
        cell_clear (c + 1);

        push (&ctx->dstack, cell_number (r));

        mpf_clear (r);
        break;
    }
    case CELL_STR: {
        size_t newlen = strlen (c[0].as.str) + strlen (c[1].as.str);
        char *new = malloc (newlen + 1);
        assert (new != NULL);

        sprintf (new, "%s%s", c[1].as.str, c[0].as.str);
        new[newlen] = 0;

        cell_clear (c);
        cell_clear (c + 1);

        push (&ctx->dstack, cell_string (new));

        free (new);
        break;
    }
    }
}

void
dispatch_print (execution_ctx *ctx, void *data)
{
    (void)data;
    const cell *c = top (&ctx->dstack);
    if (c == NULL) return;
    cell_disp (stdout, c);
    puts ("");
}

int
main (void)
{
    register_callback ('+', dispatch_plus);
    register_callback ('p', dispatch_print);

    execution_ctx ctx = { 0 };

    mpf_t n;

    mpf_init_set_ui (n, 34);
    push (&ctx.dstack, cell_number (n));

    mpf_set_ui (n, 35);
    push (&ctx.dstack, cell_number (n));

    dispatch (&ctx, '+', NULL);
    dispatch (&ctx, 'p', NULL);

    push (&ctx.dstack, cell_string ("Hatsune"));
    push (&ctx.dstack, cell_string ("Miku"));

    dump (&ctx.dstack);

    dispatch (&ctx, '+', NULL);
    dispatch (&ctx, 'p', NULL);

    mpf_clear (n);
    stack_free (&ctx.dstack);

    // const char *test = "2 2 + [Hatsune Miku :3] 69.420 xD";

    // lexer l = {
    //     .idx = 0,
    //     .src = test,
    //     .len = strlen (test),
    // };
    // token t;

    // do
    // {
    //     t = token_next (&l);
    //     switch (t.kind)
    //     {
    //     case TOKEN_CMD: printf ("<CMD> [%c]\n", t.as.cmd); break;
    //     case TOKEN_NUM:
    //         printf ("<NUM> [");
    //         mpf_out_str (stdout, 10, 0, t.as.num);
    //         printf ("]\n");
    //         break;
    //     case TOKEN_STR: printf ("<STR> [%.*s]\n", (int)t.as.str.len, t.as.str.ptr); break;
    //     case TOKEN_EOF: break;
    //     }
    // } while (t.kind != TOKEN_EOF);

    return 0;
}
