#pragma once
#include <Siv3D.hpp>

class ResponsiveValue final
{
    public:
        ResponsiveValue(double value, double dvalue = 0) : m_value(value), m_dvalue(dvalue) {}
        operator double() const;
        explicit operator float() const;
        explicit operator int() const;
        
        static void SetBaseValue(double baseValue);
        static void SetBaseFps(double baseFps);
    private:
        double m_value;
        double m_dvalue;
        static double m_baseValue;
        static double m_baseFps;
};

ResponsiveValue operator"" _sc(long double value);
ResponsiveValue operator"" _sc(unsigned long long value);
ResponsiveValue operator"" _dsc(long double value);
ResponsiveValue operator"" _dsc(unsigned long long value);