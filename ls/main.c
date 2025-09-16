#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <pwd.h>
#include <grp.h>

// TODO: make error checks for every syscalls

struct _pwd_context
{
    struct passwd data;
    struct passwd *res;
    char *buf;
};
typedef struct _pwd_context pwd_context_t; 
const int _PWD_BUFSIZE = 65536;
pwd_context_t pwd_context;

struct _grp_context
{
    struct group data;
    struct group *res;
    char *buf;
};
const int _GRP_BUFSIZE = 65536;
typedef struct _grp_context grp_context_t;
grp_context_t grp_context;


enum LS_ARGS
{
    LS_ALL = 1,
    LS_LONG = 2
};


const char *_OPLIST = "hla";
const char *_RWX = "rwx";
DIR *_dir = NULL;
int flags = 0;  // bitwise OR of the flags from LS_ARGS

void _list_routine(const char *dir);
void _close_dir_at_exit();
void _print_file(struct dirent* file);
void _free_pwd_buf_at_exit();


int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "[ls]: Not enough args! Usage: ls -h\n");
        exit(EXIT_FAILURE);
    }

    // --- option parser ---
    opterr = 0; // don't print error from getopt()
    int option;
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
                exit(EXIT_SUCCESS);
                break;
            case 'l':
                flags |= LS_LONG;
                break;
            case 'a':
                flags |= LS_ALL;
                break;
            default: 
                fprintf(stderr, "[ls]: Error: invalid option, see \"ls -h\"\n");
                exit(EXIT_FAILURE);
                break;
        }
    }

    if (optind == argc)
    {
        fprintf(stderr, "[ls]: Error: there is no file! See \"ls -h\"\n");
        exit(EXIT_FAILURE);
    }
    // -------
    _list_routine(argv[optind]);
    exit(EXIT_SUCCESS);
}


void _list_routine(const char *dir)
{
    _dir = opendir(dir); 
    if (_dir == NULL)
    {
        fprintf(stderr, "[ls]: Error while opening dir %s! %s\n", dir, strerror(errno));
        exit(EXIT_FAILURE);
    }

    atexit(_close_dir_at_exit);

    // scanning directories
    errno = 0; // for detecting error in readdir()
    for (struct dirent *cur_file; (cur_file = readdir(_dir)) != NULL; )
    {
        _print_file(cur_file); 
    }

    if (errno != 0)
    {
        fprintf(stderr, "[ls]: Error while parsing files! %s\n",
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    // after printing files without LS_LONG flag there is no '\n' at the end of
    // the stdout
    if (!(flags & LS_LONG))
    {
        putchar('\n');
    }
}

void _print_file(struct dirent *file)
{
    if (file->d_name[0] == '.' && !(flags & LS_ALL))
    {
        return;
    }

    if (flags & LS_LONG) // long output
    {
        struct stat file_info;
        lstat(file->d_name, &file_info);

        // type of the file
        if (S_ISREG(file_info.st_mode))
        {
            putchar('-');
        }
        else if (S_ISDIR(file_info.st_mode))
        {
            putchar('d');
        }
        else if (S_ISBLK(file_info.st_mode))
        {
            putchar('b');
        }
        else if (S_ISLNK(file_info.st_mode))
        {
            putchar('l');
        }
        else // another files: fifo's, sockets and so on
        {
            putchar('?');
        }

        // file permissions
        int i = 0;
        for (uint64_t mask = S_IRUSR; mask > 0; mask >>= 1, i++)
        {
            putchar(file_info.st_mode & mask ? _RWX[i%3] : '-');
        }
        putchar(' ');

        // hard links, groups, size
        //
        // printf("%lu %s %s %lu ", file_info.st_nlink, pwd_file->pw_name,
        //        grp_file->gr_name, file_info.st_size);

        printf("%s\n", file->d_name); 
    }
    else // short output
    {
        printf("%s  ", file->d_name);
    }
}

void _close_dir_at_exit() { closedir(_dir); }
