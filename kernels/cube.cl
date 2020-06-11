#define DX 0.0001f

#define GRADIENT(func, pt, norm){                    \
    float v0 = func;                                 \
    pt.x += DX; float vx = func; pt.x -= DX;         \
    pt.y += DX; float vy = func; pt.y -= DX;         \
    pt.z += DX; float vz = func; pt.z -= DX;         \
    norm = (float3)((vx - v0) / DX,                  \
                    (vy - v0) / DX,                  \
                    (vz - v0) / DX);                 \
}

#define SPHERE_TRACE(func, pt, dir, norm, found, iters, tolerance){ \
    dir = normalize(dir);                                           \
    float3 norm = (float3)(0.0f, 0.0f, 0.0f);                       \
    bool found = false;                                             \
    for (int i = 0; i < iters; i++){                                \
      float d = func;                                               \
      if (d < 0.0f) break;                                              \
      if (d < tolerance){                                               \
        GRADIENT(func, pt, norm);                                       \
        found = true;                                                   \
        break;                                                          \
      }                                                                 \
      pt += dir * d;                                                    \
    }                                                                   \
    float d = dot(normalize(norm), -dir);                               \
    float3 color = (float3)(0.2f,0.2f,0.2f)*(1.0f-d) + (float3)(0.9f,0.9f,0.9f)*d; \
    return found ? colorToInt(color) : 0xff101010;                      \
  }

uint colorToInt(float3 rgb)
{
  uint color = 0xff000000;
  color |= ((uint)(rgb.x * 255));
  color |= ((uint)(rgb.y * 255)) << 8;
  color |= ((uint)(rgb.z * 255)) << 16;
  return color;
}

