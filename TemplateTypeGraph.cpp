/////////////////////////////////////////////////////////////////////////////////////
// Type conversion is proceded in Graph-Transition fashion	        	   //
// 										   //
// Usage:									   //
// class MyClass : public Transitional<MyClass>					   //
// {...}									   //
// template <> struct TTransition<MyClass,DestType>{				   //
//   DestType transition(MyClass){...}						   //
// };										   //
// int main(){									   //
//   MyClass x;									   //
//   DestType y = x.to<DestType>()						   //
// 								        	   //
// User-defined classes are nodes of the graph.			        	   //
// 								        	   //
// template class TTransition<Source,Dest> represents graph transitions 	   //
// Specialisations of TTransition implement and enable transition       	   //
// between specific types - quite like adding new pair to map<T1,T2>    	   //
// 								        	   //
// template class Transitional<SourceT> is an interface-class	        	   //
// it implements template method to<DestT>(), and to<Dest>(Args...)     	   //
// which are used for type conversion.				      		   //
// 								      		   //
// To make it work for your just inherit from Transitional:	      		   //
//   class MyClass : public Transitional<MyClass>		        	   //
// and create TTransition specialisation:			        	   //
//     template <> struct TTransition<MyClass,DestType>{	        	   //
//     DestType transition(MyClass) const {...}			        	   //
//     or even							        	   //
//     DestType transition(MyClass,OtherArgs...) const {...}	        	   //
//   };								        	   //
// 								        	   //
// To enable TTransition to access private fields and methods add:      	   //
//   template<typename,typename>  friend class TTransition;	        	   //
// to your class definition.							   //
/////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////
// Also enabled the possibility to create cascade object transitions in flow like    //
// eye-pleasing way using intermediate argument objects				     //
// 										     //
// Usage:									     //
//   class MyClass : public OperatorTransitional<MyClass>			     //
//   {...};									     //
// // defining transitions							     //
// // ...									     //
//   using _MyOtherClass1 = DestArgT<MyOtherClass,std::tuple<ArgsT...>>;	     //
//   using _MyOtherClass2 = DestArgT<MyOtherClass,std::tuple<ArgsT...>>;	     //
//   int main(){								     //
//   MyClass x;  								     //
//   MyOtherClass2 y = x >> _MyOtherClass1 { args...} >> _MyOtherClass2 { args... }; //
//   }										     //
///////////////////////////////////////////////////////////////////////////////////////
    
#include <type_traits>
#include <iostream>

template<typename SourceT,typename DestT>
struct TTransition
{
  static_assert(!std::is_object<SourceT>::value,
		"No TTransition<SourceT,DestT> defined");
  DestT transit(SourceT t){ };
};

template<typename SourceT>
struct Transitional
{
  static_assert(std::is_object<SourceT>::value,
		"Source Type is not a valid object-type");
  template<typename DestT, typename ArgT>
  DestT to(ArgT arg) const
  {
    return TTransition<SourceT,DestT>().transit(*static_cast<const SourceT*>(this),arg);
  }
  template<typename DestT>
  DestT to() const
  {
    return TTransition<SourceT,DestT>().transit(*static_cast<const SourceT*>(this));
  }
};

template <typename T1,typename T2 = void>
struct DestArgT
{
  using DestT = T1;
  using ArgT = T2;
  ArgT value;
};

template <typename T1>
struct DestArgT<T1,void>
{
  using DestT = T1;
  using ArgT = void;
};

template<typename SourceT>
struct OperatorTransitional : public Transitional<SourceT>
{
  template<typename DAT,
	   typename std::enable_if_t<!std::is_void<typename DAT::ArgT>::value,bool> = true >
  typename DAT::DestT operator >> (DAT arg) const
  { return Transitional<SourceT>::template to<typename DAT::DestT>(arg.value); }

  template<typename DAT,
	   typename std::enable_if_t<std::is_void<typename DAT::ArgT>::value,bool> = true >
  typename DAT::DestT operator >> (DAT arg) const
  { return Transitional<SourceT>::template to<typename DAT::DestT>(); }
};

class Querry
{
  template<typename,typename>  friend class TTransition;
  std::string str;
public:
  std::string getStr() const {return str;}
  Querry(std::string arg):str(arg) {}
  Querry(){}
  ~Querry() = default;
};

struct QuerryStart: public OperatorTransitional<QuerryStart>{ };
struct Select : public Querry,public OperatorTransitional<Select>{ using Querry::Querry;};
struct From : public Querry,public OperatorTransitional<From>{ using Querry::Querry;};
struct Where : public Querry,public OperatorTransitional<Where>{ using Querry::Querry;};
struct Done : public Querry
{
  Done(std::string arg):Querry(arg){}
  void send(){ std::cout<<"sent: "<<getStr()<<std::endl; }
};

using _Select = DestArgT<Select,const char*>;
using _From = DestArgT<From,const char*>;
using _Where = DestArgT<Where,const char*>;
using _Done = DestArgT<Done>;

template<> struct TTransition<QuerryStart,Select> {
  Select transit(const QuerryStart a,const std::string what) const { return "select " + what; }
};
template<> struct TTransition<Select,From> {
  From transit(const Select a,const std::string what) const { return a.getStr() + " from " + what ; }
};
// template<> struct TTransition<Select,Where> {
//   Where transit(const Select a,const std::string what) const { return a.getStr() + " from * where " + what; }
// };
template<> struct TTransition<From,Where> {
  Where transit(const From a,const std::string what) const { return a.getStr() + " where " + what; }
};
template<> struct TTransition<Where,Done> {
  Done transit(const Where a) const { return a.getStr() + ";"; }
};

int main()
{
  QuerryStart q;
  auto querry = q >> _Select {"*"}  >> _From {"table"}  >> _Where {"id = 1"}  >> _Done {};
  querry.send();
}
