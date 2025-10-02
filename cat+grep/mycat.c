#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>


enum cat_flag
{
    FLAG_NUM = 1,
    FLAG_NENUM = 2,
    FLAG_END = 4
};

enum err_code
{
    ERR_INVALIDOPT,
    ERR_NEARGS,
    ERR_FILE_NOT_FOUND,
    ERR_MEMERROR
};

FILE *file = NULL;
uint32_t flags = 0;
const char *OPLIST = "nbEh";
int line_align = 0, neline_align = 0;

void close_file()
{
    fclose(file);
}

void list_routine(const char *file);
void invoke_error(enum err_code code);
void count_lines();


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
                printf("mycat - print file contents\n"
                       "usage: mycat [params...] [file]\n"
                       " -n - count every line in file \".\"\n"
                       " -b - count every non-empty line in file\n"
                       " -E - print delimeter in the end of lines\n---\n"
                       "mireaaaa\n");
                exit(EXIT_SUCCESS);
                break;
            case 'n':
                flags |= FLAG_NUM;
                break;
            case 'b':
                flags |= FLAG_NENUM;
                break;
            case 'E':
                flags |= FLAG_END;
                break;
            default: 
                invoke_error(ERR_INVALIDOPT);
                break;
        }
    }

    list_routine(argv[optind]);
    // -------
    exit(EXIT_SUCCESS);

}

void list_routine(const char *f)
{
    // --- file opening ---
    file = fopen(f, "r");
    if (file == NULL)
    {
        invoke_error(ERR_FILE_NOT_FOUND);
    }
    atexit(close_file);
    // --------------------
    
    ssize_t bytes = 0;
    size_t sz = 0;

    char* line = NULL;

    errno = 0;
    size_t lines = 1, nelines = 1;
    
    count_lines();

    while ((bytes = getline(&line, &sz, file)) != -1)
    {
        if (flags & FLAG_NENUM)
        {
            if (bytes > 1)
            {
                printf("   %*zu\t", neline_align, nelines++);
            }
        }
        else if (flags & FLAG_NUM)
        {
            printf("  %*zu\t", line_align, lines++);
        }
        printf("%.*s", (int)bytes - 1, line);

        if (flags & FLAG_END)
        {
            putchar('$');
        }
        putchar('\n');
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
        fprintf(stderr, "[cat]: Not enough args! Usage: mycat -h\n");
        break;
    
    case ERR_INVALIDOPT:
        fprintf(stderr, "[cat]: Error: invalid option, see \"mycat -h\"\n");
        break;

    case ERR_FILE_NOT_FOUND:
        fprintf(stderr, "[cat]: Error: file not found, see \"mycat -h\"\n");
        break;

    case ERR_MEMERROR:
        fprintf(stderr, "[cat]: Error: allocation failed!\n");
        break;
    }

    exit(EXIT_FAILURE);
}

void count_lines()
{
    ssize_t bytes = 0;
    size_t sz = 0; 
    char *line;
    size_t l = 0, nel = 0;
    while ((bytes = getline(&line, &sz, file)) != -1)
    {
        l++;
        if (bytes > 1) nel++; 
    }

    while (l > 0)
    {
        line_align++;
        l /= 10;
    }

    while (nel > 0)
    {
        neline_align++;
        nel /= 10;
    }

    if (errno)
    {
        invoke_error(ERR_MEMERROR);
    }
    rewind(file);
    free(line);
}
