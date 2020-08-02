// #define CLDEBUG
#include "raytrace.h"

void perspective_project(float3 camPos,
                         float3 camTarget,
                         uint2 coord,
                         uint2 dims,
                         float3* pos,
                         float3* dir)
{
  float st, ct, sp, cp;
  st = sincos(camPos.y, &ct);
  sp = sincos(camPos.z, &cp);

  *dir = -(float3)(camPos.x * cp * ct, camPos.x * cp * st, camPos.x * sp);
  *pos = camTarget - (*dir);
  *dir = normalize(*dir);

  /* float3 center = pos - (dir * 0.57735026f); */
  float3 center = (*pos) - ((*dir) * 2.0f);
  
  float3 x = normalize(cross(*dir, (float3)(0, 0, 1)));
  float3 y = normalize(cross(x, *dir));
  *pos += 1.5f *
    (x * (((float)coord.x - (float)dims.x / 2.0f) / ((float)dims.x / 2.0f)) +
     y * (((float)coord.y - (float)dims.y / 2.0f) / ((float)dims.x / 2.0f)));

  *dir = normalize((*pos) - center);
}

kernel void k_trace(global uint* pBuffer, // The pixel buffer
                    global uchar* packed, // Bytes of render data for simple bytes.
                    global uchar* types, // Types of simple entities in the csg tree.
                    global uchar* offsets, // The byte offsets of simple entities.
                    local float* valBuf, // The buffer for local use.
                    local float* regBuf, // More buffer for local use.
                    uint nEntities, // The number of simple entities.
                    global op_step* steps, // CSG steps.
                    uint nSteps, // Number of csg steps.
                    float3 camPos, // Camera position in spherical coords from target
                    float3 camTarget // Camera target as a point in R3 space.
#ifdef CLDEBUG
                    , uint2 mousePos // Mouse position in pixels.
#endif
                    )
{
  uint2 dims = (uint2)(get_global_size(0), get_global_size(1));
  uint2 coord = (uint2)(get_global_id(0), get_global_id(1));
  float3 pos, dir;
  perspective_project(camPos, camTarget,
                      coord, dims, &pos, &dir);
  uint i = coord.x + (coord.y * get_global_size(0));

  int iters = 500;
  float tolerance = 0.00001f;

  float dTotal = 0;

#ifdef CLDEBUG
  uchar debugFlag = (uchar)(coord.x == mousePos.x && coord.y == mousePos.y);
  if (debugFlag) printf("\n");
#endif
  
  pBuffer[i] = sphere_trace(packed, offsets, types, valBuf, regBuf,
                            nEntities, steps, nSteps, pos, dir,
                            iters, tolerance
#ifdef CLDEBUG
                            , debugFlag
#endif
                            );
#ifdef CLDEBUG
  if (debugFlag){
    printf("Screen coords: (%02d, %02d)\n", mousePos.x, mousePos.y);
    printf("Color: %08x", pBuffer[i]);
  }
#endif
}
