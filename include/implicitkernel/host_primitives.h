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
#include <kernels/primitives.clh>
#pragma pack(pop)

#undef FLT_TYPE
#undef UINT32_TYPE
#undef UINT8_TYPE
};

#include <glm/glm.hpp>
#include <iostream>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <unordered_set>

constexpr size_t MAX_ENTITY_COUNT = 32;

namespace entities
{
    struct entity;
    /**
     * \brief Reference to an entity.
     * This is just a shared pointer.
     */
    typedef std::shared_ptr<entity> ent_ref;

    /**
     * \brief Base type for all entities.
     */
    struct entity
    {
    protected:
        entity() = default;
        virtual ~entity() = default;

    public:
        /**
         * \brief The type of the entity known to the OpenCL code.
         * \return uint8_t The type represented as an integer.
         */
        virtual uint8_t type() const = 0;
        /**
         * \brief Gets a value indicating whether this entity is simple.
         * \return true If this entity is simple.
         * \return false If this entity is not simple.
         */
        virtual bool simple() const = 0;

        /**
         * \brief Gets the size of the render data to be copied to the device.
         * \param nBytes Will be set to the size of the render data in bytes.
         * \param nEntities The number of simple entities to be rendered.
         * \param nSteps Number csg operations in the csg tree of this entity.
         */
        virtual void render_data_size(size_t& nBytes, size_t& nEntities, size_t& nSteps) const;

        /**
         * \brief Copies the render data into the given destination buffers.
         * \param bytes The render data will be written to this buffer.
         * \param offsets The byte offsets of the simple entities (in the above buffer).
         * \param types The types of simple entities.
         * \param steps The csg steps to be performed on the simple entities.
         */
        void copy_render_data(uint8_t*& bytes, uint32_t*& offsets, uint8_t*& types, op_step*& steps) const;

        /**
         * \brief Copies the render data into the given destination buffers.
         * \param bytes The render data will be written to this buffer.
         * \param offsets The byte offsets of the simple entities (in the above buffer).
         * \param types The types of simple entities.
         * \param steps The csg steps to be performed on the simple entities.
         * \param entityIndex For internal use.
         * \param currentOffset For internal use.
         * \param reg For internal use.
         */
        virtual void copy_render_data_internal(
            uint8_t*& bytes, uint32_t*& offsets, uint8_t*& types, op_step*& steps,
            size_t& entityIndex, size_t& currentOffset, uint32_t reg,
            std::unordered_map<entity*, uint32_t>& regMap) const = 0;

        virtual void render_data_size_internal(size_t& nBytes, size_t& nSteps, std::unordered_set<entity*>& simpleEntities) const = 0;

        /**
         * \brief Returns a reference to the copy of the given entity.
         * \tparam T The type of the entity.
         * \param simple The entity.
         * \return ent_ref The reference to the copied entity.
         */
        template <typename T> static ent_ref wrap_simple(const T& simple)
        {
            ent_ref ref = std::make_shared<T>(simple);
            return ref;
        };
    };

    /**
     * \brief Represents a compound entity that combines two elements
     * with an operation. The operation can be either a boolean operation or a blending operation.
     */
    struct comp_entity : public entity
    {
        ent_ref left;
        ent_ref right;
        op_defn op;

    private:
        /**
         * \brief Construct a new comp entity object by combining two entities with an operation.
         * \param l First entity.
         * \param r Second entity.
         * \param op The operation to be performed on the two entities.
         */
        comp_entity(ent_ref l, ent_ref r, op_defn op);

        /**
         * \brief Construct a new comp entity object by applying an operation to a single entity.
         * \param op The operation to be applied.
         */
        comp_entity(ent_ref, op_defn op);

