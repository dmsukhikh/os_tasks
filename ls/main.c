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
#include <time.h>

enum LS_ARGS
{
    LS_ALL = 1,
    LS_LONG = 2
};

enum COLORS
{
    COL_FILE,
    COL_DIR,
    COL_EXEC,
    COL_LN
};

enum ERRCODES
{
    ERR_NEARGS,
    ERR_INVALIDOPT,
    ERR_FILE_ISNT_SPEC,
    ERR_OPENDIR,
    ERR_READDIR,
    ERR_STAT,
    ERR_PWD,
    ERR_GRP
};

// union aligns
// {
//     struct
//     {
//         char link;
//         char user;
//         char group;
//         char size;
//         char name;
//     } long_al;
//
//     struct
//     {
//         char     
//     };
// };

const int _COL_CODES[] = {39, 34, 32, 36};

const char * const _OPLIST = "hla";
const char * const _RWX = "rwx";
const char * _prefix = NULL;

const int _PATH_SIZE = 4048;
char * _path = NULL;

DIR *_dir = NULL;
size_t _files_list_size = 0;
struct dirent ** _files_list = NULL;
int flags = 0;  // bitwise OR of the flags from LS_ARGS

void _list_routine(const char *dir);
void _close_dir_at_exit();
void _print_file(struct dirent* file);
void _invoke_error(enum ERRCODES);
void _my_ls_init();
void _free_path_at_exit();
void _prepare_path(const char * file);
void _prepare_files_list();
void _free_files_list_at_exit();
int _dirent_cmp(const void* a, const void* b);


int main(int argc, char **argv)
{
    _my_ls_init();

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
                       " -l - use a long listing format\n"
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
                _invoke_error(ERR_INVALIDOPT);
                break;
        }
    }

    if (optind == argc)
    {
        _list_routine(".");
    }
    else
    {
        _list_routine(argv[optind]);
    }
    // -------
    exit(EXIT_SUCCESS);
}


