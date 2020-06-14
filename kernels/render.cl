#include "raytrace.h"

void perspective_project(float camDist,
                         float camTheta,
                         float camPhi,
                         float3 camTarget,
                         uint2 coord,
                         uint2 dims,
                         float3* pos,
                         float3* dir)
{
  float st, ct, sp, cp;
  st = sincos(camTheta, &ct);
  sp = sincos(camPhi, &cp);

  *dir = -(float3)(camDist * cp * ct, camDist * cp * st, camDist * sp);
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

float f_capsule(float3* a, float3* b, float thick, float3* pt)
{
  float3 ln = *b - *a;
  float r = min(1.0f, max(0.0f, dot(ln, *pt - *a) / dot(ln, ln)));
  return length((*a + ln * r) - *pt) - thick;
}

float f_testUnion(float3* bmin, float3* bmax, float radius, float3* pt)
{
  float a = length(((*bmin + *bmax) * 0.5f) - *pt) - radius;
  float b = f_capsule(bmin, bmax, radius * 0.5f, pt);
  return min(a, b);
}

uint trace_one(float3 pt,
               float3 dir,
               global uchar* entities,
               uint entityIndex,
               uint nEntities){
  float dMarch = 0.0f;
  return sphere_trace(entities, entityIndex, pt, dir, &dMarch, 500, 0.00001f);
}

kernel void k_trace(global uint* pBuffer, // The pixel buffer
                    global uchar* entityBuffer,
                    global uchar* types,
                    global uchar* offsets,
                    global uint* tree,
                    float camDist,
                    float camTheta,
                    float camPhi,
                    float3 camTarget)
{
  uint2 dims = (uint2)(get_global_size(0), get_global_size(1));
  uint2 coord = (uint2)(get_global_id(0), get_global_id(1));
  float3 pos, dir;
  perspective_project(camDist, camTheta, camPhi, camTarget,
                      coord, dims, &pos, &dir);
  uint i = coord.x + (coord.y * get_global_size(0));
  pBuffer[i] = trace_one(pos, dir, entities, 0, nEntities);
}
