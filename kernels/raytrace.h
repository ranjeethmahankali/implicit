#define DX 0.0001f
#define AMB_STEP 0.05f
#define STEP_FOS 0.5f

#include "kernel_primitives.h"

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
                  local float4* valBuf,
                  local float4* regBuf,
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
  float4 d;
  for (int i = 0; i < iters; i++){
    d = f_entity(packed, offsets, types, valBuf, regBuf,
                       nEntities, steps, nSteps, &pt
#ifdef CLDEBUG
                 , debugFlag
#endif
                 );
    if (d.w < 0.0f){
#ifdef CLDEBUG
      if (debugFlag) printf("Overshot into the inside of the body.\n");
#endif
      break;
    }
    if (d.w < tolerance){
      norm = normalize((float3)(d.x, d.y, d.z));
      found = true;
      break;
    }
    pt += dir * (d.w * STEP_FOS);
    dTotal += d.w * STEP_FOS;
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
  float old = d.w;
  d = f_entity(packed, offsets, types, valBuf, regBuf,
               nEntities, steps, nSteps, &pt
#ifdef CLDEBUG
               , debugFlag
#endif
               );
  float amb = (d.w - old) / AMB_STEP;
  float c = 0.2f + dot(norm, -dir) * (0.4f * amb + 0.4f);
#ifdef CLDEBUG
  if (debugFlag){
    printf("Floating point color: %.2f\n", c);
  }
#endif
  return colorToInt((float3)(c, c, c));
}
