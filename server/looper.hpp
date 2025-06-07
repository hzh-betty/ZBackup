#pragma once
#include "compress.hpp"
#include "data_manage.hpp"
#include "threadpool.hpp"
#include "../ZHttpServer/include/http/http_request.h"
#include "../ZHttpServer/include/http/http_response.h"
#include <regex>
namespace zbackup
{
    class BackupLooper
    {
    public:
        using ptr = std::shared_ptr<BackupLooper>;
        BackupLooper(Compress::ptr comp, Storage::ptr storage)
            : stop_(false),
              dataManager_(std::make_shared<DataManager>(storage)), comp_(comp)
        {
            ThreadPool::getInstance()->submitTask([this]()
                                                  { hotMonitor(); });
        }

        void download(const zhttp::HttpRequest &req, zhttp::HttpResponse &rsp)
        {
            // 1. 根据资源路径，获取文件备份信息
            BackupInfo info;
            if (dataManager_->getOneByURL(req.get_path(), &info) == false)
            {
                logger->warn("download get one by URL failed");
                rsp.set_status_code(zhttp::HttpResponse::StatusCode::NotFound);
                rsp.set_status_message("Not Found");
                rsp.set_body("Not Found");
                return;
            }
            logger->debug("download get one by URL success");

            // 2. 判断文件是否被压缩，如果被压缩，要先解压缩,
            if (info.packFlag_ == true)
            {
                if (comp_->unCompress(info.realPath_, info.packPath_) == false)
                {
                    logger->warn("download unCompress failed");
                    rsp.set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                    rsp.set_status_message("Internal Server Error");
                    rsp.set_body("Uncompress failed");
                    return;
                }
                logger->debug("download unCompress success");

                // 3. 删除压缩包，修改备份信息（已经没有被压缩）
                FileUtil fu(info.packPath_);
                if (fu.removeFile() == false)
                {
                    logger->warn("download remove file[{}] failed", info.packPath_);
                    rsp.set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                    rsp.set_status_message("Internal Server Error");
                    rsp.set_body("Remove pack file failed");
                    return;
                }
                info.packFlag_ = false;
                if (dataManager_->update(info) == false)
                {
                    logger->warn("download update file[{}] failed", info.packPath_);
                    rsp.set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                    rsp.set_status_message("Internal Server Error");
                    rsp.set_body("Update info failed");
                    return;
                }
                if (dataManager_->persistence() == false)
                {
                    logger->warn("download persistence file[{}] failed", info.packPath_);
                    rsp.set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                    rsp.set_status_message("Internal Server Error");
                    rsp.set_body("Persistence failed");
                    return;
                }
            }

            // 4. 判断是否需要断点续传
            // 这里暂不实现断点续传，直接返回全部内容
            FileUtil fu(info.realPath_);
            std::string fileContent;
            if (!fu.getContent(&fileContent))
            {
                rsp.set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                rsp.set_status_message("Internal Server Error");
                rsp.set_body("Read file failed");
                return;
            }
            rsp.set_status_code(zhttp::HttpResponse::StatusCode::OK);
            rsp.set_status_message("OK");
            rsp.set_content_type("application/octet-stream");
            rsp.set_body(fileContent);
            rsp.set_header("Accept-Ranges", "bytes");
            rsp.set_header("ETag", GetETag(info));
        }

        void listshow(const zhttp::HttpRequest &req, zhttp::HttpResponse &rsp)
        {
            std::vector<BackupInfo> arry;
            dataManager_->getAll(&arry);
            logger->debug("listshow get all backiup info success");

            std::stringstream ss;
            ss << "<html><head><title>Download</title></head>";
            ss << "<body><h1>Download</h1><table>";
            for (auto &a : arry)
            {
                ss << "<tr>";
                std::string filename = FileUtil(a.realPath_).getName();
                ss << "<td><a href='" << a.url_ << "'>" << filename << "</a></td>";
                ss << "<td align='right'>" << TimetoStr(a.mtime_) << "</td>";
                ss << "<td align='right'>" << a.fsize_ / 1024 << "k</td>";
                ss << "</tr>";
            }
            ss << "</table></body></html>";
            rsp.set_status_code(zhttp::HttpResponse::StatusCode::OK);
            rsp.set_status_message("OK");
            rsp.set_content_type("text/html; charset=UTF-8");
            rsp.set_body(ss.str());
        }

