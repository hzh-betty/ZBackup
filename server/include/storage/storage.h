#pragma once
#include "../util/util.h"
#include "../../../ZHttpServer/include/db_pool/mysql_pool.h"

namespace zbackup
{
    // 备份文件信息结构体
    struct BackupInfo
    {
        bool pack_flag_; // 是否已压缩
        size_t fsize_; // 文件大小
        time_t mtime_; // 修改时间
        time_t atime_; // 访问时间
        std::string real_path_; // 真实文件路径
        std::string pack_path_; // 压缩文件路径
        std::string url_; // 下载URL

        bool new_backup_info(const std::string &real_path); // 根据文件路径创建备份信息
    };

    // 存储接口基类
    class Storage
    {
    public:
        using ptr = std::shared_ptr<Storage>;

        virtual ~Storage() = default;

        // 基本CRUD操作
        virtual bool insert(const BackupInfo &info) = 0;

        virtual bool update(const BackupInfo &info) = 0;

        virtual bool get_one_by_url(const std::string &url, BackupInfo *info) = 0;

        virtual bool get_one_by_real_path(const std::string &real_path, BackupInfo *info) = 0;

        virtual void get_all(std::vector<BackupInfo> *arry) = 0;

        virtual bool delete_one(const BackupInfo &info) = 0;

        virtual bool delete_by_url(const std::string &url) = 0;

        virtual bool delete_by_real_path(const std::string &real_path) = 0;

        virtual bool init_load() = 0; // 初始化加载
        virtual bool persistence() = 0; // 数据持久化
    };

    // 文件存储实现类
    class FileStorage : public Storage
    {
    public:
        FileStorage();

        // 实现存储接口
        bool insert(const BackupInfo &info) override;

        bool update(const BackupInfo &info) override;

        bool get_one_by_url(const std::string &url, BackupInfo *info) override;

        bool get_one_by_real_path(const std::string &real_path, BackupInfo *info) override;

        void get_all(std::vector<BackupInfo> *arry) override;

        bool delete_one(const BackupInfo &info) override;

        bool delete_by_url(const std::string &url) override;

        bool delete_by_real_path(const std::string &real_path) override;

    private:
        bool init_load() override; // 从文件加载数据
        bool persistence() override; // 数据持久化到文件

    protected:
        std::unordered_map<std::string, BackupInfo> tables_; // 内存存储表
        std::string backup_file_; // 备份文件路径
    };

    // 数据库存储实现类
    class DatabaseStorage final : public Storage
    {
    public:
        DatabaseStorage();

        bool insert(const BackupInfo &info) override;
        bool update(const BackupInfo &info) override;
        bool get_one_by_url(const std::string &url, BackupInfo *info) override;
        bool get_one_by_real_path(const std::string &real_path, BackupInfo *info) override;
        void get_all(std::vector<BackupInfo> *arry) override;
        bool delete_one(const BackupInfo &info) override;
        bool delete_by_url(const std::string &url) override;
        bool delete_by_real_path(const std::string &real_path) override;

    private:
        bool init_load() override;
        bool persistence() override;
    private:
        zhttp::zdb::MysqlConnectionPool* pool_ = &zhttp::zdb::MysqlConnectionPool::get_instance();
    };
}
