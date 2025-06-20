#pragma once
#include "interfaces/config_manager_interface.h"
#include "interfaces/backup_storage_interface.h"
#include "interfaces/user_storage_interface.h"
#include "interfaces/data_manager_interface.h"
#include "interfaces/user_manager_interface.h"
#include "interfaces/session_manager_interface.h"
#include "interfaces/auth_manager_interface.h"
#include "interfaces/handler_factory_interface.h"
#include "interfaces/route_registry_interface.h"
#include "interfaces/compress_interface.h"
#include <memory>
#include <string>

namespace zbackup::core
{
    // 依赖注入配置结构
    struct DependencyConfig
    {
        std::string config_file = "../config/config.json";
        bool use_database_storage = true;
        bool initialize_database_pools = true;
        
        DependencyConfig() = default;
        
        // 便捷设置方法
        DependencyConfig& with_config_file(const std::string& file) {
            config_file = file;
            return *this;
        }
        
        DependencyConfig& with_file_storage() {
            use_database_storage = false;
            initialize_database_pools = false;
            return *this;
        }
        
        DependencyConfig& with_database_storage() {
            use_database_storage = true;
            initialize_database_pools = true;
            return *this;
        }
    };

    // 依赖注入管理器
    class DependencyInjector
    {
    public:
        explicit DependencyInjector(const DependencyConfig& config = DependencyConfig{});
        ~DependencyInjector() = default;

        // 执行依赖注入
        void inject_dependencies();
        
        // 获取主要服务
        interfaces::IConfigManager::ptr get_config_manager() const;
        interfaces::IRouteRegistry::ptr get_route_registry() const;
        interfaces::ICompress::ptr get_compressor() const;

    private:
        // 依赖注入步骤
        void step1_create_and_register_config_manager();
        void step2_initialize_database_pools();
        void step3_create_and_register_storage_layer();
        void step4_create_and_register_manager_layer();
        void step5_create_and_register_compressor();
        void step6_create_and_register_auth_layer();
        void step7_create_and_register_factory_and_registry();
        
        // 创建具体组件的方法
        void create_config_manager();
        void create_storage_components();
        void create_manager_components();
        void create_auth_components();
        void create_factory_and_registry();
        
        // 配置
        DependencyConfig config_;
        
        // 组件实例
        interfaces::IConfigManager::ptr config_manager_;
        interfaces::IBackupStorage::ptr backup_storage_;
        interfaces::IUserStorage::ptr user_storage_;
        interfaces::IDataManager::ptr data_manager_;
        interfaces::IUserManager::ptr user_manager_;
        interfaces::ISessionManager::ptr session_manager_;
        interfaces::IAuthenticationService::ptr auth_service_;
        interfaces::IHandlerFactory::ptr handler_factory_;
        interfaces::IRouteRegistry::ptr route_registry_;
        interfaces::ICompress::ptr compressor_;
    };
}
