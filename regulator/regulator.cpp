#include <thread>
#include <chrono>
#include <utility>
#include <iostream>
#include <string>

#include <queue>
#include <mutex>
#include <memory>
#include <atomic>

template<typename T>
class threadsafe_queue
{
private:
  mutable std::mutex mut;
  std::queue<T> data_queue;
  std::atomic<size_t> size_limit { 1000000 };
public:
  threadsafe_queue()
  {}

  bool push(T new_value)
  {
    std::lock_guard<std::mutex> lk(mut);
    if ( m_full() )
      return false;
    data_queue.push(std::move(new_value));
    return true;
  }
  bool wait_and_pop(T& value)
  {
    std::lock_guard<std::mutex> lk(mut);
    if( m_empty() )
      return false;
    value=std::move(data_queue.front());
    data_queue.pop();
    return true;
  }
  size_t size()
  {
    std::lock_guard<std::mutex> lk(mut);
    return m_size();
  }
  void set_size_limit(size_t limit)
  {
    size_limit.store(limit);
  }
private:
  size_t m_size() const
  {
    return data_queue.size();
  }
  bool m_full() const
  {
    return m_size() >= size_limit;
  }
  bool m_empty() const
  {
    return data_queue.empty();
  }
};

template <typename RegT,typename CtrlT,typename RegSPT,typename CtrlSPT>
class Regulator
{
  const RegT& m_reg;
  CtrlT& m_ctrl;
  float m_k;
  RegSPT m_reg0;
  CtrlSPT m_ctrl0;
public:
  Regulator(RegT& reg,CtrlT& ctrl,RegSPT reg0, CtrlSPT ctrl0,float k)
    : m_reg(reg),m_reg0(reg0),m_ctrl(ctrl),m_ctrl0(ctrl0),m_k(k)
  {
    m_ctrl = 0;
  }
  Regulator(const Regulator&) = default;
  Regulator(Regulator&&) = default;
  ~Regulator() = default;
public:
  void operator()(){ update(); }
  void update()
  {
    m_ctrl = m_ctrl0 - static_cast<CtrlT>(m_k*(m_reg - m_reg0));
  }
  void setpoint(RegSPT reg0, CtrlSPT ctrl0)
  {
    m_reg0 = reg0;
    m_ctrl0 = ctrl0;
  }
};

template <typename RegT,typename CtrlT,typename RegSPT,typename CtrlSPT>
Regulator<RegT,CtrlT,RegSPT,CtrlSPT> makeRegulator(RegT& reg,CtrlT& ctrl,RegSPT reg0, CtrlSPT ctrl0,float k)
{
  return Regulator<RegT,CtrlT,RegSPT,CtrlSPT>(reg,ctrl,reg0,ctrl0,k);
}

std::atomic<int> diffFlow {0};
std::atomic<int> buffSize {0};
std::atomic<bool> finFlag {false};
threadsafe_queue<int> tsq;

std::atomic<int> inFlow {0};
std::atomic<int> outFlow {0};

constexpr int setpointFlow = 2000;
constexpr int setpointSize1 = 10000;
constexpr int setpointSize2 = 50000;

using namespace std;

int main(int argc,const char* argv[])
{
  float k = (argc == 2)? std::stof(argv[1]) : 0.0f;
  auto reg = makeRegulator(buffSize,diffFlow,setpointSize1,setpointFlow,k);

  std::thread regThread([&reg](){
			  while(!finFlag)
			    {
			      auto now = std::chrono::steady_clock::now();
			      buffSize = tsq.size();
			      reg.update();
			      std::this_thread::sleep_until(now+std::chrono::milliseconds(1));
			    }
			});
  std::thread insThread([](){
			  int x=0;
			  x = cin.get();
			  while(!finFlag)
			    {
			      int setpoint = setpointFlow + diffFlow.load();
			      int flow = inFlow.load();
			      int push_err = false;
			      for(int success = 0; !finFlag & !push_err & flow < setpoint; success = 0 )
				{
				  auto min_delta = std::min(100,setpoint-flow);
				  for(int i = 0; i < min_delta; ++i )
				    {
				      if( push_err = !tsq.push(x) ) break;
				      x = cin.get();
				      ++success;
				    }
				  flow = inFlow.fetch_add(success) + success;
				}
			    sadfrog:
			      std::this_thread::yield();
			    }
			});
  std::thread remThread([](){
  			  int x=0;
  			  while(!finFlag)
  			    {
  			      int success = 0;
  			      while(success < 100 )
  				{
  				  if( !tsq.wait_and_pop(x) ) break;
  				  cout<<x;
  				  ++success;
  				}
  			      outFlow.fetch_add(success);
  			    }
  			});
  std::thread chBufSetpoint([&reg,k]()
  			    {
  			      int x = false;
				while(!finFlag)
				  {
				    auto now = std::chrono::steady_clock::now();
				    if( x = !x )
				      {
					tsq.set_size_limit(setpointSize1+10000);
					reg.setpoint(setpointSize1,setpointFlow);
				      }
				    else
				      {
					tsq.set_size_limit(setpointSize2+20000);
					reg.setpoint(setpointSize2,setpointFlow);
				      }
				    std::this_thread::sleep_until(now+std::chrono::seconds(1));
				  }
  			    });

  auto start = std::chrono::steady_clock::now();
  cerr<<"time buffSize inFlow outFlow\n";
  for(unsigned int time_c = 0; time_c < 10000;)
    {
      auto now = std::chrono::steady_clock::now();
      auto time = std::chrono::duration_cast<std::chrono::milliseconds>(now-start);
      time_c = time.count();
      cerr<<time_c<<' '<<buffSize<<' '<<inFlow<<' '<<outFlow<<'\n';
      inFlow = 0;
      outFlow = 0;
      std::this_thread::sleep_until(now + std::chrono::milliseconds(1));
    }
  finFlag = true;
  regThread.join();
  insThread.join();
  remThread.join();
  chBufSetpoint.join();
}
