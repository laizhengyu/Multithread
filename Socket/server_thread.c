#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<pthread.h>
//信息结构体
struct SockInfo
{
    struct sockaddr_in addr;
    int fd;
};

struct SockInfo infos[512];//设置成数组，记录最大连接数，c++可以用STL设置更方便

void* working(void* arg){
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
    pinfo->fd = -1;
    return NULL;
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
    //初始化结构体的fd为-1
    int max = sizeof(infos) / sizeof(infos[0]);
    for(int i = 0; i < max; ++i){
        //bzero将字符串s的前n个字节清零，也可以清零结构体
        bzero(&infos[i],sizeof(infos[i]));
        infos[i].fd = -1;
    }
    //4、阻塞等待客户端的连接

    int addrlen = sizeof(struct sockaddr_in);
    while (1)
    {
        struct SockInfo* pinfo;
        for(int i = 0; i < max; i++){
            if(infos[i].fd == -1){
                pinfo = &infos[i];
                break;
            }
        }
        int cfd = accept(fd, (struct sockaddr*)&pinfo->addr, &addrlen);
        pinfo->fd = cfd;
        if(cfd == -1){
            perror("accept");
            break;
        }
        //创建子线程
        pthread_t tid;
        pthread_create(&tid, NULL, working, pinfo);
        //不能使用join让主线程回收，因为是阻塞函数，使用线程分离可以让系统来回收线程
        pthread_detach(tid);
    }
    close(fd);

    return 0;
}

