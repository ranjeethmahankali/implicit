#define DX 0.0001f
#define BOUND 20.0f
#define BACKGROUND_COLOR 0xff101010
#define AMB_STEP 0.01f

#include "kernel_primitives.h"

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
  color |= ((uint)(rgb.x * 255));
  color |= ((uint)(rgb.y * 255)) << 8;
  color |= ((uint)(rgb.z * 255)) << 16;
  return color;
}

uint sphere_trace(global uchar* packed,
                  global uint* offsets,
                  global uchar* types,
                  local float* valBuf,
                  uint nEntities,
                  global op_step* steps,
                  uint nSteps,
                  float3 pt,
                  float3 dir,
                  int iters,
                  float tolerance)
{
  dir = normalize(dir);
  float3 norm = (float3)(0.0f, 0.0f, 0.0f);
  bool found = false;
  float d;
  for (int i = 0; i < iters; i++){
    d = f_entity(packed, offsets, types, valBuf,
                       nEntities, steps, nSteps, &pt);
    if (d < 0.0f) break;
    if (d < tolerance){
      GRADIENT(f_entity(packed, offsets, types, valBuf,
                        nEntities, steps, nSteps, &pt),
               pt, norm, d);
      found = true;
      break;
    }
    pt += dir * d;
    if (i > 5 && (fabs(pt.x) > BOUND ||
                  fabs(pt.y) > BOUND ||
                  fabs(pt.z) > BOUND)) break;
  }
  
  if (!found) return BACKGROUND_COLOR;

  pt -= dir * AMB_STEP;
  float amb = (f_entity(packed, offsets, types, valBuf,
                        nEntities, steps, nSteps, &pt) - d) / AMB_STEP;
  norm = normalize(norm);
  d = dot(norm, -dir);
  float cd = 0.2f;
  float cl = 0.4f * amb + 0.6f;
  /* printf("%.3f\n", cl); */
  float3 color1 = (float3)(cd, cd, cd)*(1.0f-d) + (float3)(cl, cl, cl)*d;
  return colorToInt(color1);
}
