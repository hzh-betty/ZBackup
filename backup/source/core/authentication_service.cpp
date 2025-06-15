#include "core/authentication_service.h"
#include "core/service_container.h"
#include "interfaces/user_manager_interface.h"
#include "log/backup_logger.h"


namespace zbackup::core
{
    AuthenticationService::AuthenticationService(interfaces::IUserManager::ptr user_manager, interfaces::ISessionManager::ptr session_manager)
        : user_manager_(std::move(user_manager)), session_manager_(std::move(session_manager))
    {
        if (!user_manager_ || !session_manager_)
        {
            ZBACKUP_LOG_ERROR("AuthenticationService requires IUserManager and ISessionManager dependencies");
            throw std::runtime_error("AuthenticationService requires IUserManager and ISessionManager dependencies");
        }
        ZBACKUP_LOG_DEBUG("AuthenticationService created, dependencies will be resolved on first use");
    }

    bool AuthenticationService::authenticate(const zhttp::HttpRequest &req)
    {
        return session_manager_->is_valid_session(req);
    }

    bool AuthenticationService::login(const std::string &username, const std::string &password,
                                      const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
    {
        if (!user_manager_->validate_user(username, password))
        {
            ZBACKUP_LOG_WARN("Invalid login attempt for user: {}", username);
            return false;
        }

        session_manager_->create_session(username, req, rsp);
        ZBACKUP_LOG_INFO("User '{}' logged in successfully", username);
        return true;
    }

    bool AuthenticationService::logout(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp)
    {
        try
        {
            auto username = session_manager_->get_username(req);
            session_manager_->destroy_session(req, rsp);
            ZBACKUP_LOG_INFO("User '{}' logged out successfully", username);
            return true;
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Error during logout: {}", e.what());
            return false;
        }
    }
}
