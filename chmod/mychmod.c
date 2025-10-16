#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>

// Состояния конечного автомата
// regex for chmod input: [ugoa]*([-+=]([rwxXst]*|[ugo]))+|[0-7]+
enum STATES
{
    STATE_ENTRY,
    STATE_SET_PERMGROUPS,
    STATE_SET_MODIFICATOR,
    STATE_SET_PERM,
    STATE_SET_COPIED_GROUP,
    STATE_EXIT
};

enum PERMGROUPS
{
    PG_ALL = 1,
    PG_USER = 2,
    PG_GROUP = 4,
    PG_OTHERS = 8
};

enum MODIFICATOR
{
    MOD_UNDEF,
    MOD_APPEND, // '+'
    MOD_ERASE,  // '-'
};

enum PERMS
{
    P_READ = 1,
    P_WRITE = 2,
    P_EXEC = 4
};

// связный список
struct changed_perm;
typedef struct changed_perm changed_perm_t;

struct changed_perm
{
    enum MODIFICATOR mod;
    bool contains_copied_pg; // элемент структуры changed_perm содержит или
                             // информацию о изменяемых правах, или информацию о
                             // тех группах, права которых мы заимствуем
    char flags; // Побитовое ИЛИ флагов
    changed_perm_t* next;
};

struct parsed_mode
{
    char perm_groups; // побитовое ИЛИ перечислений из PERMGROUPS
    changed_perm_t *perms, *end_perms;
};
typedef struct parsed_mode parsed_mode_t;

parsed_mode_t parsed_mode; // Глобальная переменная
mode_t initial_mode = 0;
bool is_number = false;

void reset_parsed_mode(parsed_mode_t *s)
{
    s->perm_groups = 0;
    s->perms = NULL;
}

void clear_changed_perm()
{
    changed_perm_t *l = parsed_mode.perms;
    while (l != NULL)
    {
        changed_perm_t* n = l->next;
        free(l);
        l = n;
    }
    parsed_mode.perms = NULL;
    parsed_mode.end_perms = NULL;
}

void add_changed_perm()
{
    changed_perm_t *new = malloc(sizeof(changed_perm_t));
    new->contains_copied_pg = false;
    new->flags = 0;
    new->mod = MOD_UNDEF;

    if (!parsed_mode.perms) parsed_mode.perms = new;
    else parsed_mode.end_perms->next = new;
    parsed_mode.end_perms = new;
}

mode_t parse_mode_str(char * mode_str);
void apply_modes();

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "[mychmod]: not enough args!\n");
        exit(EXIT_FAILURE);
    }

    char *mode = argv[1];

    atexit(clear_changed_perm);
    mode_t mask = parse_mode_str(mode);

    for (int i = 2; i < argc; ++i)
    {
        struct stat st;
        if (stat(argv[i], &st) == -1)
        {
            perror("[mychmod]: error");
            exit(EXIT_FAILURE);
        }

        initial_mode = st.st_mode;

        if (!is_number)
            apply_modes();
        else
            initial_mode = mask;

        if (chmod(argv[i], initial_mode) == -1)
        {
            perror("[mychmod]: error");
            exit(EXIT_FAILURE);
        };
    }

    exit(EXIT_SUCCESS);
}



