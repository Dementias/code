#pragma once
#include <mutex>
#include <thread>
#include <iostream>
#include <optional>

// This is my implementation of thread safe CQueue using mutex

template <typename T>
class MutexCQueue
{
public:
    MutexCQueue() :
        capacity_( STANDARD_SIZE ),
        data_( new T[STANDARD_SIZE] ),
        front_( -1 ),
        rear_( -1 )
    {
    }

    MutexCQueue( size_t capacity ) :
        capacity_( capacity ),
        data_( new T[capacity] ),
        front_( -1 ),
        rear_( -1 )
    {
    }

    bool enQueue( const T& element )
    {
        std::lock_guard<std::recursive_mutex> lock( mtx_ );
        if ( full_() )
        {
            if ( !tryBiggerQueue_() ) return false;
        }

        else if ( front_ == -1 ) front_ = 0;

        rear_ = ( rear_ + 1 ) % capacity_;
        data_[rear_] = element;

        return true;
    }

    std::optional<T> deQueue()
    {
        std::lock_guard<std::recursive_mutex> lock( mtx_ );

        if ( empty_() ) return std::nullopt;
        auto saved = data_[front_];
        data_[front_] = -1;

        if ( front_ == rear_ )
        {
            front_ = -1;
            rear_ = -1;
        }

        else front_ = ( front_ + 1 ) % capacity_;

        return saved;
    }

    void display() const
    {
        std::lock_guard<std::recursive_mutex> lock( mtx_ );

        if ( empty_() ) return;

        long long front = front_;

        while ( front != rear_ )
        {
            std::cout << data_[front] << " ";
            front = ( front + 1 ) % capacity_;
        }

        std::cout << data_[front] << std::endl;
    }

    ~MutexCQueue()
    {
        delete[] data_;
    }

private:
    T* data_; // - data
    long long front_; // - first elemnt of queue
    long long rear_; // - last element of queue
    size_t capacity_; // - queue capacity
    mutable std::recursive_mutex mtx_;
    constexpr static size_t STANDARD_SIZE = 4;

private:
    bool tryBiggerQueue_()
    {
        std::lock_guard<std::recursive_mutex> lock( mtx_ );
        T* newPointer = nullptr;

        try
        {
            newPointer = new T[2 * capacity_];
        }

        catch ( std::bad_alloc& e )
        {
            std::cout << e.what() << std::endl;
        }


        if ( rear_ < front_ )
        {
            for ( int i = 0; i < front_; ++i )
            {
                newPointer[i] = data_[i];
            }

            for ( int i = front_; i < capacity_; ++i )
            {
                newPointer[i + capacity_] = data_[i];
            }

            front_ += capacity_;
        }

        else
        {
            for ( int i = 0; i < capacity_; ++i )
            {
                newPointer[i] = data_[i];
            }
        }

        capacity_ *= 2;

        delete[] data_;
        data_ = newPointer;

        return true;
    }

    bool full_() const
    {
        std::lock_guard<std::recursive_mutex> lock( mtx_ );

        return ( ( front_ == 0 ) && ( rear_ == capacity_ - 1 ) ) || ( front_ == rear_ + 1 );
    }

    bool empty_() const
    {
        std::lock_guard<std::recursive_mutex> lock( mtx_ );

        return front_ == -1;
    }
};