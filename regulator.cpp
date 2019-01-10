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
std::atomic<bool> finFlag {0};
threadsafe_queue<int> tsq;

std::atomic<int> inFlow {0};
std::atomic<int> outFlow {0};

constexpr int setpointFlow = 200;
constexpr int setpointSize1 = 10000;
constexpr int setpointSize2 = 50000;

using namespace std;

int main(int argc,const char* argv[])
{
  float k = (argc == 2)? std::stof(argv[1]) : 0.0f;
  auto reg = makeRegulator(buffSize,diffFlow,setpointSize1,setpointFlow,k);
  finFlag = false;
  std::thread regThread([&reg](){
			  while(!finFlag)
			    {
			      buffSize = tsq.size();
			      reg.update();
			      std::this_thread::sleep_for(std::chrono::milliseconds(1));
			    }
			});
  std::thread insThread([](){
			  int x=0;
			  x = cin.get();
			  while(!finFlag)
			    {
			      int success = 0;
			      int flow = setpointFlow + diffFlow.load()/2;
			      flow = (flow >=0 )? flow : 0;
			      for(int i=0; i< flow; ++i)
				{
				  if( !tsq.push(x) ) break;
				  x = cin.get();
				  ++success;
				}
			      inFlow += success;
			      //std::this_thread::yield();
			      //std::this_thread::sleep_for(std::chrono::milliseconds(10));
			    }
			});
  std::thread remThread([](){
			  int x=0;
			  while(!finFlag)
			    {
			      int success = 0;
			      int flow = setpointFlow - diffFlow.load()/2;
			      flow = (flow >=0 )? flow : 0;
			      for(int i=0; i< flow; ++i)
				{
				  if( !tsq.wait_and_pop(x) ) break;
				  cout<<x;
				  ++success;
				}
			      outFlow += success;
			      //std::this_thread::yield();
			      //std::this_thread::sleep_for(std::chrono::milliseconds(20));
			    }
			});
  std::thread chBufSetpoint([&reg,k]()
  			    {
  			      int x = false;
				while(!finFlag)
				  {
				    if( x = !x )
				      {
					tsq.set_size_limit(setpointSize1*1.2);
					reg.setpoint(setpointSize1,setpointFlow);
				      }
				    else
				      {
					tsq.set_size_limit(setpointSize2*1.2);
					reg.setpoint(setpointSize2,setpointFlow);
				      }
				    std::this_thread::sleep_for(std::chrono::seconds(1));
				  }
  			    });

  int x = 0;
  auto start = std::chrono::high_resolution_clock::now();
  cerr<<"time buffSize inFlow outFlow\n";
  long counter = 0;
  while( ++x < 10000)
    {
      auto now = std::chrono::high_resolution_clock::now();
      auto time = std::chrono::duration_cast<std::chrono::milliseconds>(now-start);
      cerr<<time.count()<<' '<<buffSize<<' '<<inFlow<<' '<<outFlow<<'\n';
      counter+= outFlow;
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
