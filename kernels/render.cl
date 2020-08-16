/* #define CLDEBUG */
#define BACKGROUND_COLOR 0xff101010
#define BOUND_R_COLOR 0xff000020
#define BOUND_G_COLOR 0xff002000
#define BOUND_B_COLOR 0xff200000
#include "raytrace.h"

float bound_distance(__constant float* viewerData, float3* pos, float3* dir,
                     uint* color
#ifdef CLDEBUG
                     , uchar debugFlag
#endif
                     )
{
  float3 bmin = vload3(2, viewerData);
  float3 bmax = vload3(3, viewerData);
  *color = BACKGROUND_COLOR;
  if ((*dir).x < 0.0f){
    float3 t = *pos + (*dir) * ((bmin.x - (*pos).x)/((*dir).x));
    if (t.y > bmin.y && t.y < bmax.y && t.z > bmin.z && t.z < bmax.z){
      *color = BOUND_R_COLOR;
      return length(t - *pos);
    }
  }
  else if ((*dir).x > 0.0f){
    float3 t = *pos + (*dir) * ((bmax.x - (*pos).x)/((*dir).x));
    if (t.y > bmin.y && t.y < bmax.y && t.z > bmin.z && t.z < bmax.z){
      *color = BOUND_R_COLOR;
      return length(t - *pos);
    }
  }
  if ((*dir).y < 0.0f){
    float3 t = *pos + (*dir) * ((bmin.y - (*pos).y)/((*dir).y));
    if (t.x > bmin.x && t.x < bmax.x && t.z > bmin.z && t.z < bmax.z){
      *color = BOUND_G_COLOR;
      return length(t - *pos);
    }
  }
  else if ((*dir).y > 0.0f){
    float3 t = *pos + (*dir) * ((bmax.y - (*pos).y)/((*dir).y));
    if (t.x > bmin.x && t.x < bmax.x && t.z > bmin.z && t.z < bmax.z){
      *color = BOUND_G_COLOR;
      return length(t - *pos);
    }
  }
  if ((*dir).z < 0.0f){
    float3 t = *pos + (*dir) * ((bmin.z - (*pos).z)/((*dir).z));
    if (t.x > bmin.x && t.x < bmax.x && t.y > bmin.y && t.y < bmax.y){
      *color = BOUND_B_COLOR;
      return length(t - *pos);
    }
  }
  else if ((*dir).z > 0.0f){
    float3 t = *pos + (*dir) * ((bmax.z - (*pos).z)/((*dir).z));
    if (t.x > bmin.x && t.x < bmax.x && t.y > bmin.y && t.y < bmax.y){
      *color = BOUND_B_COLOR;
      return length(t - *pos);
    }
  }

  return -1.0f;
}

void perspective_project(__constant float* viewerData,
                         uint2 coord,
                         uint2 dims,
                         float3* pos,
                         float3* dir,
                         float* boundDist,
                         uint* color
#ifdef CLDEBUG
                         , uchar debugFlag
#endif
                         )
{
  float3 camPos = vload3(0, viewerData);
  float3 camTarget = vload3(1, viewerData);
  float st, ct, sp, cp;
  st = sincos(camPos.y, &ct);
  sp = sincos(camPos.z, &cp);

  *dir = -(float3)(camPos.x * cp * ct, camPos.x * cp * st, camPos.x * sp);
  *pos = camTarget - (*dir);
  *dir = normalize(*dir);

  float3 center = (*pos) - ((*dir) * 2.0f);
  
  float3 x = normalize(cross(*dir, (float3)(0, 0, 1)));
  float3 y = normalize(cross(x, *dir));
  *pos += 1.5f *
    (x * (((float)coord.x - (float)dims.x / 2.0f) / ((float)dims.x / 2.0f)) +
     y * (((float)coord.y - (float)dims.y / 2.0f) / ((float)dims.x / 2.0f)));

  *dir = normalize((*pos) - center);
  
  *boundDist = bound_distance(viewerData, pos, dir, color
#ifdef CLDEBUG
                              , debugFlag
#endif
                              );
}

kernel void k_trace(global uint* pBuffer, // The pixel buffer
                    global uchar* packed, // Bytes of render data for simple bytes.
                    global uchar* types, // Types of simple entities in the csg tree.
                    global uchar* offsets, // The byte offsets of simple entities.
                    local float4* valBuf, // The buffer for local use.
                    local float4* regBuf, // More buffer for local use.
                    uint nEntities, // The number of simple entities.
                    global op_step* steps, // CSG steps.
                    uint nSteps, // Number of csg steps.
                    __constant float* viewerData
#ifdef CLDEBUG
                    , uint2 mousePos // Mouse position in pixels.
#endif
                    )
{
  uint2 dims = (uint2)(get_global_size(0), get_global_size(1));
  uint2 coord = (uint2)(get_global_id(0), get_global_id(1));
#ifdef CLDEBUG
  uchar debugFlag = (uchar)(coord.x == mousePos.x && coord.y == mousePos.y);
  if (debugFlag) printf("\n");
#endif
  float3 pos, dir;
  float boundDist;
  uint color;
  perspective_project(viewerData, coord, dims, &pos, &dir, &boundDist, &color
#ifdef CLDEBUG
                      , debugFlag
#endif
                      );
  uint i = coord.x + (coord.y * get_global_size(0));

  int iters = 500;
  float tolerance = 0.00001f;

  if (boundDist > 0.0f){
    pBuffer[i] = sphere_trace(packed, offsets, types, valBuf, regBuf,
                              nEntities, steps, nSteps, pos, dir,
                              iters, tolerance, boundDist
#ifdef CLDEBUG
                              , debugFlag
#endif
                              );
    if (pBuffer[i] == BACKGROUND_COLOR){
      pBuffer[i] = color;
    }
  }
  else{
    pBuffer[i] = BACKGROUND_COLOR;
  }
#ifdef CLDEBUG
  if (debugFlag){
    printf("Screen coords: (%02d, %02d)\n", mousePos.x, mousePos.y);
    printf("Color: %08x", pBuffer[i]);
  }
#endif
}
