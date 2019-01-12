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

threadsafe_queue<int> tsq;

std::atomic<size_t> buffSize {0};
std::atomic<int> inFlow {0};
std::atomic<int> outFlow {0};
std::atomic<bool> finFlag {false};
using namespace std;

constexpr int setpointSize1 = 10000;
constexpr int setpointSize2 = 50000;

int main(int argc,const char* argv[])
{
  std::thread insThread([](){
			  int x=0;
			  x = cin.get();
			  while(!finFlag)
			    {
			      int success = 0;
			      for(int i=0;i<100;++i)
				{
				  if( !tsq.push(x) ) break;
				  x = cin.get();
				  ++success;
				}
			      inFlow.fetch_add(success);
			      std::this_thread::yield();
			    }
			});
  std::thread remThread([](){
			  int x=0;
			  while(!finFlag)
			    {
			      int success = 0;
			      for(int i=0;i<100;++i)
				{
				  if( !tsq.wait_and_pop(x) ) break;
				  cout<<x;
				  ++success;
				}
			      outFlow.fetch_add(success);
			      std::this_thread::yield();
			    }
			});
  std::thread chBufSetpoint([]()
  			    {
  			      int x = false;
			      while(!finFlag)
				{
				  if( x = !x )
				    tsq.set_size_limit(setpointSize1*1.2);
				  else
				    tsq.set_size_limit(setpointSize2*1.2);
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
  insThread.join();
  remThread.join();
  chBufSetpoint.join();
}
