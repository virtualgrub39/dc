#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <gmp.h>

#include "dc.h"
#include "lexer.h"

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
cell_display (FILE *f, const cell *c)
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
peek (stack *s)
{
    if (s->top == 0) return NULL;
    return &s->data[s->top - 1];
}

void
stack_free (stack *s)
{
    if (s->data == NULL) goto reset;
    for (size_t i = 0; i < s->top; ++i) cell_clear (&s->data[i]);
    free (s->data);
reset:
    s->data = NULL;
    s->top = s->cap = 0;
}

void
stack_dump (const stack *s)
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
        cell_display (stdout, c);
        printf ("\n");
    }
}

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
execute_expr (execution_ctx *ctx, const char *expr)
{
    lexer l = {
        .idx = 0,
        .src = expr,
        .len = strlen (expr),
    };
    token t = { .kind = TOKEN_EOF };

    do
    {
        token_clear (&t);
        t = token_next (&l);

        switch (t.kind)
        {
        case TOKEN_CMD: dispatch (ctx, t.as.cmd, NULL); break;
        case TOKEN_STR: push (&ctx->dstack, cell_strv (t.as.str)); break;
        case TOKEN_NUM: push (&ctx->dstack, cell_number (t.as.num));
        case TOKEN_EOF: break;
        }
    } while (t.kind != TOKEN_EOF);
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
        goto cleanup;
    }

    switch (c[0].type)
    {
    case CELL_NUM: {
        mpf_t r;
        mpf_init (r);

        mpf_add (r, c[0].as.num, c[1].as.num);

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

        push (&ctx->dstack, cell_string (new));

        free (new);
        break;
    }
    }

cleanup:
    cell_clear (c);
    cell_clear (c + 1);
}

void
dispatch_print (execution_ctx *ctx, void *data)
{
    (void)data;
    const cell *c = peek (&ctx->dstack);
    if (c == NULL)
    {
        fprintf (stderr, "dc/dispatch_print: stack is empty\n");
        return;
    }
    cell_display (stdout, c);
    puts ("");
}

void
register_defaults (void)
{
    register_callback ('+', dispatch_plus);
    register_callback ('p', dispatch_print);
}

void
execution_done (execution_ctx *ctx)
{
    stack_free (&ctx->dstack);
}
