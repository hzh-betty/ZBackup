#include "../../include/info/user_info.h"
#include "../../include/log/logger.h"
#include <nlohmann/json.hpp>

namespace zbackup::info
{
    std::string UserInfo::serialize() const
    {
        nlohmann::json j;
        j["username"] = username_;
        j["password_hash"] = password_hash_;
        j["email"] = email_;
        j["created_at"] = created_at_;
        j["is_active"] = is_active_;
        return j.dump();
    }

    bool UserInfo::deserialize(const std::string& data)
    {
        try
        {
            nlohmann::json j = nlohmann::json::parse(data);
            username_ = j.value("username", "");
            password_hash_ = j.value("password_hash", "");
            email_ = j.value("email", "");
            created_at_ = j.value("created_at", "");
            is_active_ = j.value("is_active", true);
            return true;
        }
        catch (const std::exception& e)
        {
            ZBACKUP_LOG_ERROR("Failed to deserialize UserInfo: {}", e.what());
            return false;
        }
    }

    interfaces::IBaseInfo::ptr UserInfo::clone() const
    {
        auto cloned = std::make_shared<UserInfo>();
        cloned->username_ = username_;
        cloned->password_hash_ = password_hash_;
        cloned->email_ = email_;
        cloned->created_at_ = created_at_;
        cloned->is_active_ = is_active_;
        return cloned;
    }
}
