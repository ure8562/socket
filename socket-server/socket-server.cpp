#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <pthread.h>
#include "./mariadb_work/mariadb_work.h"

#define BUF_SIZE 100    //  최대글자수
#define MAX_CLNT 255    //  최대 동시 접속 가능 수

#pragma GCC diagnostic ignored "-Wwrite-strings"

void *handle_clnt(void *arg);
void send_msg(char *msg, int len);
void error_handling(char* msg);

int clnt_cnt = 0;   //  접속한 클라이언트 수
int clnt_socks[MAX_CLNT];  
/**
 * 여러 명의 클라이언트가 접속하므로, 클라이언트 소켓은 배열이다.
 * 멀티쓰레드 시, 이 두 전역변수, clnt_cnt, clnt_socks에 여러 쓰레드가 동시 접근할 수 있기에
 * 두 변수의 사용이 들어간다면 무조건 임계영역이다. 
*/
pthread_mutex_t mutx;   //  mutex 선언 - 다중 쓰레드끼리 전역변수 사용시 데이터의 혼선 방지

int main(void)
{
    mariadb test1;
    test1.mysqlID.server = "localhost";
    test1.mysqlID.user = "root";
    test1.mysqlID.password = "1234";
    test1.mysqlID.database = "bongtest";

    test1.mysql_connections_setup();

    test1.mysql_perform_query(test1.conn, "DESC MEMBER");

    printf("MYSQL Tables inmysql database:\n");
    while((test1.row = mysql_fetch_row(test1.res)) != NULL)
    {
        printf("%s\n", test1.row[0]);
    }

    mysql_free_result(test1.res);
    mysql_close(test1.conn);

    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    int clnt_adr_sz;
    pthread_t t_id;     // thread 선언;

    //  소켓 오션을 설정을 위한 두 변수
    int option; 
    socklen_t optlen;

    //  뮤텍스 만들기
    pthread_mutex_init(&mutx, NULL);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    //  Time-wait 해제
    //  SO_REUSEADDR 를 0 에서 1로 변경
    optlen = sizeof(option);
    option = 1;
    setsockopt(serv_sock, SOL_SOCKET,SO_REUSEADDR,(void *)&option, optlen);

    //  IPv4, IP, PORT 할당
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi("9300"));

    //  주소할당
    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handling("bind() error\n");
    }

    if(listen(serv_sock, 5) == -1)
    {
        error_handling("listen() error\n");
    }
/**
 * 5로 지정했으니 5명까지만 사용가능 한 채팅이 아니다.
 * 큐의 크기일뿐이며 운영체제가 알아서 accept 진행 255명까지 가능
 * 웝세버같이 수천명의 클라이언트가 있을경우, 15로 잡는게 보통
*/

    while(1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, (socklen_t*)&clnt_adr_sz);

        //  clnt_socks[], clnt_cnt 전역변수를 사용하기 위해 뮤텍스 잠금
        pthread_mutex_lock(&mutx);
        //  클라이언트 카운터 올리고, 소켓 배정. 첫 번째 클라이언트라면, clnt_socks[0]에 들어갈 것
        clnt_socks[clnt_cnt++] = clnt_sock;
        //  뮤텍스 잠금해제
        pthread_mutex_unlock(&mutx);
        //  쓰레드 생성, 쓰레드의 main은 handle_clnt
        //  네 번째 파라미터로 accept 이후 생겨난 소켓의 파일 디스크립터 주소값을 넣어주어 handle_clnt에서 파라미터로 받을 수 있도록 함
        pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
        //  t_id로 접근했을 떄, 해당 쓰레드가 NULL 값을 리턴한 경우가 아니면 무시하고 진행
        //  만약 쓰레드가 NULL값을 리턴하면, 쓰레드 종료
        pthread_detach(t_id);
        printf("Connected client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }
    printf("Close\n");
    close(serv_sock);
    return 0;
}

void *handle_clnt(void *arg)
{
    //소켓 파일 디스크립터가 void 포인터로 들어왔으므로, int로 반환
    int clnt_sock = *((int *)arg);
    int str_len = 0;
    bool status = true;
    char msg[BUF_SIZE];

    /**
     * 클라이언트가 보낸 메시지 받음
     * 클라이언트에서 EOF 보내서, str_len 이 0이 될때까지 반복
     * EOF 를 보내는 순간은 소켓이 close 했을때
     * 클라이언트가 접속하는 동안 while문을 벗어나지 않는다.
    */
   while(status)
   {
        str_len = read(clnt_sock, msg, sizeof(msg));
        if(msg[0] == 'C'&& msg[1] == 'l' && msg[2] == 'o' && msg[3] == 's' && msg[4] == 'e')
        {
            status = false;
        }
        else
        {
            send_msg(msg, str_len);
        }
   }
   /**
    * while문을 벗어난 것은 소켓이 close되었기 때문
    * clnt_socks[] 삭제와 쓰레드 소멸
   */

    pthread_mutex_lock(&mutx);
    //연결 끊어진 클라이언트인 "현재 쓰레드에서 담당하는 소켓" 삭제
    for(int i = 0;i < clnt_cnt; i++)
    {
        if(clnt_sock == clnt_socks[i])
        {
            while(i++ < clnt_cnt - 1)
                clnt_socks[i] = clnt_socks[i + 1];
            break;
        }
    }
    //클라이언트 수 하나 줄임
    fputs("disconnection\n",stderr);
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    return NULL;
}

// 접속한 모두에게 메세지 보내기
void send_msg(char *msg, int len)
{
    //clnt_cnt, clnt_socks[] 사용을 위해 뮤텍스 잠금
    pthread_mutex_lock(&mutx);
    for(int i = 0; i < clnt_cnt; i++)
    {
        write(clnt_socks[i], msg, len);
    }
    pthread_mutex_unlock(&mutx);
}

void error_handling(char* msg)
{
    fputs(msg,stderr);
    fputc('\n',stderr);
    exit(1);
}