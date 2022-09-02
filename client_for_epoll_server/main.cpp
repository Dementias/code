#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <vector>
#include <functional>

#include <thread>
#include <chrono>
#include <mutex>

#define BUFLEN 20
#define SERVER_NAME "127.0.0.1"
#define SERVER_PORT 5555

std::mutex mtx;

int writeToServer (int fd)
{
    int nbytes;
    std::string input = "Sending message"; // текстовый буфер
    char buf[BUFLEN];
    strcpy(buf, input.c_str());

    nbytes = write(fd, buf, strlen(buf) + 1);


    if (nbytes < 0)
    {
        perror("write");
        return -1;
    }

    return 0;
}

int readFromServer (int fd)
{
    int nbytes;
    char buf[BUFLEN];

    nbytes = read(fd, buf, BUFLEN);

    if (nbytes < 0)
    {
        perror("read");
        return -1;
    }

    else if (nbytes == 0)
    {
        std::cerr << "Client: no data to read" << std::endl;
        return 0;
    }

    // else std::cout << "Success read of data: " << buf << std::endl;

    return 1;
}

// fd - файловый дескриптор

int work(int N, int Time, int i)
{
    struct sockaddr_in server_addr;
    struct hostent *hostinfo;

    // получаем информацию о сервере
    hostinfo = gethostbyname(SERVER_NAME);
    if (hostinfo == NULL)
    {
        std::cerr << "Unknown host " << SERVER_NAME << std::endl;
        return -1;
    }

    // Заполняем адресную структуру для последующего использования при установлении соединения
    server_addr.sin_family = PF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr = *(struct in_addr*) hostinfo->h_addr;

    // Создаем TCP сокет
    int cl_socket;

    //mtx.lock();
    cl_socket = socket(AF_INET, SOCK_STREAM, 0);
    //mtx.unlock();

    if (cl_socket < 0)
    {
        std::cout << "Client: socket was not created" << std::endl;
        return -2;
    }

    // Устанавливаем соединения с сервером
    int con_error;

    con_error = connect(cl_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (con_error < 0 )
    {
        std::cout << "Client: connection failure" << std::endl;
        return -3;
    }

    mtx.lock();
    std::cout << "Connection is ready " << i << std::endl;
    mtx.unlock();

    //обмен данными
    int j = 0;
    int miss_packages = 0;
    while (j < N)
    {
        if (writeToServer(cl_socket) < 0)
        {
            ++miss_packages;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(Time/2));

        if (readFromServer(cl_socket) < 0)
        {
            ++miss_packages;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(Time/2));
        ++j;
    }

    mtx.lock();
    std::cout << "Finishing work" << std::endl;
    mtx.unlock();

    // Закрываем сокет

    close(cl_socket);

    return miss_packages;
}

void f (int *miss_packages, int N, int Time, int i)
{
    int c = work(N, Time, i);
    std::lock_guard<std::mutex> gl(mtx);
    if (c > 0) miss_packages += c;
}

int main(int argc, char* argv[])
{
    std::vector<std::thread> treds;
    int M = atoi(argv[1]); // - количество клиентов
    int delay = atoi(argv[2]); // - тайм делэй между ними
    int count = atoi(argv[3]); // - количество отправленных каждым клиентом пакетов
    int time_per_operation = atoi(argv[4]); // -
    int miss_packages = 0;

    for (int i = 0; i < M; ++i)
    {
        treds.emplace_back(std::thread(f, &miss_packages, count, 1000, i));
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }

    for (int i = 0; i < M; ++i)
    {
        treds[i].join();
    }

    std::cout << "-----------------" << std::endl
              << "Total number of packages lost during work: " << miss_packages << " of " << M * count << std::endl
              << "End" << std::endl;
}