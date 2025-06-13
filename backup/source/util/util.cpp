#include <utility>
#include "../../include/util/util.h"
#include "../../include/core/service_container.h"
#include "../../../ZHttpServer/include/db_pool/mysql_pool.h"
#include "../../../ZHttpServer/include/db_pool/redis_pool.h"
#include "../../include/interfaces/config_manager_interface.h"

namespace zbackup
{
    // 文件工具类构造函数
    FileUtil::FileUtil(std::string pathname) : pathname_(std::move(pathname))
    {
    }

    // 删除单个文件
    bool FileUtil::remove_file()
    {
        // 1. 判断文件是否存在
        if (exists() == false)
        {
            return true;
        }

        if (remove(pathname_.c_str()) == -1)
        {
            ZBACKUP_LOG_ERROR("Failed to remove file [{}]: {}", pathname_, strerror(errno));
            return false;
        }
        ZBACKUP_LOG_DEBUG("File removed successfully: {}", pathname_);
        return true;
    }

    // 删除目录及其所有内容
    bool FileUtil::remove_directory()
    {
        if (!exists())
        {
            return true;
        }

        try
        {
            bool result = fs::remove_all(pathname_);
            if (result)
            {
                ZBACKUP_LOG_DEBUG("Directory removed successfully: {}", pathname_);
            }
            else
            {
                ZBACKUP_LOG_ERROR("Failed to remove directory: {}", pathname_);
            }
            return result;
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Exception while removing directory [{}]: {}", pathname_, e.what());
            return false;
        }
    }

    // 强制删除文件或目录
    bool FileUtil::force_remove()
    {
        if (!exists())
        {
            return true;
        }

        try
        {
            if (fs::is_directory(pathname_))
            {
                return remove_directory();
            }
            else
            {
                return remove_file();
            }
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Exception during force remove [{}]: {}", pathname_, e.what());
            return false;
        }
    }

    // 获取文件大小（字节）
    int64_t FileUtil::get_size() const
    {
        struct stat st{};
        if (stat(pathname_.c_str(), &st) < 0)
        {
            ZBACKUP_LOG_WARN("Failed to get file size [{}]: {}", pathname_, strerror(errno));
            return -1;
        }
        return st.st_size;
    }

    // 获取文件最后修改时间
    time_t FileUtil::get_last_mtime() const
    {
        struct stat st{};
        if (stat(pathname_.c_str(), &st) < 0)
        {
            ZBACKUP_LOG_WARN("Failed to get file mtime [{}]: {}", pathname_, strerror(errno));
            return -1;
        }
        return st.st_mtime;
    }

    // 获取文件最后访问时间
    time_t FileUtil::get_last_atime() const
    {
        struct stat st{};
        if (stat(pathname_.c_str(), &st) < 0)
        {
            ZBACKUP_LOG_WARN("Failed to get file atime [{}]: {}", pathname_, strerror(errno));
            return -1;
        }
        return st.st_atime;
    }

    // 从路径中提取文件名
    std::string FileUtil::get_name() const
    {
        std::string filename;
        size_t pos = pathname_.find_last_of('/');
        if (pos == std::string::npos)
        {
            filename = pathname_;
        }
        else
        {
            filename = pathname_.substr(pos + 1);
        }
        return filename;
    }

    // 读取文件指定位置和长度的内容
    bool FileUtil::get_pos_len(std::string *body, size_t pos, size_t len)
    {
        if (exists() == false)
        {
            ZBACKUP_LOG_WARN("File not exists: {}", pathname_);
            return false;
        }

        int64_t file_size = get_size();
        if (pos + len > file_size)
        {
            ZBACKUP_LOG_WARN("Read range out of bounds [{}]: pos={}, len={}, file_size={}", pathname_, pos, len,
                             file_size);
            return false;
        }

        std::ifstream ifs;
        ifs.open(pathname_, std::ios::in | std::ios::binary);
        if (!ifs.is_open())
        {
            ZBACKUP_LOG_ERROR("Failed to open file for reading: {}", pathname_);
            return false;
        }

        ifs.seekg(pos, std::ios::beg);
        body->resize(len);
        ifs.read(&(*body)[0], (size_t)len);

        if (!ifs.good())
        {
            ZBACKUP_LOG_ERROR("Failed to read file content: {}", pathname_);
            ifs.close();
            return false;
        }

        ifs.close();
        return true;
    }

    // 读取整个文件内容
    bool FileUtil::get_content(std::string *body)
    {
        int64_t file_size = get_size();
        return get_pos_len(body, 0, file_size);
    }

