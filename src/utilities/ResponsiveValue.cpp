#include "ResponsiveValue.hpp"

double ResponsiveValue::m_baseHeight = 600.0;
double ResponsiveValue::m_baseWidth = 800.0;
double ResponsiveValue::m_baseFps = 30.0;

ResponsiveValue::operator double() const
{
    return (m_value + m_dvalue * (Scene::DeltaTime() * m_baseFps)) * getMultipler();
}

ResponsiveValue::operator float() const
{
    return static_cast<float>((m_value + m_dvalue * (Scene::DeltaTime() * m_baseFps)) * getMultipler());
}

ResponsiveValue::operator int() const
{
    return static_cast<int>((m_value + m_dvalue * (Scene::DeltaTime() * m_baseFps)) * getMultipler());
}

ResponsiveValue ResponsiveValue::operator-() const {
    return ResponsiveValue(-m_value, -m_dvalue, m_use_width);
}

void ResponsiveValue::SetBaseHeight(double baseValue)
{
    m_baseHeight = baseValue;
}

void ResponsiveValue::SetBaseWidth(double baseValue)
{
    m_baseWidth = baseValue;
}

void ResponsiveValue::SetBaseFps(double baseFps)
{
    m_baseFps = baseFps;
}

double ResponsiveValue::getMultipler() const {
    if (m_use_width) return Scene::Width() / m_baseWidth;
    else return Scene::Height() / m_baseHeight;
}

ResponsiveValue operator"" _sch(long double value) {
    return ResponsiveValue(value);
}

ResponsiveValue operator"" _sch(unsigned long long value) {
    return ResponsiveValue(value);
}

ResponsiveValue operator"" _scw(long double value) {
    return ResponsiveValue(value, true);
}

ResponsiveValue operator"" _scw(unsigned long long value) {
    return ResponsiveValue(value, true);
}

ResponsiveValue operator"" _dsch(long double value) {
    return ResponsiveValue(0, static_cast<double>(value));
}

ResponsiveValue operator"" _dsch(unsigned long long value) {
    return ResponsiveValue(0, static_cast<double>(value));
}