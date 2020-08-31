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
  float result = -FLT_MAX;
  result = max(result, (*pt).x - bounds[3]);
  result = max(result, bounds[0] - (*pt).x);
  result = max(result, (*pt).y - bounds[4]);
  result = max(result, bounds[1] - (*pt).y);
  result = max(result, (*pt).z - bounds[5]);
  result = max(result, bounds[2] - (*pt).z);
  return result;
}

float f_sphere(global uchar* ptr,
               float3* pt)
{
  CAST_TYPE(i_sphere, sphere, ptr);
  // Vector from the center to the point.
  return length(*pt - (float3)(sphere->center[0],
                               sphere->center[1],
                               sphere->center[2])) - fabs(sphere->radius);
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
  float3 perp = r - ln * dot(ln, r);
  float result = length(perp) - fabs(cyl->radius);
  float val = dot(ln, r);
  result = max(result, dot(ln, r));
  r = p2 - (*pt);
  result = max(result, dot(-ln, r));
  return result;
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
  float fval = (sx * cy + sy * cz + sz * cx) / factor;
  float result = fabs(fval) - (thick / factor);
  return result;
}

float f_schwarz(global uchar* ptr,
                float3* pt)
{
  CAST_TYPE(i_schwarz, lattice, ptr);
  float factor = 4.0f / lattice->thickness;
  float cx = cos((*pt).x * lattice->scale);
  float cy = cos((*pt).y * lattice->scale);
  float cz = cos((*pt).z * lattice->scale);
  float result = fabs((cx + cy + cz) / factor) - (lattice->thickness / factor);
  return result;
}

float f_halfspace(global uchar* ptr,
                  float3* pt)
{
  CAST_TYPE(i_halfspace, hspace, ptr);
  float3 origin = (float3)(hspace->origin[0],
                           hspace->origin[1],
                           hspace->origin[2]);
  float3 normal = normalize((float3)(hspace->normal[0],
                                     hspace->normal[1],
                                     hspace->normal[2]));

  return dot((*pt) - origin, -normal);
}

float f_simple(global uchar* ptr,
                uchar type,
                float3* pt
#ifdef CLDEBUG
                , uchar debugFlag
#endif
                )
{
  switch (type){
  case ENT_TYPE_BOX: return f_box(ptr, pt);
  case ENT_TYPE_SPHERE: return f_sphere(ptr, pt);
  case ENT_TYPE_GYROID: return f_gyroid(ptr, pt);
  case ENT_TYPE_SCHWARZ: return f_schwarz(ptr, pt);
  case ENT_TYPE_CYLINDER: return f_cylinder(ptr, pt);
  case ENT_TYPE_HALFSPACE: return f_halfspace(ptr, pt);
  default: return 1.0f;
  }
}

float apply_union(float blend_radius,
                   float a,
                   float b,
                   float3* pt
#ifdef CLDEBUG
                   , uchar debugFlag
#endif
                   )
{
  if (a < blend_radius && b < blend_radius){
    float2 diff = (float2)(blend_radius - a, blend_radius - b);
    return blend_radius - length(diff);
  }
  else{
    return min(a, b);
  }
}

float apply_linblend(lin_blend_data op,
                     float a,
                     float b,
                     float3* pt
#ifdef CLDEBUG
                      , uchar debugFlag
#endif
                      )
{
  float lambda;
  {
    float3 p1 = (float3)(op.p1[0],
                         op.p1[1],
                         op.p1[2]);
    float3 ln = (float3)(op.p2[0],
                         op.p2[1],
                         op.p2[2]) - p1;
    lambda = min(1.0f, max(0.0f, dot((*pt) - p1, ln / dot(ln, ln)))); 
  }
  return lambda * b + (1.0f - lambda) * a;
}

float apply_smoothblend(smooth_blend_data op,
                        float a,
                        float b,
                        float3* pt
#ifdef CLDEBUG
                         , uchar debugFlag
#endif
                         )
{
  float lambda;
  {
    float3 p1 = (float3)(op.p1[0],
                         op.p1[1],
                         op.p1[2]);
    float3 ln = (float3)(op.p2[0],
                         op.p2[1],
                         op.p2[2]) - p1;
    lambda = min(1.0f, max(0.0f, dot((*pt) - p1, ln / dot(ln, ln))));
    lambda = 1.0f / (1.0f + pow(lambda / (1.0f - lambda), -2.0f));
  }
  return lambda * b + (1.0f - lambda) * a;
}

float apply_op(op_defn op,
               float a,
               float b,
               float3* pt
#ifdef CLDEBUG
                      , uchar debugFlag
#endif
                )
{
  switch(op.type){
  case OP_NONE: return a;
  case OP_UNION: return apply_union(op.data.blend_radius, a, b, pt
#ifdef CLDEBUG
                                    , uchar debugFlag
#endif
                                    );
  case OP_INTERSECTION: return max(a, b);
  case OP_SUBTRACTION: return max(a, -b);
  case OP_OFFSET: return a - op.data.offset_distance;
  case OP_LINBLEND: return apply_linblend(op.data.lin_blend, a, b, pt
#ifdef CLDEBUG
                      , debugFlag
#endif
                                          );
  case OP_SMOOTHBLEND: return apply_smoothblend(op.data.smooth_blend, a, b, pt
#ifdef CLDEBUG
                      , debugFlag
#endif
                                                );
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
                float3* pt
#ifdef CLDEBUG
                      , uchar debugFlag
#endif
                )
{
  if (nSteps == 0){
    if (nEntities > 0)
      return f_simple(packed, *types, pt
#ifdef CLDEBUG
                      , debugFlag
#endif
                      );
    else
      return 1.0f;
  }

  uint bsize = get_local_size(0);
  uint bi = get_local_id(0);
  // Compute the values of simple entities.
  for (uint ei = 0; ei < nEntities; ei++){
    valBuf[ei * bsize + bi] =
      f_simple(packed + offsets[ei], types[ei], pt
#ifdef CLDEBUG
               , debugFlag
#endif
               );
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
    
    regBuf[steps[si].dest * bsize + bi] =
      apply_op(steps[si].op, l, r, pt
#ifdef CLDEBUG
                      , debugFlag
#endif
               );
  }
  
  return regBuf[bi];
}
