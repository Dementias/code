#pragma once
#include <iostream>
#include <optional>
#include <atomic>


//  My implementation of thread safe circular queue on atomics
//  Single producer -> single consumer

template<typename T>
class AtomicCQueue
{
public:
    AtomicCQueue() :
        capacity_( STANDARD_SIZE ),
        data_( new T[STANDARD_SIZE] ),
        front_( -1 ),
        rear_( -1 )
    {
    }

    AtomicCQueue( uint32_t capacity ) :
        capacity_( capacity ),
        data_( new T[capacity] ),
        front_( -1 ),
        rear_( -1 )
    {
    }

    bool enQueue( const T& element )
    {
        auto current_rear = rear_.load();
        auto next_rear = increment_( current_rear );
        int expected = -1;

        if ( next_rear != front_.load() )
        {
            data_[next_rear] = element;
            rear_.store( next_rear );

            front_.compare_exchange_strong( expected, 0 );

            return true;
        }

        return false;
    }

    std::optional<T> deQueue()
    {
        const auto current_front = front_.load();

        if ( current_front == rear_.load() ) return std::nullopt;

        auto saved = data_[front_];
        front_.store( increment_( current_front ) );

        return saved;
    }

    void display() const
    {
        if ( empty_() ) return;

        int front = front_.load();

        while ( front != rear_.load() )
        {
            std::cout << data_[front] << " ";
            front = increment_( front );
        }

        std::cout << data_[front] << std::endl;
    }

    ~AtomicCQueue()
    {
        delete[] data_;
    }

private:
    bool full_() const
    {
        const auto next_rear = increment_( rear_.load() );
        return next_rear == front_.load();
    }

    bool empty_() const
    {
        return front_.load() == -1;
    }

    int increment_( int a ) const
    {
        return ( a + 1 ) % capacity_;
    }

    T* data_; 
    std::atomic<int> front_; // - position of the first element of cqueue
    std::atomic<int> rear_; // - position of the last element of cqueue
    uint32_t capacity_; // - cqueue capacity
    constexpr static uint32_t STANDARD_SIZE = 512; // - standard squeue capacity 
};