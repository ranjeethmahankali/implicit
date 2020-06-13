#include <stdint.h>
#define FLT_TYPE float
#define UINT_TYPE uint32_t
#define PACKED

#pragma pack(push, 1)
#include "primitives.h"
#pragma pack(pop)

#undef FLT_TYPE
#undef UINT32_TYPE
#undef UINT8_TYPE

namespace entities
{
    size_t num_entities();

    void push_back(const wrapper& entity);

    bool is_valid_entity(const wrapper& entity);

    bool is_valid_box(const i_box& box);

    bool is_valid_sphere(const i_sphere& sphere);

    bool is_valid_gyroid(const i_gyroid& gyroid);
}