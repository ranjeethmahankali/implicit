#define UINT_TYPE uint
#define FLT_TYPE float

#define PACKED __attribute__((packed))

#include "primitives.h"

#undef UINT_TYPE
#undef FLT_TYPE

#define OFFSET(src, target, dtype, var) (target*)(src + (uint)(&(((dtype*)0)->var)))

float f_box(global uchar* eptr, float3* pt)
{
  global float* bounds = OFFSET(eptr, global float, struct i_box, bounds);
  float val = -FLT_MAX;
  val = max(val, (*pt).x - bounds[3]);
  val = max(val, bounds[0] - (*pt).x);
  val = max(val, (*pt).y - bounds[4]);
  val = max(val, bounds[1] - (*pt).y);
  val = max(val, (*pt).z - bounds[5]);
  val = max(val, bounds[2] - (*pt).z);
  return val;
}

float f_gyroid(global uchar* eptr, float3* pt)
{
  float scale = *(OFFSET(eptr, global float, struct i_gyroid, scale));
  float thick = *(OFFSET(eptr, global float, struct i_gyroid, thickness));
  float sx, cx, sy, cy, sz, cz;
  sx = sincos((*pt).x * scale, &cx);
  sy = sincos((*pt).y * scale, &cy);
  sz = sincos((*pt).z * scale, &cz);
  return (fabs(sx * cy + sy * cz + sz * cx) - thick) / 10.0f;
}

float f_sphere(global uchar* eptr, float3* pt)
{
  global float* center = OFFSET(eptr, global float, struct i_sphere, center);
  float radius = *(OFFSET(eptr, global float, struct i_sphere, radius));
  return length(*pt - (float3)(center[0], center[1], center[2])) - fabs(radius);
}

float f_entity(global uchar* wrapper, float3* pt)
{
  uchar type = *(OFFSET(wrapper, global uchar, struct wrapper, type));
  global uchar* ent = OFFSET(wrapper, global uchar, struct wrapper, entity);
  switch (type){
  case ENT_TYPE_BOX: return f_box(ent, pt);
  case ENT_TYPE_SPHERE: return f_sphere(ent, pt);
  case ENT_TYPE_GYROID: return f_gyroid(ent, pt);
  default: return 1;
  }
}


