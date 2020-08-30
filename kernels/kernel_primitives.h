#define UINT32_TYPE uint
#define UINT8_TYPE uchar
#define FLT_TYPE float

#define PACKED __attribute__((packed))

#include "primitives.h"

#undef UINT_TYPE
#undef FLT_TYPE

#define CAST_TYPE(type, name, ptr) global type* name = (global type*)ptr

float4 f_box(global uchar* packed,
            float3* pt)
{
  CAST_TYPE(i_box, box, packed);
  global float* bounds = box->bounds;
  float4 result = (float4)(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);
  float val = (*pt).x - bounds[3];
  if (val > result.w){
    result.x = 1.0f;
    result.y = 0.0f;
    result.z = 0.0f;
    result.w = val;
  }
  val = bounds[0] - (*pt).x;
  if (val > result.w){
    result.x = -1.0f;
    result.y = 0.0f;
    result.z = 0.0f;
    result.w = val;
  }
  val = (*pt).y - bounds[4];
  if (val > result.w){
    result.x = 0.0f;
    result.y = 1.0f;
    result.z = 0.0f;
    result.w = val;
  }
  val = bounds[1] - (*pt).y;
  if (val > result.w){
    result.x = 0.0f;
    result.y = -1.0f;
    result.z = 0.0f;
    result.w = val;
  }
  val = (*pt).z - bounds[5];
  if (val > result.w){
    result.x = 0.0f;
    result.y = 0.0f;
    result.z = 1.0f;
    result.w = val;
  }
  val = bounds[2] - (*pt).z;
  if (val > result.w){
    result.x = 0.0f;
    result.y = 0.0f;
    result.z = -1.0f;
    result.w = val;
  }
  return result;
}

float4 f_sphere(global uchar* ptr,
               float3* pt)
{
  CAST_TYPE(i_sphere, sphere, ptr);
  // Vector from the center to the point.
  float3 dVec = *pt - (float3)(sphere->center[0],
                               sphere->center[1],
                               sphere->center[2]);
  return (float4)(dVec.x,
                  dVec.y,
                  dVec.z,
                  length(dVec) - fabs(sphere->radius));
}

float4 f_cylinder(global uchar* ptr,
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
  float4 result = (float4)(-perp.x,
                           -perp.y,
                           -perp.z,
                           length(perp) - fabs(cyl->radius));
  float val = dot(ln, r);
  if (val > result.w){
    result.x = -ln.x;
    result.y = -ln.y;
    result.z = -ln.z;
    result.w = val;
  }
  r = p2 - (*pt);
  val = dot(-ln, r);
  if (val > result.w){
    result.x = ln.x;
    result.y = ln.y;
    result.z = ln.z;
    result.w = val;
  }
  return result;
}

float4 f_gyroid(global uchar* ptr,
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
  float4 result = (float4)((cx * cy + sz * (-sx)) / factor,
                           (sx * (-sy) + cy * cz) / factor,
                           (sy * (-sz) + cz * cx) / factor,
                           fabs(fval) - (thick / factor));
  if (fval < 0.0f){
    result.x *= -1.0f;
    result.y *= -1.0f;
    result.z *= -1.0f;
  }
  return result;
}

float4 f_schwarz(global uchar* ptr,
                float3* pt)
{
  CAST_TYPE(i_schwarz, lattice, ptr);
  float factor = 4.0f / lattice->thickness;
  float sx, sy, sz, cx, cy, cz;
  sx = sincos((*pt).x * lattice->scale, &cx);
  sy = sincos((*pt).y * lattice->scale, &cy);
  sz = sincos((*pt).z * lattice->scale, &cz);
  float fval = (cx + cy + cz) / factor;
  float4 result = (float4)(-sx / factor,
                           -sy / factor,
                           -sz / factor,
                           fabs(fval) - (lattice->thickness / factor));
  if (fval < 0.0f){
    result.x *= -1.0f;
    result.y *= -1.0f;
    result.z *= -1.0f;
  }
  
  return result;
}

float4 f_halfspace(global uchar* ptr,
                  float3* pt)
{
  CAST_TYPE(i_halfspace, hspace, ptr);
  float3 origin = (float3)(hspace->origin[0],
                           hspace->origin[1],
                           hspace->origin[2]);
  float3 normal = normalize((float3)(hspace->normal[0],
                                     hspace->normal[1],
                                     hspace->normal[2]));
  return (float4)(-normal.x,
                  -normal.y,
                  -normal.z,
                  dot((*pt) - origin, -normal));
}

