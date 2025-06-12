#pragma once
#include <memory>

namespace zbackup::interfaces
{
    // 服务器生命周期管理接口
    class IServerLifecycle
    {
    public:
        using ptr = std::shared_ptr<IServerLifecycle>;
        virtual ~IServerLifecycle() = default;

        virtual bool initialize() = 0;
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual bool is_running() const = 0;
    };
}
