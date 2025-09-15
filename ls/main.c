#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

enum LS_ARGS
{
    LS_ALL = 1,
    LS_LONG = 2
};

const char *_OPLIST = "hla";

void _list_routine(const char *file, int flags);


int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "[ls]: Not enough args! Usage: ls -h\n");
        return 1;
    }

    // --- option parser ---
    opterr = 0; // don't print error from getopt
    int option;
    int ls_flags = 0; // bitwise OR of the flags from LS_ARGS
    while ((option = getopt(argc, argv, _OPLIST)) != -1)
    {
        switch (option)
        {
            case 'h':
                printf("ls - list directory contents\n"
                       "usage: ls [params...] [file]\n"
                       " -a - do not ignore entries starting with \".\"\n"
                       " -l - use a long listing format?\n"
                       " -h - print this message\n---\n"
                       "mireaaaa\n");
                break;
            case 'l':
                ls_flags |= LS_LONG;
                break;
            case 'a':
                ls_flags |= LS_ALL;
                break;
            default: 
                fprintf(stderr, "[ls]: Error: invalid option, see \"ls -h\"\n");
                return 1;
                break;
        }
    }

    if (optind == argc)
    {
        fprintf(stderr, "[ls]: Error: there is no file! See \"ls -h\"\n");
        return 1;
    }
    // -------

    _list_routine(argv[optind], ls_flags);
    return 0;
}


void _list_routine(const char *dir, int flags)
{
    

}
