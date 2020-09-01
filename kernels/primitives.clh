#define SRC_REG 1
#define SRC_VAL 2

#define ENT_TYPE_CSG                    0
#define ENT_TYPE_BOX                    1
#define ENT_TYPE_SPHERE                 2
#define ENT_TYPE_CYLINDER               3
#define ENT_TYPE_HALFSPACE              4
#define ENT_TYPE_GYROID                 5
#define ENT_TYPE_SCHWARZ                6

typedef struct PACKED
{
  FLT_TYPE bounds[6];
} i_box;

typedef struct PACKED
{
  FLT_TYPE center[3];
  FLT_TYPE radius;
} i_sphere;

typedef struct PACKED
{
    FLT_TYPE point1[3];
    FLT_TYPE point2[3];
    FLT_TYPE radius;
} i_cylinder;

typedef struct PACKED
{
    FLT_TYPE origin[3];
    FLT_TYPE normal[3];
} i_halfspace;

typedef struct PACKED
{
  FLT_TYPE scale;
  FLT_TYPE thickness;
} i_gyroid;

typedef struct PACKED
{
    FLT_TYPE scale;
    FLT_TYPE thickness;
} i_schwarz;


typedef enum
{
    OP_NONE = 0,
    OP_UNION = 1,
    OP_INTERSECTION = 2,
    OP_SUBTRACTION = 3,

    OP_OFFSET = 8,

    OP_LINBLEND = 16,
    OP_SMOOTHBLEND = 17,
} op_type;

typedef struct PACKED
{
    float p1[3];
    float p2[3];
} lin_blend_data;

typedef struct PACKED
{
    float p1[3];
    float p2[3];
} smooth_blend_data;

typedef union PACKED
{
    float blend_radius;
    float offset_distance;
    lin_blend_data lin_blend;
    smooth_blend_data smooth_blend;
} op_data;

typedef struct PACKED
{
    op_type type;
    op_data data;
} op_defn;

typedef struct PACKED
{
    op_defn op;
    UINT32_TYPE left_src;
    UINT32_TYPE left_index;
    UINT32_TYPE right_src;
    UINT32_TYPE right_index;
    UINT32_TYPE dest;
} op_step;