        void upload(const zhttp::HttpRequest &req, zhttp::HttpResponse &rsp)
        {
            std::string filename;
            std::string filecontent;

            // 1. 检查是否为 multipart/form-data
            std::string content_type = req.get_header("Content-Type");
            logger->info("upload: Content-Type = [{}]", content_type);
            
            if (content_type.find("multipart/form-data") != std::string::npos)
            {
                logger->debug("upload: detected multipart/form-data request");
                
                // 提取 boundary
                std::smatch match;
                std::regex boundary_re("boundary=([^\r\n;]+)");
                if (!std::regex_search(content_type, match, boundary_re) || match.size() < 2)
                {
                    logger->warn("multipart: boundary not found in Content-Type [{}]", content_type);
                    rsp.set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
                    rsp.set_status_message("Bad Request");
                    rsp.set_body("Missing boundary in Content-Type");
                    return;
                }
                std::string boundary = "--" + match[1].str();
                logger->debug("upload: extracted boundary = [{}]", boundary);

                // 解析multipart内容
                const std::string &body = req.get_content();
                logger->debug("upload: body size = {} bytes", body.size());
                
                size_t pos = 0;
                int part_count = 0;
                while ((pos = body.find(boundary, pos)) != std::string::npos)
                {
                    part_count++;
                    logger->debug("upload: processing part {} at position {}", part_count, pos);
                    
                    size_t part_start = pos + boundary.size();
                    if (body.substr(part_start, 2) == "--") 
                    {
                        logger->debug("upload: found end boundary");
                        break; // 结束
                    }

                    size_t header_end = body.find("\r\n\r\n", part_start);
                    if (header_end == std::string::npos) 
                    {
                        logger->warn("upload: no header end found for part {}", part_count);
                        break;
                    }

                    // 提取头部信息
                    std::string headers = body.substr(part_start, header_end - part_start);
                    logger->debug("upload: part {} headers = [{}]", part_count, headers);
                    
                    size_t content_start = header_end + 4;
                    size_t next_boundary = body.find(boundary, content_start);
                    if (next_boundary == std::string::npos) 
                    {
                        logger->warn("upload: no next boundary found for part {}", part_count);
                        break;
                    }
                    
                    size_t content_len = next_boundary - content_start;
                    std::string part_content = body.substr(content_start, content_len);
                    logger->debug("upload: part {} content size = {} bytes", part_count, content_len);

                    // 查找文件字段
                    std::smatch fname_match;
                    std::regex fname_re("name=\"file\".*filename=\"([^\"]*)\"");
                    if (std::regex_search(headers, fname_match, fname_re))
                    {
                        if (fname_match.size() > 1)
                        {
                            filename = fname_match[1].str();
                            logger->debug("upload: found file field with filename = [{}]", filename);
                        }
                        // 去除结尾换行
                        while (!part_content.empty() && (part_content.back() == '\r' || part_content.back() == '\n'))
                            part_content.pop_back();
                        filecontent = part_content;
                        logger->debug("upload: extracted file content, size = {} bytes", filecontent.size());
                        break;
                    }
                    else
                    {
                        logger->debug("upload: part {} is not a file field", part_count);
                    }
                    pos = next_boundary;
                }
                
                if (filename.empty() || filecontent.empty())
                {
                    logger->warn("multipart: file field not found or empty (processed {} parts)", part_count);
                    rsp.set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
                    rsp.set_status_message("Bad Request");
                    rsp.set_body("No file uploaded");
                    return;
                }
                logger->info("upload: multipart parsing successful");
            }
            else
            {
                logger->info("upload: using X-Filename header method");
                // 兼容原有X-Filename方式
                filename = req.get_header("X-Filename");
                filecontent = req.get_content();
                if (filename.empty())
                {
                    logger->warn("There is no X-Filename header");
                    rsp.set_status_code(zhttp::HttpResponse::StatusCode::BadRequest);
                    rsp.set_status_message("Bad Request");
                    rsp.set_body("Missing X-Filename header");
                    return;
                }
                logger->debug("upload: X-Filename = [{}], content size = {} bytes", filename, filecontent.size());
            }

            logger->debug("upload: filename={}, size={}", filename, filecontent.size());
            std::string backDir = Config::getInstance().getBackDir();
            std::string realPath = backDir + FileUtil(filename).getName();
            FileUtil fu(realPath);

            if (fu.setContent(filecontent) == false)
            {
                logger->warn("upload set file contents failed");
                rsp.set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                rsp.set_status_message("Internal Server Error");
                rsp.set_body("Write file failed");
                return;
            }
            logger->debug("upload set file contents success");

            BackupInfo info;
            if (info.newBackupInfo(realPath) == false)
            {
                logger->warn("upload new backupInfo failed");
                rsp.set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                rsp.set_status_message("Internal Server Error");
                rsp.set_body("BackupInfo failed");
                return;
            }
            logger->debug("upload new backupInfo success");
            if (dataManager_->insert(info) == false)
            {
                logger->warn("upload insert backupInfo failed");
                rsp.set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                rsp.set_status_message("Internal Server Error");
                rsp.set_body("Insert info failed");
                return;
            }
            logger->debug("upload insert backupInfo success");
            if (dataManager_->persistence() == false)
            {
                logger->warn("upload persistence file[{}] failed", info.packPath_);
                rsp.set_status_code(zhttp::HttpResponse::StatusCode::InternalServerError);
                rsp.set_status_message("Internal Server Error");
                rsp.set_body("Persistence failed");
                return;
            }
            rsp.set_status_code(zhttp::HttpResponse::StatusCode::OK);
            rsp.set_status_message("OK");
            rsp.set_body("The file was uploaded successfully");
        }

