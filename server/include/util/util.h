#pragma once
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fstream>
#include <filesystem>
#include <sys/stat.h>
#include <nlohmann/json.hpp>
#include "../log/logger.h"

namespace zbackup
{
    namespace fs = std::filesystem;

    // 文件操作工具类
    class FileUtil
    {
    public:
        explicit FileUtil(std::string pathname);

        // 文件删除操作
        bool remove_file(); // 删除单个文件
        bool remove_directory(); // 删除目录
        bool force_remove(); // 强制删除文件或目录
        bool exists() const; // 检查文件是否存在
        bool create_file(); // 创建空文件
        bool create_directory(); // 创建目录

        // 文件信息获取
        [[nodiscard]] int64_t get_size() const; // 获取文件大小
        [[nodiscard]] time_t get_last_mtime() const; // 获取最后修改时间
        [[nodiscard]] time_t get_last_atime() const; // 获取最后访问时间
        [[nodiscard]] std::string get_name() const; // 获取文件名

        // 文件内容操作
        bool get_pos_len(std::string *body, size_t pos, size_t len); // 读取指定位置和长度
        bool get_content(std::string *body); // 读取整个文件内容
        bool set_content(const std::string &body); // 写入文件内容

        // 目录操作
        void scan_directory(std::vector<std::string> *arry); // 扫描目录中的文件

    private:
        std::string pathname_; // 文件路径
    };

    // JSON序列化/反序列化工具类
    class JsonUtil
    {
    public:
        static bool serialize(const nlohmann::json &root, std::string *str); // 序列化
        static bool deserialize(nlohmann::json *root, const std::string &str); // 反序列化
    };

    // 初始化MySQL连接池
    void InitMysqlPool();
    // 初始化Redis连接池
    void InitRedisPool();
}
