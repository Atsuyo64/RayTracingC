#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "objloader.h"

int main(int argc, char const *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "%s takes an .obj file as parametter\n", argv[0]);
        exit(1);
    }
    loadObj(argv[1]);
    return 0;
}

