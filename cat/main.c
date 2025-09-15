#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

FILE *file = NULL;

void _close_file()
{
    fclose(file);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "[cat]: Not enough args!\n");
        return 1;
    }

    // --- file opening ---
    file = fopen(argv[1], "r");
    if (file == NULL)
    {
        fprintf(stderr, "[cat]: Error during open file %s: %s\n", argv[1],
                strerror(errno));
        return 1;
    }
    atexit(_close_file);
    // --------------------
    
    for (char ch; (ch = fgetc(file)) != EOF; )
    {
        putchar(ch);
    }
    return 0;
}
