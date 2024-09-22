#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stbi_image_write.h"
#include "raytracing.h"
#include "scene.h"

typedef struct threadArgs
{
   int thID;
   vec3 *origin;
   Color *image;
} threadArgs;


#define MULTITHREADED 1
#if !defined(_WIN32) && (MULTITHREADED == 1)
#include <pthread.h>
#define NUMBER_OF_THREADS 12

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
#endif

int main(int argc, char const *argv[])
{
   normalizedSunDirection = normalized(sunDirection);
   if (argc == 1)
   {
      printf("Parsing triangles...\n");
      parseTriangleFile("debugTriangles.txt");
   }
   else if (strcmp(argv[1], "--help") == 0)
   {
      printf("%s takes as (optional) param an obj file.\n");
      exit(0);
   }
   else
   {
      printf("Loading obj...\n");
      loadOBJTriangles(argv[1]);
   }
   // printAllTriangles();
   Color image[w * h];
   vec3 origin = {0, 0, -7};
   vec3 lookingAt = {0,0,0};
   float fov = 1;
   
   vec3 ez = normalized(minus(lookingAt,origin));
   vec3 up = {0,-1,0};
   vec3 ex = normalized(cross(ez,up));
   vec3 ey = normalized(cross(ez,ex));
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
      if(y%10==0)printf("[%4i/%i] Processing...\n",y,h);
      for (int x = 0; x < w; ++x)
      {
         //vec3 dir = {(x - halfW) / (float)halfH, (y - halfH) / (float)halfH, 1}; // aspect ratio respected
         float dx = (x-halfW)/(float)halfH;
         float dy = (y-halfH)/(float)halfH;
         vec3 dir = plus(plus(times(ex,dx),times(ey,dy)),times(ez,fov));//dx*ex + dy*ey + fov*ez;
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
