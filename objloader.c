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
 * UNSUPPORTED: Textures, space vertices (vp), vertex normals (vn) & vertex textures (vn), f sqares (x/x/x/x), Line elements, smooth shading (s), [w] coordinates (in v)
 **/

char stage[7] = "....xx.";
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

typedef struct ObjVert
{
    float x;
    float y;
    float z;
} ObjVert;

typedef struct ObjFace
{
    unsigned int x;
    unsigned int y;
    unsigned int z;
} ObjFace;

ObjVert *globalVerts;
ObjFace *globalFaces;
int maxVerticesPerObject = 0;
int maxFacesPerObject = 0;
int totalFaces = 0;

/**
 * Allocate Memory
 * @param filename
 *
 */
int allocateMemory(const char *filename)
{
#if PRINT_LOADING >= 2
    printf("Allocating memory...");
    fflush(stdout);
#endif
    maxVerticesPerObject = 0;
    maxFacesPerObject = 0;

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(filename, "r");
    if (fp == NULL)
    {
#if PRINT_LOADING > 0
        printf("ERROR: could not evaluate memory allocation.\n");
#endif
        exit(1);
    }
    int totalObj = 0;
    int totalVert = 0;
    totalFaces = 0;
    int vertPerObject = 0;
    int facesPerObject = 0;
    while ((read = getline(&line, &len, fp)) != -1)
    {
        if (read == 1 || line[0] == '#')
            continue;
        if (strncmp(line, "v ", 2) == 0)
            vertPerObject++;
        else if (strncmp(line, "f ", 2) == 0)
            facesPerObject++;
        else if (strncmp(line, "o ", 2) == 0)
        {
            totalObj++;
            if (vertPerObject > maxVerticesPerObject)
                maxVerticesPerObject = vertPerObject;
            if (facesPerObject > maxFacesPerObject)
                maxFacesPerObject = facesPerObject;
            totalVert += vertPerObject;
            totalFaces += facesPerObject;
            facesPerObject = 0;
            vertPerObject = 0;
        }
    }
    if (vertPerObject > maxVerticesPerObject)
        maxVerticesPerObject = vertPerObject;
    if (facesPerObject > maxFacesPerObject)
        maxFacesPerObject = facesPerObject;
    totalVert += vertPerObject;
    totalFaces += facesPerObject;

    fclose(fp);
    if (line)
        free(line);

    // allocating max+1 because OBJ start indexing at 1
    globalVerts = malloc((maxVerticesPerObject+1)*sizeof(ObjVert));
    globalFaces = malloc((maxFacesPerObject+1)*sizeof(ObjFace));
    
#if PRINT_LOADING >= 2
    printf("\rAllocated memory: objects=%d maxVertices=%d maxFaces=%d (total: v=%d f=%d)\n", totalObj, maxVerticesPerObject, maxFacesPerObject, totalVert, totalFaces);
#endif
    return 0;
}

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
        if (read == 1 || line[0] == '#')
            continue;
    }

    fclose(fp);
    if (line)
        free(line);
    return 0;
}

/**
 * loadObj function
 * @param filename the name of the.obj file
 * @param triangles a null pointer on which will be allocated array of triangles
 * @param count of triangle stored in triangles
 * 
 * TODO: Think to free() your triangles when they are unused /!\
 *
 * @return 0 if success, 1 if file not found
 */
int loadObj(const char *filename, OBJTriangle **triangles, int *count)
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

    allocateMemory(filename);
    // list allocated
    *triangles = malloc((totalFaces)*sizeof(OBJTriangle));
    *count = totalFaces;

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
    free(globalVerts);
    free(globalFaces);
    return 0;
}