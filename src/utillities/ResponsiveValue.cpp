#include "ResponsiveValue.hpp"

double ResponsiveValue::m_baseValue = 600.0;
double ResponsiveValue::m_baseFps = 30.0;

ResponsiveValue::operator double() const
{
    return (m_value + m_dvalue * (Scene::DeltaTime() * m_baseFps)) * (Scene::Height() / m_baseValue);
}

ResponsiveValue::operator float() const
{
    return static_cast<float>((m_value + m_dvalue * (Scene::DeltaTime() * m_baseFps)) * (Scene::Height() / m_baseValue));
}

ResponsiveValue::operator int() const
{
    return static_cast<int>((m_value + m_dvalue * (Scene::DeltaTime() * m_baseFps)) * (Scene::Height() / m_baseValue));
}

void ResponsiveValue::SetBaseValue(double baseValue)
{
    m_baseValue = baseValue;
}

ResponsiveValue operator"" _sc(long double value) {
    return ResponsiveValue(value);
}

ResponsiveValue operator"" _sc(unsigned long long value) {
    return ResponsiveValue(value);
}

ResponsiveValue operator"" _dsc(long double value) {
    return ResponsiveValue(0, value);
}

ResponsiveValue operator"" _dsc(unsigned long long value) {
    return ResponsiveValue(0, value);
}