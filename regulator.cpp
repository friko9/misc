#include <thread>
#include <chrono>
#include <utility>
#include <iostream>

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
public:
    threadsafe_queue()
    {}

    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(std::move(new_value));
    }

  bool wait_and_pop(T& value)
    {
      std::lock_guard<std::mutex> lk(mut);
	if( empty() )
	  return false;
        value=std::move(data_queue.front());
        data_queue.pop();
	return true;
    }

    bool empty() const
    {
      return data_queue.empty();
    }
  size_t size()
  {
    std::lock_guard<std::mutex> lk(mut);
    return data_queue.size();
  }
};

template <typename RegT,typename CtrlT,typename RegSPT,typename CtrlSPT>
class Regulator
{
  RegT& m_reg;
  CtrlT& m_ctrl;
  float m_k;
  RegSPT m_reg0;
  CtrlSPT m_ctrl0;
public:
  Regulator(RegT& reg,CtrlT& ctrl,RegSPT reg0, CtrlSPT ctrl0,float k)
    : m_reg(reg),m_reg0(reg0),m_ctrl(ctrl),m_ctrl0(ctrl0),m_k(k)
  {}
  //  Regulator(const Regulator&) = default;
  //Regulator(Regulator&&) = default;
  ~Regulator() = default;
public:
  void operator()(){ update(); }
  void update()
  {
    m_ctrl = m_ctrl0 - (m_k*(m_reg - m_reg0));
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

std::atomic<int> diffFlow;
std::atomic<int> buffSize;
threadsafe_queue<int> tsq;
constexpr int setpointFlow = 100;
constexpr int setpointSize1 = 10000;
constexpr int setpointSize2 = 50000;

auto reg = makeRegulator(buffSize,diffFlow,setpointSize1,setpointFlow,0.5);

using namespace std;

int main()
{
  std::thread regThread([](){
			  while(true)
			    {
			      buffSize = tsq.size();
			      reg.update();
			      cerr<<diffFlow<<':'<<tsq.size()<<endl;
			      std::this_thread::sleep_for(std::chrono::milliseconds(10));
			    }
			});
  std::thread insThread([](){
			  int x=0;
			  int local_diffFlow;
			  while(true)
			    {
			      local_diffFlow = diffFlow.load();
			      for(int i=0; i< setpointFlow + local_diffFlow/2; ++i)
				tsq.push(++x);
			      std::this_thread::sleep_for(std::chrono::milliseconds(10));
			    }
			});
    std::thread remThread([](){
			  int x=0;
			  int local_diffFlow;
			  while(true)
			    {
			      local_diffFlow = diffFlow.load();
			      for(int i=0; i< setpointFlow - local_diffFlow/2; ++i)
				{
				  if(tsq.wait_and_pop(x))
				    cout<<x<<endl;
				}
			      std::this_thread::sleep_for(std::chrono::milliseconds(20));
			    }
			});
    std::thread chBufSetpoint([]()
			      {
				int x = false;
				while(true)
				  {
				    if( x = !x )
				      reg.setpoint(setpointSize1,setpointFlow);
				    else
				      reg.setpoint(setpointSize2,setpointFlow);
				    std::this_thread::sleep_for(std::chrono::seconds(1));
				  }
			      });
  regThread.join();
  insThread.join();
  remThread.join();
  chBufSetpoint.join();
}
