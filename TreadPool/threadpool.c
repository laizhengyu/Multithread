#include "threadpool.h"
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
const int NUMBER = 2;

//任务结构体
typedef struct Task
{
    void (*function)(void *arg);
    void *arg;
} Task;
//线程池结构体
struct ThreadPool
{
    //任务队列相关
    Task *taskQ;
    int queueCapacity; //容量
    int queueSize;     //当前任务个数
    int queueFront;
    int queueRear;

    pthread_t managerID;  //管理者线程ID
    pthread_t *threadIDs; //工作线程ID
    int minNum;           //最大最小线程数
    int maxNum;
    int busyNum; //忙线程数
    int liveNum; //当前存活线程个数
    int exitNum; //要销毁的线程个数

    pthread_mutex_t mutexPool; //锁整个线程池
    pthread_mutex_t mutexBusy; //锁busyNum，因为变化频繁
    pthread_cond_t notFull;    //判空与判满
    pthread_cond_t notEmpty;

    int shutdown; //是否销毁线程池，1销毁0不
};

ThreadPool *threadPoolCreate(int min, int max, int queueSize)
{
    //initial
    ThreadPool *pool = (ThreadPool*)malloc(sizeof(ThreadPool));

    do
    {
        if (pool == NULL)
        {
            printf("malloc threadpool fail\n");
            break;
        }

        pool->threadIDs = (pthread_t *)malloc(sizeof(pthread_t) * max);
        if (pool->threadIDs == NULL)
        {
            printf("malloc thread fail\n");
            break;
        }
        memset(pool->threadIDs, 0, sizeof(pthread_t) * max);
        pool->minNum = min;
        pool->maxNum = max;
        pool->busyNum = 0;
        pool->liveNum = min;
        pool->exitNum = 0;

        if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
            pthread_mutex_init(&pool->mutexBusy, NULL) != 0 ||
            pthread_cond_init(&pool->notEmpty, NULL) != 0 ||
            pthread_cond_init(&pool->notFull, NULL) != 0)
        {
            printf("mutex initial fail\n");
            break;
        }

        //任务队列
        pool->taskQ = (Task *)malloc(sizeof(Task) * queueSize);
        pool->queueCapacity = queueSize;
        pool->queueSize = 0;
        pool->queueFront = 0;
        pool->queueRear = 0;

        pool->shutdown = 0;

        //创建线程
        pthread_create(&pool->managerID, NULL, manager, pool); //管理线程
        for (int i = 0; i < min; i++)
        {
            pthread_create(&pool->threadIDs[i], NULL, worker, pool); //工作线程,函数指针为worker
        }
        return pool;
    } while (0);

    //释放资源
    if (pool->threadIDs)
        free(pool->threadIDs);
    if (pool->taskQ)
        free(pool->taskQ);
    if (pool)
        free(pool);
    return NULL;
}
//void*arg 可以接收任意类型
void *worker(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;
    while (1)
    {
        pthread_mutex_lock(&pool->mutexPool);
        //判断任务队列是否为空
        while (pool->queueSize == 0 && !pool->shutdown)
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
                    threadExit(pool);
                }
                
            }
        }

        //当线程被唤醒，需要判断当前线程池是否被关闭
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            threadExit(pool);
        }
        //从任务中取出一个任务
        Task task;
        task.function = pool->taskQ[pool->queueFront].function;
        task.arg = pool->taskQ[pool->queueFront].arg;
        //维护环形队列，移动头节点
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->queueSize--;
        //消费了一个任务，唤醒生产者
        pthread_cond_signal(&pool->notFull);

        pthread_mutex_unlock(&pool->mutexPool);

        //忙线程个数加一
        pthread_mutex_lock(&pool->mutexPool);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexPool);

        //运行函数
        task.function(task.arg);
        free(task.arg);
        task.arg = NULL; //释放资源，防止野指针
        printf("thread end working\n");
        //线程执行完忙线程-1
        pthread_mutex_lock(&pool->mutexPool);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexPool);
    }
    return NULL;
}

void *manager(void *arg)
{
    ThreadPool *pool = (ThreadPool *)arg;
    while (!pool->shutdown)
    {
        //设置3s检测一次
        sleep(3);

        //取出线程池中任务数量和当前线程的数量
        pthread_mutex_lock(&pool->mutexPool);
        int queueSize = pool->queueSize;
        int liveNum = pool->liveNum;
        pthread_mutex_unlock(&pool->mutexPool);

        //忙线程的数量,放上面也可以，但是busyNum使用频繁，这样写效率更高
        pthread_mutex_lock(&pool->mutexBusy);
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutexBusy);

        //添加线程
        //当前任务个数 》 存货的线程个数 && 存活的线程数 《 最大线程数
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
        //忙线程*2 《 存活线程 && 存活线程 》 最小线程数
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
//线程退出或者销毁时的工作
void threadExit(ThreadPool *pool)
{
    pthread_t tid = pthread_self();
    for(int i = 0; i < pool->maxNum; i++){
        if(pool->threadIDs[i] == tid){
            pool->threadIDs[i] = 0;
            printf("threadExit() call., exitint..\n");
            break;
        }
    }
    pthread_exit(NULL);
}


void threadPoolAdd(ThreadPool* pool, void(*func)(void*), void *arg){
    pthread_mutex_lock(&pool->mutexPool);
    //容量已满或已关闭
    while(pool->queueSize == pool->queueCapacity && !pool->shutdown)
    {
        //阻塞生产者线程
        pthread_cond_wait(&pool->notFull, &pool->mutexPool);

    }
    if(pool->shutdown){
        pthread_mutex_unlock(&pool->mutexPool);
        return;
    }
    //添加任务
    pool->taskQ[pool->queueRear].function = func;
    pool->taskQ[pool->queueRear].arg = arg;
    pool->queueRear =  (pool->queueRear + 1) % pool->queueCapacity;
    pool->queueSize++;

    //唤醒阻塞的线程
    pthread_cond_signal(&pool->notEmpty);
    pthread_mutex_unlock(&pool->mutexPool);
}


int threadPoolBusyNum(ThreadPool* pool){
    pthread_mutex_lock(&pool->mutexBusy);
    int busyNum = pool->busyNum;
    pthread_mutex_unlock(&pool->mutexBusy);
    return busyNum;
}

int threadPoolAliveNum(ThreadPool* pool){
    pthread_mutex_lock(&pool->mutexPool);
    int aliveNum = pool->liveNum;
    pthread_mutex_unlock(&pool->mutexPool);
    return aliveNum;


}

int threadPoolDestroy(ThreadPool* pool){
    if(pool == NULL)
        return -1;
    //关闭线程池，其他函数里很多判断线程池是否关闭的操作
    pool->shutdown = 1;
    //阻塞回收管理这线程
    pthread_join(pool->managerID, NULL);
    //唤醒阻塞的消费者
    for(int i = 0; i < pool->liveNum; i++){
        pthread_cond_signal(&pool->notEmpty);
    }
    //释放堆内存
    if(pool->taskQ){
        free(pool->taskQ);
    }
    if(pool->threadIDs){
        free(pool->threadIDs);
    }
    free(pool);
    pool = NULL;
    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);
    
    return 0;    
}
