#include "host_primitives.h"
#include <vector>
#pragma warning(push)
#pragma warning(disable : 26812)

entities::box3::box3(float xmin, float ymin, float zmin, float xmax, float ymax, float zmax)
    :min(xmin, ymin, zmin), max(xmax, ymax, zmax)
{
}

uint8_t entities::box3::type() const
{
    return ENT_TYPE_BOX;
}

size_t entities::box3::num_render_bytes() const
{
    return
        sizeof(min) +
        sizeof(max);
}

void entities::box3::write_render_bytes(uint8_t*& bytes) const
{
    static_assert(sizeof(min) == 3 * sizeof(float));
    static_assert(sizeof(max) == 3 * sizeof(float));
    std::memcpy(bytes, &min, sizeof(min));
    bytes += sizeof(min);
    std::memcpy(bytes, &max, sizeof(max));
    bytes += sizeof(max);
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
    size_t& currentOffset, std::optional<bool> toLeft) const
{
    *(offsets++) = (uint32_t)currentOffset;
    currentOffset += num_render_bytes();
    write_render_bytes(bytes);
    *(types++) = type();
    entityIndex++;
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
    size_t& entityIndex, size_t& currentOffset, std::optional<bool> toLeft) const
{
    uint32_t lei = (uint32_t)entityIndex;
    left->copy_render_data(bytes, offsets, types, steps, entityIndex, currentOffset, true);
    uint32_t rei = (uint32_t)entityIndex;
    right->copy_render_data(bytes, offsets, types, steps, entityIndex, currentOffset, false);
    *(steps++) = {
        op,
        left->simple() ? lei : (uint32_t)REG_L,
        right->simple() ? rei : (uint32_t)REG_R,
        toLeft.value_or(true) ? (uint32_t)REG_L : (uint32_t)REG_R
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
    static_assert(sizeof(center) == 3 * sizeof(float));
    std::memcpy(bytes, &center, sizeof(center));
    bytes += sizeof(center);
    std::memcpy(bytes, &radius, sizeof(radius));
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
    std::memcpy(bytes, &scale, sizeof(scale));
    bytes += sizeof(scale);
    std::memcpy(bytes, &thickness, sizeof(thickness));
    bytes += sizeof(thickness);
}

#pragma warning(pop)