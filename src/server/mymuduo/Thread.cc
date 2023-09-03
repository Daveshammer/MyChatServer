#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach(); // thread类提供的设置分离线程的方法
    }
}

void Thread::start()  // 一个Thread对象，记录的就是一个新线程的详细信息
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);

    // 开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        // 获取线程的tid值
        tid_ = CurrentThread::tid();
        sem_post(&sem);
        // 开启一个新线程，专门执行该线程函数
        func_(); 
    }));

    // 这里必须等待获取上面新创建的线程的tid值
    sem_wait(&sem);
}
// 信号量也可以改成互斥锁和条件变量
// void Thread::start()
// {
//     std::mutex mtx;
//     std::condition_variable cond;
//     //  start thread
//     thread_ = std::unique_ptr<std::thread>(new std::thread([&](void)->void{

//         //  临界区: tid_ , thread func内要写;开启thread的thread要读。
//         mtx.lock();
//         tid_ = CurrentThread::tid();
//         cond.notify_one();
//         mtx.unlock();

//         //  threadFunc
//         func_();
//     }));

//     {
//         std::unique_lock<std::mutex> lock(mtx);
//         while(!(tid_ != 0)){
//             cond.wait(lock);
//         }
//     }
//     started_ = true;
// }

void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}