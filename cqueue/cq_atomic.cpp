#include <iostream>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>

//  Круговая очередь
//  Single producer -> single consumer

template<typename T>
class AtomicCQueue
{
public:
    AtomicCQueue() : 
        capacity(STANDARD_SIZE),
        data(new T[STANDARD_SIZE]),
        front(-1),
        rear(-1)
    {}

    AtomicCQueue(size_t capacity) :
        capacity(capacity), 
        data(new T[capacity]),
        front(-1),
        rear(-1)
    {}

    bool EnQueue(T element)
    {
        auto current_rear = rear.load();
        auto next_rear = increment(current_rear);
        int expected = -1;

        if (next_rear != front.load())
        {
            data[next_rear] = element;
            rear.store(next_rear);

            front.compare_exchange_strong(expected, 0);

            return true;
        }

        return false;
    }

    bool DeQueue()
    {
        const auto current_front = front.load();

        if (current_front == rear.load()) return false;

        front.store(increment(current_front));

        return true;
    }

    void Display() const
    {
        if(Empty()) return;

        int front_ = front.load();

        while(front_ != rear.load())
        {
            std::cout << data[front_] << " ";
            front_ = increment(front_);
        }

        std::cout << data[front_] << std::endl;
    }

    ~AtomicCQueue()
    {
        delete[] data;
    }

private:
    bool Full() const
    {
        const auto next_rear = increment(rear.load());
        return next_rear == front.load();
    }

    bool Empty() const
    {
        return front.load() == -1;
    }

    int increment(int a) const
    {
        return (a + 1) % capacity;
    }

    T* data; // - содержимое очереди
    std::atomic<int> front; // - указывает на первый элемент очереди
    std::atomic<int> rear; // - указывает на последний элемент очереди
    size_t capacity; // - вместимость очереди
    constexpr static size_t STANDARD_SIZE = 512; // - стандартный капасити очереди
};

int main()
{
    AtomicCQueue<int> cq;

    int N = 1;

    std::vector<std::thread> treds;

    std::function<void (int)> consumer;
    std::function<void (int, int)> producer;

    producer = [&](int a, int N)
    {
        for (int i = 0; i < N; ++i)
        {
            cq.EnQueue(a + i);
        }
    };

    consumer = [&](int N)
    {
        for (int i = 0; i < N; ++i)
        {
            cq.DeQueue();
        }
    };

    
    treds.emplace_back(std::thread(producer, 0, 400));
    treds.emplace_back(std::thread(consumer, 50));

    for (int i = 0; i < treds.size(); ++i)
    {
        treds[i].join();
    }

    cq.Display();
}