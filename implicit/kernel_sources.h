namespace cl_kernel_sources
{
	constexpr char cube[] = R"(
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
  float3 center = (*pos) - ((*dir) * 1.2f);
  
  float3 x = normalize(cross(*dir, (float3)(0, 0, 1)));
  float3 y = normalize(cross(x, *dir));
  *pos += 1.5f *
    (x * (((float)coord.x - (float)dims.x / 2.0f) / (float)(dims.x / 2)) +
     y * (((float)coord.y - (float)dims.y / 2.0f) / (float)(dims.x / 2)));
  *dir = normalize((*pos) - center);
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
  pBuffer[i] = trace_box(pos, dir,
                         (float3)(-5.0f, -5.0f, -5.0f),
                         (float3)(5.0f, 5.0f, 5.0f));
}
	)";

}
