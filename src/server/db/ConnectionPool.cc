#include "ConnectionPool.h"
#include <fstream>
#include <thread>
#include <assert.h>

ConnectionPool *ConnectionPool::getConnectionPool()
{
    static ConnectionPool pool;
    return &pool;
}

ConnectionPool::ConnectionPool()
{
    parseJsonFile();
    for (int i = 0; i < initSize_; i++)
    {
        addConnection();
    }
    std::thread produceThread(&ConnectionPool::produceConnection, this);
    std::thread recycleThread(&ConnectionPool::recycleConnection, this);
    produceThread.detach();
    recycleThread.detach();
}

ConnectionPool::~ConnectionPool()
{
    while (!connectionQue_.empty())
    {
        MysqlConn *conn = connectionQue_.front();
        connectionQue_.pop();
        delete conn;
    }
}

bool ConnectionPool::parseJsonFile()
{
    // std::ifstream file("conf.json");
    // json conf = json::parse(file);

    // ip_ = conf["ip"];
    // user_ = conf["user"];
    // passwd_ = conf["password"];
    // dbName_ = conf["dbName"];
    // port_ = conf["port"];
    // initSize_ = conf["initSize"];
    // maxSize_ = conf["maxSize"];
    // maxIdleTime_ = conf["maxIdleTime"];
    // timeOut_ = conf["timeOut"];

    ip_ = "127.0.0.1";
    user_ = "root";
    passwd_ = "123456";
    dbName_ = "chat";
    port_ = 3306;
    initSize_ = 10;
    maxSize_ = 20;
    maxIdleTime_ = 5000;
    timeOut_ = 1000;

    return true;
}

void ConnectionPool::addConnection()
{
    MysqlConn *conn = new MysqlConn();
    conn->connect(user_, passwd_, dbName_, ip_, port_);
    conn->refreshAliveTime(); // 刷新起始的空闲时间点
    connectionQue_.push(conn);
}

void ConnectionPool::produceConnection()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (connectionQue_.size() >= maxSize_)
        {
            cond_.wait(lock);
        }
        addConnection();
        cond_.notify_all();
    }
}

std::shared_ptr<MysqlConn> ConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> lock(mutex_);
    while (connectionQue_.empty())
    {
        if (std::cv_status::timeout == cond_.wait_for(lock, std::chrono::seconds(timeOut_)))
        {
            return nullptr; // 超时返回空指针
        }
    }

    // 自定义shared_ptr的释放资源的方式，把connection直接归还到queue当中
    std::shared_ptr<MysqlConn> connptr(connectionQue_.front(), [this](MysqlConn *conn)
                                       {
        std::unique_lock<std::mutex> lock(mutex_);
        conn->refreshAliveTime();
        connectionQue_.push(conn); });
    connectionQue_.pop();
    cond_.notify_all();
    return connptr;
}

void ConnectionPool::recycleConnection()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(maxIdleTime_));
        std::unique_lock<std::mutex> lock(mutex_);
        while (connectionQue_.size() > initSize_)
        {
            MysqlConn *conn = connectionQue_.front();
            if (conn->getAliveTime() >= maxIdleTime_)
            {
                connectionQue_.pop();
                delete conn;
            }
            else
            {
                break;
            }
        }
    }
}
