#include "host_primitives.h"
#include <vector>
#include <algorithm>
#include <sstream>
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
    return sizeof(i_box);
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

bool entities::simp_entity::simple() const
{
    return true;
}

void entities::simp_entity::render_data_size(size_t& nBytes, size_t& nEntities, size_t& nSteps) const
{
    nBytes += num_render_bytes();
    nEntities++;
}

void entities::simp_entity::copy_render_data(
    uint8_t*& bytes, uint32_t*& offsets, uint8_t*& types, op_step*& steps, size_t& entityIndex,
    size_t& currentOffset, std::optional<uint32_t> reg) const
{
    *(offsets++) = (uint32_t)currentOffset;
    currentOffset += num_render_bytes();
    write_render_bytes(bytes);
    *(types++) = type();
    entityIndex++;
}

entities::comp_entity::comp_entity(std::shared_ptr<entity> l, std::shared_ptr<entity> r, op_defn op)
    :left(l), right(r), op(op)
{
}

entities::comp_entity::comp_entity(std::shared_ptr<entity> a, op_defn o)
    :left(a), right(nullptr), op(o)
{
}

bool entities::comp_entity::simple() const
{
    return false;
}

uint8_t entities::comp_entity::type() const
{
    return ENT_TYPE_CSG;
}

void entities::comp_entity::render_data_size(size_t& nBytes, size_t& nEntities, size_t& nSteps) const
{
    if (left) left->render_data_size(nBytes, nEntities, nSteps);
    if (right) right->render_data_size(nBytes, nEntities, nSteps);
    nSteps++;
}

void entities::comp_entity::copy_render_data(
    uint8_t*& bytes, uint32_t*& offsets, uint8_t*& types, op_step*& steps,
    size_t& entityIndex, size_t& currentOffset, std::optional<uint32_t> reg) const
{
    uint32_t regVal = reg.value_or(0);
    bool lcsg = (left) ? !left->simple() : false;
    bool rcsg = (right) ? !right->simple() : false;

    if (regVal >= MAX_ENTITY_COUNT - 2)
    {
        std::cerr << "Too many entities. Out of resources. Aborting...\n";
        exit(1);
    }

    uint32_t lsrc = lcsg ? regVal : (uint32_t)entityIndex;
    if (left)
        left->copy_render_data(bytes, offsets, types, steps, entityIndex, currentOffset, regVal);
    uint32_t rsrc = (rcsg && lcsg) ? regVal + 1 : (rcsg ? regVal : (uint32_t)entityIndex);
    if (right)
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
    return sizeof(i_sphere);
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
    return sizeof(i_gyroid);
}

void entities::gyroid::write_render_bytes(uint8_t*& bytes) const
{
    i_gyroid ient = { scale, thickness };
    std::memcpy(bytes, &ient, sizeof(ient));
    bytes += sizeof(ient);
}

#pragma warning(pop)

entities::cylinder3::cylinder3(float p1x, float p1y, float p1z, float p2x, float p2y, float p2z, float rad)
    : point1(p1x, p1y, p1z), point2(p2x, p2y, p2z), radius(rad)
{
}

uint8_t entities::cylinder3::type() const
{
    return ENT_TYPE_CYLINDER;
}

size_t entities::cylinder3::num_render_bytes() const
{
    return sizeof(i_cylinder);
}

void entities::cylinder3::write_render_bytes(uint8_t*& bytes) const
{
    i_cylinder cyl = { point1.x, point1.y, point1.z, point2.x, point2.y, point2.z, radius };
    std::memcpy(bytes, &cyl, sizeof(cyl));
    bytes += sizeof(cyl);
}

entities::ent_ref entities::get_ent_ref(std::string refStr)
{
    auto match = s_entMap.find(refStr);
    if (match == s_entMap.end())
    {
        throw "Cannot find entitiy";
    }
    return match->second;
}

std::string entities::map_ent(ent_ref ref)
{
    const void* ptr = static_cast<const void*>(ref.get());
    std::stringstream ss;
    ss << "ent_ref@" << ptr;
    std::string refstr = ss.str();
    s_entMap[refstr] = ref;
    return refstr;
}
