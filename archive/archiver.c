#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define FILENAME_LENGTH 256
#define HEADER_ENDING_SIZE 4
#define BUFFER_SIZE 1024
#define HR_FS_BUFFER_SIZE 20

/**
 * \mainpage archiver.c - простой архиватор
 *
 * Архив имеет следующую структуру: вначале последовательно идут структуры
 * file_info, после этого - содержимое файлов, идущее подряд. Конец заголовка -
 * 4 нулевых байтов (0000 0000). Для эффективной работы с заголовком архива
 * (структуры file_info вначале) существует связный список fi_list
 *
 * При добавлении (insert) информация о файле добавляется в конец заголовка, а
 * данные добавляются в таком же порядке в конец файла
 *
 * __Ограничения__:
 * * Файл должен быть регулярным
 * * Файл должен идти на вход программы не по пути - только имя с расширением
 * Возможно, это будет сделано в будущем
 * * Имена файлов не должны повторяться (ничего не сломается, просто не надо)
 *
 *
 * \todo Сделать проверку на уникальность файла, который мы вставляем
 * \todo Сделать вставку контроллирующей последовательности в начало архива,
 * чтобы программа могла отличать файлы-архивы от файлов-неархивов
 * \todo Добавить независимость от endianness процессора (для структуры
 * file_info)
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
 * fi_list->eof  - конец связного списка
 */
struct fi_list
{
    file_info_t data;
    struct fi_list *next;
    char eof; //!< Конец списка
};
typedef struct fi_list fi_list_t;

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
    ERR_INPUT_NOAPP //!< Никакой из новых файлов не вставлен
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
 * Читает заголовок (см. документацию \ref index "к основной странице"), заполняя
 * связный список head. После устанавливает указатель в файле на начало (rewind)
 * \param head Указатель на начало связного списка
 * \param arch_fd Файловый дескриптор архива
 */
void read_header(fi_list_t *head, int arch_fd);

/**
 * Очищает связный список head
 */
void free_fi_list(fi_list_t *head);

/**
 * Возвращает конец связного списка head
 *
 * \return Указатель на конец global_fi_list или NULL, если не инициализировано
 */
fi_list_t *end_fi_list(fi_list_t *head);

/**
 * Создает пустую ноду в конце связного списка head
 *
 * \return Новую ноду в конце head 
 */
fi_list_t *make_new_node_in_fi_list(fi_list_t *head);

/**
 * Создает связный список
 */
fi_list_t *init_fi_list();

/**
 * Обновляет заголовок, вставляя информацию о файлах fnames в конец заголовка
 * \param Указатель на начало связного списка - заголовка архива
 * \param start Указатель на переменную, куда функция запишет позицию в списке,
 * откуда начинаются новые файлы. Если start == 0, то ничего не будет записано
 * \return количество вставленных файлов
 */
int update_header_for_input(
    fi_list_t* header, int fnums, char** fnames, fi_list_t** start);

/**
 * Обновляет оффсеты в заголовке
 */
void update_offsets_in_header_for_input(fi_list_t *header);

/**
 * Непосредственно вставляет файлы в архив
 *
 * Вставка происходит посредством создания нового файла и копирования туда всего
 * содержимого из старого архива
 * \param header Заголовок архива
 * \param oldfd Файловый дескриптор открытого архива
 * \param newfd Файловый дескриптор нового архива, куда будут добавлены файлы
 * \param fnums Количество файлов
 * \param fnames Массив названий файлов, которые необходимо добавить в архив
 */
void insert_files_routine(fi_list_t *header, int oldfd, int newfd, int fnums, char **fnames);

/**
 * "Human-readable" размер файла
 *
 * Перевести количество байт в нормальный, понятный для человека вид. Сохраняет
 * строку в буффер buf
 *
 * \param buf Статический массив длины HR_FS_BUFFER_SIZE (включая '\0'), в
 * который будет сохраняться строка
 */
void hr_file_size(uint64_t s, char buf[HR_FS_BUFFER_SIZE]);

/**
 * Выводит статистику файла в стандартный поток вывода
 */
void stat_archive(char *archive);

/**
 * Получить файлы из архива
 * \param archive Название архива
 * \param fnums Количество файлов, которые надо извлечь. Если передан 0, то будет извлечен весь архив
 * \param fnames Массив файлов, которые необходимо извлачь
 */
void extract_files(char *archive, int fnums, char **fnames);

/**
 * Удаляет ноду, следующую за _to_ из связного списка _h_
 * \warning Перед использованием функции добавьте фиктивный узел в начало
 * списка, и передавайте этот фиктивный узел в _h_
 *
 * \code
 * fi_list_t *header = init_fi_list();
 * //...
 * fi_list_t fictive;
 * fictive.next = header;
 * for (fi_list_t *i = header; !i->eof; i = i->next)
 * {
 *     if (something_wrong_with(i))
 *     {
 *         remove_node_from_fi_list(&fictive, i);
 *         header = fictive.next;
 *     }
 * }
 *
 * \endcode
 * \param h - голова связного списка
 * \param to - нода, предыдущая той, которую надо удалить или нода, следующая за
 * головой, если is_head == 1
 * \return 0, если _to_ был удален из h, -1 - иначе
 */
