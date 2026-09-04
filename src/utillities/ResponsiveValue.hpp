#pragma once
#include <Siv3D.hpp>

class ResponsiveValue final
{
    public:
        ResponsiveValue(double value) : m_value(value) {}
        operator double() const;
        explicit operator float() const;
        explicit operator int() const;
        
        static void SetBaseValue(double baseValue);
    private:
        double m_value;
        static double m_baseValue;
};

ResponsiveValue operator"" _sc(long double value);
ResponsiveValue operator"" _sc(unsigned long long value);