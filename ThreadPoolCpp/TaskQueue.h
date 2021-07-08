#include<queue>
#include<pthread.h>
using callback = void (*)(void* arg);

struct Task
{
    Task()
    {
        function = nullptr;
        arg = nullptr;
    }
    Task(callback f, void* arg){
        this->arg = arg;
        function = f;
    }
    callback function;
    void* arg;
};

//任务队列
class TaskQueue{
public:
    TaskQueue();
    ~TaskQueue();
    void addTask(Task task);
    //重载，类型为callback 和 void* arg
    void addTask(callback f, void* arg);
    Task takeTask();
    //获取当前任务的个数，可以直接调用mtask的函数，使用内联函数提高效率
    inline size_t taskNumber(){
        //不会进行代码块替换，直接压栈，因此效率更高，适合简单的代码
        return m_taskQ.size();
    }

private:
    pthread_mutex_t m_mutex;
    std::queue<Task> m_taskQ;
};
