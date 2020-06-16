#define UINT32_TYPE uint
#define UINT8_TYPE uchar
#define FLT_TYPE float

#define PACKED __attribute__((packed))

#include "primitives.h"

#undef UINT_TYPE
#undef FLT_TYPE

#define CAST_TYPE(type, name, ptr) global type* name = (global type*)ptr

float f_box(global uchar* packed,
            float3* pt)
{
  CAST_TYPE(i_box, box, packed);
  global float* bounds = box->bounds;
  float val = -FLT_MAX;
  val = max(val, (*pt).x - bounds[3]);
  val = max(val, bounds[0] - (*pt).x);
  val = max(val, (*pt).y - bounds[4]);
  val = max(val, bounds[1] - (*pt).y);
  val = max(val, (*pt).z - bounds[5]);
  val = max(val, bounds[2] - (*pt).z);
  return val;
}

float f_sphere(global uchar* ptr,
               float3* pt)
{
  CAST_TYPE(i_sphere, sphere, ptr);
  global float* center = sphere->center;
  float radius = sphere->radius;
  return length(*pt - (float3)(center[0], center[1], center[2])) - fabs(radius);
}

float f_gyroid(global uchar* ptr,
               float3* pt)
{
  CAST_TYPE(i_gyroid, gyroid, ptr);
  float scale = gyroid->scale;
  float thick = gyroid->thickness;
  float sx, cx, sy, cy, sz, cz;
  sx = sincos((*pt).x * scale, &cx);
  sy = sincos((*pt).y * scale, &cy);
  sz = sincos((*pt).z * scale, &cz);
  return (fabs(sx * cy + sy * cz + sz * cx) - thick) / 10.0f;
}

float f_simple(global uchar* ptr,
               uchar type,
               float3* pt)
{
  switch (type){
  case ENT_TYPE_BOX: return f_box(ptr, pt);
  case ENT_TYPE_SPHERE: return f_sphere(ptr, pt);
  case ENT_TYPE_GYROID: return f_gyroid(ptr, pt);
  default: return 1.0f;
  }
}

float apply_op(op_type op, float a, float b)
{
  switch(op){
  case OP_NONE: return a;
  case OP_BOOL_UNION: return min(a, b);
  case OP_BOOL_INTERSECTION: return max(a, b);
  default: return a;
  }
}

float f_entity(global uchar* packed,
               global uint* offsets,
               global uchar* types,
               local float* valBuf,
               uint nEntities,
               global op_step* steps,
               uint nSteps,
               float3* pt)
{
  if (nSteps == 0){
    if (nEntities > 0)
      return f_simple(packed, *types, pt);
    else
      return 1.0f;
  }

  uint bsize = get_local_size(0);
  uint bi = get_local_id(0);
  // Compute the values of simple entities.
  for (uint ei = 0; ei < nEntities; ei++){
    valBuf[ei * bsize + bi] = f_simple(packed + offsets[ei], types[ei], pt);
  }

  float regL = 1.0f, regR;
  // Perform the csg operations.
  for (uint si = 0; si < nSteps; si++){
    uint ls = steps[si].left_src;
    uint rs = steps[si].right_src;
    uint ds = steps[si].dest;
    if ((ls != REG_L && ls >= nEntities) ||
        (rs != REG_R && rs >= nEntities) ||
        (ds != REG_L && ds != REG_R)){
      return 1.0f;
    }
    float l = ls == REG_L ? regL : valBuf[ls * bsize + bi];
    float r = rs == REG_R ? regR : valBuf[rs * bsize + bi];
    float v = apply_op(steps[si].type, l, r);
    
    if (ds == REG_R)
      regR = v;
    else
      regL = v;
  }
  
  return regL;
}
