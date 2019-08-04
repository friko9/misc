/////////////////////////////////////////////////////////////////////////////////////
// Type conversion is proceded in Graph-Transition fashion	        	   //
// 										   //
// Usage:									   //
// class MyClass					                           //
// {...}									   //
// template <> struct TTransition<DestType>(MyClass,ArgT){...}			   //
// using _DestType = DestArgT<MyClass,ArgT>;                                       //
// int main(){									   //
//   MyClass x;									   //
//   DestType y = x >> _DestType(ArgT);						   //
//   }								        	   //
// User-defined classes are nodes of the graph.			        	   //
// 								        	   //
// template class TTransition<Dest,Source> represents graph transitions 	   //
// Specialisations of TTransition implement and enable transition       	   //
// between specific types - quite like adding new pair to map<T1,T2>    	   //
// 								        	   //
// To make it work just create TTransition specialisation:			   //
//    template <> struct TTransition<DestType>(MyClass,ArgT){...}		   //
// 								        	   //
// To enable TTransition to access private fields and methods add:      	   //
//  template<typename DestT,typename SourceT,typename ArgT>                        //
//  friend  DestT TTransition(SourceT s, ArgT arg);		        	   //
//  template<typename DestT,typename SourceT>		        	           //
//  friend  DestT TTransition(SourceT s);		        	           //
// to your class definition.							   //
/////////////////////////////////////////////////////////////////////////////////////

#include <type_traits>
#include <iostream>

template<typename DestT,typename SourceT,typename ArgT>
inline DestT TTransition(SourceT s, ArgT arg)
{
  static_assert(!std::is_object<SourceT>::value,
		"No TTransition<SourceT,DestT> defined");
}

template<typename DestT,typename SourceT>
inline DestT TTransition(SourceT s)
{
  static_assert(!std::is_object<SourceT>::value,
		"No TTransition<SourceT,DestT> defined");
}

template<typename SourceT,
	 typename DAT,
	 std::enable_if_t<!std::is_void<typename DAT::ArgT>::value,int> = 0 >
inline
typename DAT::DestT
transit (SourceT s,DAT arg)
{ return TTransition<typename DAT::DestT>(s,arg.value); }

template<typename SourceT,
	 typename DAT,
	 std::enable_if_t<std::is_void<typename DAT::ArgT>::value,int> = 0>
inline
typename DAT::DestT
transit (SourceT s,DAT arg)
{ return TTransition<typename DAT::DestT>(s); }

template<typename SourceT, typename DAT>
inline
typename DAT::DestT
operator >> (SourceT s,DAT arg)
{ return transit(s,arg); }

template <typename T1,typename T2 = void>
struct DestArgT
{
  using DestT = T1;
  using ArgT = T2;
  ArgT value;
  DestArgT(const DestArgT& arg) = default;
  DestArgT(DestArgT&& arg) = default;
};

template <typename T1>
struct DestArgT<T1,void>
{
  using DestT = T1;
  using ArgT = void;
};

class Querry
{
  template<typename DestT,typename SourceT,typename ArgT>
  friend  DestT TTransition(SourceT s, ArgT arg);
  template<typename DestT,typename SourceT>
  friend  DestT TTransition(SourceT s);
  std::string str;
public:
  std::string getStr() const {return str;}
  Querry(std::string arg):str(arg) {}
  Querry() = default;
  Querry(const Querry& arg) = default;
  Querry(Querry&& arg) = default;
};

struct QuerryStart : public Querry { using Querry::Querry; };
struct Select : public Querry{ using Querry::Querry;};
struct From : public Querry{ using Querry::Querry;};
struct Where : public Querry{ using Querry::Querry;};
struct Done : public Querry
{
  Done(std::string arg):Querry(arg){}
  void send(){ std::cout<<"sent: "<<getStr()<<std::endl; }
};

using _Select = DestArgT<Select,const char*>;
using _From = DestArgT<From,const char*>;
using _Where = DestArgT<Where,const char*>;
using _Done = DestArgT<Done>;

struct Warden{
    Warden(std::string arg){}
  //  Warden(std::string arg){ std::cout<<"Move "<<arg<<'\n'; }
};


template<>
inline Select TTransition<Select>(QuerryStart a,const char* what) { Warden {"Select"}; return (a.str += "select ") += what; }

template<>
inline From TTransition<From>(Select a,const char * what) {
  Warden {"From"};
  (a.str += " from ") += what;
  return std::move(a.str);
}

template<>
inline Where TTransition<Where>(From a,const char* what){
  Warden {"Where"};
  (a.str += " where ") += what;
  return std::move(a.str);
}

template<>
inline Done TTransition<Done>(Where a) { Warden {"Done"}; return a.str; }

int main()
{
  QuerryStart q;
  (q >> _Select {"*"} >> _From {"table"}  >> _Where {"id = 1"}  >> _Done {}).send();
}
