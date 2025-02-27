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

vec3 sunDirection = {-30, -85, 100};
// vec3 skyColorHorizon = {1, 1, 1};
// vec3 skyColorZenith = {0.263, 0.969, 0.871};
// vec3 groundColor;// = {.66, .66, .66};
// float sunFocus = 22;
// float sunIntensity = .75;
vec3 normalizedSunDirection;
Scene scene = {
   // normalizedSunDirection
   {0,0,0},
   // skyColorHorizon, skyColorZenith, groundColor
   {1, 1, 1}, {0.263, 0.969, 0.871}, {.66, .66, .66},
   // sunFocus, sunIntensity
   22, .75
};

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
   printf("%s [-h|--help]\
   \n   # FILE SETTINGS\
   \n\t[-i|--input path/to/file.obj]\n\t[-o|--output <filename>]\
   \n   # POSITION & LOOKING-AT POSITION (TRACK)\
   \n\t[-p|--pos <posX> <posY> <posZ>]\n\t[-t|--track <trackX> <trackY> <trackZ>]\
   \n   # CAMERA SETTINGS\
   \n\t[-f|--fov <fov>]\n\t[-s|--size <width> <height>]\n\t[-b|--max-bounce <maxBounce>]\
   \n   # SCENE SETTINGS\
   \n\t[-gc|--ground-color <R> <G> <B>]\n\t[-sch|--sky-color-horizon <R> <G> <B>]\n\t[-scz|--sky-color-zenith <R> <G> <B>]\
   \n   # SUN SETTINGS\
   \n\t[--sun <x> <y> <z> <focus> <intensity>]\
   \n",
          progName);
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
            accumulatedColor = plus(accumulatedColor, times(calcColor(ray, thArgs->trianglesOnly, maxBounce, scene), 1. / accumulationCount));
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

      // FILE SETTINGS
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

      // POSITIONS
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

      // CAMERA SETTINGS
      else if (strcmp(argv[i], "--fov") == 0 || strcmp(argv[i], "-f") == 0)
      {
         if (argc - 1 < i + 1)
            printHelp2(argv[0], "ERROR: --fov/-i takes 1 more param (float f)\n");
         fov = atof(argv[i + 1]);
         i += 1;
      }
      else if (strcmp(argv[i], "--max-bounce") == 0 || strcmp(argv[i], "-b") == 0)
      {
         if (argc - 1 < i + 1)
            printHelp2(argv[0], "ERROR: --max-bounce/-b takes 1 more param (int f)\n");
         maxBounce = atoi(argv[i + 1]);
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

      // SCENE SETTINGS
      else if (strcmp(argv[i], "--ground-color") == 0 || strcmp(argv[i], "-gc") == 0)
      {
         if (argc - 1 < i + 3)
            printHelp2(argv[0], "ERROR: --ground-color/-gc takes 3 more params (float r,g,b)\n");
         scene.groundColor.x = atof(argv[i + 1]);
         scene.groundColor.y = atof(argv[i + 2]);
         scene.groundColor.z = atof(argv[i + 3]);
         i += 3;
      }
      else if (strcmp(argv[i], "--sky-color-horizon") == 0 || strcmp(argv[i], "-sch") == 0)
      {
         if (argc - 1 < i + 3)
            printHelp2(argv[0], "ERROR: --sky-color-horizon/-sch takes 3 more params (float r,g,b)\n");
         scene.skyColorHorizon.x = atof(argv[i + 1]);
         scene.skyColorHorizon.y = atof(argv[i + 2]);
         scene.skyColorHorizon.z = atof(argv[i + 3]);
         i += 3;
      }
      else if (strcmp(argv[i], "--sky-color-zenith") == 0 || strcmp(argv[i], "-scz") == 0)
      {
         if (argc - 1 < i + 3)
            printHelp2(argv[0], "ERROR: --sky-color-zenith/-scz takes 3 more params (float r,g,b)\n");
         scene.skyColorZenith.x = atof(argv[i + 1]);
         scene.skyColorZenith.y = atof(argv[i + 2]);
         scene.skyColorZenith.z = atof(argv[i + 3]);
         i += 3;
      }

      // SUN SETTINGS
      else if (strcmp(argv[i], "--sun") == 0)
      {
         if (argc - 1 < i + 5)
            printHelp2(argv[0], "ERROR: --sun takes 5 more params (float x,y,z,focus,intensity)\n");
         sunDirection.x = atof(argv[i + 1]);
         sunDirection.y = atof(argv[i + 2]);
         sunDirection.z = atof(argv[i + 3]);
         scene.sunFocus = atof(argv[i + 4]);
         scene.sunIntensity = atof(argv[i + 5]);
         i += 5;
      }

      // THROW ERROR
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
   scene.normalizedSunDirection.x = normalizedSunDirection.x;
   scene.normalizedSunDirection.y = normalizedSunDirection.y;
   scene.normalizedSunDirection.z = normalizedSunDirection.z;
   // printf("SUNDIRECTION: %f, %f, %f\n", scene.normalizedSunDirection.x, scene.normalizedSunDirection.y, scene.normalizedSunDirection.z);
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
