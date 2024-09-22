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
 * UNSUPPORTED: Textures, space vertices (vp) & vertex textures (vn), f sqares (x/x/x/x), Line elements, smooth shading (s), [w] coordinates (in v)
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

const ObjVert DEFAULT_COLOR = {
    1.,
    1.,
    1.,
};

ObjVert *globalVerts;
ObjVert *globalNorms;

int maxVerticesPerObject = 0;
int maxNormsPerObject = 0;
int maxFacesPerObject = 0;
int totalFaces = 0;

#ifdef _WIN32
//getline is POSIX-only so we have to make our own

ssize_t getline(char** buffer,size_t* len,FILE* file)
{
   if(*buffer==NULL || *len == 0)
   {
      if((*buffer=malloc(BUFSIZ))==NULL)
         return -1;
      *len=BUFSIZ;
   }
   char* ptr = *buffer;
   char* endptr = *buffer + *len;
   while(1)
   {
      int c = fgetc(file);
      if(c==-1)
      {
         if(feof(file))
            return ptr == *buffer ? -1 : ptr - *buffer;
         else
            return -1;
      }
      
      *ptr = c;
      if(c=='\n')
      {
         *ptr = '\0';
         return ptr - *buffer;
      }
      
      if(ptr+2>=endptr)
      {
         char* newbuffer;
         size_t newsize = *len * 2;
         ssize_t span = ptr - *buffer;
         if((newbuffer = realloc(*buffer,newsize))==NULL)
            return -1;
         *buffer = newbuffer;
         *len = newsize;
         ptr = *buffer + span;
         endptr = *buffer + *len;
      }
   }
}
#endif


/**
 * Print OBJTriangle
 * @param triangle
 *
 */
void printOBJTriangle(OBJTriangle triangle)
{
    // printf("\nTRIANGLE:\nA(%f,%f,%f) ; Y(%f,%f,%f) ; C(%f,%f,%f)\n", triangle.pos[0][0], triangle.pos[0][1], triangle.pos[0][2], triangle.pos[1][0], triangle.pos[1][1], triangle.pos[1][2], triangle.pos[2][0], triangle.pos[2][1], triangle.pos[2][2]);
    printf("\nTRIANGLE:\nA(%f,%f,%f) ; B(%f,%f,%f) ; C(%f,%f,%f)\n", triangle.posA.x, triangle.posA.y, triangle.posA.z, triangle.posB.x, triangle.posB.y, triangle.posB.z, triangle.posC.x, triangle.posC.y, triangle.posC.z);
    printf("Normal=(%f,%f,%f)\n", triangle.normal.x, triangle.normal.y, triangle.normal.z);
    printf("SHADING: Color=(%f,%f,%f) emission=%f, smoothness=%f\n\n", triangle.color.x, triangle.color.y, triangle.color.z, triangle.emission, triangle.smoothness);
}

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
    int totalNorm = 0;
    totalFaces = 0;
    int vertPerObject = 0;
    int normsPerObject = 0;
    int facesPerObject = 0;
    while ((read = getline(&line, &len, fp)) != -1)
    {
        if (read == 1 || line[0] == '#')
            continue;
        if (strncmp(line, "v ", 2) == 0)
            vertPerObject++;
        if (strncmp(line, "vn ", 3) == 0)
            normsPerObject++;
        else if (strncmp(line, "f ", 2) == 0)
            facesPerObject++;
        else if (strncmp(line, "o ", 2) == 0)
        {
            totalObj++;
            if (vertPerObject > maxVerticesPerObject)
                maxVerticesPerObject = vertPerObject;
            if (normsPerObject > maxNormsPerObject)
                maxNormsPerObject = normsPerObject;
            if (facesPerObject > maxFacesPerObject)
                maxFacesPerObject = facesPerObject;
            totalVert += vertPerObject;
            totalNorm += normsPerObject;
            totalFaces += facesPerObject;
            facesPerObject = 0;
            normsPerObject = 0;
            vertPerObject = 0;
        }
    }
    if (vertPerObject > maxVerticesPerObject)
        maxVerticesPerObject = vertPerObject;
    if (normsPerObject > maxNormsPerObject)
        maxNormsPerObject = normsPerObject;
    if (facesPerObject > maxFacesPerObject)
        maxFacesPerObject = facesPerObject;
    totalVert += vertPerObject;
    totalNorm += normsPerObject;
    totalFaces += facesPerObject;

   fclose(fp);
   if (line)
      free(line);

    // allocating max+1 because OBJ start indexing at 1
    globalVerts = malloc((maxVerticesPerObject + 1) * sizeof(ObjVert));
    globalNorms = malloc((maxNormsPerObject + 1) * sizeof(ObjVert));

#if PRINT_LOADING >= 2
    printf("\rAllocated memory: objects=%d maxVertices=%d maxNormals=%d maxFaces=%d (total: v=%d n=%d f=%d)\n", totalObj, maxVerticesPerObject, maxNormsPerObject, maxFacesPerObject, totalVert, totalNorm, totalFaces);
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
 * objTriangleArrayFree
 * @param arr of triangles
 * @param count of triangles
 *
 * frees array of Triangles
 */
void objTriangleArrayFree(OBJTriangle **triangles, int count)
{
    for (size_t i = 0; i < count; i++)
    {
        free(triangles[i]);
    }
    free(triangles);
}

/**
 * loadObj function
 * @param filename the name of the.obj file
 * @param triangles a null pointer on which will be allocated array of triangles
 * @param count of triangle stored in triangles
 *
 * TODO: Think to use objTriangleArrayFree() your triangle arrat when you have finished to use it /!\
 *
 * @return 0 if success, 1 if file not found
 */
