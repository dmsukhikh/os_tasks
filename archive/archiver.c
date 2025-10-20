#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Состояния программы
 *
 * Состояние контролируется флагами, передаваемыми при запуске программы
 */
enum prog_mode
{
    MODE_INPUT, //!< Вставить файл в архив
    MODE_REMOVE, //!< Удалить файл из архива
    MODE_EXTRACT, //!< Получить файлы из архива
    MODE_STAT, //!< Получить информацию о файлах в архиве
    MODE_HELP, //!< Вывести информацию о файлах в архиве
    MODE_UNDEF //!< Неопознанное состояние (ошибка в аргументах)
};

/**
 * Определить режим работы по аргументам программы
 */
enum prog_mode parse_args(int argc, char **argv);

/**
 * Вывести сообщение-справку о работе программы
 */
void print_help();

int main(int argc, char **argv)
{
    switch(parse_args(argc, argv))
    {
        case MODE_HELP:
            print_help();
            break;
        default:
            break;
    }
    exit(EXIT_SUCCESS);
}

enum prog_mode parse_args(int argc, char **argv)
{
    if (argc < 2) // without args
    {
        return MODE_UNDEF;
    }

    if (strcmp(argv[1], "-h") || strcmp(argv[1], "--help"))
    {
        return MODE_HELP;
    }

    if (strcmp(argv[2], "-i") || strcmp(argv[2], "--input"))
    {
        return MODE_INPUT;
    }

    if (strcmp(argv[2], "-e") || strcmp(argv[2], "--extract"))
    {
        return MODE_EXTRACT;
    }

    if (strcmp(argv[2], "-r") || strcmp(argv[2], "--remove"))
    {
        return MODE_EXTRACT;
    }

    if (strcmp(argv[2], "-s") || strcmp(argv[2], "--stat"))
    {
        return MODE_EXTRACT;
    }

    return MODE_UNDEF;
}

void print_help()
{
    printf("archiver - простой архиватор\n"
           "Флаги:\n"
           " [АРХИВ] -r(--remove)  [ФАЙЛ,...] - Удалить файл(ы) из архива\n"
           " [АРХИВ] -i(--insert)  [ФАЙЛ,...] - Вставить файл(ы) в архив\n"
           " [АРХИВ] -e(--extract) [ФАЙЛ,...] - Получить файлы из архива\n"
           "         Если не указывать файлы, то извлечется все содержимое архива\n"
           " [АРХИВ] -s(--stat)               - Вывести информацию о архиве\n"
           "---\n"
           "prod. by dmsukhikh\n");
}
