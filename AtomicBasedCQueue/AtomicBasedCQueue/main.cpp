#include <thread>
#include <functional>
#include <vector>
#include "AtomicCQueue.hxx"

int main()
{
    AtomicCQueue<int> cq;

    int N = 1;

    std::vector<std::thread> treds;

    std::function<void( int )> consumer;
    std::function<void( int, int )> producer;

    producer = [&]( int a, int N )
    {
        for ( int i = 0; i < N; ++i )
        {
            cq.enQueue( a + i );
        }
    };

    consumer = [&]( int N )
    {
        for ( int i = 0; i < N; ++i )
        {
            cq.deQueue();
        }
    };


    treds.emplace_back( std::thread( producer, 0, 400 ) );
    treds.emplace_back( std::thread( consumer, 100 ) );

    for ( int i = 0; i < treds.size(); ++i )
    {
        treds[i].join();
    }

    cq.display();
}