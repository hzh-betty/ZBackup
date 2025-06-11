#include "../../include/core/authentication_service.h"
#include "../../include/core/service_container.h"
#include "../../include/user/user_manager.h"
#include "../../include/log/logger.h"

namespace zbackup::core
{
    AuthenticationService::AuthenticationService()
    {
        ZBACKUP_LOG_DEBUG("AuthenticationService created, dependencies will be resolved on first use");
    }

    bool AuthenticationService::authenticate(const zhttp::HttpRequest& req)
    {
        if (!ensure_dependencies()) {
            return false;
        }
        return session_manager_->is_valid_session(req);
    }

    bool AuthenticationService::login(const std::string& username, const std::string& password,
                                     const zhttp::HttpRequest& req, zhttp::HttpResponse* rsp)
    {
        if (!ensure_dependencies()) {
            ZBACKUP_LOG_ERROR("Authentication services not available");
            return false;
        }

        if (!user_manager_->validate_user(username, password)) {
            ZBACKUP_LOG_WARN("Invalid login attempt for user: {}", username);
            return false;
        }

        session_manager_->create_session(username, req, rsp);
        ZBACKUP_LOG_INFO("User '{}' logged in successfully", username);
        return true;
    }

    bool AuthenticationService::logout(const zhttp::HttpRequest& req, zhttp::HttpResponse* rsp)
    {
        if (!ensure_dependencies()) {
            return false;
        }

        try {
            auto username = session_manager_->get_username(req);
            session_manager_->destroy_session(req, rsp);
            ZBACKUP_LOG_INFO("User '{}' logged out successfully", username);
            return true;
        }
        catch (const std::exception& e) {
            ZBACKUP_LOG_ERROR("Error during logout: {}", e.what());
            return false;
        }
    }

    bool AuthenticationService::ensure_dependencies()
    {
        if (!user_manager_ || !session_manager_) {
            auto& container = ServiceContainer::get_instance();
            
            if (!user_manager_) {
                user_manager_ = container.resolve<UserManager>();
                if (!user_manager_) {
                    ZBACKUP_LOG_ERROR("Failed to resolve UserManager");
                    return false;
                }
            }
            
            if (!session_manager_) {
                session_manager_ = container.resolve<interfaces::ISessionManager>();
                if (!session_manager_) {
                    ZBACKUP_LOG_ERROR("Failed to resolve ISessionManager");
                    return false;
                }
            }
        }
        return true;
    }
}
