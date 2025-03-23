#include"server.hpp"
int main()
{
    zbackup::Log::Init(zlog::LogLevel::value::WARNING);
    zbackup::Compress::ptr compress(new zbackup::SnappyCompress());
    zbackup::Storage::ptr storage(new zbackup::FileStorage());
    zbackup::BackupServer::ptr server(new zbackup::BackupServer(compress,storage));
    server->run();
    return 0;
}