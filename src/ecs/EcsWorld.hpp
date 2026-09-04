#pragma once

#include <cstdint>
#include <memory>
#include <typeindex>
#include <unordered_map>

using Entity = uint32_t;

class EcsWorld
{
public:
    [[nodiscard]] Entity createEntity() { return m_nextEntity++; }

    void clear()
    {
        m_storages.clear();
        m_nextEntity = 1;
    }

    void destroyEntity(const Entity entity)
    {
        for (auto& [_, storage] : m_storages)
        {
            storage->erase(entity);
        }
    }

    template <class Component, class... Args>
    Component& addComponent(const Entity entity, Args&&... args)
    {
        auto& components = storage<Component>().values;
        return components.insert_or_assign(entity, Component{ std::forward<Args>(args)... }).first->second;
    }

    template <class Component>
    [[nodiscard]] Component* getComponent(const Entity entity)
    {
        auto& components = storage<Component>().values;
        const auto it = components.find(entity);
        return (it == components.end()) ? nullptr : &it->second;
    }

    template <class Component>
    void removeComponent(const Entity entity)
    {
        storage<Component>().values.erase(entity);
    }

    template <class Component, class Function>
    void each(Function&& function)
    {
        for (auto& [entity, component] : storage<Component>().values)
        {
            function(entity, component);
        }
    }

private:
    struct IStorage { virtual ~IStorage() = default; virtual void erase(Entity entity) = 0; };

    template <class Component>
    struct Storage final : IStorage
    {
        std::unordered_map<Entity, Component> values;
        void erase(const Entity entity) override { values.erase(entity); }
    };

    template <class Component>
    Storage<Component>& storage()
    {
        const std::type_index type{ typeid(Component) };
        auto it = m_storages.find(type);
        if (it == m_storages.end())
        {
            it = m_storages.emplace(type, std::make_unique<Storage<Component>>()).first;
        }
        return static_cast<Storage<Component>&>(*it->second);
    }

    Entity m_nextEntity = 1;
    std::unordered_map<std::type_index, std::unique_ptr<IStorage>> m_storages;
};
