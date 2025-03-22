#include"hot.hpp"
#include"server.hpp"
zbackup::DataManager *data_ = nullptr;
void hot()
{
    zbackup::HotManager hot;
    hot.runModule();
}
void server()
{
    zbackup::Service server;
    server.run();
}
void run()
{
    std::thread thread_hot_manager(hot);
    std::thread thread_service(server);
    thread_hot_manager.join();
    thread_service.join();
}

int main()
{
    zbackup::Log::Init(zlog::LogLevel::value::WARNING);
    data_ = new zbackup::DataManager();
    run();
    return 0;
}