    public:
        virtual bool simple() const;
        virtual uint8_t type() const;
        virtual void render_data_size_internal(size_t& nBytes, size_t& nSteps, std::unordered_set<entity*>& simpleEntities) const;
        virtual void copy_render_data_internal(
            uint8_t*& bytes, uint32_t*& offsets, uint8_t*& types, op_step*& steps,
            size_t& entityIndex, size_t& currentOffset, uint32_t reg,
            std::unordered_map<entity*, uint32_t>& regMap) const;

        comp_entity(const comp_entity&) = delete;
        const comp_entity& operator=(const comp_entity&) = delete;

        /**
         * \brief Creates a new compound entity on the heap and returns a reference.
         * The new entity combines the given entities with an operation.
         * \tparam T1 The type of the first entity.
         * \tparam T2 The type of the second entity.
         * \param l The first entity.
         * \param r The second entity.
         * \param op The operation.
         * \return ent_ref The reference to the new entity.
         */
        template <typename T1, typename T2>
        static ent_ref make_csg(T1 l, T2 r, op_defn op)
        {
            ent_ref ls(l);
            ent_ref rs(r);
            return ent_ref(new comp_entity(ls, rs, op));
        }

        /**
         * \brief Creates a new compound entity on the heap and returns a reference.
         * The new entity combines the given entities with an operation.
         * \tparam T1 The type of the first entity.
         * \tparam T2 The type of the second entity.
         * \param l The first entity.
         * \param r The second entity.
         * \param op The operation.
         * \return ent_ref The reference to the new entity.
         */
        template <typename T1, typename T2>
        static ent_ref make_csg(T1 l, T2 r, op_type op)
        {
            op_defn opdef;
            opdef.type = op;
            opdef.data.blend_radius = 0;
            return make_csg(l, r, opdef);
        };

        /**
         * \brief Creates a new entity on the heap by applying an offset operation
         * to the given entity.
         * \tparam T The type of the entity.
         * \param ent The entity.
         * \param distance The offset distance.
         * \return ent_ref The reference to the new entity.
         */
        template <typename T>
        static ent_ref make_offset(T ent, float distance)
        {
            ent_ref ep(ent);
            op_defn op;
            op.type = op_type::OP_OFFSET;
            op.data.offset_distance = distance;
            return ent_ref(new comp_entity(ep, op));
        };

        /**
         * \brief Creates a new entity on the heap by combining the given entities with a linear blend operation.
         * \tparam T1 The type of the entities.
         * \tparam T2 The type of the entities.
         * \param l The first entity.
         * \param r The second entity.
         * \param p1 The first point for the interpolation.
         * \param p2 The second point for the interpolation.
         * \return ent_ref The reference to the new entity.
         */
        template <typename T1, typename T2>
        static ent_ref make_linblend(T1 l, T2 r, glm::vec3 p1, glm::vec3 p2)
        {
            op_defn op;
            op.type = op_type::OP_LINBLEND;
            op.data.lin_blend =
            {
                {p1.x, p1.y, p1.z},
                {p2.x, p2.y, p2.z},
            };
            return ent_ref(new comp_entity(l, r, op));
        };

        /**
         * \brief Creates a new entity on the heap by applying a smooth (s-shaped) blend operation
         * to the given entities.
         * \tparam T1 The type of the first entity.
         * \tparam T2 The type of the second entity.
         * \param l The first entity.
         * \param r The second entity.
         * \param p1 The first point for interpolation.
         * \param p2 The second point for interpolation.
         * \return ent_ref The reference to the newly created entity.
         */
        template <typename T1, typename T2>
        static ent_ref make_smoothblend(T1 l, T2 r, glm::vec3 p1, glm::vec3 p2)
        {
            op_defn op;
            op.type = op_type::OP_SMOOTHBLEND;
            op.data.smooth_blend =
            {
                {p1.x, p1.y, p1.z},
                {p2.x, p2.y, p2.z},
            };
            return ent_ref(new comp_entity(l, r, op));
        };
    };

