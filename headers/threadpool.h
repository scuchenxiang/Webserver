//
// Created by zzh on 2022/4/19.
//

#ifndef MY_WEBSERVER_THREADPOOL_H
#define MY_WEBSERVER_THREADPOOL_H

#include<mutex>
#include<queue>
#include<thread>
#include<functional>
#include<condition_variable>
#include<assert.h>

class ThreadPool {
public:
    /*
     * 构造函数中根据传入的参数构建线程池
     * 线程是调用detach
     */
    explicit ThreadPool(size_t threadCount = 8) : pool_(std::make_shared<Pool>()) {
        assert(threadCount > 0);

        for (size_t i = 0; i < threadCount; i++) {
            /*使用lambda表达式构建执行对象*/
            std::thread([pool = pool_] {
                /*初始化一个unique lock后面通过此对象的lock与unlock方法进行加锁与解锁，这样比每次都初始化一个unique lock对象要省资源*/
                std::unique_lock<std::mutex> locker(pool->mtx);

                while (true) {
                    if (!pool->tasks.empty()) {
                        /*任务队列不为空，进入本段代码，采用右值的方式取出任务，任务取出成功，解锁，执行完任务重新获取锁*/
                        auto task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();
                        /*执行任务*/
                        task();
                        locker.lock();
                    } else if (pool->isClosed) {
                        /*说明线程池收到了关闭信号，直接跳出循环*/
                        break;
                    } else {
                        /*到了此分支，说明任务队列为空，此时条件变量等待，wait方法会自动释放锁，当收到notify信号时重新尝试获取锁*/
                        //如果是因为等待条件变量阻塞，只能由notify_one()或者notify_all()来唤醒；
                        //如果是为尝试获得锁而阻塞，只能由操作系统在锁的状态发生变化时唤醒；
                        pool->cond.wait(locker);
                    }
                }
            }).detach();  /*detach的形式运行线程*/
        }
    }

    /*
     * 让编译器生成一个默认的移动构造函数
     */
    ThreadPool(ThreadPool &&) = default;

    /*
     * 析构函数中将pool_->isClosed = true;标志置为false，这样detach的线程会自动关闭
     */
    ~ThreadPool(){
        if(static_cast<bool>(pool_)){
            {
                std::lock_guard<std::mutex> locker(pool_->mtx);
                pool_->isClosed = true;
            }
            /*唤醒所有线程，这个线程池的逻辑是执行完了任务队列中的所有任务后才会退出线程*/
            pool_->cond.notify_all();
        }
    }

    /*
     * 类成员模板函数，参数自动推断，向任务队列中添加任务
     * 这里可以设置一个最大任务数量，若超过此数量，禁止向队列加入任务
     */
    template<class F>
    void addTask(F &&task){
        /*利用RAII自动加锁解锁下面这块作用域*/
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            /*完美转发*/
            pool_->tasks.emplace(std::forward<F>(task));
        }
        /*加入一个任务，唤醒一个线程*/
        pool_->cond.notify_one();
    }

private:
    /*定义一个结构体，保存相关变量*/
    //一个保存任务的poll，里面包含条件变量和mutex，然后还包含一个标志表示是否关闭线程池，有一个任务队列
    //std::functional可以指向全局和静态函数，还可以指向彷函数，lambda表达式，类成员函数，
    //甚至函数签名不一致的函数，可以说几乎所有可以调用的对象都可以当做std::function,可以配合bind调用成员函数
    struct Pool {
        std::mutex mtx;  /*互斥量*/
        std::condition_variable cond;  /*条件变量*/
        bool isClosed;  /*标志变量，表示是否关闭线程池*/
        std::queue<std::function<void()>> tasks;  /*任务队列*/
    };
    //智能指针，指向任务队列对象的指针，使得其能被自动delete，因为多个线程都要delete，所以要使用shard_ptr
    std::shared_ptr<Pool> pool_;  /*因为线程是在detach模式下运行的，所以这里使用动态申请的堆内存空间，使用shareptr管理*/
};

#endif //MY_WEBSERVER_THREADPOOL_H
