#include <thread>
#include <chrono>
#include <utility>
#include <iostream>
#include <string>

#include <array>
#include <mutex>
#include <memory>
#include <atomic>

template<typename T, int T_size = 32768 >
class nonblocking_queue
{
  using ptr_t = T*;
  using container_t = std::array<std::atomic<ptr_t>,T_size>;
  using iterator_t = typename container_t::iterator;
  //static_assert( std::atomic<iterator_t>::is_always_lock_free, "atomic<array::iterator> is not lock free");
  //static_assert( std::atomic<ptr_t>::is_always_lock_free, "atomic<T*> is not lock free");
private:
  container_t space {};
  std::atomic<iterator_t> push_it {space.begin()};
  std::atomic<iterator_t> pop_it {space.begin()};
  std::atomic<size_t> size_limit { T_size };
public:
  nonblocking_queue() = default;
  bool is_empty(){ return is_empty_cell(pop_it.load()); }
  bool is_full(){ return !is_empty_cell(push_it.load()); }
  
  size_t size()
  {
    if( is_empty()) return 0;
    if( is_full() ) return T_size;
    auto push_pos = push_it.load();
    auto pop_pos = pop_it.load();
    auto diff = push_pos - pop_pos;
    diff += ( diff < 0 )? T_size : 0;
    return diff;
  }

  ptr_t try_push(ptr_t ptr)
  {
    iterator_t locked_it = push_it.load();
    iterator_t next_it = m_next(locked_it);

    if( is_empty_cell(locked_it) &&
	push_it.compare_exchange_weak(locked_it,next_it) )
      {
	locked_it->store(std::move(ptr));
	ptr = nullptr;
      }

    return std::move(ptr);
  }

  ptr_t try_push_till_full(ptr_t ptr)
  {
    iterator_t locked_it = push_it.load();
    iterator_t next_it = m_next(locked_it);

    bool empty;
    while ( (empty = is_empty_cell(locked_it)) &&
	    !push_it.compare_exchange_weak(locked_it,next_it) )
	next_it = m_next(locked_it);
    
    if( empty )
      {
	locked_it->store(std::move(ptr));
	ptr = nullptr;
      }

    return std::move(ptr);
  }

  ptr_t try_pop()
  {
    ptr_t result = nullptr;
    iterator_t locked_it = pop_it.load();
    iterator_t next_it = m_next(locked_it);

    if( !is_empty_cell(locked_it) &&
	pop_it.compare_exchange_weak(locked_it,next_it) )
      result = locked_it->exchange(nullptr);

    return std::move(result);
  }

  ptr_t try_pop_till_empty()
  {
    iterator_t locked_it = pop_it.load();
    iterator_t next_it = m_next(locked_it);

    bool filled;
    while ( (filled = !is_empty_cell(locked_it)) &&
	    !pop_it.compare_exchange_weak(locked_it,next_it) )
      next_it = m_next(locked_it);

    return filled ? locked_it->exchange(nullptr) : nullptr;
  }

  void set_size_limit(size_t limit)
  {
    //size_limit.store(limit);
  }
private:
  bool is_empty_cell( iterator_t it )
  {
    return *it == nullptr;
  }
  iterator_t m_next(iterator_t it)
  {
    it = std::next(it);
    return it != space.end() ? it : space.begin();
  }
};

nonblocking_queue<int> tsq;

std::atomic<int> inFlow {0};
std::atomic<int> outFlow {0};
std::atomic<bool> finFlag {false};
using namespace std;

constexpr int setpointSize1 = 10000;
constexpr int setpointSize2 = 50000;

int main(int argc,const char* argv[])
{
  std::thread insThread([](){
			  int* x=0;
			  x = new int {cin.get()};
			  while(!finFlag)
			    {
			      int success = 0;
			      for(int i=0;i<100;++i)
				{
				  if( tsq.try_push_till_full(x) != nullptr ) break;
				  x = new int {cin.get()};
				  ++success;
				}
			      inFlow.fetch_add(success);
			    }
			});
  std::thread remThread([](){
			  int* x=0;
			  while(!finFlag)
			    {
			      int success = 0;
			      for(int i=0;i<100;++i)
				{
				  if( (x = tsq.try_pop_till_empty()) == nullptr ) break;
				  cout<<*x;
				  delete x;
				  ++success;
				}
			      outFlow.fetch_add(success);
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
  auto start = std::chrono::steady_clock::now();
  cerr<<"time buffSize inFlow outFlow\n";
  for(unsigned int time_c = 0; time_c < 10000;)
    {
      auto now = std::chrono::steady_clock::now();
      auto time = std::chrono::duration_cast<std::chrono::milliseconds>(now-start);
      time_c = time.count();
      cerr<<time_c<<' '<<tsq.size()<<' '<<inFlow<<' '<<outFlow<<'\n';
      inFlow = 0;
      outFlow = 0;
      std::this_thread::sleep_until(now + std::chrono::milliseconds(1));
    }
  finFlag = true;
  insThread.join();
  remThread.join();
  chBufSetpoint.join();
}
