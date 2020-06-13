namespace cl_kernel_sources
{
	constexpr char render[] = R"(
#define DX 0.0001f
#define BOUND 20.0f
#define BACKGROUND_COLOR 0xff101010
#define UINT_TYPE uint
#define FLT_TYPE float
#define PACKED __attribute__((packed))
#define ENT_TYPE_BOX 1
struct i_box
{
  FLT_TYPE bounds[6];
} PACKED;
#define ENT_TYPE_SPHERE 2
struct i_sphere
{
  FLT_TYPE center[3];
  FLT_TYPE radius;
} PACKED;
#define ENT_TYPE_GYROID 3
struct i_gyroid
{
  FLT_TYPE scale;
  FLT_TYPE thickness;
} PACKED;
#define ENT_TYPE_BOOLEAN_UNION 4
struct i_boolean_union
{
  UINT_TYPE index_a;
  UINT_TYPE index_b;
} PACKED;
#define ENT_TYPE_BOOLEAN_INTERSECTION 5
struct i_boolean_intersection
{
  UINT_TYPE index_a;
  UINT_TYPE index_b;
} PACKED;
union i_entity
{
  struct i_box box;
  struct i_sphere sphere;
  struct i_gyroid gyroid;
  struct i_boolean_union boolean_union;
  struct i_boolean_intersection boolean_intersection;
};
struct wrapper
{
  union i_entity entity;
  UINT_TYPE type;
} PACKED;
#undef UINT_TYPE
#undef FLT_TYPE
float f_entity(global struct wrapper* entities, uint index, float3* pt);
float f_box(global union i_entity* eptr,
            float3* pt,
            global struct wrapper* entities)
{
  global float* bounds = eptr->box.bounds;
  float val = -FLT_MAX;
  val = max(val, (*pt).x - bounds[3]);
  val = max(val, bounds[0] - (*pt).x);
  val = max(val, (*pt).y - bounds[4]);
  val = max(val, bounds[1] - (*pt).y);
  val = max(val, (*pt).z - bounds[5]);
  val = max(val, bounds[2] - (*pt).z);
  return val;
}
float f_sphere(global union i_entity* eptr,
               float3* pt,
               global struct wrapper* entities)
{
  global float* center = eptr->sphere.center;
  float radius = eptr->sphere.radius;
  return length(*pt - (float3)(center[0], center[1], center[2])) - fabs(radius);
}
float f_gyroid(global union i_entity* eptr,
               float3* pt,
               global struct wrapper* entities)
{
  float scale = eptr->gyroid.scale;
  float thick = eptr->gyroid.thickness;
  float sx, cx, sy, cy, sz, cz;
  sx = sincos((*pt).x * scale, &cx);
  sy = sincos((*pt).y * scale, &cy);
  sz = sincos((*pt).z * scale, &cz);
  return (fabs(sx * cy + sy * cz + sz * cx) - thick) / 10.0f;
}
float f_boolean_union(global union i_entity* eptr,
                      float3* pt,
                      global struct wrapper* entities){
  uint a = eptr->boolean_union.index_a;
  uint b = eptr->boolean_union.index_b;
  return min(f_entity(entities, a, pt), f_entity(entities, b, pt));
}
float f_boolean_intersection(global union i_entity* eptr,
                             float3* pt,
                             global struct wrapper* entities){
  uint a = eptr->boolean_union.index_a;
  uint b = eptr->boolean_union.index_b;
  return max(f_entity(entities, a, pt), f_entity(entities, b, pt));
}
float f_entity(global struct wrapper* entities, uint index, float3* pt)
{
  global struct wrapper* wrap = entities + index;
  uint type = wrap->type;
  global union i_entity* ent = &(wrap->entity);
  switch (type){
  case ENT_TYPE_BOX: return f_box(ent, pt, entities);
  case ENT_TYPE_SPHERE: return f_sphere(ent, pt, entities);
  case ENT_TYPE_GYROID: return f_gyroid(ent, pt, entities);
  case ENT_TYPE_BOOLEAN_UNION: return f_boolean_union(ent, pt, entities);
  case ENT_TYPE_BOOLEAN_INTERSECTION:
    return f_boolean_intersection(ent, pt, entities);
  default: return 1;
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
uint sphere_trace(global struct wrapper* entities,
                  uint index,
                  float3 pt,
                  float3 dir,
                  float* dTotal,
                  int iters,
                  float tolerance)
{
  dir = normalize(dir);
  *dTotal = 0.0f;
  float3 norm = (float3)(0.0f, 0.0f, 0.0f);
  bool found = false;
  for (int i = 0; i < iters; i++){
    float d = f_entity(entities, index, &pt);
    if (d < 0.0f) break;
    if (d < tolerance){
      GRADIENT(f_entity(entities, index, &pt), pt, norm);
      found = true;
      break;
    }
    pt += dir * d;
    *dTotal += d;
    if (i > 5 && (fabs(pt.x) > BOUND ||
                  fabs(pt.y) > BOUND ||
                  fabs(pt.z) > BOUND)) break;
  }
  float d = dot(normalize(norm), -dir);
  float3 color = (float3)(0.2f,0.2f,0.2f)*(1.0f-d) + (float3)(0.9f,0.9f,0.9f)*d;
  return found ? colorToInt(color) : BACKGROUND_COLOR;
}
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
uint trace_all(float3 pt, float3 dir, global struct wrapper* entities, uint nEntities)
{
  float dMarch = FLT_MAX;
  float dBest = FLT_MAX;
  uint color, colorBest;
  for (uint i = 0; i < nEntities; i++){
    color = sphere_trace(entities, i, pt, dir, &dMarch, 500, 0.00001f);
    if (dMarch < dBest){
      colorBest = color;
      dBest = dMarch;
    }
  }
  return colorBest;
}
uint trace_one(float3 pt,
               float3 dir,
               global struct wrapper* entities,
               uint entityIndex,
               uint nEntities){
  float dMarch = 0.0f;
  return sphere_trace(entities, entityIndex, pt, dir, &dMarch, 500, 0.00001f);
}
kernel void k_trace(global uint* pBuffer, // The pixel buffer
                    global struct wrapper* entities,
                    uint entityIndex,
                    uint nEntities,
                    float camDist,
                    float camTheta,
                    float camPhi,
                    float3 camTarget)
{
  if (entityIndex >= nEntities)
    return;
  uint2 dims = (uint2)(get_global_size(0), get_global_size(1));
  uint2 coord = (uint2)(get_global_id(0), get_global_id(1));
  float3 pos, dir;
  perspective_project(camDist, camTheta, camPhi, camTarget,
                      coord, dims, &pos, &dir);
  uint i = coord.x + (coord.y * get_global_size(0));
  pBuffer[i] = trace_one(pos, dir, entities, entityIndex, nEntities);
}
	)";

}
