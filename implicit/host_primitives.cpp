#include "host_primitives.h"
#include <vector>

static std::vector<wrapper> s_entities;

size_t entities::num_entities()
{
    return s_entities.size();
}

void entities::push_back(const wrapper& entity)
{
    s_entities.push_back(entity);
}

bool entities::is_valid_entity(const wrapper& entity)
{
    switch (entity.type)
    {
    case ENT_TYPE_BOX: return is_valid_box(entity.entity.box);
    case ENT_TYPE_SPHERE: return is_valid_sphere(entity.entity.sphere);
    case ENT_TYPE_GYROID: return is_valid_gyroid(entity.entity.gyroid);
    case ENT_TYPE_BOOLEAN_UNION: return is_valid_union(entity.entity.boolean_union);
    case ENT_TYPE_BOOLEAN_INTERSECTION: return is_valid_intersection(entity.entity.boolean_intersection);
    default: return false;
    }
}

bool entities::is_valid_box(const i_box& box)
{
    return
        box.bounds[3] > box.bounds[0] &&
        box.bounds[4] > box.bounds[1] &&
        box.bounds[5] > box.bounds[2];
}

bool entities::is_valid_sphere(const i_sphere& sphere)
{
    return sphere.radius > 0;
}

bool entities::is_valid_gyroid(const i_gyroid& gyroid)
{
    return gyroid.scale > 0.0f && gyroid.thickness > 0.0f;
}

bool entities::is_valid_union(const i_boolean_union& booleanUnion)
{
    if (booleanUnion.index_a >= s_entities.size() || booleanUnion.index_b >= s_entities.size())
        return false;
    return is_valid_entity(s_entities[booleanUnion.index_a]) && is_valid_entity(s_entities[booleanUnion.index_b]);
}

bool entities::is_valid_intersection(const i_boolean_intersection& booleanIntersection)
{
    if (booleanIntersection.index_a >= s_entities.size() || booleanIntersection.index_b >= s_entities.size())
        return false;
    return is_valid_entity(s_entities[booleanIntersection.index_a]) && is_valid_entity(s_entities[booleanIntersection.index_b]);
}
