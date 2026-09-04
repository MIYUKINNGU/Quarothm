#include "ResponsiveValue.hpp"

double ResponsiveValue::m_baseValue = 600.0;

ResponsiveValue::operator double() const
{
    return m_value * (Scene::Height() / m_baseValue);
}

ResponsiveValue::operator float() const
{
    return static_cast<float>(m_value * (Scene::Height() / m_baseValue));
}

ResponsiveValue::operator int() const
{
    return static_cast<int>(m_value * (Scene::Height() / m_baseValue));
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