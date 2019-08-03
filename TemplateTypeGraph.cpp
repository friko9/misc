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

template<typename SourceT>
struct DefaultTransitional : public Transitional<SourceT>
{
  template<typename DestT, typename ArgT>
  DestT _(ArgT arg) const
  { return this->template to<DestT,ArgT>(arg); }
  
  template<typename DestT>
  DestT _() const
  { return this->template to<DestT>(); }
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

struct QuerryStart: public DefaultTransitional<QuerryStart>{ };
struct Select : public Querry,public DefaultTransitional<Select>{ using Querry::Querry;};
struct From : public Querry,public DefaultTransitional<From>{ using Querry::Querry;};
struct Where : public Querry,public DefaultTransitional<Where>{ using Querry::Querry;};
struct Done : public Querry
{
  Done(std::string arg):Querry(arg){}
  void send(){ std::cout<<"sent: "<<getStr()<<std::endl; }
};

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
  q._<Select>("*")
    ._<From>("table")
    ._<Where>("id = 1")
    ._<Done>()
    .send();
}