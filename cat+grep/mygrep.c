#include <errno.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ERRBUF_SIZE 4096

enum err_code
{
    ERR_NEARGS,
    ERR_FILE_NOT_FOUND,
    ERR_MEMERROR,
    ERR_REGEX
};

struct
{
    regex_t comp_regex;
    regmatch_t pmatch[1];
    int err_code;
} regex_context;

char errbuf[ERRBUF_SIZE];


FILE *file = NULL;
const char *OPLIST = "h";

void list_routine(const char *pattern, const char *file);
void invoke_error(enum err_code code);
void close_file() { fclose(file); }
void reg_free() { regfree(&regex_context.comp_regex); }

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        invoke_error(ERR_NEARGS);
    }

    opterr = 0; // don't print error from getopt()
    int option;
    while ((option = getopt(argc, argv, OPLIST)) != -1)
    {
        switch (option)
        {
            case 'h':
                printf("mygrep - print lines that satisfy the given regexp\n"
                       "usage: mygrep [pattern] {file}\n"
                       "mireaaaa\n");
                exit(EXIT_SUCCESS);
                break;
        }
    }

    list_routine(argv[1], argc == 2 ? NULL : argv[2]);
    // -------
    exit(EXIT_SUCCESS);

}

void list_routine(const char *pattern, const char *f)
{
    // --- file opening ---
    if (f == NULL)
    {
        file = stdin;
    }
    else
    {
        file = fopen(f, "r");
        if (file == NULL)
        {
            invoke_error(ERR_FILE_NOT_FOUND);
        }
        atexit(close_file);
    }
    // --------------------
    
    ssize_t bytes = 0;
    size_t sz = 0;
    char* line = NULL;  
    errno = 0;

    // --- regex compiling ---
    regex_context.err_code = regcomp(&regex_context.comp_regex, pattern, REG_EXTENDED);
    if (regex_context.err_code)
    {
        regerror(regex_context.err_code, &regex_context.comp_regex, errbuf,
            ERRBUF_SIZE);
        invoke_error(ERR_REGEX);
    }
    atexit(reg_free);

    while ((bytes = getline(&line, &sz, file)) != -1)
    {
        if (regexec(&regex_context.comp_regex, line,
                sizeof regex_context.pmatch / sizeof regex_context.pmatch[0],
                regex_context.pmatch, 0)
            != REG_NOMATCH)
        {
            printf("%s", line);
        }
    }
    
    if (errno)
    {
        invoke_error(ERR_MEMERROR);
    }
    free(line);
}

void invoke_error(enum err_code err)
{
    switch (err)
    {
    case ERR_NEARGS:
        fprintf(stderr, "[grep]: Not enough args! Usage: mycat -h\n");
        break;
    
    case ERR_FILE_NOT_FOUND:
        fprintf(stderr, "[grep]: Error: file not found, see \"mycat -h\"\n");
        break;

    case ERR_MEMERROR:
        fprintf(stderr, "[grep]: Error: allocation failed!\n");
        break;

    case ERR_REGEX:
        fprintf(stderr, "[grep]: Error: %s\n", errbuf);
        break;
    }

    exit(EXIT_FAILURE);
}

