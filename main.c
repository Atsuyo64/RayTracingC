#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stbi_image_write.h"
#include "raytracing.h"
#include "scene.h"

int width = 128;
int height = 128;
int maxBounce = 10;
vec3 sunDirection = {-30,-85,100};

typedef struct threadArgs
{
   int thID;
   int trianglesOnly;
   vec3 *origin;
   Color *image;
   vec3 ex, ey, ez;
   float fov;
} threadArgs;

#define MULTITHREADED 1
#if !defined(_WIN32) && (MULTITHREADED == 1)
#include <pthread.h>
#define NUMBER_OF_THREADS 12

// HELP SECTION WITH OVERLOAD

void printHelp4(char const *progName, char const *errorDescription, char const *precision, char const *afterPrecision)
{
   fprintf(stderr, "%s%s%s", errorDescription, precision, afterPrecision);
   printf("%s [-h|--help]\n\t[-i|--input path/to/file.obj]\n\t[-p|--pos <posX> <posY> <posZ>]\n\t[-t|--track <trackX> <trackY> <trackZ>]\n\t[-f|--fov <fov>]\n\t[-s|--size <width> <height>]\n\t[-o|--output <filename>]\n", progName);
   exit(0);
}

void printHelp2(char const *progName, char const *errorDescription)
{
   printHelp4(progName, errorDescription, "", "");
}

void printHelp1(char const *progName)
{
   printHelp2(progName, "");
}

void printHelp()
{
   printHelp1("RayTracingC");
}

void *rowThread(void *thArgsPtr)
{
   threadArgs *thArgs = (threadArgs *)thArgsPtr;
   for (int y = thArgs->thID; y < height; y += NUMBER_OF_THREADS)
   {
      for (int x = 0; x < width; ++x)
      {
         float dx = (x - width / 2) / (float)(height / 2);
         float dy = (y - (height / 2)) / (float)(height / 2);
         vec3 dir = plus(plus(times(thArgs->ex, dx), times(thArgs->ey, dy)), times(thArgs->ez, thArgs->fov));
         // vec3 dir = {(x - width / 2) / (float)(height / 2), (y - (height / 2)) / (float)(height / 2), 1}; // aspect ratio respected
         // dir = minus(dir,origin);//fixed camera dir
         dir = normalized(dir);
         Ray ray = {*(thArgs->origin), dir};
         rngState = x + y * width;
         vec3 accumulatedColor = {0, 0, 0};
         for (int i = 0; i < accumulationCount; ++i)
            accumulatedColor = plus(accumulatedColor, times(calcColor(ray, thArgs->trianglesOnly, maxBounce), 1. / accumulationCount));
         thArgs->image[x + y * width] = vec3ToColor(accumulatedColor);
      }
   }
   free(thArgs);
}
#endif

