
kernel void k_add_noise(image2d_t buffer)
{
  int2 coord = (uint2)(get_global_id(0), get_global_id(1));
  int seed = coord.x + globalID;
  int t = seed ^ (seed << 11);  
  int result = coord.y ^ (coord.y >> 19) ^ (t ^ (t >> 8));

  float val = ((float)(result % 256) / 256.0f);
  write_imagef(buffer, coord, (float4)(val, val, val, 1.0));
}
