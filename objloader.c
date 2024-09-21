#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <libgen.h>
#include <string.h>
#include <stddef.h>
#include <sys/types.h>
#include "objloader.h"
/**
 * 0 display nothing
 * 1 display errors/warnings
 * 2 display read objects
 * 3 display object loading
 * 4 display read lines
 * */
#define PRINT_LOADING 3

/**
 * Object loader minimal lib
 *
 * UNSUPPORTED: Textures, space vertices (vp), f sqares, Line elements, smooth shading
 **/

enum reading_stage
{
    RS_INITIAL,
    RS_MATLOAD,
    RS_OBJECT,
    RS_VERTICES,
    RS_VERNORM,
    RS_VERTEXT,
    RS_FACE
};



/**
 * loadMtl function
 * @param filename
 *
 * @return 0 if success, 1 if file not found
 */
int loadMtl(const char *filename)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int ret;

    enum reading_stage oldState = RS_INITIAL;
    enum reading_stage newState = RS_INITIAL;

#if PRINT_LOADING > 1
    printf("Loading material %s...\n", filename);
#endif

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
#if PRINT_LOADING > 0
        printf("WARNING: No material found.\n");
#endif
        return 1;
    }

    while ((read = getline(&line, &len, fp)) != -1)
    {
#if PRINT_LOADING > 3
        printf("Retrieved line of length %zu:\n", read);
        printf("%s", line);
#endif
    }

    fclose(fp);
    if (line)
        free(line);
    return 0;
}

/**
 * loadObj function
 * @param filename
 *
 * @return 0 if success, 1 if file not found
 */
int loadObj(const char *filename)
{
    char filenamemod[512];
    strcpy(filenamemod, filename);
    char *fileDirectory = dirname(filenamemod);
    char *textureFile = strcat(fileDirectory, "/");

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int ret;

    enum reading_stage oldState = RS_INITIAL;
    enum reading_stage newState = RS_INITIAL;

#if PRINT_LOADING >= 2
    printf("Loading %s...\n", filename);
#endif

    fp = fopen(filename, "r");
    if (fp == NULL)
        return 1;

    while ((read = getline(&line, &len, fp)) != -1)
    {
#if PRINT_LOADING >= 4
        printf("Retrieved line of length %zu:\n", read);
        printf("%s", line);
#endif

        // define material file
        if (strncmp(line, "mtllib ", 7) == 0)
        {
            char mtlFileName[256];
            if (ret = sscanf(line, "mtllib %s", mtlFileName)) {
                strcat(textureFile, mtlFileName);
                loadMtl(textureFile);
            }
        }
    }

    fclose(fp);
    if (line)
        free(line);

#if PRINT_LOADING >= 2
    printf("File reading ended.\n");
#endif
    return 1;
}