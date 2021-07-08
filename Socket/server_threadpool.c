#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<pthread.h>
#include"threadpool.h"
//信息结构体
struct SockInfo
{
    struct sockaddr_in addr;
    int fd;
};

typedef struct PoolInfo
{
    ThreadPool* p;
    int fd;
}PoolInfo;


void working(void* arg){
    struct SockInfo* pinfo = (struct SockInfo*)arg;
 //连接成果，打印信息
    char ip[32];
    printf("client IP: %s, port: %d\n", inet_ntop(AF_INET,&pinfo->addr.sin_addr.s_addr,ip, sizeof(ip)),
            ntohs(pinfo->addr.sin_port));

    //5、通信
    while(1){
        char buff[1024];
        int len = recv(pinfo->fd,buff,sizeof(buff),0);
        if(len > 0){
            printf("clinet say: %s\n", buff);
            send(pinfo->fd, buff, len, 0);
        }
        else if(len == 0){
            printf("client close\n");
            break;
        }
        else{
            perror("recv");
            break;
        }
    }

    close(pinfo->fd);

}

void acceptCon(void* arg){
    PoolInfo *poolInfo = (PoolInfo*) arg;
    int addrlen = sizeof(struct sockaddr_in);
    while (1)
    {
        struct SockInfo* pinfo;
        pinfo = (struct SockInfo*)malloc(sizeof(struct SockInfo));
        pinfo->fd = accept(poolInfo->fd, (struct sockaddr*)&pinfo->addr, &addrlen);

        if(pinfo->fd == -1){
            perror("accept");
            break;
        }

        threadPoolAdd(poolInfo->p,working,pinfo);
    }
    close(poolInfo->fd);
}

int main(){
    //1、创建监听套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1){
        perror("socket");
        return -1;
    }
    //2、绑定本地的IP和端口
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);//转字节序,host to net
    saddr.sin_addr.s_addr = INADDR_ANY;
    int ret = bind(fd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(ret == -1){
        perror("bind");
        return -1;
    }

    //3、设置监听
    ret = listen(fd,128);
    if(ret == -1){
        perror("listen");
        return -1;
    }


    //创建线程池
    ThreadPool *pool = threadPoolCreate(3,8,100);
    PoolInfo* info = (PoolInfo*)malloc(sizeof(PoolInfo));
    info->p = pool;
    info->fd = fd;
    threadPoolAdd(pool, acceptCon, info);

    pthread_exit(NULL);

    return 0;
}
    //4、阻塞等待客户端的连接