        ~BackupLooper()
        {
            stop_ = true;
        }

    private:
        void hotMonitor()
        {
            Config &config = Config::getInstance();
            std::string backDir = config.getBackDir();
            int hotTime = config.getHotTime();
            while (!stop_)
            {
                // 1. 遍历备份目录，获取所有文件名
                FileUtil fu(backDir);
                std::vector<std::string> arry;
                fu.scanDirectory(&arry);
                // logger->debug("hotMonitor scan directory succsess");

                // 2. 判断是否为热点文件
                for (auto &str : arry)
                {
                    if (hotJudge(str, hotTime) == false)
                        continue;
                    ThreadPool::getInstance()->submitTask([&]()
                                                          { this->dealTask(str); });
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }

        void dealTask(const std::string &str)
        {
            // 3. 获取文件信息
            BackupInfo bi;
            if (dataManager_->getOneByRealPath(str, &bi) == false)
            {
                logger->warn("there is a file[{}] present, but there is no backup information", str);
                bi.newBackupInfo(str);
            }

            // 4. 对热点文件进行压缩
            comp_->compress(str, bi.packPath_);
            FileUtil tmp(str);

            // 5. 删除源文件
            tmp.removeFile();
            bi.packFlag_ = true;
            if (dataManager_->update(bi) == false)
            {
                logger->warn("hotMonitor update file[{}] failed", bi.packPath_);
                return;
            }
            if (dataManager_->persistence() == false)
            {
                logger->warn("upload persistence file[{}] failed", bi.packPath_);
            }
        }
        // 非热点文件-返回真；热点文件-返回假
        bool hotJudge(const std::string &filename, int hotTime)
        {
            FileUtil fu(filename);
            time_t lastAtime = fu.getLastATime();
            time_t curTime = time(nullptr);
            if (curTime - lastAtime > hotTime)
            {
                logger->debug("file[{}] is hot", filename);
                return true;
            }
            logger->debug("file[{}] is not hot", filename);
            return false;
        }

        static std::string TimetoStr(time_t t)
        {
            std::string tmp = std::ctime(&t);
            logger->debug("time to str {}", tmp);
            return tmp;
        }

        static std::string GetETag(const BackupInfo &info)
        {
            // etg :  filename-fsize-mtime
            FileUtil fu(info.realPath_);
            std::string etag = fu.getName();
            etag += "-";
            etag += std::to_string(info.fsize_);
            etag += "-";
            etag += std::to_string(info.mtime_);
            logger->debug("create ETag {}", etag);
            return etag;
        }

    private:
        std::atomic<bool> stop_;
        DataManager::ptr dataManager_;
        Compress::ptr comp_;
    };
};
