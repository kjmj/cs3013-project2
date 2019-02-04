#include<stdio.h>
#include <stdlib.h>

int main() {
    FILE *ptr_file;
    char buf[1000];

    ptr_file = fopen("input.txt", "r");
    if (!ptr_file) {
        printf("There is an error opening/ finding the error");
        return 1;
    }

    while (fgets(buf, 1000, ptr_file) != NULL)
        printf("%s", buf);
    printf("\n");
    fclose(ptr_file);
    return 0;
}