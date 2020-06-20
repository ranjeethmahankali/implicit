#include <stdint.h>
#pragma warning(push)
#pragma warning(disable : 26812)
extern "C" 
{
#define FLT_TYPE float
#define UINT32_TYPE uint32_t
#define UINT8_TYPE uint8_t
#define PACKED

#pragma pack(push, 1)
#include "primitives.h"
#pragma pack(pop)

#undef FLT_TYPE
#undef UINT32_TYPE
#undef UINT8_TYPE
};

#include <glm.hpp>
#include <optional>
#include <iostream>
#include <algorithm>
#include <memory>

constexpr size_t MAX_ENTITY_COUNT = 32;

namespace entities
{
    struct entity
    {
    protected:
        entity() = default;

    public:
        virtual uint8_t type() const = 0;
        virtual bool simple() const = 0;
        virtual void render_data_size(size_t& nBytes, size_t& nEntities, size_t& nSteps) const = 0;
        virtual void copy_render_data(
            uint8_t*& bytes, uint32_t*& offsets, uint8_t*& types, op_step*& steps,
            size_t& entityIndex, size_t& currentOffset, std::optional<uint32_t> reg = std::nullopt) const = 0;
    };

    struct csg_entity : public entity
    {
        std::shared_ptr<entity> left;
        std::shared_ptr<entity> right;
        op_type op;

        csg_entity(entity* l, entity* r, op_type op);

        virtual bool simple() const;
        virtual uint8_t type() const;
        virtual void render_data_size(size_t& nBytes, size_t& nEntities, size_t& nSteps) const;
        virtual void copy_render_data(
            uint8_t*& bytes, uint32_t*& offsets, uint8_t*& types, op_step*& steps,
            size_t& entityIndex, size_t& currentOffset, std::optional<uint32_t> reg = std::nullopt) const;
    };

    struct simple_entity : public entity
    {
        virtual bool simple() const;
        virtual void render_data_size(size_t& nBytes, size_t& nEntities, size_t& nSteps) const;
        virtual size_t num_render_bytes() const = 0;
        virtual void write_render_bytes(uint8_t*& bytes) const = 0;
        virtual void copy_render_data(
            uint8_t*& bytes, uint32_t*& offsets, uint8_t*& types, op_step*& steps,
            size_t& entityIndex, size_t& currentOffset, std::optional<uint32_t> reg = std::nullopt) const;
    };

    struct box3 : public simple_entity
    {
        glm::vec3 min;
        glm::vec3 max;
        box3(float xmin, float ymin, float zmin, float xmax, float ymax, float zmax);

        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };

    struct sphere3 : public simple_entity
    {
        glm::vec3 center;
        float radius;
        sphere3(float xcenter, float ycenter, float zcenter, float radius);
        
        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };

    struct gyroid : public simple_entity
    {
        float scale;
        float thickness;
        gyroid(float scale, float thickness);
        
        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };
}
#pragma warning(pop)