mode_t parse_mode_str(char * mode_str)
{
    if (strspn(mode_str, "01234567") == strlen(mode_str))
    {
        is_number = true;
        return strtoul(mode_str, NULL, 8);
    }

    // Симулируем конечный автомат по regex'у
    enum STATES state = STATE_ENTRY;
    reset_parsed_mode(&parsed_mode);

    int str_index = -1;

    while (state != STATE_EXIT)
    {
        char cur_char;
        switch (state)
        {
            case STATE_ENTRY:
            {
                str_index++;
                cur_char = mode_str[str_index];

                if (cur_char == '-' || cur_char == '+')
                {
                    state = STATE_SET_MODIFICATOR;
                }

                else if (cur_char == 'u' || cur_char == 'g' || cur_char == 'o' || cur_char == 'a')
                {
                    state = STATE_SET_PERMGROUPS;
                }

                else
                {
                    fprintf(stderr, "[mychmod]: invalid string!\n");
                    exit(EXIT_FAILURE);
                }
                break;
            } 

            case STATE_SET_PERMGROUPS:
            {
                switch (cur_char)
                {
                    case 'u':
                        parsed_mode.perm_groups |= PG_USER;
                        break;
                    case 'g':
                        parsed_mode.perm_groups |= PG_GROUP;
                        break;
                    case 'a':
                        parsed_mode.perm_groups |= PG_ALL;
                        break;
                    case 'o':
                        parsed_mode.perm_groups |= PG_OTHERS;
                        break;
                }
                state = STATE_ENTRY;
                break;

            }

            case STATE_SET_MODIFICATOR:
            {
                add_changed_perm();
                switch (cur_char)
                {
                   case '+':
                       parsed_mode.end_perms->mod = MOD_APPEND;
                       break;
                    case '-':
                       parsed_mode.end_perms->mod = MOD_ERASE;
                       break;
                }
                
                str_index++;
                cur_char = mode_str[str_index];
                
                if (cur_char == 'u' || cur_char == 'g' || cur_char == 'o')
                {
                    parsed_mode.end_perms->contains_copied_pg = true;
                    state = STATE_SET_COPIED_GROUP;
                }
                else if (cur_char == 'r' || cur_char == 'w' || cur_char == 'x')
                {
                    parsed_mode.end_perms->contains_copied_pg = false;
                    state = STATE_SET_PERM;
                }
                else if (cur_char == '\0')
                {
                    state = STATE_EXIT;
                }
                else
                {
                    fprintf(stderr, "[mychmod]: invalid string!\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }

            case STATE_SET_COPIED_GROUP:
            {
                switch (cur_char)
                {
                case 'u':
                    parsed_mode.end_perms->flags |= PG_USER;
                    break;
                case 'g':
                    parsed_mode.end_perms->flags |= PG_GROUP;
                    break;
                case 'o':
                    parsed_mode.end_perms->flags |= PG_OTHERS;
                    break;
                }

                str_index++;
                cur_char = mode_str[str_index];
                
                if (cur_char == 'u' || cur_char == 'g' || cur_char == 'o')
                {
                    state = STATE_SET_COPIED_GROUP;
                }
                else if (cur_char == '-' || cur_char == '+')
                {
                    state = STATE_SET_MODIFICATOR;
                }
                else if (cur_char == '\0')
                {
                    state = STATE_EXIT;
                }
                else
                {
                    fprintf(stderr, "[mychmod]: invalid string!\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }

            case STATE_SET_PERM:
            {
                switch (cur_char)
                {
                case 'r':
                    parsed_mode.end_perms->flags |= P_READ;
                    break;
                case 'w':
                    parsed_mode.end_perms->flags |= P_WRITE;
                    break;
                case 'x':
                    parsed_mode.end_perms->flags |= P_EXEC;
                    break;
                }

                str_index++;
                cur_char = mode_str[str_index];
                
                if (cur_char == 'r' || cur_char == 'w' || cur_char == 'x')
                {
                    state = STATE_SET_PERM;
                }
                else if (cur_char == '-' || cur_char == '+')
                {
                    state = STATE_SET_MODIFICATOR;
                }
                else if (cur_char == '\0')
                {
                    state = STATE_EXIT;
                }
                else
                {
                    fprintf(stderr, "[mychmod]: invalid string!\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }

            case STATE_EXIT:
                break;
        }
    }

    if (!parsed_mode.perms)
    {
        fprintf(stderr, "[mychmod]: invalid string!\n");
        exit(EXIT_FAILURE);
    }


    return ~0;
}

void apply_modes()
{
    mode_t bmask = 0;
    if (parsed_mode.perm_groups == 0) parsed_mode.perm_groups |= PG_ALL;

    if (parsed_mode.perm_groups & PG_OTHERS)
        bmask |= (S_IROTH | S_IWOTH | S_IXOTH);
    if (parsed_mode.perm_groups & PG_GROUP)
        bmask |= (S_IRGRP | S_IWGRP | S_IXGRP);
    if (parsed_mode.perm_groups & PG_USER)
        bmask |= (S_IRUSR | S_IWUSR | S_IXUSR);

    if (parsed_mode.perm_groups & PG_ALL)
    {
        bmask |= (S_IROTH | S_IWOTH | S_IXOTH);
        bmask |= (S_IRGRP | S_IWGRP | S_IXGRP);
        bmask |= (S_IRUSR | S_IWUSR | S_IXUSR);
    }


    for (changed_perm_t *i = parsed_mode.perms; i != NULL; i = i->next)
    {
        mode_t pmask = 0;
        if (i->contains_copied_pg)
        {
            if (i->flags & PG_OTHERS) pmask |= (S_IROTH | S_IWOTH | S_IXOTH);
            if (i->flags & PG_GROUP) pmask |= (S_IRGRP | S_IWGRP | S_IXGRP);
            if (i->flags & PG_USER) pmask |= (S_IRUSR | S_IWUSR | S_IXUSR);
        }
        else
        {
            if (i->flags & P_READ) pmask |= (S_IROTH | S_IRGRP | S_IRUSR);
            if (i->flags & P_WRITE) pmask |= (S_IWOTH | S_IWGRP | S_IWUSR);
            if (i->flags & P_EXEC) pmask |= (S_IXOTH | S_IXGRP | S_IXUSR);
        }
        pmask &= bmask;

        if (i->mod == MOD_APPEND)
        {
            initial_mode |= pmask;
        }
        else if (i->mod == MOD_ERASE)
        {
            initial_mode &= ~pmask;
        }
    }
}
