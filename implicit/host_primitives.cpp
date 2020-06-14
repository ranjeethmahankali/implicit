#include "host_primitives.h"
#include <vector>

size_t entities::entity::render_data_size() const
{
    return sizeof(uint32_t) + num_render_bytes();
}

void entities::entity::copy_render_data(void* dest) const
{
    uint32_t t = type();
    std::memcpy(dest, &t, sizeof(t));
    copy_render_bytes((uint8_t*)dest + sizeof(t));
}

entities::box3::box3(float xmin, float ymin, float zmin, float xmax, float ymax, float zmax)
    :min(xmin, ymin, zmin), max(xmax, ymax, zmax)
{
}

uint8_t entities::box3::type() const
{
    return ENT_TYPE_BOX;
}

bool entities::simple_entity::simple() const
{
    return true;
}

bool entities::csg_entity::simple() const
{
    return false;
}

entities::sphere3::sphere3(float xcenter, float ycenter, float zcenter, float rad)
    : center(xcenter, ycenter, zcenter), radius(rad)
{
}

uint8_t entities::sphere3::type() const
{
    return ENT_TYPE_SPHERE;
}

entities::gyroid::gyroid(float sc, float th)
    : scale(sc), thickness(th)
{
}

uint8_t entities::gyroid::type() const
{
    return ENT_TYPE_GYROID;
}
