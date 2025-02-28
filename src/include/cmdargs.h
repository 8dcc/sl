
#ifndef CMDARGS_H_
#define CMDARGS_H_ 1

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h> /* FILE */

/*
 * Structure representing an input file specified by the user in the command
 * line.
 */
typedef struct {
    FILE* fd;
    bool silent_eval;
} CmdArgsInputFile;

/*
 * Structure representing the relevant information that was extracted after
 * parsing the command-line arguments.
 */
typedef struct {
    CmdArgsInputFile* input_files;
    size_t input_files_sz;

    bool load_sys_stdlib;
} CmdArgs;

/*----------------------------------------------------------------------------*/

/*
 * Parse the command-line arguments, and return a higher-level data structure
 * with the relevant information.
 */
CmdArgs cmdargs_parse(int argc, char** argv);

/*
 * Close all the files that were open in 'cmdargs_parse'.
 */
void cmdargs_close_files(CmdArgs* args);

#endif /* CMDARGS_H_ */
