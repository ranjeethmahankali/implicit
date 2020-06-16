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
    (x * (((float)coord.x - (float)dims.x / 2.0f) / (float)(dims.x / 2)) +
     y * (((float)coord.y - (float)dims.y / 2.0f) / (float)(dims.x / 2)));

  *dir = normalize((*pos) - center);
}

kernel void k_trace(global uint* pBuffer, // The pixel buffer
                    global uchar* packed,
                    global uchar* types,
                    global uchar* offsets,
                    local float* valBuf,
                    uint nEntities,
                    global op_step* steps,
                    uint nSteps,
                    float3 camPos, // Camera position in spherical coordinates
                    float3 camTarget)
{
  uint2 dims = (uint2)(get_global_size(0), get_global_size(1));
  uint2 coord = (uint2)(get_global_id(0), get_global_id(1));
  float3 pos, dir;
  perspective_project(camPos, camTarget,
                      coord, dims, &pos, &dir);
  uint i = coord.x + (coord.y * get_global_size(0));

  int iters = 500;
  float tolerance = 0.00001f;
  uint nBlocks = (dims.x * dims.y) / (get_local_size(0) * get_local_size(1));

  float dTotal = 0;
  pBuffer[i] = sphere_trace(packed, offsets, types, valBuf,
                            nEntities, steps, nSteps, pos, dir,
                            iters, tolerance, nBlocks);
}
