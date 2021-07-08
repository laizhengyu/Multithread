#ifndef _THREADPOOL_H
#define _THREADPOOL_H

typedef struct ThreadPool ThreadPool;

//创建线程池并初始化
ThreadPool *threadPoolCreate(int min, int max, int queueSize);

//销毁线程池
int threadPoolDestroy(ThreadPool* pool);

//添加任务
void threadPoolAdd(ThreadPool* pool, void(*func)(void*), void *arg);

//获取线程池中的线程个数
int threadPoolBusyNum(ThreadPool* pool);

//获取线程池中活着（不一定在工作）的线程个数
int threadPoolAliveNum(ThreadPool* pool);

//---------------------------------
void* worker(void* arg);
void* manager(void* arg);
void threadExit(ThreadPool* pool);
#endif //THREADPOOL_H