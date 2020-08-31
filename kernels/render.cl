/* #define CLDEBUG */
#define BACKGROUND_COLOR 0xff101010
#define BOUND_R_COLOR 0xff000020
#define BOUND_G_COLOR 0xff002000
#define BOUND_B_COLOR 0xff200000

#define DX 0.0001f
#define AMB_STEP 0.05f
#define STEP_FOS 0.9f
#define EPSILON 0.0001
#define NUM_ITERS 500
#define TOLERANCE 0.00001f

#include "kernel_primitives.h"

#define GRADIENT(func, point, val, grad) {                  \
    point.x += EPSILON; grad.x = (func - val) / EPSILON; point.x -= EPSILON;  \
    point.y += EPSILON; grad.y = (func - val) / EPSILON; point.y -= EPSILON;  \
    point.z += EPSILON; grad.z = (func - val) / EPSILON; point.z -= EPSILON;  \
  }

uint colorToInt(float gray)
{
  uint color = 0xff000000;
  color |= ((uint)(min(1.0f, max(0.0f, gray)) * 255));
  color |= ((uint)(min(1.0f, max(0.0f, gray)) * 255)) << 8;
  color |= ((uint)(min(1.0f, max(0.0f, gray)) * 255)) << 16;
  return color;
}

/*
Casts the ray to the bounding box of the viewer and returns the distance
of the backface of the bounding box. This can be used to optimize the ray
tracing of implicit bodies by ignoring anything that is beyond this distance.

If the distance is set to -1, that means the ray completely misses box, in which
case background color can be rendered.
*/
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
                       nEntities, steps, nSteps, &pt
#ifdef CLDEBUG
                 , debugFlag
#endif
                 );

    if (d < 0.0f && dTotal == 0.0f) break; // Too close to camera.
    if (d < tolerance && (-tolerance) < d){
      GRADIENT(f_entity(packed, offsets, types, valBuf, regBuf,
                        nEntities, steps, nSteps, &pt
#ifdef CLDEBUG
                        , debugFlag
#endif
                        ),
               pt, d, norm);
      norm = normalize(norm);
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
  float old = d;
  d = f_entity(packed, offsets, types, valBuf, regBuf,
               nEntities, steps, nSteps, &pt
#ifdef CLDEBUG
               , debugFlag
#endif
               );
  float amb = (d - old) / AMB_STEP;
  float c = 0.2f + dot(norm, -dir) * (0.6f * amb + 0.3f);
#ifdef CLDEBUG
  if (debugFlag){
    printf("Gradient: (%.2f, %.2f, %.2f)\n", norm.x, norm.y, norm.z);
    printf("RayDir:   (%.2f, %.2f, %.2f)\n", -dir.x, -dir.y, -dir.z);
    printf("Dot: %.2f\n", dot(norm, -dir));
    printf("Floating point color: %.2f\n", c);
  }
#endif
  return colorToInt(c);
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
                    __constant float* viewerData,
                    uchar levelOfDetail
#ifdef CLDEBUG
                    , uint2 mousePos // Mouse position in pixels.
#endif
                    )
{
  uint2 dims = (uint2)(get_global_size(0), get_global_size(1));
  uint2 coord = (uint2)(get_global_id(0), get_global_id(1));
  uint step = 1 << levelOfDetail;
#ifdef CLDEBUG
  uchar debugFlag = (uchar)(coord.x == mousePos.x && coord.y == mousePos.y);
  if (debugFlag) printf("\n");
#endif
  if (coord.x % step == 0 && coord.y % step == 0){
    float3 pos, dir;
    float boundDist;
    uint color;
    perspective_project(viewerData, coord, dims, &pos, &dir, &boundDist, &color
#ifdef CLDEBUG
                        , debugFlag
#endif
                        );
    uint i = coord.x + (coord.y * get_global_size(0));
    if (boundDist > 0.0f){
      pBuffer[i] = sphere_trace(packed, offsets, types, valBuf, regBuf,
                                nEntities, steps, nSteps, pos, dir,
                                NUM_ITERS, TOLERANCE, boundDist
#ifdef CLDEBUG
                                , debugFlag
#endif
                                );
      if (pBuffer[i] == BACKGROUND_COLOR) pBuffer[i] = color;
    }
    else{
      pBuffer[i] = BACKGROUND_COLOR;
    }
  }
  else{
    uint i = coord.x + (coord.y * get_global_size(0));
    coord.x -= coord.x % step;
    coord.y -= coord.y % step;
    pBuffer[i] = pBuffer[coord.x + (coord.y * get_global_size(0))];
  }
#ifdef CLDEBUG
  if (debugFlag){
    printf("Screen coords: (%02d, %02d)\n", mousePos.x, mousePos.y);
    printf("Color: %08x\n", pBuffer[i]);
  }
#endif
}
