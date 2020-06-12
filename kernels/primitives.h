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

union i_entity
{
  struct i_box box;
  struct i_sphere sphere;
  struct i_gyroid gyroid;
  struct i_boolean_union boolean_union;
};

struct wrapper
{
  union i_entity entity;
  UINT_TYPE type;
} PACKED;
