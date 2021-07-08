#include<stdio.h>
#include"threadpool.h"
#include<pthread.h>
#include<unistd.h>
#include<stdlib.h>
void taskFunction(void* arg){
    int num = *(int*) arg;
    printf("thread  is working, number = %d, tid = %ld", num, pthread_self());
    usleep(1000);//1000us
}

int main(){
    ThreadPool* pool = threadPoolCreate(3,10,100);
    for(int i = 0; i < 100; i++){
        int* num = (int*) malloc(sizeof(int));//传入堆数据
        *num = i + 100;
        threadPoolAdd(pool,taskFunction,num);
    }

    //为了保证工作线程已经处理完毕要睡眠
    sleep(30);
    threadPoolDestroy(pool);

    return 0;
}
