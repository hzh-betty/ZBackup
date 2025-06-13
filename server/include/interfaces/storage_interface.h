#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "base_info_interface.h"

namespace zbackup::interfaces
{
    // 泛型存储接口
    template<typename InfoType>
    class IStorage
    {
    public:
        using ptr = std::shared_ptr<IStorage<InfoType>>;
        virtual ~IStorage() = default;

        // 基本CRUD操作 - 内部自动处理持久化
        virtual bool insert(const InfoType &info) = 0;
        virtual bool update(const InfoType &info) = 0;
        virtual bool get_one_by_id(const std::string &id, InfoType *info) = 0;
        virtual void get_all(std::vector<InfoType> *arry) = 0;
        virtual bool delete_one(const InfoType &info) = 0;
        virtual bool delete_by_id(const std::string &id) = 0;
        
        // 扩展查询接口
        virtual std::vector<InfoType> find_by_condition(const std::function<bool(const InfoType&)>& condition) = 0;
    };
}
