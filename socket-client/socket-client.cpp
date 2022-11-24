#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20
//test
#pragma GCC diagnostic ignored "-Wwrite-strings"

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(char *msg);

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;

    pthread_t snd_thread, rcv_thread;
    /**
     * 송신 쓰레드와 수신쓰레드 2개 쓰레드 선언
     * 내 메시지를 보내야하고, 상대방의 메시지도 받아야 한다.
    */

    void *thread_return;    //  pthread_join에 사용

    if(argc != 4)
    {
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    sprintf(name, "[%s]", argv[3]);

    sock = socket(PF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = PF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == 1)
        error_handling("connect() error\n");

    //  두개의 쓰레드 생성하고, 각각의 main 은 send_msg, recv_msg
    pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);
    pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);

    //  쓰레드 종료 대기 및 소멸 유도
    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);

    close(sock);
    return 0;
}

void *send_msg(void *arg)
{
    //void 형 int로 변환
    int sock = *((int *)arg);

    char name_msg[NAME_SIZE + BUF_SIZE];
    while(1)
    {
        fgets(msg, BUF_SIZE, stdin);
        //Q입력시 종료
        if(!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
        {
            
            write(sock, "Close", 5);
            //  서버에 EOF 전송
            close(sock);
            exit(0);
        }
        sprintf(name_msg, "%s %s", name, msg);
        write(sock, name_msg, strlen(name_msg));
    }
    return NULL;
}

void *recv_msg(void *arg)
{
    int sock = *((int *)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];
    int str_len;
    while(1)
    {
        //서버에서 들어온 메세지 수신
        str_len = read(sock, name_msg, NAME_SIZE + BUF_SIZE - 1);
        if(str_len == -1)
            return (void *)-1;  //pthread_join를 실행시키기 위해
        /**
         * str_len 이 -1이면 소켓과 연결이 끊어진 것
         * Why? 
         * send_msg 에서 close(sock)이 실행되고
         * 서버로 EOF 가 갔으면, 서버는 그것을 받아서 "자기가 가진" 클라이언트 소켓을 close 할것
         * 그러면 read 했을 때 결과가 -1 */

       name_msg[str_len] = 0;
       fputs(name_msg, stdout);
    }
    return NULL;
}

void error_handling(char *msg)
{
    fputs(msg,stderr);
    fputc('\n',stderr);
    exit(1);
}