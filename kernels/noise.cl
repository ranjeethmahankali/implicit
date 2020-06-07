
uint rand(uint2 coord)
{
  uint seed = coord.x + coord.y;
  uint t = seed ^ (seed << 11);  
  uint result = coord.y ^ (coord.y >> 19) ^ (t ^ (t >> 8));
  return result;
}

kernel void k_add_noise(global uint* pBuffer)
{
  uint2 coord = (uint2)(get_global_id(0), get_global_id(1));
  /* uint result = rand(coord); */
  uint i = coord.x + (coord.y * get_global_size(0));
  pBuffer[i] = rand(coord);
}
