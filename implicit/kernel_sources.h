namespace cl_kernel_sources
{
	constexpr char render[] = R"(
#define DX 0.0001f
#define BOUND 20.0f
#define BACKGROUND_COLOR 0xff101010
#define UINT32_TYPE uint
#define UINT8_TYPE uchar
#define FLT_TYPE float
#define PACKED __attribute__((packed))
#define REG_L -1
#define REG_R -2
#define ENT_TYPE_BOX 1
typedef struct PACKED
{
  FLT_TYPE bounds[6];
} i_box;
#define ENT_TYPE_SPHERE 2
typedef struct PACKED
{
  FLT_TYPE center[3];
  FLT_TYPE radius;
} i_sphere;
#define ENT_TYPE_GYROID 3
typedef struct PACKED
{
  FLT_TYPE scale;
  FLT_TYPE thickness;
} i_gyroid;
#define ENT_TYPE_CSG 0
typedef enum
{
    none = 0,
    bool_union = 1,
    bool_intersection = 2,
} op_type;
typedef struct PACKED
{
    op_type type;
    UINT32_TYPE left_src;
    UINT32_TYPE right_src;
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
float f_entity(global uchar* ptr,
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
/*Macro to numerically compute the gradient vector of a given
implicit function.*/
#define GRADIENT(func, pt, norm){                    \
    float v0 = func;                                 \
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
  color |= ((uint)(rgb.x * 255));
  color |= ((uint)(rgb.y * 255)) << 8;
  color |= ((uint)(rgb.z * 255)) << 16;
  return color;
}
uint sphere_trace_simple(global uchar* packed,
                         global uint* offsets,
                         uchar type,
                         float3 pt,
                         float3 dir,
                         int iters,
                         float tolerance)
{
  dir = normalize(dir);
  float3 norm = (float3)(0.0f, 0.0f, 0.0f);
  bool found = false;
  for (int i = 0; i < iters; i++){
    float d = f_entity(packed, type, &pt);
    if (d < 0.0f) break;
    if (d < tolerance){
      GRADIENT(f_entity(packed, type, &pt), pt, norm);
      found = true;
      break;
    }
    pt += dir * d;
    if (i > 5 && (fabs(pt.x) > BOUND ||
                  fabs(pt.y) > BOUND ||
                  fabs(pt.z) > BOUND)) break;
  }
  float d = dot(normalize(norm), -dir);
  float3 color = (float3)(0.2f,0.2f,0.2f)*(1.0f-d) + (float3)(0.9f,0.9f,0.9f)*d;
  return found ? colorToInt(color) : BACKGROUND_COLOR;
}
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
                    uint nEntities,
                    global uint* steps,
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
  uint color = BACKGROUND_COLOR;
  if (nSteps == 0){
    float dTotal = 0;
    uchar type = *types;
    color = sphere_trace_simple(packed, offsets, type,
                                pos, dir, iters, tolerance);
  }
  else{
    // nothing.
  }
  
  pBuffer[i] = color;
}
	)";

}
