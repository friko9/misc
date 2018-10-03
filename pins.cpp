#include <iostream>
#include <thread>
#include <chrono>
#include <cuchar>

using namespace std;

u_char out_pins[16];

template < int BYTE, int BIT >
struct Pin
{
    static constexpr int byte = BYTE;
    static constexpr int bit = BIT;
    static void set(){ out_pins[byte] |= 1u<<bit;}
    static void reset(){ out_pins[byte] &= (255u^(1<<bit));}
};

void delay(int miliseconds)
{
    std::chrono::duration<int,std::milli> d(miliseconds);
    std::this_thread::sleep_for(d);
}

template < typename PIN, int period>
void blink()
{
    constexpr size_t _half_T = period>>1;
    PIN::set();
//    cout<<(int)out_pins[PIN::byte]<<endl;
    delay(_half_T);
    PIN::reset();
//    cout<<(int)out_pins[PIN::byte]<<endl;
    delay(_half_T);
}

template < typename PIN, typename... REST>
struct PinSet
{
    static constexpr int byte = PIN::byte;
    static void set()
	{
	    PIN::set();
	    PinSet<REST...>::set();
	}
    static void reset()
	{
	    PIN::reset();
	    PinSet<REST...>::reset();
	}
};

template <typename PIN>
struct PinSet<PIN>
{
    static void set(){PIN::set();}
    static void reset(){PIN::reset();}
};



int main()
{
    using pinA1 = Pin<0xF1,0>;
    using PortC = PinSet<Pin<0,0>,Pin<0,1>,Pin<0,2>,Pin<0,3>,Pin<0,4>,Pin<0,5>,Pin<0,6>,Pin<0,7>>;

    while(true)
    {
	blink<PortC,1000>();
    }
}