int remove_node_from_fi_list(fi_list_t* h, fi_list_t* to);

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

        case MODE_STAT:
            stat_archive(argv[1]);
            break;

        case MODE_EXTRACT:
            extract_files(argv[1], argc-3, argv+3);
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
        return MODE_REMOVE;
    }

    if (strcmp(argv[2], "-s") == 0 || strcmp(argv[2], "--stat") == 0)
    {
        return MODE_STAT;
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

    int new_fd = open(new_fd_name, O_CREAT | O_RDWR, 0666);
    int fd = open(archive, O_CREAT | O_RDONLY, 0666); 
    if (fd == -1 || new_fd == -1)
    {
        print_err(ERR_OPEN);
    }

    fi_list_t *new_header = init_fi_list();
    read_header(new_header, fd);

    if (fnums == 0)
    {
        close(fd);
        close(new_fd);
        free_fi_list(new_header);
        print_err(ERR_INPUT_NOFILES);
    }

    if (update_header_for_input(new_header, fnums, fnames, 0) == 0)
    {
        close(fd);
        close(new_fd);
        free_fi_list(new_header);
        print_err(ERR_INPUT_NOAPP);
    }

    update_offsets_in_header_for_input(new_header);
    insert_files_routine(new_header, fd, new_fd, fnums, fnames);

    remove(archive);
    rename(new_fd_name,
        archive); // странно, но это работает
                  // Если мы назовем файл "temp/nice.txt", то все сработает

    close(fd);
    close(new_fd);
    free_fi_list(new_header);
}

void read_header(fi_list_t *header, int arch_fd)
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
                free_fi_list(header);
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
        fi_list_t *info = make_new_node_in_fi_list(header);

        read(arch_fd, &info->data, sizeof(file_info_t));
    }
    lseek(arch_fd, 0, SEEK_SET);
}

void free_fi_list(fi_list_t *header)
{
    for (fi_list_t *i = header; i != NULL; ) // тут именно != NULL, а не !eof
    {
        fi_list_t *d = i->next;
        free(i);
        i = d;
    }
}

fi_list_t* end_fi_list(fi_list_t *header)
{
    fi_list_t* i = header;
    if (!i)
        return i;

    for (; !i->eof; i = i->next);
    return i;
}

fi_list_t* make_new_node_in_fi_list(fi_list_t *h)
{
    fi_list_t *i = h;
    for (; !i->eof; i = i->next);

    i->next = init_fi_list();
    i->eof = 0;
    return i;
}

int update_header_for_input(fi_list_t *header, int fnums, char **fnames, fi_list_t ** start)
{
    int inserted_files = 0; 
    struct stat stat_file;
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
        memcpy(fi.filename, fnames[i], strlen(fnames[i])+1);
        
        // Добавляем размер файла
        {
            // Мы открывали файл раньше, так что проверок не делаем
            int _offset_fd = open(fnames[i], O_RDONLY);
            fi.filesize = lseek(_offset_fd, 0, SEEK_END);
            close(_offset_fd);
        }

        fi.mask = stat_file.st_mode & 0777;
        fi._offset = 0; // Будет добавлено позднее в коде
       
        fi_list_t *n = make_new_node_in_fi_list(header);

        if (start != NULL && *start == 0) *start = n;
        
        memcpy(&n->data, &fi, sizeof(file_info_t));
        inserted_files++;
    }

    return inserted_files;
}

void insert_files_routine(fi_list_t *header, int fd, int new_fd, int fnums, char** fnames)
{
    for (fi_list_t* i = header; !i->eof; i = i->next)
    {
        write(new_fd, &i->data, sizeof(file_info_t));
    }

    // Пишем заголовок
    uint32_t t = 0;
    write(new_fd, &t, HEADER_ENDING_SIZE);

    // Читаем старый архив
    fi_list_t *oldh = init_fi_list();
    read_header(oldh, fd);
    uint64_t off = oldh->data._offset;
    free_fi_list(oldh);

    // Перемещение к началу сырых данных
    if (lseek(fd, 0, SEEK_END) != 0) // Проверка на непустой массив
    {
        lseek(fd, off, SEEK_SET);
    }
    char buf[BUFFER_SIZE];
    ssize_t reads = 0;

    while ((reads = read(fd, buf, BUFFER_SIZE)) > 0)
    {
        write(new_fd, buf, reads);
    }

    for (int i = 0; i < fnums; ++i)
    {
        int app_fd = open(fnames[i], O_RDONLY);
        if (fd == -1)
            continue;
        while ((reads = read(app_fd, buf, BUFFER_SIZE)) > 0)
        {
            write(new_fd, buf, reads);
        }
        close(app_fd);
    }
}