int main(int argc, char const *argv[])
{
   // DEFAULT SETTINGS
   // mode is 'default' or a .obj path/filename
   char mode[256] = "default";
   char outputFileName[256] = "out.bmp";
   int trianglesOnly = 0;
   vec3 origin = {-4.75, -1.5, -4.75};
   vec3 lookingAt = {0.9, -1.2, 1};
   float fov = 1;

   // READING PARAMS
   for (unsigned int i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
         printHelp1(argv[0]);
      else if (strcmp(argv[i], "--pos") == 0 || strcmp(argv[i], "-p") == 0)
      {
         if (argc - 1 < i + 3)
            printHelp2(argv[0], "ERROR: --pos/-p takes 3 more params (float x,y,z)\n");
         origin.x = atof(argv[i + 1]);
         origin.y = atof(argv[i + 2]);
         origin.z = atof(argv[i + 3]);
         i += 3;
      }
      else if (strcmp(argv[i], "--track") == 0 || strcmp(argv[i], "-t") == 0)
      {
         if (argc - 1 < i + 3)
            printHelp2(argv[0], "ERROR: --track/-t takes 3 more params (float x,y,z)\n");
         lookingAt.x = atof(argv[i + 1]);
         lookingAt.y = atof(argv[i + 2]);
         lookingAt.z = atof(argv[i + 3]);
         i += 3;
      }
      else if (strcmp(argv[i], "--fov") == 0 || strcmp(argv[i], "-f") == 0)
      {
         if (argc - 1 < i + 1)
            printHelp2(argv[0], "ERROR: --fov/-i takes 1 more param (float f)\n");
         fov = atof(argv[i + 1]);
         i += 1;
      }
      else if (strcmp(argv[i], "--size") == 0 || strcmp(argv[i], "-s") == 0)
      {
         if (argc - 1 < i + 2)
            printHelp2(argv[0], "ERROR: --size/-s takes 2 more params (int w,h)\n");
         width = atoi(argv[i + 1]);
         height = atoi(argv[i + 2]);
         i += 2;
      }
      else if (strcmp(argv[i], "--input") == 0 || strcmp(argv[i], "-i") == 0)
      {
         if (argc - 1 < i + 1)
            printHelp2(argv[0], "ERROR: --input/-i takes 1 more param (string path/to/filename.obj)\n");
         strcpy(mode, argv[i + 1]);
         i += 1;
      }
      else if (strcmp(argv[i], "--output") == 0 || strcmp(argv[i], "-o") == 0)
      {
         if (argc - 1 < i + 1)
            printHelp2(argv[0], "ERROR: --output/-o takes 1 more param (string outImage.bmp)\n");
         strcpy(outputFileName, argv[i + 1]);
         i += 1;
      }
      else
      {
         printHelp4(argv[0], "ERROR: UNKNOWN ARGUMENT \"", argv[i], "\"\n");
      }
   }

   printf("Starting RayTracingC in %s mode", mode);
   if (strcmp(mode, "default") == 0)
   {
      printf("Parsing triangles...\n");
      parseTriangleFile("triangles.txt");
   }
   else
   {
      trianglesOnly = 1;
      printf("Loading obj...\n");
      loadOBJTriangles(mode);
   }

   Color image[width * height];
   normalizedSunDirection = normalized(sunDirection);
   vec3 ez = normalized(minus(lookingAt, origin));
   vec3 up = {0, -1, 0};
   vec3 ex = normalized(cross(ez, up));
   vec3 ey = normalized(cross(ez, ex));
   /*
   vec3 dir={0,0,1};
   dir=normalized(dir);
   Ray ray = {origin,dir};
   rngState = 0;
   calcColor(ray,3);
   */
   printf("Starting RENDERING...\n");
#if defined(_WIN32) || (MULTITHREADED == 0)
   for (int y = 0; y < height; ++y)
   {
      if (y % 10 == 0)
         printf("[%4i/%i] Processing...\n", y, height);
      for (int x = 0; x < width; ++x)
      {
         // vec3 dir = {(x - width / 2) / (float)(height / 2), (y - (height / 2)) / (float)(height / 2), 1}; // aspect ratio respected
         float dx = (x - width / 2) / (float)(height / 2);
         float dy = (y - (height / 2)) / (float)(height / 2);
         vec3 dir = plus(plus(times(ex, dx), times(ey, dy)), times(ez, fov)); // dx*ex + dy*ey + fov*ez;
         dir = normalized(dir);
         Ray ray = {origin, dir};
         rngState = x + y * width;
         vec3 accumulatedColor = {0, 0, 0};
         for (int i = 0; i < accumulationCount; ++i)
            accumulatedColor = plus(accumulatedColor, times(calcColor(ray, trianglesOnly, maxBounce), 1. / accumulationCount));
         image[x + y * width] = vec3ToColor(accumulatedColor);
      }
   }
#else
   pthread_t tIDs[NUMBER_OF_THREADS];
   for (int i = 0; i < NUMBER_OF_THREADS; i++)
   {
      threadArgs *args = malloc(sizeof(threadArgs));
      args->thID = i;
      args->origin = &origin;
      args->image = image;
      args->trianglesOnly = trianglesOnly;
      args->ex = ex;
      args->ey = ey;
      args->ez = ez;
      args->fov = fov;
      pthread_create(&tIDs[i], NULL, rowThread, args);
   }
   for (int i = 0; i < NUMBER_OF_THREADS; i++)
   {
      pthread_join(tIDs[i], NULL);
   }

#endif
   stbi_write_bmp(outputFileName, width, height, 3, (void const *)image);

   return 0;
}
