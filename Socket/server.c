#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>

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

    //4、阻塞等待客户端的连接
    struct sockaddr_in caddr;
    int addrlen = sizeof(caddr);
    int cfd = accept(fd, (struct sockaddr*)&caddr, &addrlen);
    if(cfd == -1){
        perror("accept");
        return -1;
    }

    //连接成果，打印信息
    char ip[32];
    printf("client IP: %s, port: %d\n", inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip, sizeof(ip)),
            ntohs(caddr.sin_port));

    //5、通信
    while(1){
        char buff[1024];
        int len = recv(cfd,buff,sizeof(buff),0);
        if(len > 0){
            printf("clinet say: %s\n", buff);
            send(cfd, buff, len, 0);
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
    //关闭文件描述符
    close(fd);
    close(cfd);



    return 0;
}