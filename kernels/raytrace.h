#define DX 0.0001f
#define BOUND 20.0f
#define BACKGROUND_COLOR 0xff101010

#include "kernel_primitives.h"

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
