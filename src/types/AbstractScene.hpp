#pragma once
#include "../Common.hpp"

class AbstractSceneBase : public App::Scene
{
public:
    explicit AbstractSceneBase(const InitData& init);
    void update() override;
    void updateFadeIn(double t) override;
    void updateFadeOut(double t) override;
    void drawFadeIn(double t) const override;
    void drawFadeOut(double t) const override;
    void draw() const override;
    ~AbstractSceneBase() override;

protected:
    virtual void initializeScene() = 0;
    virtual void updateScene() = 0;
    virtual void drawScene() const = 0;
    virtual void updateFadeInScene(double) = 0;
    virtual void updateFadeOutScene(double) = 0;

private:
    void initializeIfNeeded();
    static void drawTransitionWipe(double progress);

    bool m_initialized = false;
    double m_fadeInProgress = 0.0;
    mutable bool m_fadeWarmupComplete = false;
    bool m_fadeClockStarted = false;
    double m_fadeOutStartTime = 0.0;
    bool m_fadeOutStarted = false;
};

template <typename Derived>
class AbstractScene : public AbstractSceneBase
{
public:
    using AbstractSceneBase::AbstractSceneBase;

protected:
    void initializeScene() override {
        static_cast<Derived*>(this)->initialize();
    }

    void updateScene() override {
        static_cast<Derived*>(this)->updateImpl();
    }

    void drawScene() const override {
        static_cast<const Derived*>(this)->drawImpl();
    }

    void updateFadeInScene(double t) override {
        if constexpr (requires(Derived& scene) { scene.updateFadeInImpl(t); })
        {
            static_cast<Derived*>(this)->updateFadeInImpl(t);
        }
    }

    void updateFadeOutScene(double t) override {
        if constexpr (requires(Derived& scene) { scene.updateFadeOutImpl(t); })
        {
            static_cast<Derived*>(this)->updateFadeOutImpl(t);
        }
    }
};
