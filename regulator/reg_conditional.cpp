#include <thread>
#include <chrono>
#include <utility>
#include <iostream>
#include <string>

#include <queue>
#include <mutex>
#include <memory>
#include <atomic>
#include <condition_variable>

template<typename T>
class threadsafe_queue
{
private:
  mutable std::mutex mut;
  mutable std::mutex cv_mut;
  std::queue<T> data_queue;
  std::atomic<size_t> size_limit { 1000000 };
  std::atomic<size_t> size_low;
  std::atomic<size_t> size_high;
  std::condition_variable cv;
  std::atomic<bool>& fin;
public:
  threadsafe_queue(std::atomic<bool>& finFlag)
    :fin(finFlag)
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
    size_low.store(limit*0.3);
    size_high.store(limit*0.7);
  }
  bool empty()
  {
    std::lock_guard<std::mutex> lk(mut);
    return m_empty();
  }
  void wait_till_high()
  {
    std::unique_lock<std::mutex> ul(cv_mut);
    cv.wait(ul, [this](){return m_size() >= size_high || fin;});
  }
  void wait_till_low()
  {
    std::unique_lock<std::mutex> ul(cv_mut);
    cv.wait(ul, [this](){return m_size() <= size_low || fin;});
  }
  void notify()
  {
    cv.notify_one();
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

std::atomic<size_t> buffSize {0};
std::atomic<int> inFlow {0};
std::atomic<int> outFlow {0};
std::atomic<bool> finFlag {false};

threadsafe_queue<int> tsq(finFlag);

constexpr int setpointSize1 = 10000;
constexpr int setpointSize2 = 50000;

using namespace std;

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
				  if( !tsq.push(x) ) tsq.wait_till_low();
;
				  x = cin.get();
				  ++success;
				}
			      tsq.notify();
			      inFlow.fetch_add(success);
			    }
			});
  std::thread remThread([](){
			  int x=0;
			  while(!finFlag)
			    {
			      int success = 0;
			      for(int i=0;i<100;++i)
				{
				  if( !tsq.wait_and_pop(x) ) tsq.wait_till_high();
				  cout<<x;
				  ++success;
				}
			      tsq.notify();
			      outFlow.fetch_add(success);
			    }
			});
    std::thread chBufSetpoint([]()
  			    {
  			      int x = false;
				while(!finFlag)
				  {
				    auto now = std::chrono::steady_clock::now();
				    if( x = !x )
					tsq.set_size_limit(setpointSize1+10000);
				    else
					tsq.set_size_limit(setpointSize2+20000);
				    std::this_thread::sleep_until(now+std::chrono::seconds(1));
				  }
  			    });
  int x = 0;
  auto start = std::chrono::steady_clock::now();
  cerr<<"time buffSize inFlow outFlow\n";
  long counter = 0;
  while( ++x < 10000)
    {
      auto now = std::chrono::steady_clock::now();
      auto time = std::chrono::duration_cast<std::chrono::milliseconds>(now-start);
      cerr<<time.count()<<' '<<tsq.size()<<' '<<inFlow<<' '<<outFlow<<'\n';
      counter+= outFlow;
      inFlow = 0;
      outFlow = 0;
      std::this_thread::sleep_until(now + std::chrono::milliseconds(1));
    }
  finFlag = true;
  tsq.notify();
  tsq.notify();
  insThread.join();
  remThread.join();
  chBufSetpoint.join();
}