void update_offsets_in_header_for_input(fi_list_t* header)
{
    uint64_t raw_off = 0, total_header_nodes = 0;

    for (fi_list_t* i = header; !i->eof;
        i = i->next, total_header_nodes++);

    for (fi_list_t* i = header; !i->eof; i = i->next)
    {
        i->data._offset = sizeof(file_info_t) * total_header_nodes
            + HEADER_ENDING_SIZE + raw_off;
        raw_off += i->data.filesize;
    }
}

fi_list_t *init_fi_list()
{
    fi_list_t *t = malloc(sizeof(fi_list_t));
    t->next = NULL;
    t->eof = 1;
    return t;
}

void stat_archive(char *archive)
{
    int fd = open(archive, O_RDONLY);     
    if (fd == -1)
    {
        print_err(ERR_OPEN);
    }

    fi_list_t *h = init_fi_list();
    read_header(h, fd);

    // выравнивание
    unsigned int name_align = 4, size_align = 6;
    for (fi_list_t *i = h; !i->eof; i = i->next)
    {
        char buf[HR_FS_BUFFER_SIZE];
        hr_file_size(i->data.filesize, buf);
        size_align = size_align > strlen(buf) ? size_align : strlen(buf);
        name_align = name_align > strlen((char*)i->data.filename)
            ? size_align
            : strlen((char*)i->data.filename);
    }


    printf("--- Архив: %s ---\n", archive);
    printf("%-*s %-*s\n", name_align+4, "Файл", size_align+6, "Размер");
    for (fi_list_t* i = h; !i->eof; i = i->next)
    {
        char buf[HR_FS_BUFFER_SIZE];
        hr_file_size(i->data.filesize, buf);
        printf("%-*s %-*s\n", name_align, i->data.filename, size_align, buf);
    }
}

void hr_file_size(uint64_t s, char buf[HR_FS_BUFFER_SIZE])
{
    memset(buf, '\0', HR_FS_BUFFER_SIZE);
    if (s < 512)
    {
        snprintf(buf, HR_FS_BUFFER_SIZE, "%luБ", s);
    }
    else if (s < 1024*512)
    {
        snprintf(buf, HR_FS_BUFFER_SIZE, "%.2fКБ", s/1024.f);
    }
    else if (s < 1024*1024*512)
    {
        snprintf(buf, HR_FS_BUFFER_SIZE, "%.2fMБ", s/1024.f/1024.f);
    }
    else if (s < 1024*1024*1024*512ll)
    {
        snprintf(buf, HR_FS_BUFFER_SIZE, "%.2fГБ", s/1024.f/1024.f/1024.f);
    }

}

void extract_files(char *archive, int fnums, char **fnames)
{
    int fd = open(archive, O_RDONLY);     
    if (fd == -1)
    {
        print_err(ERR_OPEN);
    }

    fi_list_t *header = init_fi_list();
    read_header(header, fd);
    
    if (fnums != 0)
    {
        fi_list_t fictive;
        fictive.eof = 0;
        fictive.next = header;
        for (fi_list_t *i = &fictive; !i->next->eof; )
        {
            char is_there = 0;
            for (int j = 0; j < fnums; ++j)
                is_there |= strcmp((const char*)i->next->data.filename, fnames[j]) == 0;

            if (!is_there)
            {
                remove_node_from_fi_list(&fictive, i);
                header = fictive.next;
                continue;
            }
            i = i->next;
        }
    }
    
    for (fi_list_t *i = header; !i->eof; i = i->next)
    {
        printf("%s\n", i->data.filename);
        int new_fd = open((const char *)i->data.filename, O_CREAT | O_WRONLY);
        if (new_fd == -1)
        {
            fprintf(
                stderr, "[archiver]: Ошибка! %s. Пропущено\n", strerror(errno));
            continue;
        }
        chmod((const char *)i->data.filename, i->data.mask);
        lseek(fd, i->data._offset, SEEK_SET);

        uint64_t bytes = i->data.filesize;
        char buf[BUFFER_SIZE];
        while (bytes > BUFFER_SIZE)
        {
            read(fd, buf, BUFFER_SIZE);
            write(new_fd, buf, BUFFER_SIZE);
            bytes -= BUFFER_SIZE;
        }

        if (bytes > 0)
        {
            read(fd, buf, bytes);
            write(new_fd, buf, bytes);
        }

        close(fd);
    }


    free_fi_list(header);
    close(fd);
}

int remove_node_from_fi_list(fi_list_t *h, fi_list_t *to)
{
    if (h->eof || to->eof) return -1; // Мы не хотим удалить конец, сломав список
    
    fi_list_t *to_del = to->next;
    to->next = to->next->next;
    free(to_del);
    return 0;
}