    // 写入内容到文件
    bool FileUtil::set_content(const std::string &body)
    {
        std::ofstream ofs;
        ofs.open(pathname_, std::ios::out | std::ios::binary);
        if (!ofs.is_open())
        {
            ZBACKUP_LOG_ERROR("Failed to open file for writing: {}", pathname_);
            return false;
        }

        ofs.write(&body[0], body.size());
        if (!ofs.good())
        {
            ZBACKUP_LOG_ERROR("Failed to write file content: {}", pathname_);
            ofs.close();
            return false;
        }

        ofs.close();
        ZBACKUP_LOG_DEBUG("File written successfully: {} ({} bytes)", pathname_, body.size());
        return true;
    }

    // 检查文件或目录是否存在
    bool FileUtil::exists() const
    {
        return fs::exists(pathname_);
    }

    // 创建空文件
    bool FileUtil::create_file()
    {
        std::ofstream create_file(pathname_);
        if (create_file.is_open() == false)
        {
            ZBACKUP_LOG_ERROR("Failed to create file: {}", pathname_);
            return false;
        }
        create_file.close();
        ZBACKUP_LOG_DEBUG("File created successfully: {}", pathname_);
        return true;
    }

    // 创建目录（递归创建）
    bool FileUtil::create_directory()
    {
        if (exists())
            return true;
        bool ret = fs::create_directories(pathname_);
        if (ret == false)
        {
            ZBACKUP_LOG_ERROR("Failed to create directory: {}", pathname_);
        }
        else
        {
            ZBACKUP_LOG_DEBUG("Directory created successfully: {}", pathname_);
        }
        return ret;
    }

    // 浏览目录
    void FileUtil::scan_directory(std::vector<std::string> *arry)
    {
        try
        {
            for (auto &p : fs::directory_iterator(pathname_))
            {
                if (fs::is_directory(p))
                    continue;
                std::string dir = fs::path(p).relative_path().string();
                arry->push_back(dir);
            }
            ZBACKUP_LOG_DEBUG("Directory scanned: {} files found in {}", arry->size(), pathname_);
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("Failed to scan directory [{}]: {}", pathname_, e.what());
        }
    }

    // JSON序列化：将JSON对象转换为字符串
    bool JsonUtil::serialize(const nlohmann::json &root, std::string *str)
    {
        try
        {
            *str = root.dump();
            return true;
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("JSON serialize failed: {}", e.what());
            return false;
        }
    }

    // JSON反序列化：将字符串转换为JSON对象
    bool JsonUtil::deserialize(nlohmann::json *root, const std::string &str)
    {
        try
        {
            *root = nlohmann::json::parse(str);
            return true;
        }
        catch (const std::exception &e)
        {
            ZBACKUP_LOG_ERROR("JSON deserialize failed: {}", e.what());
            return false;
        }
    }

    void InitMysqlPool()
    {
        auto &container = core::ServiceContainer::get_instance();
        auto config = container.resolve<interfaces::IConfigManager>();
        
        if (!config) {
            ZBACKUP_LOG_FATAL("ConfigManager not available for MySQL pool initialization");
            throw std::runtime_error("ConfigManager not found in service container");
        }

        auto &pool = zhttp::zdb::MysqlConnectionPool::get_instance();
        pool.init(
            config->get_string("mysql_host", "localhost"),
            config->get_string("mysql_user", "root"),
            config->get_string("mysql_password", ""),
            config->get_string("mysql_db", "zbackup"),
            config->get_int("mysql_pool_size", 10));
        ZBACKUP_LOG_INFO("MySQL connection pool init success: {}@{}:{}/{} pool_size={}",
                         config->get_string("mysql_user", "root"),
                         config->get_string("mysql_host", "localhost"),
                         config->get_int("mysql_port", 3306),
                         config->get_string("mysql_db", "zbackup"),
                         config->get_int("mysql_pool_size", 10));
    }

    void InitRedisPool()
    {
        auto &container = core::ServiceContainer::get_instance();
        auto config = container.resolve<interfaces::IConfigManager>();
        
        if (!config) {
            ZBACKUP_LOG_FATAL("ConfigManager not available for Redis pool initialization");
            throw std::runtime_error("ConfigManager not found in service container");
        }

        auto &pool = zhttp::zdb::RedisConnectionPool::get_instance();
        pool.init(
            config->get_string("redis_host", "127.0.0.1"),
            config->get_int("redis_port", 6379),
            config->get_string("redis_password", ""),
            config->get_int("redis_db", 0),
            config->get_int("redis_pool_size", 10),
            config->get_int("redis_timeout_ms", 5000));
        ZBACKUP_LOG_INFO("Redis connection pool init success: {}:{} db={} pool_size={} timeout_ms={}",
                         config->get_string("redis_host", "127.0.0.1"),
                         config->get_int("redis_port", 6379),
                         config->get_int("redis_db", 0),
                         config->get_int("redis_pool_size", 10),
                         config->get_int("redis_timeout_ms", 5000));
    }

    std::string time_to_str(time_t timestamp)
    {
        struct tm *time_info = localtime(&timestamp);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time_info);
        return std::string(buffer);
    }
} // namespace zbackup
