#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gmp.h>

#include "dc.h"

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
cell_number (const dc_num_t num)
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

cell
cell_clone (const cell *c)
{
    switch (c->type)
    {
    case CELL_NUM: return cell_number (c->as.num);
    case CELL_STR: return cell_string (c->as.str);
    }

    fprintf (stderr, "dc/cell_clone: UNREACHABLE\n");
    abort ();
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

    for (size_t i = 0; i < n; ++i)
    {
        s->top -= 1;
        c[i] = s->data[s->top];
        s->data[s->top].type = CELL_STR;
        s->data[s->top].as.str = NULL;
    }
    return true;
}

const cell *
peek (stack *s)
{
    if (s->top == 0) return NULL;
    return &s->data[s->top - 1];
}

void
stack_set (stack *s, cell c)
{
    if (s->top == 0)
        push (s, c);
    else
    {
        cell_clear (&s->data[s->top]);
        s->data[s->top] = c;
    }
}

void
reg_store (execution_ctx *ctx, int reg, cell v)
{
    if (reg < 0 || reg >= REG_COUNT)
    {
        fprintf (stderr, "dc/reg_store: invalid register identifier\n");
        return;
    }
    stack_set (&ctx->r[reg], v);
}

void
reg_push (execution_ctx *ctx, int reg, cell v)
{
    if (reg < 0 || reg >= REG_COUNT)
    {
        fprintf (stderr, "dc/reg_push: invalid register identifier\n");
        return;
    }
    push (&ctx->r[reg], v);
}

bool
reg_load (execution_ctx *ctx, int reg, cell *c)
{
    if (reg < 0 || reg >= REG_COUNT)
    {
        fprintf (stderr, "dc/reg_load: invalid register identifier\n");
        return false;
    }
    const cell *temp = peek (&ctx->r[reg]);
    if (temp == NULL) return false;

    *c = cell_clone (temp);
    return true;
}
bool
reg_pop (execution_ctx *ctx, int reg, cell *c)
{
    if (reg < 0 || reg >= REG_COUNT)
    {
        fprintf (stderr, "dc/reg_pop: invalid register identifier\n");
        return false;
    }
    return popn (&ctx->r[reg], c, 1);
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
        const char *cmd_fmt = isprint (cmd) ? "%c" : "0x%02X";
        fprintf (stderr, "dc/dispatch(");
        fprintf (stderr, cmd_fmt, cmd);
        fprintf (stderr, "): command not implemented\n");
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
    ctx->l = (lexer){
        .idx = 0,
        .src = expr,
        .len = strlen (expr),
    };
    token t = { .kind = TOKEN_EOF };

    int level = ctx->exec_level++;

    do
    {
        token_clear (&t);
        t = token_next (&ctx->l);

        switch (t.kind)
        {
        case TOKEN_CMD: dispatch (ctx, t.as.cmd, NULL); break;
        case TOKEN_STR: push (&ctx->dstack, cell_strv (t.as.str)); break;
        case TOKEN_NUM: push (&ctx->dstack, cell_number (t.as.num)); break;
        case TOKEN_EOF:
            break;

            // case TOKEN_CMD: printf ("<CMD> [%c]\n", t.as.cmd); break;
            // case TOKEN_NUM:
            //     printf ("<NUM> [");
            //     mpf_out_str (stdout, 10, 0, t.as.num);
            //     printf ("]\n");
            //     break;
            // case TOKEN_STR: printf ("<STR> [%.*s]\n", (int)t.as.str.len, t.as.str.ptr); break;
            // case TOKEN_EOF: break;
        }

        if (ctx->exec_level <= level) break;

    } while (t.kind != TOKEN_EOF);

    ctx->exec_level--;
}

