#pragma once
#include <Siv3D.hpp>

class ResponsiveValue final
{
    public:
        ResponsiveValue(double value, double dvalue, bool use_width = false) : m_value(value), m_dvalue(dvalue), m_use_width(use_width) {}
        ResponsiveValue(double value, bool use_width = false) : m_value(value), m_dvalue(0.0), m_use_width(use_width) {}
        
        operator double() const;
        explicit operator float() const;
        explicit operator int() const;
        ResponsiveValue operator-() const;
        
        static void SetBaseHeight(double baseValue);
        static void SetBaseWidth(double baseValue);
        static void SetBaseFps(double baseFps);
    private:
        double m_value;
        double m_dvalue;
        bool m_use_width;
        static double m_baseHeight;
        static double m_baseWidth;
        static double m_baseFps;

        double getMultipler() const;
};

ResponsiveValue operator"" _sch(long double value);
ResponsiveValue operator"" _sch(unsigned long long value);
ResponsiveValue operator"" _scw(long double value);
ResponsiveValue operator"" _scw(unsigned long long value);
ResponsiveValue operator"" _dsch(long double value);
ResponsiveValue operator"" _dsch(unsigned long long value);