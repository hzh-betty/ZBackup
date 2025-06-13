#pragma once
#include <string>
#include <memory>

namespace zbackup::interfaces
{
    // 基础信息接口
    class IBaseInfo
    {
    public:
        using ptr = std::shared_ptr<IBaseInfo>;
        virtual ~IBaseInfo() = default;
        
        // 获取唯一标识符
        virtual std::string get_id() const = 0;
        
        // 设置唯一标识符
        virtual void set_id(const std::string& id) = 0;
        
        // 序列化为字符串
        virtual std::string serialize() const = 0;
        
        // 从字符串反序列化
        virtual bool deserialize(const std::string& data) = 0;
        
        // 克隆对象
        virtual ptr clone() const = 0;
    };
}
