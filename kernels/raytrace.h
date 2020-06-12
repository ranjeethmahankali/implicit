#define DX 0.0001f
#define BOUND 10.0f

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

/*Macro to perform sphere tracing for a given implicit function that
terminates based on the given tolerance or max-iterations.*/
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
      if (fabs(pt.x) > BOUND ||                                         \
          fabs(pt.y) > BOUND ||                                         \
          fabs(pt.z) > BOUND) break;                                    \
    }                                                                   \
    float d = dot(normalize(norm), -dir);                               \
    float3 color = (float3)(0.2f,0.2f,0.2f)*(1.0f-d) + (float3)(0.9f,0.9f,0.9f)*d; \
    return found ? colorToInt(color) : 0xff101010;                      \
}
