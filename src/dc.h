#ifndef DC_H
#define DC_H

#include <stdbool.h>
#include <stdio.h>

#include <gmp.h>

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
int reg_next (lexer *l);
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

cell cell_strv (dc_strv_t strv);
cell cell_string (const char *str);
cell cell_number (const dc_num_t num);
void cell_display (FILE *f, const cell *c, size_t prec);
void cell_clear (cell *c);
cell cell_clone (const cell *c);

typedef struct
{
    cell *data;
    size_t top, cap;
} stack;
#define INITIAL_STACK_SIZE 64

void push (stack *s, cell c);
bool popn (stack *s, cell *c, size_t n);
const cell *peek (stack *s);
void stack_set (stack *s, cell c);
void stack_free (stack *s);
void stack_dump (const stack *s, size_t prec);

#define REG_COUNT 128
typedef struct
{
    stack dstack;
    lexer l;
    stack r[REG_COUNT];
    int exec_level;
    size_t prec;
} execution_ctx;

typedef void (*command_cb) (execution_ctx *, void *);
static command_cb dispatch_table[256] = { 0 };

void dispatch (execution_ctx *ctx, int cmd, void *userdata);
void register_callback (int cmd, command_cb cb);
void register_defaults (void);

void reg_store (execution_ctx *ctx, int reg, cell v);
void reg_push (execution_ctx *ctx, int reg, cell v);
bool reg_load (execution_ctx *ctx, int reg, cell *c);
bool reg_pop (execution_ctx *ctx, int reg, cell *c);

void execute_expr (execution_ctx *ctx, const char *expr);

void execution_done (execution_ctx *ctx);

#endif
