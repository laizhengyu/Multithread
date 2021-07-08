#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

typedef struct socketinfo
{
    int fd;
    int epfd;
} SocketInfo;
void *acceptCon(void *arg)
{
    printf("acceptcon tid:%ld\n", pthread_self());
    SocketInfo *info = (SocketInfo *)arg;
    int cfd = accept(info->fd, NULL, NULL);
    //设置非阻塞
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);
    //将通信的fd加入到树，下次循环就可以检测
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; //检测读事件并使用ET模式
    ev.data.fd = cfd;

    int ret = epoll_ctl(info->epfd, EPOLL_CTL_ADD, cfd, &ev);
    if (ret == -1)
    {
        perror("epoll_ctl_accept");
        exit(0);
    }
    free(info);
    return NULL;
}
void *communication(void *arg)
{
    printf("communication tid:%ld\n", pthread_self());
    SocketInfo *info = (SocketInfo *)arg;
    int fd = info->fd;
    int epfd =info->epfd;
    //如果是通信fd
    // 接收数据
    char buf[5];
    memset(buf, 0, sizeof(buf));
    while (1)
    {
        int len = recv(fd, buf, sizeof(buf), 0);
        if (len > 0)
        {
            printf("客户端say: %s\n", buf);
            send(fd, buf, len, 0);
        }
        else if (len == 0)
        {
            printf("客户端断开了连接...\n");
            //从epoll树删除节点
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            close(fd);
            break;
        }
        else
        {
            if (errno == EAGAIN)
            {
                printf("数据读取完毕。。。\n");
                break;
            }
            else
            {
                perror("recv");
                break;
            }
        }
    }
    free(info);
    return NULL;
}

int main()
{
    // 1. 创建监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1)
    {
        perror("socket");
        exit(0);
    }

    // 2. 将socket()返回值和本地的IP端口绑定到一起
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(10000); // 大端端口
    // INADDR_ANY代表本机的所有IP, 假设有三个网卡就有三个IP地址
    // 这个宏可以代表任意一个IP地址
    // 这个宏一般用于本地的绑定操作
    addr.sin_addr.s_addr = INADDR_ANY; // 这个宏的值为0 == 0.0.0.0
                                       //    inet_pton(AF_INET, "192.168.237.131", &addr.sin_addr.s_addr);
    int ret = bind(lfd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("bind");
        exit(0);
    }

    // 3. 设置监听
    ret = listen(lfd, 128);
    if (ret == -1)
    {
        perror("listen");
        exit(0);
    }

    //现在只有监听的文件描述符，所有文件描述符对应读写缓冲区状态都是委托内核检测的
    //创建epoll实例
    int epfd = epoll_create(1);
    if (epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN; //检测读事件
    ev.data.fd = lfd;
    ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
    if (ret == -1)
    {
        perror("epoll_ctl");
        exit(0);
    }
    //检测
    //已就绪的文件描述符信息数组
    struct epoll_event evs[5];
    int size = sizeof(evs) / sizeof(evs[0]);
    while (1)
    {
        int num = epoll_wait(epfd, evs, size, -1);
        printf("num = %d\n", num);
        pthread_t tid;
        for (int i = 0; i < num; i++)
        {
            SocketInfo* info = (SocketInfo*)malloc(sizeof(SocketInfo));
            
            int fd = evs[i].data.fd;
            info->fd = fd;
            info->epfd = epfd;
            //就绪队列有监听，说明有客户端连接，用accept建立连接（相当于第二次握手阶段）
            if (fd == lfd)
            {
                pthread_create(&tid,NULL,acceptCon,info);
                pthread_detach(tid);//线程分离，让系统自动回收
            }
            else
            {
                pthread_create(&tid,NULL,communication,info);
                pthread_detach(tid);
            }
        }
    }

    close(lfd);

    return 0;
}
