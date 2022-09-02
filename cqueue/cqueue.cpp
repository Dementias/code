#include <iostream>
#include <vector>
#include <functional>
#include <mutex>
#include <thread>

template <typename T>
class CQueue
{
public:
    CQueue() : 
        capacity(STANDARD_SIZE),
        data(new T[STANDARD_SIZE]),
        front(-1),
        rear(-1)
    {}

    CQueue(size_t capacity) :
        capacity(capacity), 
        data(new T[capacity]),
        front(-1),
        rear(-1)
    {}

    bool EnQueue(T element)
    {
        std::lock_guard<std::recursive_mutex> lock(mtx);
        if (Full())
        {
            if (!TryBiggerQueue()) return false;
        }
        
        else if (front == -1) front = 0;

        rear = (rear + 1) % capacity;
        data[rear] = element;
    
        return true;
    }

    bool DeQueue()
    {
        std::lock_guard<std::recursive_mutex> lock(mtx);

        if(Empty()) return false;

        data[front] = -1;

        if (front == rear)
        {
            front = -1;
            rear = -1;
        }

        else front = (front + 1) % capacity;

        return true;
    }

    void Display() const
    {
        std::lock_guard<std::recursive_mutex> lock(mtx);

        if(Empty()) return;

        long long front_ = front;

        while(front_ != rear)
        {
            std::cout << data[front_] << " ";
            front_ = (front_ + 1) % capacity;
        }

        std::cout << data[front_] << std::endl;
    }

    ~CQueue()
    {
        delete[] data; 
    }

private:
    bool TryBiggerQueue()
    {
        std::lock_guard<std::recursive_mutex> lock(mtx);
        T* new_pointer;

        try
        {
            new_pointer = new T[2 * capacity];
        }
        
        catch (std::bad_alloc& e)
        {
            std::cout << e.what() << std::endl;
        }
        

        if (rear < front)
        {
            for (int i = 0; i < front; ++i)
            {
                new_pointer[i] = data[i];
            }

            for (int i = front; i < capacity; ++i)
            {
                new_pointer[i + capacity] = data[i];
            }

            front += capacity;
        }

        else
        {
            for (int i = 0; i < capacity; ++i)
            {
                new_pointer[i] = data[i];
            }
        }

        capacity *= 2;

        delete[] data;
        data = new_pointer;
        
        return true;
    }

    bool Full() const
    {
        std::lock_guard<std::recursive_mutex> lock(mtx);

        return ((front == 0) && (rear == capacity - 1)) || (front == rear + 1);
    }

    bool Empty() const
    {
        std::lock_guard<std::recursive_mutex> lock(mtx);

        return front == -1;
    }

    T* data; // - содержимое очереди
    long long front; // - указывает на первый элемент очереди
    long long rear; // - указывает на последний элемент очереди
    size_t capacity; // - вместимость очереди
    mutable std::recursive_mutex mtx; // - мьютекс
    constexpr static size_t STANDARD_SIZE = 2; // - стандартный 
};

int main()
{
    CQueue<int> cq;

    int N = 10;

    std::vector<std::thread> treds;
    std::function<void (int)> f;

    f = [&](int a)
    {
        cq.EnQueue(a);
        cq.EnQueue(a + 1);
        cq.DeQueue();
        cq.EnQueue(a + 2);
        cq.DeQueue();
    };

    for (int i = 0; i < N; ++i)
    {
        treds.emplace_back(std::thread (f, 3 * i));
    }

    for (int i = 0; i < treds.size(); ++i)
    {
        treds[i].join();
    }

    cq.Display();
    cq.Display();
}