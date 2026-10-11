#include "AbstractScene.hpp"

AbstractSceneBase::AbstractSceneBase(const InitData& init)
    : IScene{ init }
{
}

void AbstractSceneBase::update()
{
    initializeIfNeeded();

    const double fadeDuration = getData().fadeDuration;
    if (not m_fadeWarmupComplete)
    {
    }
    else if (not std::isfinite(fadeDuration) || (fadeDuration <= 0.0))
    {
        m_fadeInProgress = 1.0;
    }
    else if (not m_fadeClockStarted)
    {
        m_fadeClockStarted = true;
    }
    else if (m_fadeInProgress < 1.0)
    {
        const double progress = m_fadeInProgress + (Scene::DeltaTime() / fadeDuration);
        m_fadeInProgress = std::clamp(progress, 0.0, 1.0);
        updateFadeInScene(m_fadeInProgress);
    }

    updateScene();
}

void AbstractSceneBase::updateFadeIn(double)
{
}

void AbstractSceneBase::updateFadeOut(const double t)
{
    if (not m_fadeOutStarted)
    {
        m_fadeOutStarted = true;
        m_fadeOutStartTime = Scene::Time();
    }

    const double elapsed = (Scene::Time() - m_fadeOutStartTime);
    if (elapsed > 0.0)
    {
        getData().fadeDuration = elapsed;
    }

    updateFadeOutScene(t);
}

void AbstractSceneBase::drawFadeIn(double) const
{
    drawTransitionWipe(1.0);
}

void AbstractSceneBase::drawFadeOut(const double t) const
{
    if (not m_initialized)
    {
        return;
    }

    drawScene();
    drawTransitionWipe(std::clamp(t, 0.0, 1.0));
}

void AbstractSceneBase::draw() const
{
    if (not m_initialized)
    {
        return;
    }

    drawScene();

    if (not m_fadeWarmupComplete)
    {
        m_fadeWarmupComplete = true;
        drawTransitionWipe(1.0);
    }
    else if (m_fadeInProgress < 1.0)
    {
        drawTransitionWipe(1.0 - m_fadeInProgress);
    }
}

AbstractSceneBase::~AbstractSceneBase()
{
    getData().cache.save(U"saves/cache_data.json");
}

void AbstractSceneBase::initializeIfNeeded()
{
    if (not m_initialized)
    {
        initializeScene();
        m_initialized = true;
    }
}

void AbstractSceneBase::drawTransitionWipe(const double progress)
{
    const double width = Scene::Width();
    const double height = Scene::Height();
    const double halfWidth = width / 2.0;
    const double halfHeight = height / 2.0;
    const Color wipeColor{ U"#464366" };

    RectF{ 0.0, 0.0, halfWidth * progress, halfHeight * progress }.draw(wipeColor);
    RectF{ width - halfWidth * progress, 0.0, halfWidth * progress, halfHeight * progress }.draw(wipeColor);
    RectF{ 0.0, height - halfHeight * progress, halfWidth * progress, halfHeight * progress }.draw(wipeColor);
    RectF{ width - halfWidth * progress, height - halfHeight * progress, halfWidth * progress, halfHeight * progress }.draw(wipeColor);
}
