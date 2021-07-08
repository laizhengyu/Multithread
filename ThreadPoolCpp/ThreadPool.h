#include"TaskQueue.h"
#include"TaskQueue.cpp"
class ThreadPool{
public:

    //由于在类里，基于成员变量可以节省很多参数，并且可以将部分属性隐藏，因此不需要像c++那样添加参数在getBusyNum之类
    //创建线程池并初始化,直接使用构造函数
    ThreadPool(int min, int max);

    //销毁线程池，直接使用析构
    ~ThreadPool();

    //添加任务
    void addTask(Task task);

    //获取线程池中的线程个数
    int getBusyNum();

    //获取线程池中活着（不一定在工作）的线程个数
    int getAliveNum();

    void threadExit(ThreadPool* pool);



private:
    //任务队列相关
    TaskQueue *taskQ;


    pthread_t managerID;  //管理者线程ID
    pthread_t *threadIDs; //工作线程ID
    int minNum;           //最大最小线程数
    int maxNum;
    int busyNum; //忙线程数
    int liveNum; //当前存活线程个数
    int exitNum; //要销毁的线程个数

    pthread_mutex_t mutexPool; //锁整个线程池
    pthread_cond_t notEmpty;

    bool shutdown; //是否销毁线程池，1销毁0不

    static const int NUMBER = 2;
    /*封装为私有，不需要调用者看到,
    因为两种线程需要在线程create传入地址，使用友元会破坏类的封装
    因此使用静态成员函数的方式*/
    static void* worker(void* arg);
    static void* manager(void* arg);
    void threadExit();

};