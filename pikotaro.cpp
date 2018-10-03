#include <iostream>
#include <utility>
using namespace std;

template <typename Pikotaro_Base,
	  template <class X> typename Type1,
	  template <class X> typename... Types
	  >
struct Pikotaro2 : public Type1<Pikotaro_Base>,public Pikotaro2<Pikotaro_Base,Types...>
{
    template<typename Arg1>
    Pikotaro2(Arg1 arg1):Type1<Pikotaro_Base>(arg1){}
    template<typename Arg1,typename...Args>
    Pikotaro2(Arg1 arg1,Args... args):Type1<Pikotaro_Base>(arg1),Pikotaro2<Pikotaro_Base,Types...>(args...){}
    Pikotaro2(){}
};

template <typename Pikotaro_Base,
	  template <class X> typename Type1
	  >
struct Pikotaro2<Pikotaro_Base,Type1> : public Type1<Pikotaro_Base>
{
    template<typename Arg1>
    Pikotaro2(Arg1 arg1):Type1<Pikotaro_Base>(arg1){}
    Pikotaro2(){}
};
class nullClass{};
template <class Y=nullClass,template <class X> typename... Types>
struct Pikotaro : public Pikotaro2<Pikotaro<Y,Types...>,Types...>//public Types<Pikotaro<Types...>>...
{
    template<typename... Args>
    Pikotaro(Args... args):Pikotaro2<Pikotaro<Y,Types...>,Types...>(args...){}
};

struct XPen
{
    string pen;
    XPen(string s):pen(s){};
};
    
template <class X>
struct Pen : public XPen {
    using XPen::XPen;
    void draw(){cout<<(static_cast<X*>(this)->X::color() + pen)<<endl;} // Here be magic
};
struct XApple{
    string apple {"Apple-"};
    string color(){return apple;}
    string color2(){return "SuperPineapple-";}
};
template <class X>
struct Apple: public XApple{};

struct XPineapple{
    string color(){return "Pineapple-";}
};

template <class X>
struct Pineapple: public XPineapple{};

template <typename X>
using ApplePen = Pikotaro<X,Pen,Apple>;
template <typename X>
using PineapplePen = Pikotaro<X,Pen,Pineapple>;
template <typename X>
using PenPineappleApplePen = Pikotaro<X,PineapplePen,ApplePen>;

int main()
{
    ApplePen<nullClass> a(string("PenA"));
    PineapplePen<nullClass> b(string("PenB"));
    PenPineappleApplePen<nullClass> c(string("PenA"),string("PenB"));

    cout<<"Apple-Pen Size:"<<sizeof(ApplePen<nullClass>)<<endl;
    a.draw();
    cout<<"Pineapple-Pen Size:"<<sizeof(PineapplePen<nullClass>)<<endl;
    b.draw();
    cout<<"Pen-Pineapple-Apple-Pen Size:"<<sizeof(PenPineappleApplePen<nullClass>)<<endl;
    c.PineapplePen<PenPineappleApplePen<nullClass>>::draw();

    return 0;
}