uint trace_box(float3 rayPt, float3 rayDir, float3 boxMin, float3 boxMax)
{
  float rbest = FLT_MAX;
  float3 norm = (float3)(0, 0, 0);
  if (rayPt.x < boxMin.x){
    float r = (boxMin.x - rayPt.x) / rayDir.x;
    float3 p = rayPt + (rayDir * r);
    if (r > 0 &&
        p.y > boxMin.y && p.y < boxMax.y && p.z > boxMin.z && p.z < boxMax.z &&
        r < rbest){
      rbest = r;
      norm = (float3)(-1.0f, 0.0f, 0.0f);
    }
  }

  if (rayPt.x > boxMax.x){
    float r = (boxMax.x - rayPt.x) / rayDir.x;
    float3 p = rayPt + (rayDir * r);
    if (r > 0 &&
        p.y > boxMin.y && p.y < boxMax.y && p.z > boxMin.z && p.z < boxMax.z &&
        r < rbest){
      rbest = r;
      norm = (float3)(1.0f, 0.0f, 0.0f);
    }
  }

  if (rayPt.y < boxMin.y){
    float r = (boxMin.y - rayPt.y) / rayDir.y;
    float3 p = rayPt + (rayDir * r);
    if (r > 0 &&
        p.x > boxMin.x && p.x < boxMax.x && p.z > boxMin.z && p.z < boxMax.z &&
        r < rbest){
      rbest = r;
      norm = (float3)(0.0f, -1.0f, 0.0f);
    }
  }

  if (rayPt.y > boxMax.y){
    float r = (boxMax.y - rayPt.y) / rayDir.y;
    float3 p = rayPt + (rayDir * r);
    if (r > 0 &&
        p.x > boxMin.x && p.x < boxMax.x && p.z > boxMin.z && p.z < boxMax.z &&
        r < rbest){
      rbest = r;
      norm = (float3)(0.0f, 1.0f, 0.0f);
    }
  }

  if (rayPt.z < boxMin.z){
    float r = (boxMin.z - rayPt.z) / rayDir.z;
    float3 p = rayPt + (rayDir * r);
    if (r > 0 &&
        p.x > boxMin.x && p.x < boxMax.x && p.y > boxMin.y && p.y < boxMax.y &&
        r < rbest){
      rbest = r;
      norm = (float3)(0.0f, 0.0f, -1.0f);
    }
  }

  if (rayPt.z > boxMax.z){
    float r = (boxMax.z - rayPt.z) / rayDir.z;
    float3 p = rayPt + (rayDir * r);
    if (r > 0 &&
        p.x > boxMin.x && p.x < boxMax.x && p.y > boxMin.y && p.y < boxMax.y &&
        r < rbest){
      rbest = r;
      norm = (float3)(0.0f, 0.0f, 1.0f);
    }
  }

  float d = dot(normalize(norm), normalize(-rayDir));
  float3 dark = (float3)(0.2f, 0.2f, 0.2f);
  float3 lite = (float3)(0.9f, 0.9f, 0.9f);
  return rbest == FLT_MAX ?
    0xff101010 :
    colorToInt(dark * (1.0f - d) + lite * d);
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

float f_box(float3* bmin, float3* bmax, float3* pt)
{
  float val = -FLT_MAX;
  val = max(val, (*pt).x - (*bmax).x);
  val = max(val, (*bmin).x - (*pt).x);
  val = max(val, (*pt).y - (*bmax).y);
  val = max(val, (*bmin).y - (*pt).y);
  val = max(val, (*pt).z - (*bmax).z);
  val = max(val, (*bmin).z - (*pt).z);
  return val;
}

float f_capsule(float3* a, float3* b, float thick, float3* pt)
{
  float3 ln = *b - *a;
  float r = min(1.0f, max(0.0f, dot(ln, *pt - *a) / dot(ln, ln)));
  return length((*a + ln * r) - *pt) - thick;
}

float f_gyroid(float3* bmin, float3* bmax, float3* pt)
{
  float sx, cx, sy, cy, sz, cz;
  sx = sincos((*pt).x * 2.0f, &cx);
  sy = sincos((*pt).y * 2.0f, &cy);
  sz = sincos((*pt).z * 2.0f, &cz);
  /* return (sx * cy + sy * cz + sz * cx) / 10.0f; */
  return max(length(*pt) - 5.0f,
             (fabs(sx * cy + sy * cz + sz * cx) - 0.2f) / 10.0f);
}

float f_testUnion(float3* bmin, float3* bmax, float radius, float3* pt)
{
  float a = length(((*bmin + *bmax) * 0.5f) - *pt) - radius;
  float b = f_capsule(bmin, bmax, radius * 0.5f, pt);
  return min(a, b);
}

uint trace_any(float3 pt, float3 dir, float3 bmin, float3 bmax)
{
  SPHERE_TRACE(
               /* f_box(&bmin, &bmax, &pt), */
               /* f_capsule(&bmin, &bmax, 3, &pt), */
               /* f_testUnion(&bmin, &bmax, 2.0f, &pt), */
               f_gyroid(&bmin, &bmax, &pt),
               pt, dir, norm, found, 500, 0.00001f);
}

kernel void k_traceCube(global uint* pBuffer, // The pixel buffer
                        float camDist,
                        float camTheta,
                        float camPhi,
                        float3 camTarget)
{
  uint2 dims = (uint2)(get_global_size(0), get_global_size(1));
  uint2 coord = (uint2)(get_global_id(0), get_global_id(1));
  float3 pos, dir;
  perspective_project(camDist, camTheta, camPhi, camTarget,
                      coord, dims, &pos, &dir);
  uint i = coord.x + (coord.y * get_global_size(0));
  /* pBuffer[i] = trace_box(pos, dir, */
  /*                        (float3)(-5.0f, -5.0f, -5.0f), */
  /*                        (float3)(5.0f, 5.0f, 5.0f)); */
  pBuffer[i] = trace_any(pos, dir,
                         (float3)(-5.0f, -5.0f, -5.0f),
                         (float3)(5.0f, 5.0f, 5.0f));
}
