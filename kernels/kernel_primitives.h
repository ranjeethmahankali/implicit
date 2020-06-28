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
  return (length(*pt -
                 (float3)(center[0], center[1], center[2])) -
          fabs(radius));
}

float f_cylinder(global uchar* ptr,
                 float3* pt)
{
  CAST_TYPE(i_cylinder, cyl, ptr);
  float3 p1 = (float3)(cyl->point1[0],
                       cyl->point1[1],
                       cyl->point1[2]);
  float3 p2 = (float3)(cyl->point2[0],
                       cyl->point2[1],
                       cyl->point2[2]);
  float3 ln = normalize(p2 - p1);
  float3 r = p1 - (*pt);
  float dist = length(r - ln * dot(ln, r)) - fabs(cyl->radius);
  dist = max(dist, dot(ln, r));
  r = p2 - (*pt);
  dist = max(dist, dot(-ln, r));
  return dist;
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
  float factor = 4.0f / thick;
  return (fabs(sx * cy + sy * cz + sz * cx) - thick) / factor;
}

float f_schwarz(global uchar* ptr,
                float3* pt)
{
  CAST_TYPE(i_schwarz, lattice, ptr);
  float scale = lattice->scale;
  float thick = lattice->thickness;
  float factor = 8.0f / thick;
  return (fabs(cos((*pt).x * scale) +
               cos((*pt).y * scale) +
               cos((*pt).z * scale)) - thick) / factor;
}

float f_simple(global uchar* ptr,
               uchar type,
               float3* pt)
{
  switch (type){
  case ENT_TYPE_BOX: return f_box(ptr, pt);
  case ENT_TYPE_SPHERE: return f_sphere(ptr, pt);
  case ENT_TYPE_GYROID: return f_gyroid(ptr, pt);
  case ENT_TYPE_SCHWARZ: return f_schwarz(ptr, pt);
  case ENT_TYPE_CYLINDER: return f_cylinder(ptr, pt);
  default: return 1.0f;
  }
}

float apply_linblend(lin_blend_data op, float a, float b, float3* pt)
{
    float3 p1 = (float3)(op.p1[0],
                         op.p1[1],
                         op.p1[2]);
    float3 p2 = (float3)(op.p2[0],
                         op.p2[1],
                         op.p2[2]);
    float3 ln = p2 - p1;
    float modLn = length(ln);
    ln = normalize(ln);
    float comp = dot((*pt) - p1, ln) / modLn;
    comp = min(1.0f, max(0.0f, comp));
    return (1.0f - comp) * a + comp * b;
}

float apply_op(op_defn op, float a, float b, float3* pt)
{
  switch(op.type){
  case OP_NONE: return a;
  case OP_UNION: return min(a, b);
  case OP_INTERSECTION: return max(a, b);
  case OP_SUBTRACTION: return max(a, -b);

  case OP_OFFSET: return a - op.data.offset_distance;

  case OP_LINBLEND: return apply_linblend(op.data.lin_blend, a, b, pt);
    
  default: return a;
  }
}

float f_entity(global uchar* packed,
               global uint* offsets,
               global uchar* types,
               local float* valBuf,
               local float* regBuf,
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

  // Perform the csg operations.
  for (uint si = 0; si < nSteps; si++){
    uint i = steps[si].left_index;
    float l = steps[si].left_src == SRC_REG ?
      regBuf[i * bsize + bi] :
      valBuf[i * bsize + bi];
    
    i = steps[si].right_index;
    float r = steps[si].right_src == SRC_REG ?
      regBuf[i * bsize + bi] :
      valBuf[i * bsize + bi];
    
    regBuf[steps[si].dest * bsize + bi] = apply_op(steps[si].op, l, r, pt);
  }
  
  return regBuf[bi];
}
