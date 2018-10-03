#include <iostream>
#include <complex>
#include <typeinfo>
using namespace std;


struct Second {static constexpr char ABBR='s';};
struct Meter {static constexpr char ABBR='m';};
struct Gramm {static constexpr char ABBR='g';};
struct Kelvin {static constexpr char ABBR='K';};

struct mili{static constexpr char ABBR='m';};
struct centi {static constexpr char ABBR='c';};
struct kilo{static constexpr char ABBR='k';};

template <typename Unit1>
struct SameUnit{
    template <typename Unit2>
    static constexpr const bool same_unit(Unit2){ return false;}
    static constexpr const bool same_unit(Unit1){ return true; };
};

template <typename Unit1,typename Unit2>
constexpr const bool same_unit(){ return SameUnit<Unit1>::same_unit(Unit2());}


template <typename From,typename To>
struct Conv{};
template<>
struct Conv<mili,mili>{static constexpr long double val = 1;};
template<>
struct Conv<mili,centi>{static constexpr long double val = 10;};
template<>
struct Conv<mili,kilo>{static constexpr long double val = 0.000001;};
template<>
struct Conv<centi,mili>{static constexpr long double val = 0.1;};
template<>
struct Conv<centi,centi>{static constexpr long double val = 1;};
template<>
struct Conv<centi,kilo>{static constexpr long double val = 0.00001;};
template<>
struct Conv<kilo,kilo>{static constexpr long double val = 1;};
template<>
struct Conv<kilo,mili>{static constexpr long double val = 1000000;};
template<>
struct Conv<kilo,centi>{static constexpr long double val = 100000;};

template <typename T,typename From,typename To>
inline T f_conv(T val){return Conv<From,To>::val*val;};

template <typename T,typename Mag,typename Unit>
struct PhysValue
{
    T val;
public:
    constexpr inline PhysValue(T arg):val(arg){}
    template <typename T2,typename Mag2>
    inline PhysValue(PhysValue<T2,Mag2,Unit> arg):val(f_conv<T2,Mag2,Mag>(arg.val)){}
    inline PhysValue<T,Mag,Unit>& operator = (const PhysValue<T,Mag,Unit>& arg){ val = arg.val; return *this;}
};

template <typename T,typename T2,typename Mag,typename Mag2,typename Unit1,typename Unit2>
inline PhysValue<T,Mag,Unit1> operator + (const PhysValue<T,Mag,Unit1> arg1,const PhysValue<T2,Mag2,Unit2> arg2)
{
    static_assert( same_unit<Unit1,Unit2>(),"Units are different");
	return PhysValue<T,Mag,Unit1>(arg1.val+Conv<Mag,Mag2>::val*arg2.val);
}

template <typename T,typename T2,typename Mag,typename Mag2,typename Unit1,typename Unit2>
inline PhysValue<T,Mag,Unit1> operator - (const PhysValue<T,Mag,Unit1> arg1,const PhysValue<T2,Mag2,Unit2> arg2)
{
    static_assert( same_unit<Unit1,Unit2>(),"Units are different");
    return PhysValue<T,Mag,Unit1>(arg1.val-Conv<Mag,Mag2>::val*arg2.val);
}

template <typename T,typename Mag,typename Unit>
inline std::ostream& operator << (std::ostream& out,PhysValue<T,Mag,Unit> arg2)
{ return out<<arg2.val<<(Mag::ABBR)<<(Unit::ABBR); }

inline PhysValue<long double,kilo,Gramm> operator "" _kg (unsigned long long arg) { return PhysValue<long double,kilo,Gramm>(arg); }
inline PhysValue<long double,kilo,Gramm> operator "" _mg (unsigned long long arg) { return PhysValue<long double,mili,Gramm>(arg); }
inline PhysValue<long double,kilo,Gramm> operator "" _kg (long double arg) { return PhysValue<long double,kilo,Gramm>(arg); }
inline PhysValue<long double,kilo,Gramm> operator "" _mg (long double arg) { return PhysValue<long double,mili,Gramm>(arg); }

inline PhysValue<long double,kilo,Meter> operator "" _km (unsigned long long arg) { return PhysValue<long double,kilo,Meter>(arg); }
inline PhysValue<long double,kilo,Meter> operator "" _mm (unsigned long long arg) { return PhysValue<long double,mili,Meter>(arg); }
inline PhysValue<long double,kilo,Meter> operator "" _km (long double arg) { return PhysValue<long double,kilo,Meter>(arg); }
inline PhysValue<long double,kilo,Meter> operator "" _mm (long double arg) { return PhysValue<long double,mili,Meter>(arg); }

template <typename Mag>
using Mass = PhysValue<double,Mag,Gramm>;
template <typename Mag>
using Distance = PhysValue<double,Mag,Second>;


template <typename Mag>
using Distance_Cmp = PhysValue<complex<long double>,Mag,Second>;
using namespace std;




int main()
{
    Distance_Cmp<mili> a = complex<long double>{1,1};
    Distance_Cmp<mili> b = complex<long double>{1,1};

    b=a+b;
    
    double x = Mass<mili>(99.0_kg-999.0_mg).val;
    double y = 99000000.0-999.0;
    double z;
    cout<<"Enter value: "<<endl;
    cin>>z;
    cout<<x<<endl;
    cout<<y<<endl;
    cout<<(x+y+z)<<endl;
    auto k = 10_kg+120_mg;
    auto l = 10_km+120_mm;
    //cout<<(k+l).val<<endl;
    //cout<<10_kg+300_km<<endl;
    return 0;
}
