

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/cmdargs.h"

#define CMDARGS_FATAL(...)                                                     \
    do {                                                                       \
        fprintf(stderr, __VA_ARGS__);                                          \
        fputc('\n', stderr);                                                   \
        exit(2);                                                               \
    } while (0)

/*
 * TODO: Remove compile-time limit.
 */
#define MAX_INPUT_FILES 10
static CmdArgsInputFile g_input_files[MAX_INPUT_FILES];

static void cmdargs_load_defaults(CmdArgs* args) {
    args->input_files     = g_input_files;
    args->input_files_sz  = 0;
    args->load_sys_stdlib = true;
}

/*
 * TODO: Add '--help' option, allow '--' argument to denote the end of
 * options, just like most GNU programs. Perhaps just use a library like
 * <argp.h>.
 */
CmdArgs cmdargs_parse(int argc, char** argv) {
    CmdArgs result;
    cmdargs_load_defaults(&result);

    /*
     * True if the previous argument was '-s' or '--silent'.
     */
    bool got_silent_opt = false;

    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];

        /*
         * If the argument doesn't start with a dash, it's not an option, and
         * it's assumed to be a filename.
         */
        if (arg[0] != '-') {
            if (result.input_files_sz >= MAX_INPUT_FILES)
                CMDARGS_FATAL("Exceeded the input file limit (%d). Aborting.",
                              MAX_INPUT_FILES);

            FILE* fd = fopen(arg, "r");
            if (fd == NULL)
                CMDARGS_FATAL("Error opening '%s': %s.", arg, strerror(errno));

            result.input_files[result.input_files_sz].fd = fd;
            result.input_files[result.input_files_sz].silent_eval =
              got_silent_opt;
            result.input_files_sz++;
            got_silent_opt = false;
            continue;
        }

        /*
         * If the last argument was '-s' or '--silent' and we reached this
         * point, the user specified another option.
         */
        if (got_silent_opt)
            CMDARGS_FATAL("Expected a filename after '%s' option.",
                          (i > 1) ? argv[i - 1] : "--silent");

        /*
         * Check for individual command-line arguments.
         */
        if (!strcmp(arg, "-s") || !strcmp(arg, "--silent")) {
            if (i >= argc - 1)
                CMDARGS_FATAL("Expected an argument after '%s' option.", arg);

            got_silent_opt = true;
        } else if (!strcmp(arg, "--no-stdlib")) {
            result.load_sys_stdlib = false;
        }
    }

    return result;
}

void cmdargs_close_files(CmdArgs* args) {
    for (size_t i = 0; i < args->input_files_sz; i++)
        fclose(args->input_files[i].fd);
}