void
dispatch_add (execution_ctx *ctx, void *data)
{
    (void)data;

    cell c[2];
    if (!popn (&ctx->dstack, c, 2)) return;

    if (c[0].type != c[1].type)
    {
        fprintf (stderr, "dc/dispatch_add: can't add string to a number\n");
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
dispatch_quit (execution_ctx *ctx, void *data) // "q"
{
    (void)data;
    if (ctx->exec_level > 0) ctx->exec_level--;
}
void
dispatch_clear (execution_ctx *ctx, void *data)
{
    (void)data;
    ctx->dstack.top = 0;
}

void
dispatch_print_f (execution_ctx *ctx, void *data)
{
    (void)data;
    stack_dump (&ctx->dstack);
}

void
dispatch_sub (execution_ctx *ctx, void *data)
{
    (void)data;

    cell c[2];
    if (!popn (&ctx->dstack, c, 2)) return;

    if (c[0].type != c[1].type || c[0].type != CELL_NUM)
    {
        fprintf (stderr, "dc/dispatch_sub: can't subtract a string\n");
        goto cleanup;
    }

    mpf_t r;
    mpf_init (r);

    mpf_sub (r, c[1].as.num, c[0].as.num);

    push (&ctx->dstack, cell_number (r));

    mpf_clear (r);

cleanup:
    cell_clear (c);
    cell_clear (c + 1);
}

void
dispatch_mul (execution_ctx *ctx, void *data)
{
    (void)data;

    cell c[2];
    if (!popn (&ctx->dstack, c, 2)) return;

    if (c[0].type != c[1].type || c[0].type != CELL_NUM)
    {
        fprintf (stderr, "dc/dispatch_mul: can't multiply with a string\n");
        goto cleanup;
    }

    mpf_t r;
    mpf_init (r);

    mpf_mul (r, c[1].as.num, c[0].as.num);

    push (&ctx->dstack, cell_number (r));

    mpf_clear (r);

cleanup:
    cell_clear (c);
    cell_clear (c + 1);
}

void
dispatch_div (execution_ctx *ctx, void *data)
{
    (void)data;

    cell c[2];
    if (!popn (&ctx->dstack, c, 2)) return;

    if (c[0].type != c[1].type || c[0].type != CELL_NUM)
    {
        fprintf (stderr, "dc/dispatch_div: can't divide with a string\n");
        goto cleanup;
    }

    mpf_t r;
    mpf_init (r);

    mpf_div (r, c[1].as.num, c[0].as.num);

    push (&ctx->dstack, cell_number (r));

    mpf_clear (r);

cleanup:
    cell_clear (c);
    cell_clear (c + 1);
}

static void
mpf_rem (mpf_t result, mpf_t a, mpf_t b)
{
    mpf_t t1, t2;
    mpf_init_set (t1, a);
    mpf_init (t2);

    for (;;)
    {
        mpf_sub (t2, t1, b);
        if (mpf_cmp_ui (t2, 0) < 0) // t2 < 0
        {
            mpf_set (result, t1);
            mpf_clear (t1);
            mpf_clear (t2);
            return;
        }
        mpf_set (t1, t2);
    }
}

void
dispatch_rem (execution_ctx *ctx, void *data)
{
    (void)data;

    cell c[2];
    if (!popn (&ctx->dstack, c, 2)) return;

    if (c[0].type != c[1].type || c[0].type != CELL_NUM)
    {
        fprintf (stderr, "dc/dispatch_rem: can't divide with a string\n");
        goto cleanup;
    }

    mpf_t r;
    mpf_init (r);

    mpf_rem (r, c[1].as.num, c[0].as.num);

    push (&ctx->dstack, cell_number (r));

    mpf_clear (r);

cleanup:
    cell_clear (c);
    cell_clear (c + 1);
}

void
dispatch_exp (execution_ctx *ctx, void *data)
{
    (void)data;

    cell c[2];
    if (!popn (&ctx->dstack, c, 2)) return;

    if (c[0].type != c[1].type || c[0].type != CELL_NUM)
    {
        fprintf (stderr, "dc/dispatch_exp: can't exponentiate with a string\n");
        goto cleanup;
    }

    mpf_t r;
    mpf_init (r);

    mpf_pow_ui (r, c[1].as.num, mpf_get_ui (c[0].as.num));

    push (&ctx->dstack, cell_number (r));

    mpf_clear (r);

cleanup:
    cell_clear (c);
    cell_clear (c + 1);
}

void
dispatch_sqrt (execution_ctx *ctx, void *data)
{
    (void)data;

    cell c;
    if (!popn (&ctx->dstack, &c, 1)) return;

    if (c.type != CELL_NUM)
    {
        fprintf (stderr, "dc/dispatch_sqrt: can't sqrt a string\n");
        goto cleanup;
    }

    mpf_t r;
    mpf_init (r);

    mpf_sqrt (r, c.as.num);

    push (&ctx->dstack, cell_number (r));

    mpf_clear (r);

cleanup:
    cell_clear (&c);
}

void
dispatch_prec (execution_ctx *ctx, void *data)
{
    (void)data;

    cell c;
    if (!popn (&ctx->dstack, &c, 1)) return;

    if (c.type != CELL_NUM)
    {
        fprintf (stderr, "dc/dispatch_prec: expected number on top of the stack\n");
        goto cleanup;
    }

    size_t prec_d = mpf_get_ui (c.as.num);
    double prec_b = log2 (10) * prec_d;
    mpf_set_default_prec (ceil (prec_b));

cleanup:
    cell_clear (&c);
}

void
dispatch_dup (execution_ctx *ctx, void *data)
{
    (void)data;

    const cell *c = peek (&ctx->dstack);
    if (!c)
    {
        fprintf (stderr, "dc/dispatch_dup: stack is empty\n");
        return;
    }

    push (&ctx->dstack, cell_clone (c));
}

void
dispatch_rstore (execution_ctx *ctx, void *data)
{
    (void)data;

    int reg = reg_next (&ctx->l);
    if (reg < 0)
    {
        fprintf (stderr, "dc/dispatch_rstore: expected register identifier\n");
        return;
    }

    cell c;
    if (!popn (&ctx->dstack, &c, 1)) return;

    reg_store (ctx, reg, c);
}

void
dispatch_rload (execution_ctx *ctx, void *data)
{
    (void)data;

    int reg = reg_next (&ctx->l);
    if (reg < 0)
    {
        fprintf (stderr, "dc/dispatch_rload: expected register identifier\n");
        return;
    }

    cell c;
    if (!reg_load (ctx, reg, &c)) return;

    push (&ctx->dstack, c);
}

void
dispatch_rpush (execution_ctx *ctx, void *data)
{
    (void)data;

    int reg = reg_next (&ctx->l);
    if (reg < 0)
    {
        fprintf (stderr, "dc/dispatch_rpush: expected register identifier\n");
        return;
    }

    cell c;
    if (!popn (&ctx->dstack, &c, 1)) return;

    reg_push (ctx, reg, c);
}

void
dispatch_rpop (execution_ctx *ctx, void *data)
{
    (void)data;

    int reg = reg_next (&ctx->l);
    if (reg < 0)
    {
        fprintf (stderr, "dc/dispatch_rpop: expected register identifier\n");
        return;
    }

    cell c;
    if (!reg_pop (ctx, reg, &c)) return;

    push (&ctx->dstack, c);
}

static void
execute_macro (execution_ctx *ctx, const char *expr)
{
    lexer l = ctx->l; // lexer does not persist between execute_expr calls.
    execute_expr (ctx, expr);
    ctx->l = l;
}

void
dispatch_exec (execution_ctx *ctx, void *data)
{
    (void)data;

    cell c;
    if (!popn (&ctx->dstack, &c, 1)) return;

    if (c.type == CELL_NUM)
        push (&ctx->dstack, c);
    else
        execute_macro (ctx, c.as.str);

    cell_clear (&c);
}

void
dispatch_read_exec (execution_ctx *ctx, void *data)
{
    (void)data;

    char *line;
    size_t linelen = 0;

    ssize_t read = getline (&line, &linelen, stdin);
    if (read == -1) return;

    execute_macro (ctx, line);

    free (line);
}

void
dispatch_quit_n (execution_ctx *ctx, void *data)
{
    (void)data;
    cell c;
    if (!popn (&ctx->dstack, &c, 1)) return;

    if (c.type != CELL_NUM)
    {
        cell_clear (&c);
        return;
    }

    int levels = mpf_get_ui (c.as.num);
    cell_clear (&c);

    ctx->exec_level -= levels;
    if (ctx->exec_level < 0) ctx->exec_level = 0;
}

void
register_defaults (void)
{
    register_callback ('p', dispatch_print);
    register_callback ('q', dispatch_quit);
    register_callback ('f', dispatch_print_f);
    register_callback ('c', dispatch_clear);
    register_callback ('k', dispatch_prec);
    register_callback ('d', dispatch_dup);
    register_callback ('Q', dispatch_quit_n);

    register_callback ('+', dispatch_add);
    register_callback ('-', dispatch_sub);
    register_callback ('*', dispatch_mul);
    register_callback ('/', dispatch_div);
    register_callback ('%', dispatch_rem);
    register_callback ('^', dispatch_exp);
    register_callback ('v', dispatch_sqrt);

    register_callback ('s', dispatch_rstore);
    register_callback ('l', dispatch_rload);
    register_callback ('S', dispatch_rpush);
    register_callback ('L', dispatch_rpop);

    register_callback ('x', dispatch_exec);
    register_callback ('?', dispatch_read_exec);
}

void
execution_done (execution_ctx *ctx)
{
    stack_free (&ctx->dstack);
}
