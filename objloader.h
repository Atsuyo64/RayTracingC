#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


typedef struct OBJVec3
{
    float x;
    float y;
    float z;
} OBJVec3;
typedef struct OBJTriangle
{
    OBJVec3 posA, posB, posC;
    OBJVec3 normal;
    OBJVec3 color;
    float emission;
    float smoothness;
} OBJTriangle;

typedef struct OBJMat
{
    char name[128];
    OBJVec3 color;
    float emission;
    float smoothness;
} OBJMat;

/**
 * Print OBJTriangle
 * @param triangle
 *
 */
void printOBJTriangle(OBJTriangle triangle);

/**
 * objTriangleArrayFree
 * @param arr of triangles
 * @param count of triangles
 * 
 * frees array of Triangles
 */
void objTriangleArrayFree(OBJTriangle **triangles, int count);

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
int loadObj(const char *filename, OBJTriangle ***triangles, int *count);