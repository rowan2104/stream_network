//Created by Rowan Barr
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

long int cMemory = 0;

void print_memUse(char * string) {
    struct rusage r_usage;
    getrusage(RUSAGE_SELF, &r_usage);
    if (cMemory == 0 || cMemory > r_usage.ru_maxrss) {
        printf("Memory usage: %ld KB\n", r_usage.ru_maxrss);
        cMemory = r_usage.ru_maxrss;
    }
    return;
}