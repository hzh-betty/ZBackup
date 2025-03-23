#pragma once
#include "logger.hpp"
#include "data_manage.hpp"
#include "httplib.h"
#include <memory>

namespace zbackup
{
#define SERVER_ADDR "139.155.157.93"
#define SERVER_PORT 8888
    class Client
    {
    public:
        Client(const std::string &back_dir, const std::string &back_file) : 
        backDir_(back_dir), data_(std::make_unique<DataManager>(back_file))
        {
        }
        std::string getFileIdentifier(const std::string &filename)
        {
            // a.txt-fsize-mtime
            FileUtil fu(filename);
            std::stringstream ss;
            ss << fu.getName() << "-" << fu.getSize() << "-" << fu.getLastMTime();
            logger->info("get file[{}] identifier success", filename);
            return ss.str();
        }

        bool upload(const std::string &filename)
        {
            // 1. 获取文件数据
            FileUtil fu(filename);
            std::string body;
            if (fu.getContent(&body) == false)
            {
                logger->error("upload file[{}] get content failed", filename);
                return false;
            }

            // 2. 搭建http客户端上传文件数据
            httplib::Client client(SERVER_ADDR, SERVER_PORT);
            httplib::MultipartFormData item;
            item.content = body;
            item.filename = fu.getName();
            item.name = "file";
            item.content_type = "application/octet-stream";
            httplib::MultipartFormDataItems items;
            items.push_back(item);

            auto res = client.Post("/upload", items);
            if (!res || res->status != 200)
            {
                return false;
            }
            return true;
        }
        bool checkUpload(const std::string &filename)
        {
            // 1. 判断文件标识符是否改变
            std::string id;
            if (data_->getOneByKey(filename, &id) != false)
            {
                std::string new_id = getFileIdentifier(filename);
                if (new_id == id)
                {
                    return false; // 不需要被上传-上次上传后没有被修改过
                }
            }

            // 2. 3秒钟之内刚修改过--认为文件还在修改中
            FileUtil fu(filename);
            if (time(NULL) - fu.getLastMTime() < 3)
            {
                return false;
            }

            return true;
        }
        bool run()
        {
            while (true)
            {
                // 1. 遍历获取指定文件夹中所有文件
                FileUtil fu(backDir_);
                if(fu.exists() == false)
                {
                    fu.createDirectory();
                    continue;
                }
                std::vector<std::string> arry;
                fu.scanDirectory(&arry);
                // 2. 逐个判断文件是否需要上传
                for (auto &a : arry)
                {
                    if (checkUpload(a) == false)
                    {
                        continue;
                    }
                    // 3. 如果需要上传则上传文件
                    if (upload(a) == true)
                    {
                        data_->insert(a, getFileIdentifier(a)); // 新增文件备份信息
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }

    private:
        std::string backDir_;
        std::unique_ptr<DataManager> data_;
    };
}
