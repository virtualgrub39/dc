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
cell cell_number (dc_num_t num);
void cell_display (FILE *f, const cell *c);
void cell_clear (cell *c);

typedef struct
{
    cell *data;
    size_t top, cap;
} stack;
#define INITIAL_STACK_SIZE 64

void push (stack *s, cell c);
bool popn (stack *s, cell *c, size_t n);
const cell *peek (stack *s);
void stack_free (stack *s);
void stack_dump (const stack *s);

typedef struct
{
    stack dstack;
    bool quit;
} execution_ctx;

typedef void (*command_cb) (execution_ctx *, void *);
static command_cb dispatch_table[256] = { 0 };

void dispatch (execution_ctx *ctx, int cmd, void *userdata);
void register_callback (int cmd, command_cb cb);
void register_defaults (void);

void execute_expr (execution_ctx *ctx, const char *expr);

void execution_done (execution_ctx *ctx);

#endif
