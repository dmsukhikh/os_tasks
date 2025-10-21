#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>

#define FILENAME_LENGTH 255
#define HEADER_ENDING_SIZE 4
#define BUFFER_SIZE 1024

/**
 * \file archiver.c - простой архиватор
 *
 * Архив имеет следующую структуру: вначале последовательно идут структуры
 * file_info, после этого - содержимое файлов, идущее подряд. Конец заголовка -
 * 4 нулевых байтов (0000 0000). Для эффективной работы с заголовком архива
 * (структуры file_info вначале) существует связный список fi_list
 *
 * При добавлении (insert) информация о файле добавляется в конец заголовка, а
 * данные добавляются в таком же порядке в конец файла
 */

/**
 * Информация о файле в архиве
 *
 * \note Для переносимости файла архива используются следующие техники:
 * * Определяется выравнивание `#pragma pack(1)`
 * * Для записи чтения/записи чисел используется ntohl/htonl, для создания
 * архивов, переносимых на системы с другим порядком байт (endianness)
 */
struct file_info;
typedef struct file_info file_info_t;

; // Штука, чтобы предотварить ошибочное предупреждение clangd
#pragma pack(push, 1)
struct file_info
{
    uint8_t  filename[FILENAME_LENGTH]; //!< Имя файла
    uint64_t filesize; //!< Размер файла (в файле)
    uint64_t mask; //!< Маска прав доступа к файлу
    uint64_t _offset; //!< Положение файла в архиве
};
#pragma pack(pop)


/**
 * Декодированный заголовок архива
 *
 * fi_list == NULL - конец связного списка
 */
struct fi_list
{
    file_info_t data;
    struct fi_list *next;
};
typedef struct fi_list fi_list_t;

/**
 * Глобальная переменная для доступа к заголовку архива
 *
 * \see read_header
 */
fi_list_t *global_fi_list = NULL;

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
 * Тип ошибки
 *
 * Передается в print_err для вывода сообщения ошибки
 */
enum err_code
{
    ERR_ARGS, //!< Ошибка при парсинге аргументов
    ERR_INPUT_NOFILES, //!< Не переданы файлы для вставки в архив
    ERR_OPEN, //!< Ошибка при открытии файла
    ERR_INPUT_NOAPP
};

/**
 * Определить режим работы по аргументам программы
 */
enum prog_mode parse_args(int argc, char **argv);

/**
 * Вывести сообщение-справку о работе программы
 */
void print_help();

/**
 * Вывести сообщение об ошибке и завершить программу
 *
 * После вывода сообщения о ошибке в stderr завершает программу с кодом EXIT_FAILURE
 *
 * \param code Тип ошибки
 */
void print_err(enum err_code code);

/**
 * Поместить в архив файлы
 * \param archive Название ошибки
 * \param fnums Количество файлов
 * \param fnames Массив названий файлов
 */
void input_files(char* archive, int fnums, char** fnames);

/**
 * Читает заголовок (см. документацию к файлу archiver.c), заполняя
 * global_fi_list. После устанавливает указатель в файле на начало (rewind)
 */
void read_header(int arch_fd);

/**
 * Очищает global_fi_list
 */
void free_fi_list();

/**
 * Возвращает конец связного списка global_fi_list
 *
 * \return Указатель на конец global_fi_list или NULL, если не инициализировано
 */
fi_list_t *end_global_fi_list();

/**
 * Создает пустую ноду в конце global_fi_list
 *
 * \return Новую ноду в конце global_fi_list
 */
fi_list_t *make_new_node_in_global_fi_list();

// Алгоритм поворота
// 4 3 2 1 -> 3 4 1 2 -> 1 2 3 4

