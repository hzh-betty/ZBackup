#pragma once
#include "base_handler.h"

namespace zbackup
{
    class DeleteHandler final : public DataHandler
    {
    public:
        explicit DeleteHandler(interfaces::IDataManager::ptr data_manager);

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;
    };
}