void _list_routine(const char *dir)
{
    _dir = opendir(dir); 
    if (_dir == NULL)
    {
        _invoke_error(ERR_OPENDIR);
    }

    atexit(_close_dir_at_exit);

    _prefix = dir;

    // scanning directories
    errno = 0; // for detecting error in readdir()

    _prepare_files_list();
    for (size_t i = 0; i < _files_list_size; ++i)
    {
        _print_file(_files_list[i]);
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

    _prepare_path(file->d_name);

    enum COLORS filename_color = COL_FILE;
    struct stat file_info;
    if (lstat(_path, &file_info) == -1) 
    {
        // printf("%s\n", _path);
        _invoke_error(ERR_STAT);
    }


    if (flags & LS_LONG) // long output
    {
        // type of the file

        if (S_ISREG(file_info.st_mode))
        {
            putchar('-');
            // check whether the file is executable
            if (file_info.st_mode & S_IXUSR)
            {
                filename_color = COL_EXEC;
            }
        }
        else if (S_ISDIR(file_info.st_mode))
        {
            putchar('d');
            filename_color = COL_DIR;
        }
        else if (S_ISBLK(file_info.st_mode))
        {
            putchar('b');
        }
        else if (S_ISLNK(file_info.st_mode))
        {
            putchar('l');
            filename_color = COL_LN;
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

        errno = 0;
        struct passwd *pwd_file = getpwuid(file_info.st_uid);
        if (pwd_file == NULL && errno)
        {
            _invoke_error(ERR_PWD);
        }

        // valgrind reports about a memory leak here 
        errno = 0;
        struct group *grp_file = getgrgid(file_info.st_gid);
        if (grp_file == NULL && errno)
        {
            _invoke_error(ERR_PWD);
        }

        // hard links, groups, size
        printf("%lu %s %s %lu ", file_info.st_nlink, pwd_file->pw_name,
               grp_file->gr_name, file_info.st_size);

        // time 
        char *time_str = ctime(&file_info.st_mtim.tv_sec); 
        char output_time_str[13];
        strncpy(output_time_str, time_str + 4, 12); 
        output_time_str[12] = '\0';
        printf("%s ", output_time_str);

        // name 
        // filename: "several words" -> `several words`
        if (strchr(file->d_name, ' ') != NULL)
        {
            printf("\x1b[;%dm`%s`\x1b[0m\n", _COL_CODES[filename_color],
                   file->d_name);
        }
        else
        {
            printf("\x1b[;%dm%s\x1b[0m\n", _COL_CODES[filename_color],
                   file->d_name);
        }
    }
    else // short output
    {
        if (S_ISREG(file_info.st_mode) && (file_info.st_mode & S_IXUSR))
        {
            filename_color = COL_EXEC;
        }
        else if (S_ISDIR(file_info.st_mode))
        {
            filename_color = COL_DIR;
        }
        else if (S_ISLNK(file_info.st_mode))
        {
            filename_color = COL_LN;
        }

        // filename: "several words" -> `several words`
        if (strchr(file->d_name, ' ') != NULL)
        {
            printf("\x1b[;%dm`%s`\x1b[0m  ", _COL_CODES[filename_color],
                   file->d_name);
        }
        else
        {
            printf("\x1b[;%dm%s\x1b[0m  ", _COL_CODES[filename_color],
                   file->d_name);
        }
    }
}

void _close_dir_at_exit() { closedir(_dir); }

void _invoke_error(enum ERRCODES err)
{
    switch (err)
    {
    case ERR_NEARGS:
        fprintf(stderr, "[ls]: Not enough args! Usage: ls -h\n");
        break;
    
    case ERR_INVALIDOPT:
        fprintf(stderr, "[ls]: Error: invalid option, see \"ls -h\"\n");
        break;

    case ERR_FILE_ISNT_SPEC:
        fprintf(stderr, "[ls]: Error! File isn't specified\n");
        break;

    case ERR_OPENDIR:
        fprintf(stderr, "[ls]: Error while opening directory! %s\n",
                strerror(errno));
        break;

    case ERR_READDIR:
        fprintf(stderr, "[ls]: Error while parsing files! %s\n",
                strerror(errno));
        break;

    case ERR_STAT:
        fprintf(stderr, "[ls]: error while getting stat! %s\n",
                strerror(errno));
        break;

    case ERR_PWD:
        fprintf(stderr, "[ls]: error while getting username! %s\n",
                strerror(errno));
        break;

    case ERR_GRP:
        fprintf(stderr, "[ls]: error while getting groupname! %s\n",
                strerror(errno));
        break;
    }

    exit(EXIT_FAILURE);
}

void _my_ls_init()
{
    _path = (char *)malloc(_PATH_SIZE);
    atexit(_free_path_at_exit);
}

void _free_path_at_exit() { free(_path); }

void _prepare_path(const char * file)
{
    _path[0] = '\0';
    strcat(_path, _prefix);
    strcat(_path, "/");
    strcat(_path, file);
}

void _prepare_files_list()
{
    errno = 0;
    for (struct dirent *cur_file; (cur_file = readdir(_dir)) != NULL;
         _files_list_size++)
        ;

    if (errno)
    {
        _invoke_error(ERR_READDIR);
    }

    _files_list =
        (struct dirent **)malloc(_files_list_size * sizeof(struct dirent *));
    atexit(_free_files_list_at_exit);

    rewinddir(_dir);

    int i = 0;
    for (struct dirent *cur_file; (cur_file = readdir(_dir)) != NULL;
         _files_list[i++] = cur_file)
        ;

    qsort(_files_list, _files_list_size, sizeof(struct dirent *), _dirent_cmp);
}

void _free_files_list_at_exit() { free(_files_list); }

int _dirent_cmp(const void *a, const void *b)
{
    struct dirent *f = *(struct dirent **)a, *s = *(struct dirent **)b;
    return strcmp(f->d_name, s->d_name);
}
