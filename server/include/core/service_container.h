#pragma once
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <functional>
#include <mutex>
#include "../log/logger.h"

namespace zbackup::core
{
    // 服务容器 - 依赖注入管理
    class ServiceContainer
    {
    public:
        using ServiceFactory = std::function<std::shared_ptr<void>()>;

        static ServiceContainer& get_instance()
        {
            static ServiceContainer instance;
            return instance;
        }

        // 注册服务工厂
        template<typename Interface, typename Implementation>
        void register_service()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto factory = []() -> std::shared_ptr<void> {
                return std::make_shared<Implementation>();
            };
            services_[std::type_index(typeid(Interface))] = factory;
            ZBACKUP_LOG_DEBUG("Service registered: {} -> {}", 
                            typeid(Interface).name(), typeid(Implementation).name());
        }

        // 注册单例服务
        template<typename Interface, typename Implementation>
        void register_singleton()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto factory = []() -> std::shared_ptr<void> {
                static std::weak_ptr<Implementation> weak_instance;
                auto shared_instance = weak_instance.lock();
                if (!shared_instance) {
                    shared_instance = std::make_shared<Implementation>();
                    weak_instance = shared_instance;
                }
                return shared_instance;
            };
            services_[std::type_index(typeid(Interface))] = factory;
            ZBACKUP_LOG_DEBUG("Singleton service registered: {} -> {}", 
                            typeid(Interface).name(), typeid(Implementation).name());
        }

        // 注册实例
        template<typename Interface>
        void register_instance(std::shared_ptr<Interface> instance)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto factory = [instance]() -> std::shared_ptr<void> {
                return instance;
            };
            services_[std::type_index(typeid(Interface))] = factory;
            ZBACKUP_LOG_DEBUG("Instance registered: {}", typeid(Interface).name());
        }

        // 获取服务
        template<typename Interface>
        std::shared_ptr<Interface> resolve()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = services_.find(std::type_index(typeid(Interface)));
            if (it != services_.end()) {
                return std::static_pointer_cast<Interface>(it->second());
            }
            ZBACKUP_LOG_ERROR("Service not found: {}", typeid(Interface).name());
            return nullptr;
        }

        // 检查服务是否已注册
        template<typename Interface>
        bool is_registered() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return services_.find(std::type_index(typeid(Interface))) != services_.end();
        }

        // 清理所有服务
        void clear()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            services_.clear();
            ZBACKUP_LOG_DEBUG("All services cleared");
        }

    private:
        ServiceContainer() = default;
        mutable std::mutex mutex_;
        std::unordered_map<std::type_index, ServiceFactory> services_;
    };
}
