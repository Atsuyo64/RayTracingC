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
 * UNSUPPORTED: Textures, space vertices (vp), f sqares, Line elements, smooth shading, [w] coordinates
 **/

char stage[7] = "xxxxxxx";
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
    stage[newState] = '~';

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
#if PRINT_LOADING >= 3
        if (oldState != newState)
        {
            stage[oldState] = 'v';
            stage[newState] = '~';
            oldState = newState;
            printf("\rinit|matload|object|vert|norm|text|face: %c|%c|%c|%c|%c|%c|%c", stage[0], stage[1], stage[2], stage[3], stage[4], stage[5], stage[6]);
            fflush(stdout);
        }
#endif
        // Skip line if comment or empty
        if (read == 1 || line[0] == '#')
            continue;

        // define material file
        if (strncmp(line, "mtllib ", 7) == 0)
        {
            char mtlFileName[256];
            if (ret = sscanf(line, "mtllib %s", mtlFileName))
            {
                strcat(textureFile, mtlFileName);
                loadMtl(textureFile);
                newState = RS_MATLOAD;
            }
        }

        // define object name
        if (strncmp(line, "o ", 2) == 0)
        {
            newState = RS_OBJECT;
            char objectName[256];
            if (ret = sscanf(line, "o %s", objectName))
            {
#if PRINT_LOADING >= 3
                printf("\nNEW OBJECT: %s\n", objectName);
#endif
            }
        }

        // define vertices
        if (strncmp(line, "v ", 2) == 0)
        {
            newState = RS_VERTICES;
            char mtlFileName[256];
            if (ret = sscanf(line, "v %s", mtlFileName))
            {
            }
        }
    }

    fclose(fp);
    if (line)
        free(line);

#if PRINT_LOADING >= 2
    printf("\nFile reading ended.\n");
#endif
    return 1;
}