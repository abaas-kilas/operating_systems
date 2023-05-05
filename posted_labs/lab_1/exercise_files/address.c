#include "include/main.h"
#include "include/address.h"

void getAddress(char * address) {
    printf("Enter your address: ");
    fgets(address, maxLen, stdin);
    fflush(stdin);
}