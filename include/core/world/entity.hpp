#ifndef PAPERMC_CORE_WORLD_ENTITY_HPP
#define PAPERMC_CORE_WORLD_ENTITY_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <expected>
#include <string_view>
#include "core/world/block.hpp"

namespace papermc::core::world {

using EntityID = uint32_t;

struct Position {
    double x{0.0};
    double y{0.0};
    double z{0.0};
    float yaw{0.0f};
    float pitch{0.0f};
};

struct Velocity {
    double vx{0.0};
    double vy{0.0};
    double vz{0.0};
};

/**
 * Cache-friendly Data-Oriented Entity representation.
 */
class alignas(32) Entity {
public:
    explicit Entity(EntityID id, std::string type_id)
        : id_(id), type_id_(std::move(type_id)) {}

    [[nodiscard]] EntityID id() const noexcept { return id_; }
    [[nodiscard]] std::string_view type_id() const noexcept { return type_id_; }

    [[nodiscard]] const Position& position() const noexcept { return position_; }
    void set_position(const Position& pos) noexcept { position_ = pos; }

    [[nodiscard]] const Velocity& velocity() const noexcept { return velocity_; }
    void set_velocity(const Velocity& vel) noexcept { velocity_ = vel; }

private:
    EntityID id_;
    std::string type_id_;
    Position position_{};
    Velocity velocity_{};
};

/**
 * Entity ECS Repository Manager.
 */
class EntityManager {
public:
    EntityManager() = default;

    EntityID spawn_entity(std::string_view type_id) {
        EntityID id = next_id_++;
        entities_.push_back(std::make_unique<Entity>(id, std::string(type_id)));
        return id;
    }

    [[nodiscard]] std::expected<Entity*, std::string_view> get_entity(EntityID id) noexcept {
        for (auto& entity : entities_) {
            if (entity->id() == id) {
                return entity.get();
            }
        }
        return std::unexpected("Entity ID not found");
    }

    [[nodiscard]] std::size_t count() const noexcept {
        return entities_.size();
    }

private:
    EntityID next_id_{1};
    std::vector<std::unique_ptr<Entity>> entities_;
};

} // namespace papermc::core::world

#endif // PAPERMC_CORE_WORLD_ENTITY_HPP
