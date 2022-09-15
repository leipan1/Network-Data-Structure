#include <stdio.h>
#include "sfmm.h"

int main(int argc, char const *argv[]) {
    sf_set_magic(0x0);//DELETE!!
    double* ptr = sf_malloc(sizeof(double));
    *ptr = 320320320e-320;

    printf("%f\n", *ptr);

    sf_free(ptr);


    return EXIT_SUCCESS;
}
