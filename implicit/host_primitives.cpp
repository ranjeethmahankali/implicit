#include "host_primitives.h"
#include <vector>
#include <algorithm>
#pragma warning(push)
#pragma warning(disable : 26812)

entities::box3::box3(float xmin, float ymin, float zmin, float xmax, float ymax, float zmax)
    :min(xmin, ymin, zmin), max(xmax, ymax, zmax)
{
}

uint8_t entities::box3::type() const
{
    return (uint8_t)ENT_TYPE_BOX;
}

size_t entities::box3::num_render_bytes() const
{
    return
        sizeof(min) +
        sizeof(max);
}

void entities::box3::write_render_bytes(uint8_t*& bytes) const
{
    i_box ient = {
        min.x, min.y, min.z,
        max.x, max.y, max.z
    };
    std::memcpy(bytes, &ient, sizeof(ient));
    bytes += sizeof(ient);
}

bool entities::simple_entity::simple() const
{
    return true;
}

void entities::simple_entity::render_data_size(size_t& nBytes, size_t& nEntities, size_t& nSteps) const
{
    nBytes += num_render_bytes();
    nEntities++;
}

void entities::simple_entity::copy_render_data(
    uint8_t*& bytes, uint32_t*& offsets, uint8_t*& types, op_step*& steps, size_t& entityIndex,
    size_t& currentOffset, std::optional<uint32_t> reg) const
{
    *(offsets++) = (uint32_t)currentOffset;
    currentOffset += num_render_bytes();
    write_render_bytes(bytes);
    *(types++) = type();
    entityIndex++;
}

entities::csg_entity::csg_entity(entity* l, entity* r, op_type o)
    : left(l), right(r), op(o)
{
}

bool entities::csg_entity::simple() const
{
    return false;
}

uint8_t entities::csg_entity::type() const
{
    return ENT_TYPE_CSG;
}

void entities::csg_entity::render_data_size(size_t& nBytes, size_t& nEntities, size_t& nSteps) const
{
    left->render_data_size(nBytes, nEntities, nSteps);
    right->render_data_size(nBytes, nEntities, nSteps);
    nSteps++;
}

void entities::csg_entity::copy_render_data(
    uint8_t*& bytes, uint32_t*& offsets, uint8_t*& types, op_step*& steps,
    size_t& entityIndex, size_t& currentOffset, std::optional<uint32_t> reg) const
{
    uint32_t regVal = reg.value_or(0);
    bool lcsg = !left->simple();
    bool rcsg = !right->simple();

    if (regVal >= MAX_ENTITY_COUNT - 2)
    {
        std::cerr << "Too many entities. Out of resources. Aborting...\n";
        exit(1);
    }

    uint32_t lsrc = lcsg ? regVal : (uint32_t)entityIndex;
    left->copy_render_data(bytes, offsets, types, steps, entityIndex, currentOffset, regVal);
    uint32_t rsrc = (rcsg && lcsg) ? regVal + 1 : (rcsg ? regVal : (uint32_t)entityIndex);
    right->copy_render_data(bytes, offsets, types, steps, entityIndex, currentOffset, (lcsg && rcsg) ? (regVal + 1) : regVal);

    *(steps++) = {
        op,
        lcsg ? (uint32_t)SRC_REG :(uint32_t)SRC_VAL,
        lsrc,
        rcsg ? (uint32_t)SRC_REG : (uint32_t)SRC_VAL,
        rsrc,
        regVal
    };
}

entities::sphere3::sphere3(float xcenter, float ycenter, float zcenter, float rad)
    : center(xcenter, ycenter, zcenter), radius(rad)
{
}

uint8_t entities::sphere3::type() const
{
    return ENT_TYPE_SPHERE;
}

size_t entities::sphere3::num_render_bytes() const
{
    return
        sizeof(center) +
        sizeof(radius);
}

void entities::sphere3::write_render_bytes(uint8_t*& bytes) const
{
    i_sphere ient = {
        { center.x, center.y, center.z },
        radius
    };
    std::memcpy(bytes, &ient, sizeof(ient));
    bytes += sizeof(ient);
}

entities::gyroid::gyroid(float sc, float th)
    : scale(sc), thickness(th)
{
}

uint8_t entities::gyroid::type() const
{
    return ENT_TYPE_GYROID;
}

size_t entities::gyroid::num_render_bytes() const
{
    return
        sizeof(scale) +
        sizeof(thickness);
}

void entities::gyroid::write_render_bytes(uint8_t*& bytes) const
{
    i_gyroid ient = { scale, thickness };
    std::memcpy(bytes, &ient, sizeof(ient));
    bytes += sizeof(ient);
}

#pragma warning(pop)