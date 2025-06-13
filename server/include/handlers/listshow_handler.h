#pragma once
#include "base_handler.h"

namespace zbackup
{
    class ListShowHandler final : public DataHandler
    {
    public:
        explicit ListShowHandler(interfaces::IDataManager::ptr data_manager);

        void handle_request(const zhttp::HttpRequest &req, zhttp::HttpResponse *rsp) override;
    };
}
