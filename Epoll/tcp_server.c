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
    struct epoll_event evs[1024];
    int size = sizeof(evs) / sizeof(evs[0]);
    while (1)
    {
        int num = epoll_wait(epfd, evs, size, -1);
        printf("num = %d\n", num);
        for (int i = 0; i < num; i++)
        {
            int fd = evs[i].data.fd;
            //就绪队列有监听，说明有客户端连接，用accept建立连接（相当于第二次握手阶段）
            if (fd == lfd)
            {
                int cfd = accept(fd, NULL, NULL);
                //将通信的fd加入到树，下次循环就可以检测
                // struct epoll_event ev;可不写，因为是拷贝一个副本进了内核
                ev.events = EPOLLIN; //检测读事件
                ev.data.fd = cfd;
                ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
                if (ret == -1)
                {
                    perror("epoll_ctl_accept");
                    exit(0);
                }
            }
            else
            {
                //如果是通信fd
                // 接收数据
                char buf[1024];
                memset(buf, 0, sizeof(buf));
                int len = read(fd, buf, sizeof(buf));
                if (len > 0)
                {
                    printf("客户端say: %s\n", buf);
                    write(fd, buf, len);
                    for(int i = 0; i < len; i++){
                        buf[i] = toupper(buf[i]);
                    }
                    printf("after buf = =%s\n", buf);
                    //大写发给客户端
                    ret = send(fd, buf,strlen(buf)+1,0);
                    if(ret == -1){
                        perror("send error");
                        exit(1);
                    }
                }
                else if (len == 0)
                {
                    printf("客户端断开了连接...\n");
                    epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
                    close(fd);
                    break;
                }
                else
                {
                    perror("read");
                    exit(1);
                    break;
                }
            }
        }
    }

   
    close(lfd);

    return 0;
}
