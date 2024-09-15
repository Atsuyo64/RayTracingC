#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stbi_image_write.h"
#include "raytracing.h"
#include "scene.h"

int main()
{
   normalizedSunDirection = normalized(sunDirection);
   printf("Parsing triangles...\n");
   parseTriangleFile("triangles.txt");
   //printAllTriangles();
   Color image[w*h];
   vec3 origin={0,-1,-1};
   /*
   vec3 dir={0,0,1};
   dir=normalized(dir);
   Ray ray = {origin,dir};
   rngState = 0;
   calcColor(ray,3);
   */
   for(int y=0;y<h;++y)
   {
      for(int x=0;x<w;++x)
      {
         vec3 dir={(x-halfW)/(float)halfH,(y-halfH)/(float)halfH,1};//aspect ratio respected
         //dir = minus(dir,origin);//fixed camera dir
         dir = normalized(dir);
         Ray ray = {origin,dir};
         rngState = x+y*w;
         vec3 accumulatedColor = {0,0,0};
         for(int i=0;i<accumulationCount;++i)
            accumulatedColor = plus(accumulatedColor,times(calcColor(ray,maxBounce),1./accumulationCount));
         image[x+y*w]=vec3ToColor(accumulatedColor);
      }
   }
   stbi_write_bmp("out.bmp",w,h,3,(void const*)image);
   
   return 0;
}