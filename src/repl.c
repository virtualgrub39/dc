#include "dc.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

void
execute_file (execution_ctx *ctx, FILE *f)
{
    char *line = NULL;
    ssize_t linelen;
    size_t linesize = 0;

    while ((linelen = getline (&line, &linesize, f)) != -1)
    {
        execute_expr (ctx, line);
        if (ctx->exec_level < 0) break;
    }

    free (line);
    if (ferror (f)) perror ("dc/getline: ");
}

void
execute_path (execution_ctx *ctx, const char *path)
{
    FILE *f = fopen (path, "rb");
    if (!f)
    {
        fprintf (stderr, "dc/execute_path: ");
        perror (NULL);
        return;
    }
    execute_file (ctx, f);
    fclose (f);
}

int fileset = false, exprset = false;

static const char *shortopts = "hve:f:";
static const struct option longopts[] = {
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'v' },
    { "file", required_argument, NULL, 'f' },
    { "expression", required_argument, NULL, 'e' },
    { 0, 0, 0, 0 },
};

void
disp_help (const char *progname)
{
    printf ("USAGE: %s [OPTION] [file...]\n", progname);
}

void
disp_version (const char *progname)
{
    printf ("%s version " _DC_VERSION " (" __DATE__ " " __TIME__ ")\n", progname);
}

int
main (int argc, const char *argv[])
{
    int c;

    register_defaults ();
    execution_ctx ctx = { 0 };

    while ((c = getopt_long (argc, (char *const *)argv, shortopts, longopts, 0)) != -1)
    {
        switch (c)
        {
        case 'e':
            exprset = true;
            execute_expr (&ctx, optarg);
            break;
        case 'f':
            fileset = true;
            execute_path (&ctx, optarg);
            break;
        case 'h': disp_help (argv[0]); return 0;
        case 'v': disp_version (argv[0]); return 0;
        default: disp_help (argv[0]); return 1;
        }
    }

    while (optind != argc)
    {
        execute_path (&ctx, argv[optind]);
        optind += 1;
        fileset = true;
    }

    if (!exprset && !fileset) execute_file (&ctx, stdin);

    execution_done (&ctx);

    return 0;
}
