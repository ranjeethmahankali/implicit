namespace cl_kernel_sources
{
	constexpr char render[] = R"(
/* #define CLDEBUG */
#define BACKGROUND_COLOR 0xff101010
#define BOUND_R_COLOR 0xff000020
#define BOUND_G_COLOR 0xff002000
#define BOUND_B_COLOR 0xff200000
#define DX 0.0001f
#define AMB_STEP 0.05f
#define STEP_FOS 0.5f
#define UINT32_TYPE uint
#define UINT8_TYPE uchar
#define FLT_TYPE float
#define PACKED __attribute__((packed))
#define SRC_REG 1
#define SRC_VAL 2
#define ENT_TYPE_CSG                    0
#define ENT_TYPE_BOX                    1
#define ENT_TYPE_SPHERE                 2
#define ENT_TYPE_CYLINDER               3
#define ENT_TYPE_HALFSPACE              4
#define ENT_TYPE_GYROID                 5
#define ENT_TYPE_SCHWARZ                6
typedef struct PACKED
{
  FLT_TYPE bounds[6];
} i_box;
typedef struct PACKED
{
  FLT_TYPE center[3];
  FLT_TYPE radius;
} i_sphere;
typedef struct PACKED
{
    FLT_TYPE point1[3];
    FLT_TYPE point2[3];
    FLT_TYPE radius;
} i_cylinder;
typedef struct PACKED
{
    FLT_TYPE origin[3];
    FLT_TYPE normal[3];
} i_halfspace;
typedef struct PACKED
{
  FLT_TYPE scale;
  FLT_TYPE thickness;
} i_gyroid;
typedef struct PACKED
{
    FLT_TYPE scale;
    FLT_TYPE thickness;
} i_schwarz;
typedef enum
{
    OP_NONE = 0,
    OP_UNION = 1,
    OP_INTERSECTION = 2,
    OP_SUBTRACTION = 3,
    OP_OFFSET = 8,
    OP_LINBLEND = 16,
    OP_SMOOTHBLEND = 17,
} op_type;
typedef struct PACKED
{
    float p1[3];
    float p2[3];
} lin_blend_data;
typedef struct PACKED
{
    float p1[3];
    float p2[3];
} smooth_blend_data;
typedef union PACKED
{
    float blend_radius;
    float offset_distance;
    lin_blend_data lin_blend;
    smooth_blend_data smooth_blend;
} op_data;
typedef struct PACKED
{
    op_type type;
    op_data data;
} op_defn;
typedef struct PACKED
{
    op_defn op;
    UINT32_TYPE left_src;
    UINT32_TYPE left_index;
    UINT32_TYPE right_src;
    UINT32_TYPE right_index;
    UINT32_TYPE dest;
} op_step;
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
               float3* pt)
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
float apply_smoothblend(smooth_blend_data op, float a, float b, float3* pt)
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
    comp = 1.0f / (1.0f + pow(comp / (1.0f - comp), -2.0f));
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
  case OP_SMOOTHBLEND: return apply_smoothblend(op.data.smooth_blend, a, b, pt);
    
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
/*Macro to numerically compute the gradient vector of a given
implicit function.*/
#define GRADIENT(func, pt, norm, v0){                \
    pt.x += DX; float vx = func; pt.x -= DX;         \
    pt.y += DX; float vy = func; pt.y -= DX;         \
    pt.z += DX; float vz = func; pt.z -= DX;         \
    norm = (float3)((vx - v0) / DX,                  \
                    (vy - v0) / DX,                  \
                    (vz - v0) / DX);                 \
}
uint colorToInt(float3 rgb)
{
  uint color = 0xff000000;
  color |= ((uint)(min(1.0f, max(0.0f, rgb.x)) * 255));
  color |= ((uint)(min(1.0f, max(0.0f, rgb.y)) * 255)) << 8;
  color |= ((uint)(min(1.0f, max(0.0f, rgb.z)) * 255)) << 16;
  return color;
}
uint sphere_trace(global uchar* packed,
                  global uint* offsets,
                  global uchar* types,
                  local float* valBuf,
                  local float* regBuf,
                  uint nEntities,
                  global op_step* steps,
                  uint nSteps,
                  float3 pt,
                  float3 dir,
                  int iters,
                  float tolerance,
                  float boundDist
#ifdef CLDEBUG
                  , uchar debugFlag
#endif
                  )
{
  if (nEntities == 0)
    return BACKGROUND_COLOR;
  
  dir = normalize(dir);
  float3 norm = (float3)(0.0f, 0.0f, 0.0f);
  bool found = false;
  float dTotal = 0.0f;
  float d;
  for (int i = 0; i < iters; i++){
    d = f_entity(packed, offsets, types, valBuf, regBuf,
                       nEntities, steps, nSteps, &pt);
    if (d < 0.0f) break;
    if (d < tolerance){
      GRADIENT(f_entity(packed, offsets, types, valBuf, regBuf,
                        nEntities, steps, nSteps, &pt),
               pt, norm, d);
      found = true;
      break;
    }
    pt += dir * (d * STEP_FOS);
    dTotal += d * STEP_FOS;
    if (i > 3 && dTotal > boundDist) break;
  }
  
  if (!found){
#ifdef CLDEBUG
    if (debugFlag)
      printf("Can't find intersection. Rendering background color\n");
#endif
    return BACKGROUND_COLOR;
  }
  pt -= dir * AMB_STEP;
  float amb = (f_entity(packed, offsets, types, valBuf, regBuf,
                        nEntities, steps, nSteps, &pt) - d) / AMB_STEP;
  norm = normalize(norm);
  d = dot(norm, -dir);
  float cd = 0.2f;
  float cl = 0.4f * amb + 0.6f;
  float3 color1 = (float3)(cd, cd, cd)*(1.0f-d) + (float3)(cl, cl, cl)*d;
#ifdef CLDEBUG
  if (debugFlag){
    printf("Floating point color: (%.2f, %.2f, %.2f)\n",
           color1.x, color1.y, color1.z);
  }
#endif
  return colorToInt(color1);
}
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
                    local float* valBuf, // The buffer for local use.
                    local float* regBuf, // More buffer for local use.
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
	)";

}
