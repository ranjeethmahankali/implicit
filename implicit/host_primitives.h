#include <stdint.h>

namespace kernel
{
#define FLT_TYPE float
#define UINT_TYPE uint32_t
#define PACKED

#pragma pack(push, 1)
#include "primitives.h"
#pragma pack(pop)

#undef FLT_TYPE
#undef UINT32_TYPE
#undef UINT8_TYPE
};

#include <glm.hpp>

namespace entities
{
    struct entity
    {
    protected:
        entity() = default;
        virtual size_t num_render_bytes() const = 0;
        virtual void copy_render_bytes(void* const dest) const = 0;
    public:
        virtual uint8_t type() const = 0;

        virtual bool simple() const = 0;
        size_t render_data_size() const;
        void copy_render_data(void* dest) const;
    };

    struct simple_entity : public entity
    {
        virtual bool simple() const;
    };

    struct csg_entity : public entity
    {
        virtual bool simple() const;
    };

    struct box3 : public simple_entity
    {
        glm::vec3 min;
        glm::vec3 max;
        box3(float xmin, float ymin, float zmin, float xmax, float ymax, float zmax);

        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void copy_render_bytes(void* const dest) const;
    };

    struct sphere3 : public simple_entity
    {
        glm::vec3 center;
        float radius;
        sphere3(float xcenter, float ycenter, float zcenter, float radius);
        
        virtual size_t num_render_bytes() const;
        virtual void copy_render_bytes(void* const dest) const;
        virtual uint8_t type() const;
    };

    struct gyroid : public simple_entity
    {
        float scale;
        float thickness;
        gyroid(float scale, float thickness);
        
        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void copy_render_bytes(void* const dest) const;
    };
}