int main(int argc, char **argv)
{
    switch(parse_args(argc, argv))
    {
        case MODE_HELP:
            print_help();
            break;

        case MODE_INPUT:
            input_files(argv[1], argc-3, argv+3);
            break;

        case MODE_UNDEF:
            print_err(ERR_ARGS);
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

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
        return MODE_HELP;
    }

    if (argc < 3) // with only one arg
    {
        return MODE_UNDEF;
    }

    if (strcmp(argv[2], "-i") == 0 || strcmp(argv[2], "--input") == 0)
    {
        return MODE_INPUT;
    }

    if (strcmp(argv[2], "-e") == 0 || strcmp(argv[2], "--extract") == 0)
    {
        return MODE_EXTRACT;
    }

    if (strcmp(argv[2], "-r") == 0 || strcmp(argv[2], "--remove") == 0)
    {
        return MODE_EXTRACT;
    }

    if (strcmp(argv[2], "-s") == 0 || strcmp(argv[2], "--stat") == 0)
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

void print_err(enum err_code code)
{
    char *errmsg = 0;
    switch (code)
    {
        case ERR_ARGS:
            errmsg = "Ошибка при разборе аргументов";
            break;

        case ERR_INPUT_NOFILES:
            errmsg = "Не указаны файлы для вставки в архив";
            break;

        case ERR_OPEN:
            errmsg = "Ошибка при открытии файла";
            break;

        case ERR_INPUT_NOAPP:
            errmsg = "Не добавлен ни один указанный файл";
            break;
    }

    if (errno == 0)
    {
        fprintf(stderr, "[archiver]: %s. См. \"archiver --help\" для справки\n", errmsg);
    }
    else
    {
        fprintf(stderr,
            "[archiver]: %s, %s. \nСм. \"archiver --help\" для справки\n", errmsg,
            strerror(errno));
    }
    exit(EXIT_FAILURE);
}


void input_files(char* archive, int fnums, char** fnames)
{
    const char *new_fd_name = ".supertemp.egleser";

    int new_fd = open(new_fd_name, O_CREAT | O_RDWR, 0644);
    int fd = open(archive, O_CREAT | O_RDONLY, 0644); 
    if (fd == -1 || new_fd == -1)
    {
        print_err(ERR_OPEN);
    }

    read_header(fd);

    if (fnums == 0)
    {
        close(fd);
        close(new_fd);
        free_fi_list();
        print_err(ERR_INPUT_NOFILES);
    }

    struct stat stat_file;
    fi_list_t* app_files_start
        = 0; // Позиция в связном списке, откуда начинается набор новых фильмов
    int inserted_files = 0;

    for (int i = 0; i < fnums; ++i)
    {
        if (stat(fnames[i], &stat_file) == -1)
        {
            fprintf(stderr,
                "[archiver]: Ошибка при вставке файла \"%s\": %s. Пропущено\n",
                fnames[i], strerror(errno));
            continue;
        }

        if (!S_ISREG(stat_file.st_mode))
        {
            fprintf(stderr,
                "[archiver]: Ошибка при вставке файла \"%s\": Файл не "
                "регулярный. Пропущено\n",
                fnames[i]);
            continue;
        }

        file_info_t fi;
        memcpy(fi.filename, fnames[i], strlen(fnames[i]));
        
        // Добавляем размер файла
        {
            // Мы открывали файл раньше, так что проверок не делаем
            int _offset_fd = open(fnames[i], O_RDONLY);
            fi.filesize = lseek(_offset_fd, 0, SEEK_END);
            close(_offset_fd);
            printf("dbg: %s %lu\n", fnames[i], fi.filesize);
        }

        fi.mask = stat_file.st_mode & 0777;
        fi._offset = 0; // Будет добавлено позднее в коде
       
        fi_list_t *n = make_new_node_in_global_fi_list();
        memcpy(&n->data, &fi, sizeof(file_info_t));
        if (!app_files_start) app_files_start = n;
        inserted_files++;
    }

    if (inserted_files == 0)
    {
        close(fd);
        close(new_fd);
        free_fi_list();
        print_err(ERR_INPUT_NOAPP);
    }

    // Обновление оффсетов в global_fi_list
    {
        uint64_t res_offset;
        for (fi_list_t *i = global_fi_list; i != app_files_start; i = i->next)
        {
            res_offset += i->data.filesize;
            i->data._offset += sizeof(file_info_t) * inserted_files;
        }

        for (fi_list_t *i = app_files_start; i != NULL; i = i->next)
        {
            i->data._offset = sizeof(file_info_t) * inserted_files + res_offset;
            res_offset += i->data.filesize;
        }
    }

    // Добавляем новую инфу в temp
    {
        for (fi_list_t *i = global_fi_list; i != NULL; i = i->next)
        {
            write(new_fd, &i->data, sizeof(file_info_t));
        }

        // Пишем заголовок
        uint32_t t = 0;
        write(new_fd, &t, HEADER_ENDING_SIZE);
        
        lseek(fd, 0, SEEK_SET);
        char buf[BUFFER_SIZE];

        while (read(fd, buf, BUFFER_SIZE) > 0)
        {
            write(new_fd, buf, BUFFER_SIZE);
        }
        lseek(fd, 0, SEEK_END);

        for (int i = 0; i < fnums; ++i)
        {
            int app_fd = open(fnames[i], O_RDONLY);
            if (fd == -1) continue;
            while (read(app_fd, buf, BUFFER_SIZE) > 0)
            {
                write(new_fd, buf, BUFFER_SIZE);
            }
            close(app_fd);
        }
    }

    remove(archive);
    rename(new_fd_name,
        archive); // странно, но это работает
                  // Если мы назовем файл "temp/nice.txt", то все сработает

    close(fd);
    close(new_fd);
    free_fi_list();
}

void read_header(int arch_fd)
{
    uint32_t header_ending = 0, buf;
    for (;;)
    {
        // check for header ending
        if (read(arch_fd, &buf, HEADER_ENDING_SIZE) == 0) 
        {
            if (!errno) break; // empty file 
            else
            {
                close(arch_fd);
                free_fi_list();
                perror("err: ");
                exit(EXIT_FAILURE);
            }
        }

        // Здесь, нас не заботит byte order, поскольку значение 0x0 симметрично
        // Если условие выполняется, заголовок кончился
        if (header_ending == buf)
            break;
        // Возвращаемся назад...
        lseek(arch_fd, -HEADER_ENDING_SIZE, SEEK_CUR); 

        // ... И читаем file_info
        fi_list_t *info = make_new_node_in_global_fi_list();

        read(arch_fd, &info->data, sizeof(file_info_t));
    }
    lseek(arch_fd, 0, SEEK_SET);
}

void free_fi_list()
{
    for (fi_list_t *i = global_fi_list; i != NULL; )
    {
        fi_list_t *d = i->next;
        free(i);
        i = d;
    }
}

fi_list_t* end_global_fi_list()
{
    fi_list_t* i = global_fi_list;
    if (!i)
        return i;

    for (; i->next != NULL; i = i->next);
    return i;
}

fi_list_t* make_new_node_in_global_fi_list()
{

    fi_list_t* info = end_global_fi_list();
    if (!info)
    {
        global_fi_list = malloc(sizeof(fi_list_t));
        info = global_fi_list;
    }
    else
    {
        info->next = malloc(sizeof(fi_list_t));
        info = info->next;
    }
    info->next = NULL;
    return info;
}
