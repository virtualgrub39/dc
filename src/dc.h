#ifndef DC_H
#define DC_H

#include <stdbool.h>
#include <stdio.h>

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

cell cell_strv (dc_strv_t strv);
cell cell_string (const char *str);
cell cell_number (const dc_num_t num);
void cell_display (FILE *f, const cell *c);
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
void stack_dump (const stack *s);

#define REG_COUNT 128
typedef struct
{
    stack dstack;
    bool quit;
    lexer l;
    stack r[REG_COUNT];
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
