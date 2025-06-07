#include "server.hpp"
#include <muduo/base/Logging.h>


int main()
{
    muduo::Logger::setLogLevel(muduo::Logger::ERROR);
    zhttp::Log::Init(zlog::LogLevel::value::INFO);
    zbackup::Log::Init(zlog::LogLevel::value::INFO);
    zbackup::Compress::ptr compress(new zbackup::SnappyCompress());
    zbackup::Storage::ptr storage(new zbackup::FileStorage());
    zbackup::BackupServer::ptr server(new zbackup::BackupServer(compress, storage));
    zbackup::ThreadPool::getInstance()->start();
    server->run();
    return 0;
}