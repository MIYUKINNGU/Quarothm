#pragma once
#include "../Common.hpp"

template <typename Derived>
class AbstractScene : public App::Scene
{
public:
    AbstractScene(const InitData& init) : IScene{ init } { }
    void initialize();
    void update() override {
        if (!m_initialized)
        {
            m_initialized = true;
            static_cast<Derived*>(this)->initialize();
        }
        static_cast<Derived*>(this)->updateImpl();
    };
    void draw() const override {
        static_cast<const Derived*>(this)->drawImpl();
    };
    ~AbstractScene() override {
        getData().cache.save(U"saves/cache_data.json");
    };
private:
    bool m_initialized = false;
};