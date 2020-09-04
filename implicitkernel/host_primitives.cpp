#include "host_primitives.h"
#include <vector>
#include <algorithm>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#pragma warning(push)
#pragma warning(disable : 26812)

entities::box3::box3(float xcenter, float ycenter, float zcenter, float xhalf, float yhalf, float zhalf)
    :center(xcenter, ycenter, zcenter), halfsize(xhalf, yhalf, zhalf)
{
}

uint32_t entities::box3::type() const
{
    return (uint32_t)ENT_TYPE_BOX;
}

entities::ent_ref entities::box3::clone() const
{
    return ent_ref(new box3(*this));
}

size_t entities::box3::num_render_bytes() const
{
    return sizeof(i_box);
}

void entities::box3::write_render_bytes(uint8_t*& bytes) const
{
    i_box ient = {
        center.x, center.y, center.z,
        halfsize.x, halfsize.y, halfsize.z
    };
    std::memcpy(bytes, &ient, sizeof(ient));
    bytes += sizeof(ient);
}

bool entities::simp_entity::simple() const
{
    return true;
}

void entities::simp_entity::render_data_size_internal(size_t& nBytes, size_t& nSteps, std::unordered_set<entity*>& simpleEntities) const
{
    nBytes += num_render_bytes();
    simpleEntities.insert((entity*)this);
}

