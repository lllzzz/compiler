#include<stdio.h>

void main()
{
    char *str[] = {"abc", "ccc"};
    int i = 0;
    for (i = 0; i < 2; i++) {
        puts(str[i]);
        printf("%s\n", str[i]);
    }
}
