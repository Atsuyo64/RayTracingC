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

typedef struct threadArgs
{
   int thID;
   int trianglesOnly;
   vec3 *origin;
   Color *image;
} threadArgs;

#define MULTITHREADED 0
#if !defined(_WIN32) && (MULTITHREADED == 1)
#include <pthread.h>
#define NUMBER_OF_THREADS 12

void *rowThread(void *thArgsPtr)
{
   threadArgs *thArgs = (threadArgs *)thArgsPtr;
   for (int y = thArgs->thID; y < height; y += NUMBER_OF_THREADS)
   {
      for (int x = 0; x < width; ++x)
      {
         vec3 dir = {(x - width / 2) / (float)(height / 2), (y - (height / 2)) / (float)(height / 2), 1}; // aspect ratio respected
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
   int trianglesOnly = 0;
   normalizedSunDirection = normalized(sunDirection);
   if (argc < 2 || strcmp(argv[1], "--help") == 0 || (argc != 2 && argc != 5 && argc != 8 && argc != 9 && argc != 11))
   {
      printf("%s needs at least one param.\n", argv[0]);
      printf("%s <default|path/to/file.obj> [posX posY posZ] [trackX trackY trackZ] [fov] [width height].\n", argv[0]);
      exit(0);
   }
   else if (strcmp(argv[1], "default") == 0) {
      printf("Parsing triangles...\n");
      parseTriangleFile("triangles.txt");
   }
   else
   {
      trianglesOnly = 1;
      printf("Loading obj...\n");
      loadOBJTriangles(argv[1]);
   }
   // printAllTriangles();
   vec3 origin = {-4.75, -1.5, -4.75};
   if (argc >= 5) {
      origin.x = atof(argv[2]);
      origin.y = atof(argv[3]);
      origin.z = atof(argv[4]);
   }

   vec3 lookingAt = {0.9,-1.2,1};
   if (argc >= 8) {
      lookingAt.x = atof(argv[5]);
      lookingAt.y = atof(argv[6]);
      lookingAt.z = atof(argv[7]);
   }
   float fov = 1;
   if (argc >= 9) {
      fov = atof(argv[8]);
   }
   if (argc >= 11) {
      width = atoi(argv[9]);
      height = atoi(argv[10]);
   }

   Color image[width * height];
   
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
   for (int y = 0; y < height; ++y)
   {
      if(y%10==0)printf("[%4i/%i] Processing...\n",y,height);
      for (int x = 0; x < width; ++x)
      {
         //vec3 dir = {(x - width / 2) / (float)(height / 2), (y - (height / 2)) / (float)(height / 2), 1}; // aspect ratio respected
         float dx = (x-width / 2)/(float)(height / 2);
         float dy = (y-(height / 2))/(float)(height / 2);
         vec3 dir = plus(plus(times(ex,dx),times(ey,dy)),times(ez,fov));//dx*ex + dy*ey + fov*ez;
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
      pthread_create(&tIDs[i], NULL, rowThread, args);
   }
   for (int i = 0; i < NUMBER_OF_THREADS; i++)
   {
      pthread_join(tIDs[i], NULL);
   }

#endif
   stbi_write_bmp("out.bmp", width, height, 3, (void const *)image);

   return 0;
}
