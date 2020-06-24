#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void) {

    char *strMem = strdup("hello");

    char *s = strMem;

    while(*s++) {

        putchar(*s);
    }
    putchar('\n');

    free(strMem);

    return 0;
}
