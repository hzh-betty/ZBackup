#include "info/backup_info.h"
#include "core/service_container.h"
#include "interfaces/config_manager_interface.h"
#include "util/util.h"
#include "log/backup_logger.h"
#include <nlohmann/json.hpp>

namespace zbackup::info
{
    BackupInfo::BackupInfo(const std::string &real_path)
    {
        new_backup_info(real_path);
    }

    bool BackupInfo::new_backup_info(const std::string &real_path)
    {
        util::FileUtil fu(real_path);
        if (!fu.exists())
        {
            ZBACKUP_LOG_ERROR("File not exists when creating backup info: {}", real_path);
            return false;
        }

        auto &container = core::ServiceContainer::get_instance();
        auto config = container.resolve<interfaces::IConfigManager>();

        std::string pack_dir = config->get_string("pack_dir", "./pack/");
        std::string pack_suffix = config->get_string("packfile_suffix", ".pack");
        std::string down_str = config->get_download_prefix();

        pack_flag_ = false;
        fsize_ = fu.get_size();
        mtime_ = fu.get_last_mtime();
        atime_ = fu.get_last_atime();
        real_path_ = real_path;
        pack_path_ = pack_dir + fu.get_name() + pack_suffix;
        url_ = down_str + fu.get_name();

        ZBACKUP_LOG_DEBUG("Backup info created: {} -> {}", real_path, url_);
        return true;
    }

    std::string BackupInfo::serialize() const
    {
        nlohmann::json j;
        j["pack_flag"] = pack_flag_;
        j["fsize"] = fsize_;
        j["mtime"] = mtime_;
        j["atime"] = atime_;
        j["real_path"] = real_path_;
        j["pack_path"] = pack_path_;
        j["url"] = url_;
        return j.dump();
    }

    bool BackupInfo::deserialize(const std::string &data)
    {
        try
        {
            nlohmann::json j = nlohmann::json::parse(data);
            pack_flag_ = j.value("pack_flag", false);
            fsize_ = j.value("fsize", 0);
            mtime_ = j.value("mtime", 0);
            atime_ = j.value("atime", 0);
            real_path_ = j.value("real_path", "");
            pack_path_ = j.value("pack_path", "");
            url_ = j.value("url", "");
            return true;
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Failed to deserialize BackupInfo: {}", e.what());
            return false;
        }
    }

    interfaces::IBaseInfo::ptr BackupInfo::clone() const
    {
        auto cloned = std::make_shared<BackupInfo>();
        cloned->pack_flag_ = pack_flag_;
        cloned->fsize_ = fsize_;
        cloned->mtime_ = mtime_;
        cloned->atime_ = atime_;
        cloned->real_path_ = real_path_;
        cloned->pack_path_ = pack_path_;
        cloned->url_ = url_;
        return cloned;
    }
}
