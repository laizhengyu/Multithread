#include"ThreadPool.h"
#include"ThreadPool.cpp"
#include<unistd.h>
#include<iostream>
#include<stdio.h>

void taskFunction(void* arg){
    int num = *(int*) arg;
    printf("thread  is working, number = %d, tid = %ld", num, pthread_self());
    usleep(1000);//1000us
}

int main(){
    ThreadPool pool (3,10);
    for(int i = 0; i < 100; i++){
        int* num = new int(i+100);//传入堆数据
        pool.addTask(Task(taskFunction,num));
    }

    //为了保证工作线程已经处理完毕要睡眠
    sleep(20);
    //调用完自动就析构了，不需要再写销毁

    return 0;
}
