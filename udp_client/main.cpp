#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>

#include <vector>
#include <functional>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <string>

#define BUFLEN 128
#define SERVER_PORT 5555

std::atomic<int> send_misses;
std::atomic<int> receive_misses;
std::atomic<int> msgs;
std::atomic<int> bad_receives;
std::mutex mtx;

/*int timeout_recvfrom (int sock, char *buf, int *length, struct sockaddr_in *connection, int timeoutinseconds)
{
    fd_set socks;
    struct timeval t;
    FD_ZERO(&socks);
    FD_SET(sock, &socks);
    t.tv_sec = timeoutinseconds;
    if (select(sock + 1, &socks, NULL, NULL, &t) &&
        recvfrom(sock, buf, *length, 0, (struct sockaddr *)connection, length)!=-1)
    {
        return 1;
    }
    else
        return 0;
}*/

void SetSockaddr (struct sockaddr_in *addr)
{
    bzero((char *) addr, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(SERVER_PORT);
}

void WriteToServer(int sockfd, const char * message, struct sockaddr_in *servaddr, int i)
{
    if (sendto(sockfd, (const char *) message, strlen(message),
               0, (const struct sockaddr *) servaddr, sizeof(*servaddr)) == -1)
    {
        char * msg;
        sprintf(msg, "Unable to send a message to server, client %d", i);
        ++send_misses;

        //perror(msg);
        //exit(2);
    }
}

int ReadFromServer(int sockfd, struct sockaddr_in *servaddr, int i, const char * message)
{
    unsigned int len = sizeof(*servaddr);
    char buffer[BUFLEN];


    int n = recvfrom(sockfd, buffer, BUFLEN,MSG_WAITALL,
                     (struct sockaddr *) servaddr,&len);

    if (n == 0)
        return 0;

    if (n < 0)
    {
        char msg[64];
        sprintf(msg, "Unable to receive a message from server, client %d", i);
        ++receive_misses;

        //perror(msg);
        //exit(3);
        return -1;
    }
    buffer[n] = '\0';

    std::string s1 = message;
    std::string s2 = buffer;

    //buffer[n] = '\0';
    if (s1.compare(s2) != 0)
    {
        ++bad_receives;
        mtx.lock();
        std::cout << buffer << std::endl;
        std::cout << message << std::endl << "------------" << std::endl;
        mtx.unlock();
    }


    ++msgs;
    return 1;
}

int DoWork(int sockfd, const char * message, struct sockaddr_in *servaddr, int Time, int i)
{
    WriteToServer(sockfd, message, servaddr, i);
    std::this_thread::sleep_for(std::chrono::milliseconds(Time / 2));

    int n = ReadFromServer(sockfd, servaddr, i, message);
    std::this_thread::sleep_for(std::chrono::milliseconds(Time / 2));

    return n;
}

void RunClient(int N, int Time, int i)
{
    int sockfd;
    struct sockaddr_in servaddr;

    //Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(1);
    }

    SetSockaddr(&servaddr);

    int j = 0;

    while(j < N)
    {
        ++j;
        char message[BUFLEN];
        sprintf(message, "This is %d-th message sent by Client %d", j, i);
        int n = DoWork(sockfd, message, &servaddr, Time, i);
    }

    close(sockfd);
}

int main(int argc, char* argv[])
{
    send_misses = 0;
    receive_misses = 0;
    msgs = 0;
    bad_receives = 0;

    std::vector<std::thread> treds;
    int M = atoi(argv[1]); // - количество клиентов
    int delay = atoi(argv[2]); // - тайм делэй между ними
    int count = atoi(argv[3]); // - количество отправленных каждым клиентом пакетов
    int time_per_operation = atoi(argv[4]); // - время на 1 операцию
    int finished = 0;
    std::function<void (int, int, int)> f;

    f = [&finished](int N, int Time, int i)
    {
        RunClient(N, Time, i);
        std::lock_guard<std::mutex> lg(mtx);
        std::cout << "Threads finished: " << ++finished << " " << msgs << std::endl;
    };

    for (int i = 0; i < M; ++i)
    {
        treds.emplace_back(std::thread(f, count, time_per_operation, i));
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }

    for (int i = 0; i < treds.size(); ++i)
    {
        treds[i].join();
    }

    std::cout << "-----------------" << std::endl
              << "Total number of packages lost during work: " << std::endl << "Send misses: " <<
              send_misses << std::endl << "Receive misses: " << receive_misses << std::endl << "Bad receives: " << bad_receives << std::endl << "of " << M * count << std::endl
              << "End" << std::endl;
}