    /**
     * \brief Base type for simple implicit entities
     * that can be represented typically a single equation.
     */
    struct simp_entity : public entity
    {
        const simp_entity& operator=(const simp_entity&) = delete;
    protected:
        simp_entity() = default;
        virtual ~simp_entity() = default;

        virtual bool simple() const;
        virtual void render_data_size_internal(size_t& nBytes, size_t& nSteps, std::unordered_set<entity*>& simpleEntities) const;
        virtual size_t num_render_bytes() const = 0;
        virtual void write_render_bytes(uint8_t*& bytes) const = 0;
        virtual void copy_render_data_internal(
            uint8_t*& bytes, uint32_t*& offsets, uint8_t*& types, op_step*& steps,
            size_t& entityIndex, size_t& currentOffset, uint32_t reg,
            std::unordered_map<entity*, uint32_t>& regMap) const;
    };

    /**
     * \brief Represents a 3 dimensional box.
     */
    struct box3 : public simp_entity
    {
        glm::vec3 center;
        glm::vec3 halfsize;
        /**
         * \brief Construct a new box3 object
         * \param xcenter The maximum x coordinate
         * \param ycenter The maximum y coordinate
         * \param zcenter The maximum z coordinate
         * \param xhalf The minimum x coordinate
         * \param xhalf The minimum y coordinate
         * \param xhalf The minimum z coordinate
         */
        box3(float xcenter, float ycenter, float zcenter, float xhalf, float yhalf, float zhalf);

        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };

    /**
     * \brief Represents a sphere.
     */
    struct sphere3 : public simp_entity
    {
        glm::vec3 center;
        float radius;
        /**
         * \brief Construct a new sphere3 object
         * \param xcenter The x coordinate of the center.
         * \param ycenter The y coordinate of the center.
         * \param zcenter The z coordinate of the center.
         * \param radius The radius.
         */
        sphere3(float xcenter, float ycenter, float zcenter, float radius);
        
        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };

    /**
     * \brief Represents a cylinder
     */
    struct cylinder3 : public simp_entity
    {
        glm::vec3 point1;
        glm::vec3 point2;
        float radius;
        /**
         * \brief Construct a new cylinder3 object
         * \param p1x The x coordinate of the start point of the cylinder.
         * \param p1y The y coordinate of the start point of the cylinder.
         * \param p1z The z coordinate of the start point of the cylinder.
         * \param p2x The x coordinate of the end point of the cylinder.
         * \param p2y The y coordinate of the end point of the cylinder.
         * \param p2z The z coordinate of the end point of the cylinder.
         * \param radius The radius of the cylinder.
         */
        cylinder3(float p1x, float p1y, float p1z, float p2x, float p2y, float p2z, float radius);

        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };

    /**
     * \brief Represents a gyroid lattice.
     */
    struct gyroid : public simp_entity
    {
        float scale;
        float thickness;
        /**
         * \brief Construct a new gyroid object
         * \param scale The scale of the lattice.
         * \param thickness The wall thickness.
         */
        gyroid(float scale, float thickness);
        
        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };

    /**
     * \brief Represents a schwarz lattice.
     */
    struct schwarz : public simp_entity
    {
        float scale;
        float thickness;
        /**
         * \brief Construct a new schwarz object
         * \param scale The scale of the lattice.
         * \param thickness The wall thickness.
         */
        schwarz(float scale, float thickness);

        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };

    /**
     * \brief Represents a halfspace, defined by plane.
     * The plane is defined by an origin and normal.
     */
    struct halfspace : public simp_entity
    {
        glm::vec3 origin;
        glm::vec3 normal;
        /**
         * \brief Construct a new halfspace object
         * \param origin The origin of the plane.
         * \param normal The normal of the plane.
         */
        halfspace(glm::vec3 origin, glm::vec3 normal);

        virtual uint8_t type() const;
        virtual size_t num_render_bytes() const;
        virtual void write_render_bytes(uint8_t*& bytes) const;
    };
}
#pragma warning(pop)