void entities::simp_entity::copy_render_data_internal(
    uint8_t*& bytes, uint32_t*& offsets, uint32_t*& types, op_step*& steps, size_t& entityIndex,
    size_t& currentOffset, uint32_t reg,
    std::unordered_map<entity*, uint32_t>& regMap) const
{
    auto match = regMap.find((entity*)this);
    if (match == regMap.end())
    {
        *(offsets++) = (uint32_t)currentOffset;
        currentOffset += num_render_bytes();
        write_render_bytes(bytes);
        *(types++) = type();
        regMap.emplace((entity*)this, (uint32_t)entityIndex);
        entityIndex++;
    }
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

uint32_t entities::comp_entity::type() const
{
    return ENT_TYPE_CSG;
}

void entities::comp_entity::render_data_size_internal(size_t& nBytes, size_t& nSteps, std::unordered_set<entities::entity*>& simpleEntities) const
{
    if (left) left->render_data_size_internal(nBytes, nSteps, simpleEntities);
    if (right) right->render_data_size_internal(nBytes, nSteps, simpleEntities);
    nSteps++;
}

void entities::comp_entity::copy_render_data_internal(
    uint8_t*& bytes, uint32_t*& offsets, uint32_t*& types, op_step*& steps,
    size_t& entityIndex, size_t& currentOffset, uint32_t regVal,
    std::unordered_map<entity*, uint32_t>& regMap) const
{
    bool lcsg = (left) ? !left->simple() : false;
    bool rcsg = (right) ? !right->simple() : false;
    auto lmatch = lcsg ? regMap.end() : regMap.find(left.get());
    auto rmatch = rcsg ? regMap.end() : regMap.find(right.get());

    if (regVal >= MAX_ENTITY_COUNT - 2)
    {
        std::cerr << "Too many entities. Out of resources. Aborting...\n";
        exit(1);
    }

    uint32_t lsrc =
        lcsg ? regVal :
        lmatch == regMap.end() ? (uint32_t)entityIndex :
        lmatch->second;

    if (left)
        left->copy_render_data_internal(bytes, offsets, types, steps, entityIndex, currentOffset, regVal, regMap);

    uint32_t rsrc =
        (rcsg && lcsg) ? regVal + 1 :
        rcsg ? regVal :
        rmatch == regMap.end() ? (uint32_t)entityIndex :
        rmatch->second;

    if (right)
        right->copy_render_data_internal(bytes, offsets, types, steps, entityIndex, currentOffset, (lcsg && rcsg) ? (regVal + 1) : regVal, regMap);

    *(steps++) = {
        op,
        lcsg ? (uint32_t)SRC_REG :(uint32_t)SRC_VAL,
        lsrc,
        rcsg ? (uint32_t)SRC_REG : (uint32_t)SRC_VAL,
        rsrc,
        regVal
    };
}

entities::ent_ref entities::comp_entity::clone() const
{
    return (left && right) ? ent_ref(new comp_entity(left->clone(), right->clone(), op)) :
        left ? ent_ref(new comp_entity(left->clone(), op)) :
        ent_ref(nullptr);
}

entities::sphere3::sphere3(float xcenter, float ycenter, float zcenter, float rad)
    : center(xcenter, ycenter, zcenter), radius(rad)
{
}

uint32_t entities::sphere3::type() const
{
    return ENT_TYPE_SPHERE;
}

entities::ent_ref entities::sphere3::clone() const
{
    return ent_ref(new sphere3(*this));
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

uint32_t entities::gyroid::type() const
{
    return ENT_TYPE_GYROID;
}

entities::ent_ref entities::gyroid::clone() const
{
    return ent_ref(new gyroid(*this));
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

uint32_t entities::cylinder3::type() const
{
    return ENT_TYPE_CYLINDER;
}

entities::ent_ref entities::cylinder3::clone() const
{
    return ent_ref(new cylinder3(*this));
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

entities::schwarz::schwarz(float s, float t)
    :scale(s), thickness(t)
{
}

uint32_t entities::schwarz::type() const
{
    return ENT_TYPE_SCHWARZ;
}

entities::ent_ref entities::schwarz::clone() const
{
    return ent_ref(new schwarz(*this));
}

size_t entities::schwarz::num_render_bytes() const
{
    return sizeof(i_schwarz);
}

void entities::schwarz::write_render_bytes(uint8_t*& bytes) const
{
    i_schwarz ient = { scale, thickness };
    std::memcpy(bytes, &ient, sizeof(ient));
    bytes += sizeof(ient);
}

entities::halfspace::halfspace(glm::vec3 o, glm::vec3 n)
    :origin(o), normal(n)
{
}

uint32_t entities::halfspace::type() const
{
    return ENT_TYPE_HALFSPACE;
}

entities::ent_ref entities::halfspace::clone() const
{
    return ent_ref(new halfspace(*this));
}

size_t entities::halfspace::num_render_bytes() const
{
    return sizeof(i_halfspace);
}

void entities::halfspace::write_render_bytes(uint8_t*& bytes) const
{
    i_halfspace ient = { {origin.x, origin.y, origin.z} , {normal.x, normal.y, normal.z} };
    std::memcpy(bytes, &ient, sizeof(ient));
    bytes += sizeof(ient);
}

void entities::entity::render_data_size(size_t& nBytes, size_t& nEntities, size_t& nSteps) const
{
    std::unordered_set<entity*> simples;
    render_data_size_internal(nBytes, nSteps, simples);
    nEntities = simples.size();
}

void entities::entity::copy_render_data(uint8_t*& bytes, uint32_t*& offsets, uint32_t*& types, op_step*& steps) const
{
    size_t entityIndex = 0;
    size_t currentOffset = 0;
    std::unordered_map<entity*, uint32_t> regMap;
    copy_render_data_internal(bytes, offsets, types, steps, entityIndex, currentOffset, 0, regMap);
}

entities::ent_ref entities::entity::make_transformed(const glm::mat4& transform) const
{
    transformed_entity* ptr = new transformed_entity();
    ptr->operand = clone();
    ptr->inv_transform = glm::inverse(transform);
    return ent_ref(ptr);
}

bool entities::transformed_entity::simple() const
{
    return operand->simple();
}

uint32_t entities::transformed_entity::type() const
{
    return operand->type() | ENT_MASK_TRANSFORMED;
}

void entities::transformed_entity::render_data_size_internal(
    size_t& nBytes,
    size_t& nSteps,
    std::unordered_set<entity*>& simpleEntities) const
{
    operand->render_data_size_internal(nBytes, nSteps, simpleEntities);
    nBytes += sizeof(inv_transform);
}

void entities::transformed_entity::copy_render_data_internal(
    uint8_t*& bytes,
    uint32_t*& offsets,
    uint32_t*& types,
    op_step*& steps,
    size_t& entityIndex,
    size_t& currentOffset,
    uint32_t reg,
    std::unordered_map<entity*, uint32_t>& regMap) const
{
    size_t actualOffset = currentOffset;
    currentOffset += sizeof(inv_transform);
    std::memcpy(bytes, glm::value_ptr(inv_transform), sizeof(inv_transform));
    bytes += sizeof(inv_transform);
    throw "not implemented";
}

entities::ent_ref entities::transformed_entity::clone() const
{
    transformed_entity* ptr = new transformed_entity();
    ptr->operand = operand->clone();
    ptr->inv_transform = inv_transform;
    return ent_ref(dynamic_cast<entity*>(ptr));
}

entities::ent_ref entities::transformed_entity::make_transformed(const glm::mat4& transform) const
{
    transformed_entity* ptr = new transformed_entity();
    ptr->operand = operand->clone();
    ptr->inv_transform = inv_transform * glm::inverse(transform);
    return ent_ref(ptr);
}
