#pragma once
#include <string>
#include <memory>

namespace zbackup::interfaces
{
    // 配置管理器接口
    class IConfigManager
    {
    public:
        using ptr = std::shared_ptr<IConfigManager>;
        virtual ~IConfigManager() = default;

        // 加载配置文件
        virtual bool load_config() = 0;
        
        // 获取配置项
        virtual std::string get_string(const std::string& key, const std::string& default_value = "") = 0;
        virtual int get_int(const std::string& key, int default_value = 0) = 0;
        virtual bool get_bool(const std::string& key, bool default_value = false) = 0;
        
        // 获取服务器配置
        virtual uint16_t get_port() const = 0;
        virtual std::string get_ip() const = 0;
        virtual std::string get_download_prefix() const = 0;
    };
}
