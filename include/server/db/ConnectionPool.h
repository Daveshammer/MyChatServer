#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include "MysqlConn.h"
#include "json.hpp"
using json = nlohmann::json;

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ConnectionPool
{
public:
    static ConnectionPool* getConnectionPool();
    std::shared_ptr<MysqlConn> getConnection();
    ~ConnectionPool();
private:
    ConnectionPool();
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    bool parseJsonFile();
    void produceConnection();
    void recycleConnection();
    void addConnection();

    std::string ip_;
    std::string user_;
    std::string passwd_;
    std::string dbName_;
    unsigned short port_;
    int initSize_;
    int maxSize_;
    int maxIdleTime_;
    int timeOut_;

    std::queue<MysqlConn*> connectionQue_;
    std::atomic_int connectionCnt_; // 记录连接所创建的connection连接的总数量 
    std::mutex mutex_;
    std::condition_variable cond_;
};
#endif // CONNECTION_POOL_H