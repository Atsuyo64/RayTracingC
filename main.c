#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stbi_image_write.h"
#include "raytracing.h"
#include "scene.h"

#define MULTITHREADED 0
#if defined(_WIN32) || (MULTITHREADED == 0)
#include <pthread.h>
#define NUMBER_OF_THREADS 12
#endif

typedef struct threadArgs
{
   int thID;
   vec3 *origin;
   Color *image;
} threadArgs;

void *rowThread(void *thArgsPtr)
{
   threadArgs *thArgs = (threadArgs *)thArgsPtr;
   for (int y = thArgs->thID; y < h; y += NUMBER_OF_THREADS)
   {
      for (int x = 0; x < w; ++x)
      {
         vec3 dir = {(x - halfW) / (float)halfH, (y - halfH) / (float)halfH, 1}; // aspect ratio respected
         // dir = minus(dir,origin);//fixed camera dir
         dir = normalized(dir);
         Ray ray = {*(thArgs->origin), dir};
         rngState = x + y * w;
         vec3 accumulatedColor = {0, 0, 0};
         for (int i = 0; i < accumulationCount; ++i)
            accumulatedColor = plus(accumulatedColor, times(calcColor(ray, maxBounce), 1. / accumulationCount));
         thArgs->image[x + y * w] = vec3ToColor(accumulatedColor);
      }
   }
   free(thArgs);
}

int main()
{
   normalizedSunDirection = normalized(sunDirection);
   printf("Parsing triangles...\n");
   parseTriangleFile("triangles.txt");
   // printAllTriangles();
   Color image[w * h];
   vec3 origin = {0, -1, -1};
   /*
   vec3 dir={0,0,1};
   dir=normalized(dir);
   Ray ray = {origin,dir};
   rngState = 0;
   calcColor(ray,3);
   */

#if defined(_WIN32) || (MULTITHREADED == 0)
   for (int y = 0; y < h; ++y)
   {
      for (int x = 0; x < w; ++x)
      {
         vec3 dir = {(x - halfW) / (float)halfH, (y - halfH) / (float)halfH, 1}; // aspect ratio respected
         // dir = minus(dir,origin);//fixed camera dir
         dir = normalized(dir);
         Ray ray = {origin, dir};
         rngState = x + y * w;
         vec3 accumulatedColor = {0, 0, 0};
         for (int i = 0; i < accumulationCount; ++i)
            accumulatedColor = plus(accumulatedColor, times(calcColor(ray, maxBounce), 1. / accumulationCount));
         image[x + y * w] = vec3ToColor(accumulatedColor);
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
      pthread_create(&tIDs[i], NULL, rowThread, args);
   }
   for (int i = 0; i < NUMBER_OF_THREADS; i++)
   {
      pthread_join(tIDs[i], NULL);
   }

#endif
   stbi_write_bmp("out.bmp", w, h, 3, (void const *)image);

   return 0;
}
