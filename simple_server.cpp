/* run using ./server <port> */

#include <stdio.h>
#include "http_server.hh"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <queue>

#include <netinet/in.h>

#include<csignal>
#include <pthread.h>

int maxS = 30000;
queue<int> fdQueue;
int Server =0;
pthread_t thread_pool[15];


void error(char *msg)
{
    perror(msg);
    exit(1);
}

// void signal()

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cons = PTHREAD_COND_INITIALIZER;
pthread_cond_t prod = PTHREAD_COND_INITIALIZER;

void sigHandler(int sig){
  Server=1;
  pthread_cond_broadcast(&cons);
  for (int i=0; i <15; i++){
    pthread_join(thread_pool[i],NULL);
  }
  exit(0);
}

void *handleClient(void *arg)
{

    while (1)
    {
        pthread_mutex_lock(&lock1);
        while (fdQueue.empty() || Server==1){
            if(Server==1){
                pthread_mutex_unlock(&lock1);
                pthread_exit(0);
            }
            pthread_cond_wait(&cons, &lock1);
        }
        int clientFd = fdQueue.front();
        fdQueue.pop();
        pthread_cond_signal(&prod);
        pthread_mutex_unlock(&lock1);
        char buffer[1000];
        int n;
        if (clientFd < 0)
            error("ERROR on accept");

        /* read message from client */

        bzero(buffer, 1000);
        n = read(clientFd, buffer, 999);
        if (n < 0)
            error("ERROR reading from socket");
        else if (n == 0)
        {
            close(clientFd);
            continue;
        }

        // cout << buffer << endl;

        struct HTTP_Response *res = handle_request(buffer);

        string resString = res->get_string();
        // cout<< resString <<endl;

        n = write(clientFd, resString.c_str(), resString.length());
        delete res;
        if (n < 0)
            error("ERROR writing to socket");

        close(clientFd);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int sockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    signal(SIGINT,sigHandler);

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }
    fprintf(stdout, "hello");
    for (int i = 0; i < 15; i++)
    {
        pthread_create(&thread_pool[i], NULL, handleClient, NULL);
        cout << "thread created";
    }

    /* create socket */

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* fill in port number to listen on. IP address can be anything (INADDR_ANY)
     */

    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* bind socket to this port number on this machine */

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    /* listen for incoming connection requests */
    listen(sockfd, 2000);
    clilen = sizeof(cli_addr);

    while (1)
    {
        /* accept a new request, create a newsockfd */
        int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        pthread_mutex_lock(&lock1);
        while (maxS == fdQueue.size())
        {
            pthread_cond_wait(&prod, &lock1);
        }
        fdQueue.push(newsockfd);
        pthread_cond_signal(&cons);
        pthread_mutex_unlock(&lock1);
        // pthread_t thread_id;
        // pthread_create(&thread_id, NULL, handleClient, queue);
    }
    return 0;
}
