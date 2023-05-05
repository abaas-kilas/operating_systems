#include "include/main.h"
#include "include/major.h"

void getMajor(char * major) {
    printf("Enter your major: ");
    fgets(major, maxLen, stdin);
    fflush(stdin);
}