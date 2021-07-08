#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>

int main(){
    //1、创建通信套接字，tcp IPv4协议，TCP流，协议类型
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1){
        perror("socket");
        return -1;
    }
    //2、连接服务器
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(9999);//转字节序,host to net
    //客户端需要指定IP地址,point to net 点分十进制到网络地址
    inet_pton(AF_INET,"192.168.234.128",&saddr.sin_addr.s_addr);
    int ret = connect(fd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(ret == -1){
        perror("connect");
        return -1;
    }

    //3、通信
    int number = 0;
    while(1){
        //发送数据
        char buff[1024];
        sprintf(buff,"hello, %d", number++);
        send(fd,buff,strlen(buff)+1,0);//因为发送时最后一个\0不会被发送

        //接收数据
        memset(buff,0,sizeof(buff));
        int len = recv(fd,buff,sizeof(buff),0);
        if(len > 0){
            printf("server say: %s\n", buff);
        }
        else if(len == 0){
            printf("server close\n");
            break;
        }
        else{
            perror("recv");
            break;
        }
        sleep(1);
    }
    //关闭文件描述符
    close(fd);

    return 0;
}