float4 f_simple(global uchar* ptr,
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
  default: return (float4)(FLT_MAX, FLT_MAX, FLT_MAX, 1.0f);
  }
}

float4 apply_union(float blend_radius,
                   float4 a,
                   float4 b,
                   float3* pt
#ifdef CLDEBUG
                   , uchar debugFlag
#endif
                   )
{
  if (a.w < blend_radius && b.w < blend_radius){
    float2 diff = (float2)(blend_radius - a.w, blend_radius - b.w);
    float3 grad =
      diff.x * normalize((float3)(a.x, a.y, a.z)) +
      diff.y * normalize((float3)(b.x, b.y, b.z));
    return (float4)(grad.x, grad.y, grad.z, blend_radius - length(diff));
  }
  else{
    return a.w < b.w ? a : b;
  }
}

float4 apply_linblend(lin_blend_data op, float4 a, float4 b, float3* pt
#ifdef CLDEBUG
                      , uchar debugFlag
#endif
                      )
{
    float3 p1 = (float3)(op.p1[0],
                         op.p1[1],
                         op.p1[2]);
    float3 p2 = (float3)(op.p2[0],
                         op.p2[1],
                         op.p2[2]);
    float3 gLambda = (p2 - p1) / dot(p2 - p1, p2 - p1);
    float lambda = dot((*pt) - p1, gLambda);
    lambda = min(1.0f, max(0.0f, lambda));
    float3 grad;
    if (lambda == 0.0f){
      grad = (float3)(a.x, a.y, a.z);
    }
    else if (lambda == 1.0f){
      grad = (float3)(b.x, b.y, b.z);
    }
    else{
      grad =
        b.w * gLambda +
        lambda * ((float3)(b.x, b.y, b.z)) - a.w * gLambda
        + (1.0f - lambda) * ((float3)(a.x, a.y, a.z));
    }

    return (float4)(grad.x, grad.y, grad.z,
                    lambda * b.w + (1.0f - lambda) * a.w);
}

float4 apply_smoothblend(smooth_blend_data op, float4 a, float4 b, float3* pt
#ifdef CLDEBUG
                         , uchar debugFlag
#endif
                         )
{
    float3 p1 = (float3)(op.p1[0],
                         op.p1[1],
                         op.p1[2]);
    float3 p2 = (float3)(op.p2[0],
                         op.p2[1],
                         op.p2[2]);
    float3 gLambda = p2 - p1;
    gLambda /= dot(gLambda, gLambda);
    float lambda = dot((*pt) - p1, gLambda);
    float4 result;
    if (lambda <= 0.0f){
      result = a;
    }
    else if (lambda >= 1.0f){
      result = b;
    }
    else{
      gLambda *= (2 * (1.0f - lambda) * lambda) /
        pow(2 * lambda * lambda - 2 * lambda + 1, 2.0f);
      lambda = 1.0f / (1.0f + pow(lambda / (1.0f - lambda), -2.0f));
      float3 grad =
        b.w * gLambda +
        lambda * ((float3)(b.x, b.y, b.z)) - a.w * gLambda
        + (1.0f - lambda) * ((float3)(a.x, a.y, a.z));
      result = (float4)(grad.x, grad.y, grad.z,
                        lambda * b.w + (1.0f - lambda) * a.w);
    }
    
    return result;
}

float4 apply_op(op_defn op, float4 a, float4 b, float3* pt
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
  case OP_INTERSECTION: return a.w < b.w ? b : a;//max(a, b);
  case OP_SUBTRACTION: return a.w < (-b.w) ? (-b) : a; //max(a, -b);

  case OP_OFFSET: return (float4)(a.x, a.y, a.z,
                                  a.w - op.data.offset_distance);

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

float4 f_entity(global uchar* packed,
                global uint* offsets,
                global uchar* types,
                local float4* valBuf,
                local float4* regBuf,
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
    float4 l = steps[si].left_src == SRC_REG ?
      regBuf[i * bsize + bi] :
      valBuf[i * bsize + bi];
    
    i = steps[si].right_index;
    float4 r = steps[si].right_src == SRC_REG ?
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
