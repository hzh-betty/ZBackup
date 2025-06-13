#include "../../../include/storage/file/file_user_storage.h"
#include "../../../include/core/service_container.h"
#include "../../../include/interfaces/config_manager_interface.h"
#include "../../../include/util/util.h"
#include "../../../include/log/logger.h"
#include "../../../include/info/user_info.h"

namespace zbackup::storage
{
    FileUserStorage::FileUserStorage()
    {
        auto& container = core::ServiceContainer::get_instance();
        auto config = container.resolve<interfaces::IConfigManager>();
        user_file_ = config->get_string("user_file", "./data/users.dat");
        
        // 确保数据目录存在
        FileUtil data_dir("./data/");
        data_dir.create_directory();
        
        init_load();
        ZBACKUP_LOG_INFO("FileUserStorage initialized with file: {}", user_file_);
    }

    bool FileUserStorage::insert(const info::UserInfo &info)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        if (users_.find(info.username_) != users_.end())
        {
            ZBACKUP_LOG_WARN("User already exists: {}", info.username_);
            return false;
        }
        
        // 检查邮箱是否已存在
        for (const auto& pair : users_)
        {
            if (pair.second.email_ == info.email_)
            {
                ZBACKUP_LOG_WARN("Email already exists: {}", info.email_);
                return false;
            }
        }
        
        users_[info.username_] = info;
        bool result = save_to_file();
        if (result)
        {
            ZBACKUP_LOG_DEBUG("User inserted: {}", info.username_);
        }
        return result;
    }

    bool FileUserStorage::update(const info::UserInfo &info)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        auto it = users_.find(info.username_);
        if (it == users_.end())
        {
            ZBACKUP_LOG_WARN("User not found for update: {}", info.username_);
            return false;
        }
        
        it->second = info;
        bool result = save_to_file();
        if (result)
        {
            ZBACKUP_LOG_DEBUG("User updated: {}", info.username_);
        }
        return result;
    }

    bool FileUserStorage::get_one_by_id(const std::string &id, info::UserInfo *info)
    {
        return get_by_username(id, info);
    }

    void FileUserStorage::get_all(std::vector<info::UserInfo> *arry)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        arry->clear();
        for (const auto &pair : users_)
        {
            arry->push_back(pair.second);
        }
        ZBACKUP_LOG_DEBUG("Retrieved all user info: {} entries", arry->size());
    }

    bool FileUserStorage::delete_one(const info::UserInfo &info)
    {
        return delete_by_id(info.username_);
    }

    bool FileUserStorage::delete_by_id(const std::string &id)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        auto it = users_.find(id);
        if (it == users_.end())
        {
            ZBACKUP_LOG_WARN("User not found for deletion: {}", id);
            return false;
        }
        
        users_.erase(it);
        bool result = save_to_file();
        if (result)
        {
            ZBACKUP_LOG_DEBUG("User deleted: {}", id);
        }
        return result;
    }

    std::vector<info::UserInfo> FileUserStorage::find_by_condition(const std::function<bool(const info::UserInfo&)>& condition)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        std::vector<info::UserInfo> result;
        for (const auto &pair : users_)
        {
            if (condition(pair.second))
            {
                result.push_back(pair.second);
            }
        }
        return result;
    }

    bool FileUserStorage::get_by_username(const std::string &username, info::UserInfo *user)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        auto it = users_.find(username);
        if (it != users_.end())
        {
            *user = it->second;
            return true;
        }
        return false;
    }

    bool FileUserStorage::get_by_email(const std::string &email, info::UserInfo *user)
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        for (const auto &pair : users_)
        {
            if (pair.second.email_ == email)
            {
                *user = pair.second;
                return true;
            }
        }
        return false;
    }

    bool FileUserStorage::init_load()
    {
        FileUtil fu(user_file_);
        if (!fu.exists())
        {
            ZBACKUP_LOG_INFO("User file not found, starting with empty storage: {}", user_file_);
            return true;
        }

        std::string body;
        if (!fu.get_content(&body))
        {
            ZBACKUP_LOG_ERROR("Failed to read user file: {}", user_file_);
            return false;
        }

        if (body.empty())
        {
            ZBACKUP_LOG_INFO("User file is empty, starting with empty storage: {}", user_file_);
            return true;
        }

        nlohmann::json root;
        if (!JsonUtil::deserialize(&root, body))
        {
            ZBACKUP_LOG_ERROR("Failed to parse user file: {}", user_file_);
            return false;
        }

        for (const auto& item : root)
        {
            info::UserInfo user;
            user.username_ = item["username"];
            user.password_hash_ = item["password_hash"];
            user.email_ = item["email"];
            user.created_at_ = item["created_at"];
            users_[user.username_] = user;
        }

        ZBACKUP_LOG_INFO("Loaded {} user entries from file", users_.size());
        return true;
    }

    bool FileUserStorage::save_to_file()
    {
        nlohmann::json root = nlohmann::json::array();
        
        for (const auto& pair : users_)
        {
            const auto& user = pair.second;
            nlohmann::json item;
            item["username"] = user.username_;
            item["password_hash"] = user.password_hash_;
            item["email"] = user.email_;
            item["created_at"] = user.created_at_;
            root.push_back(item);
        }

        std::string body;
        if (!JsonUtil::serialize(root, &body))
        {
            ZBACKUP_LOG_ERROR("Failed to serialize user data");
            return false;
        }

        FileUtil fu(user_file_);
        if (!fu.set_content(body))
        {
            ZBACKUP_LOG_ERROR("Failed to write user file: {}", user_file_);
            return false;
        }

        return true;
    }
}
