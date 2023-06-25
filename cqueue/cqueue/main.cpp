#include <functional>
#include <thread>
#include <vector>
#include "MutexBasedCQueue.hxx"

int main()
{
    MutexBasedCQueue<int> cq;
    std::vector<std::thread> treds;
    std::function<void( int )> f;

    f = [&]( int a )
    {
        cq.enQueue( a + 1 );
        cq.enQueue( a + 2 );
        auto c = cq.deQueue();
        std::cout << *c << " ";
        cq.enQueue( a + 3 );
        std::cout << *cq.deQueue() << " ";
        
    };

    int N = 2;
    for ( int i = 0; i < N; ++i )
    {
        treds.emplace_back( std::thread( f, 10 * i ) );
    }

    for ( int i = 0; i < treds.size(); ++i )
    {
        treds[i].join();
    }

    std::cout << std::endl;
    cq.display();
}