int loadObj(const char *filename, OBJTriangle ***triangles, int *count)
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
    int lineNb = 0;

   enum reading_stage oldState = RS_INITIAL;
   enum reading_stage newState = RS_INITIAL;

#if PRINT_LOADING >= 2
   printf("Loading %s...\n", filename);
#endif

   fp = fopen(filename, "r");
   if (fp == NULL)
      return 1;

    allocateMemory(filename);
    // list allocation
    // triangles = malloc((totalFaces) * sizeof(OBJTriangle));
    (*triangles) = malloc((totalFaces) * sizeof(OBJTriangle *));
    for (size_t i = 0; i < totalFaces; i++)
    {
        (*triangles)[i] = malloc(sizeof(OBJTriangle));
    }
    *count = totalFaces;

    int vertPerObject = 0;
    int normPerObject = 0;
    int numberOfTriangles = 0;

    while ((read = getline(&line, &len, fp)) != -1)
    {
        lineNb++;
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
                vertPerObject = 0;
                normPerObject = 0;
#if PRINT_LOADING >= 2
                printf("\nNEW OBJECT: %s\n", objectName);
#endif
         }
      }

        // define vertices
        if (strncmp(line, "v ", 2) == 0)
        {
            // float vx, vy, vz;
            newState = RS_VERTICES;
            vertPerObject++;
            if (ret = sscanf(line, "v %f %f %f", &globalVerts[vertPerObject].x, &globalVerts[vertPerObject].y, &globalVerts[vertPerObject].z))
            {
                // printf("V: %d (%f, %f, %f)\n", vertPerObject , globalVerts[vertPerObject].x, globalVerts[vertPerObject].y, globalVerts[vertPerObject].z);
            }
            else
            {
                fprintf(stderr, "\n# ERROR while reading %dth vertex (line %d)\n", vertPerObject, lineNb);
            }
        }

        // define vertex normals
        if (strncmp(line, "vn ", 3) == 0)
        {
            float vx, vy, vz;
            newState = RS_VERTICES;
            normPerObject++;
            if (ret = sscanf(line, "vn %f %f %f", &globalNorms[normPerObject].x, &globalNorms[normPerObject].y, &globalNorms[normPerObject].z))
            {
                // printf("VN: %d (%f, %f, %f)\n", normPerObject, globalNorms[normPerObject].x, globalNorms[normPerObject].y, globalNorms[normPerObject].z);
            }
            else
            {
                fprintf(stderr, "\n# ERROR while reading %dth vertex normal (line %d)\n", normPerObject, lineNb);
            }
        }

        // define faces
        if (strncmp(line, "f ", 2) == 0)
        {
            int av, at, an, bv, bt, bn, cv, ct, cn;

            newState = RS_FACE;
            if (ret = sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", &av, &at, &an, &bv, &bt, &bn, &cv, &ct, &cn))
            {
                // OBJTriangle t = {0};
                // t.smoothness = .5;
                // printf("av=%d, at=%d, an=%d, bv=%d, bt=%d, bn=%d, cv=%d, ct=%d, cn=%d\n", av, at, an, bv, bt, bn, cv, ct, cn);
                // printf("B: %f, %f, %f\n", globalVerts[bv].x, globalVerts[bv].y, globalVerts[bv].z);
                memcpy(&(*triangles)[numberOfTriangles]->posA, &globalVerts[av], sizeof(ObjVert));
                memcpy(&(*triangles)[numberOfTriangles]->posB, &globalVerts[bv], sizeof(ObjVert));

                memcpy(&(*triangles)[numberOfTriangles]->posC, &globalVerts[cv], sizeof(ObjVert));
                memcpy(&(*triangles)[numberOfTriangles]->normal, &globalNorms[an], sizeof(ObjVert));

                memcpy(&(*triangles)[numberOfTriangles]->color, &DEFAULT_COLOR, sizeof(ObjVert));
                (*triangles)[numberOfTriangles]->emission = 0;
                (*triangles)[numberOfTriangles]->smoothness = 0;
                // memcpy(triangles[numberOfTriangles], &t, sizeof(OBJTriangle));
                // printf("\nHAY: %f\n", t.smoothness);
                // printf("\nHEY: %f\n", triangles[numberOfTriangles]->pos[0][0]);
                // printOBJTriangle(*(*triangles)[numberOfTriangles]);
            }
            else if (ret = sscanf(line, "f %d//%d %d//%d %d//%d", &av, &an, &bv, &bn, &cv, &cn))
            {
                fprintf(stderr, "\n# ERROR while reading %dth triangle (line %d)\n", numberOfTriangles, lineNb);
                exit(69);
                // OBJTriangle t = {0};
                // t.smoothness = .5;
                // memcpy(&t.color, &DEFAULT_COLOR, sizeof(ObjVert));
                // memcpy(&t.pos[0], &globalNorms[av], sizeof(ObjVert));
                // memcpy(&t.pos[2], &globalNorms[bv], sizeof(ObjVert));
                // memcpy(&t.pos[3], &globalNorms[cv], sizeof(ObjVert));
                // memcpy(&t.normal, &globalNorms[an], sizeof(ObjVert));
                // memcpy(&triangles[numberOfTriangles], &t, sizeof(OBJTriangle));
            }
            else
            {
                fprintf(stderr, "\n# ERROR while reading %dth triangle (line %d)\n", numberOfTriangles, lineNb);
            }
            numberOfTriangles++;
        }
    }

   fclose(fp);
   if (line)
      free(line);

#if PRINT_LOADING >= 2
   printf("\nFile reading ended.\n");
#endif
    free(globalVerts);
    free(globalNorms);

    return 0;
}