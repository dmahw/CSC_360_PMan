#include <stdio.h>

int main() {
    unsigned long long int first = 0;
    unsigned long long int second = 0;
    unsigned long long int next;
    unsigned long long int c;
    unsigned long long int n = 1000000000000;

    for (c = 0; c< n; c++) {
        if(c <= 1)
            next = c;
            else {
                next = first + second;
                first = second;
                second = next;
            }
    }
    printf("Fibbonacci %llu\n", next);
    
    return 0;
}