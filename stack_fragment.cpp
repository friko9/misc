#include <iostream>
#include <algorithm>
using namespace std;

#define PRT(X) \
    size += sizeof(X); \
    min_addr = min(min_addr,(void*)(&X));	\
    max_addr = max(max_addr,(void*)(&X));		\
    cout<<(#X"(")<<sizeof(X)<<"): "<<(void*)(&X)<<endl

    void *min_addr,*max_addr;
    size_t size = 0;
int main()
{
    int i1;
    char c1;
    double d1;
    char c2;
    int i2;
    char c3;
    int i3;
    char c4;
    int i5;

    min_addr = max_addr = &i1;
    
    cout<<"sizeof(double): "<<sizeof(int)<<endl;
    cout<<"sizeof(int): "<<sizeof(int)<<endl;
    cout<<"sizeof(char): "<<sizeof(int)<<endl;
    
    PRT(i1);
    PRT(c1);
    PRT(d1);
    PRT(c2);
    PRT(i2);
    PRT(c3);
    PRT(i3);
    PRT(c4);
    PRT(i5);
    cout<<"stack size: "<<((long int)max_addr-(long int)min_addr)<<endl;
    cout<<"elems size: "<<size<<endl;
}
