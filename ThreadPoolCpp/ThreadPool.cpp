#include"ThreadPool.h"
#include<iostream>
#include<string.h>
#include<string>
#include<unistd.h>
using namespace std;


 ThreadPool::ThreadPool(int min, int max){
     //实例化任务队列
     
     do{
            taskQ = new TaskQueue;
            if(taskQ == nullptr){
                    cout<<"threadIDs fail..."<<endl;
                    break;
                }
            threadIDs = new pthread_t[max];
            if(threadIDs == nullptr){
                cout<<"threadIDs fail..."<<endl;
                break;
            }
            //直接访问成员变量
            memset(threadIDs, 0, sizeof(pthread_t) * max);
            minNum = min;
            maxNum = max;
            busyNum = 0;
            liveNum = min;
            exitNum = 0;

            if (pthread_mutex_init(&mutexPool, NULL) != 0 ||
            pthread_cond_init(&notEmpty, NULL) != 0 )
            {
                cout<<"mutex initial fail"<<endl;
                break;
            }
            shutdown = false;
            //创建线程
        pthread_create(&managerID, NULL, manager, this); //管理线程
        for (int i = 0; i < min; i++)
        {
            pthread_create(&threadIDs[i], NULL, worker, this); //工作线程,函数指针为worker
        }
        return;

     }while(0);
    //释放资源
    if (threadIDs)
        delete[](threadIDs);
    if (taskQ)
        delete(taskQ);


 }

ThreadPool::~ThreadPool(){
    //关闭线程池，其他函数里很多判断线程池是否关闭的操作
    shutdown = true;
    //阻塞回收管理这线程
    pthread_join(managerID, NULL);
    //唤醒阻塞的消费者
    for(int i = 0; i < liveNum; i++){
        pthread_cond_signal(&notEmpty);
    }
    //释放堆内存
    if(taskQ){
        delete taskQ;
    }
    if(threadIDs){
        delete[] threadIDs;
    }
  
    pthread_mutex_destroy(&mutexPool);
    pthread_cond_destroy(&notEmpty);
    return;    
}

 void *ThreadPool::worker(void *arg)
{
    // ThreadPool *pool = (ThreadPool *)arg;c语言的强制转换
    ThreadPool *pool = static_cast<ThreadPool*> (arg);
    while (1)
    {
        pthread_mutex_lock(&pool->mutexPool);
        //判断任务队列是否为空
        while (pool->taskQ->taskNumber() == 0 && !pool->shutdown)
        {
            //阻塞工作线程
            pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

            //判断是不是要销毁线程
            if (pool->exitNum > 0)
            {
                pool->exitNum--;
                if(pool->liveNum > pool->minNum){
                    pool->liveNum--;
                    //因为wait时当前线程已经拿到锁，此时需要解锁
                    pthread_mutex_unlock(&pool->mutexPool);
                    pool->threadExit();
                }
                
            }
        }

        //当线程被唤醒，需要判断当前线程池是否被关闭
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            pool->threadExit();
        }
        //从任务中取出一个任务
        Task task = pool->taskQ->takeTask();
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexPool);

        cout<<"thread" <<to_string(pthread_self()) <<"start working"<<endl;
        //运行函数
        task.function(task.arg);
        delete (task.arg);
        task.arg = nullptr; //释放资源，防止野指针

        cout<<"thread" <<to_string(pthread_self()) <<"end working"<<endl;
        //线程执行完忙线程-1
        pthread_mutex_lock(&pool->mutexPool);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexPool);
    }
    return NULL;
}

void *ThreadPool::manager(void *arg)
{
    ThreadPool *pool =  static_cast<ThreadPool*>(arg);
    while (!pool->shutdown)
    {
        //设置3s检测一次
        sleep(3);

        //取出线程池中任务数量和当前线程的数量
        pthread_mutex_lock(&pool->mutexPool);
        int queueSize = pool->taskQ->taskNumber();
        int liveNum = pool->liveNum;
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutexPool);

        //添加线程
        //当前任务个数 > 存货的线程个数 && 存活的线程数 < 最大线程数
        if (queueSize > liveNum && liveNum < pool->maxNum)
        {
            int counter = 0;
            pthread_mutex_lock(&pool->mutexPool);
            for (int i = 0; i < pool->maxNum && counter < NUMBER && liveNum < pool->maxNum; i++)
            {
                //分配空闲线程id
                if (pool->threadIDs[i] == 0)
                {
                    pthread_create(&pool->threadIDs[i], NULL, worker, pool);
                    counter++;
                    //线程池存活线程增加
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }

        //销毁线程
        //忙线程*2 < 存活线程 && 存活线程 > 最小线程数
        if (busyNum * 2 < liveNum && liveNum > pool->minNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum = NUMBER;
            pthread_mutex_unlock(&pool->mutexPool);
            //工作线程自销毁
            //唤醒两个线程
            for (int i = 0; i < NUMBER; i++)
            {
                //唤醒一个阻塞的线程
                pthread_cond_signal(&pool->notEmpty);
            }
        }
    }

    return NULL;
}

void ThreadPool:: threadExit(ThreadPool *pool)
{
    pthread_t tid = pthread_self();
    for(int i = 0; i < maxNum; i++){
        if(threadIDs[i] == tid){
            threadIDs[i] = 0;
            cout<<"threadExit() call..."<<to_string(tid)<<" exitint..."<<endl;
            break;
        }
    }
    pthread_exit(nullptr);
}


void ThreadPool::addTask(Task task) {
    //因为taskQ类里的addTask里已经有锁，因此在这里不需要再加锁
    if(shutdown){

        return;
    }
    //添加任务
    taskQ->addTask(task);

    //唤醒阻塞的线程
    pthread_cond_signal(&notEmpty);

}

int ThreadPool::getBusyNum(){
    pthread_mutex_lock(&mutexPool);
    int busyNum = this->busyNum;
    pthread_mutex_unlock(&mutexPool);
    return busyNum;
}
int ThreadPool::getAliveNum(){
    pthread_mutex_lock(&mutexPool);
    int aliveNum = this->liveNum;
    pthread_mutex_unlock(&mutexPool);
    return aliveNum;
}

