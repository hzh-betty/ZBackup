#include "../util.hpp"
#include "../config.hpp"
#include "../data_manage.hpp"
#include "../hot.hpp"
// 测试util.hpp中文件的信息获取功能
void test1(const std::string &pathname)
{
    zbackup::FileUtil fh(pathname);
    std::cout << fh.getSize() << std::endl;
    std::cout << fh.getLastATime() << std::endl;
    std::cout << fh.getLastMTime() << std::endl;
    std::cout << fh.getName() << std::endl;
}

// 测试util.hpp中读写文件的功能
void test2(const std::string &pathname)
{
    zbackup::FileUtil fh(pathname);
    std::string body;
    fh.getContent(&body);

    zbackup::FileUtil ft("./hello.txt");
    ft.setContent(body);
}

// 测试util.hpp中压缩文件的功能
void test3(const std::string &pathname)
{
    zbackup::FileUtil fh(pathname);
    std::string packed = pathname + ".snappy";
    fh.compress(packed);

    zbackup::FileUtil fu(packed);
    fu.unCompress(pathname + "-1");
}

// 测试util.hpp中创建与浏览目录的功能
void test4(const std::string &pathname)
{
    zbackup::FileUtil fu(pathname);
    fu.createDirectory();
    std::vector<std::string> arry;
    fu.scanDirectory(&arry);
    for (auto &arr : arry)
    {
        std::cout << arr << std::endl;
    }
}

// 测试util.hpp中序列化与反序列化
void test5()
{
    std::string name = "betty";
    int age = 20;
    float score[] = {89, 78, 82.5};
    Json::Value root;
    root["姓名"] = name;
    root["年龄"] = age;
    root["成绩"].append(score[0]);
    root["成绩"].append(score[1]);
    root["成绩"].append(score[2]);
    std::string tmp;
    zbackup::JsonUtil::Serialize(root, &tmp);
    std::cout << tmp << std::endl;

    Json::Value val;
    zbackup::JsonUtil::Deserialize(&val, tmp);
    std::cout << val["姓名"].asString() << std::endl;
    std::cout << val["年龄"].asInt() << std::endl;
    for (int i = 0; i < val["成绩"].size(); i++)
    {
        std::cout << val["成绩"][i].asFloat() << std::endl;
    }
}

// 测试config.hpp中获取配置文件信息
void test6()
{
    std::cout << zbackup::Config::getInstance().getHotTime() << std::endl;
    std::cout << zbackup::Config::getInstance().getPort() << std::endl;
    std::cout << zbackup::Config::getInstance().getIp() << std::endl;
    std::cout << zbackup::Config::getInstance().getDownloadPrefix() << std::endl;
    std::cout << zbackup::Config::getInstance().getPackFilePrefix() << std::endl;
    std::cout << zbackup::Config::getInstance().getBackDir() << std::endl;
    std::cout << zbackup::Config::getInstance().getPackDir() << std::endl;
    std::cout << zbackup::Config::getInstance().getBackupFile() << std::endl;
}

// 测试文件数据的管理
void test7(const std::string &pathname)
{
    zbackup::BackupInfo info;
    info.newBackupInfo(pathname);
    zbackup::DataManager datas;
    datas.insert(info);
    zbackup::BackupInfo tmp;
    datas.getOneByURL("/download/test.cc", &tmp);
    std::cout << tmp.mtime_ << std::endl;
    std::cout << tmp.atime_ << std::endl;
    std::cout << tmp.fsize_ << std::endl;
    std::cout << tmp.packFlag_ << std::endl;
    std::cout << tmp.realPath_ << std::endl;
    std::cout << tmp.packPath_ << std::endl;
    std::cout << tmp.url_ << std::endl;
    std::cout << "--------------test getOneByURL-----------" << std::endl;
    info.packFlag_ = true;
    datas.update(info);
    std::vector<zbackup::BackupInfo> arry;
    datas.getAll(&arry);
    for (auto &str : arry)
    {
        std::cout << str.mtime_ << std::endl;
        std::cout << str.atime_ << std::endl;
        std::cout << str.fsize_ << std::endl;
        std::cout << str.packFlag_ << std::endl;
        std::cout << str.realPath_ << std::endl;
        std::cout << str.packPath_ << std::endl;
        std::cout << str.url_ << std::endl
                  << std::endl;
    }
    std::cout << "--------------test getAll-----------" << std::endl;

    datas.getOneByRealPath(pathname, &tmp);
    std::cout << tmp.mtime_ << std::endl;
    std::cout << tmp.atime_ << std::endl;
    std::cout << tmp.fsize_ << std::endl;
    std::cout << tmp.packFlag_ << std::endl;
    std::cout << tmp.realPath_ << std::endl;
    std::cout << tmp.packPath_ << std::endl;
    std::cout << tmp.url_ << std::endl;
    std::cout << "--------------test getOneByRealPath-----------" << std::endl;
}

// 测试文件数据初始化读取配置文件的管理
void test8()
{
    zbackup::DataManager datas;
    zbackup::BackupInfo tmp;
    datas.getOneByURL("/download/test.cc", &tmp);
    std::cout << tmp.mtime_ << std::endl;
    std::cout << tmp.atime_ << std::endl;
    std::cout << tmp.fsize_ << std::endl;
    std::cout << tmp.packFlag_ << std::endl;
    std::cout << tmp.realPath_ << std::endl;
    std::cout << tmp.packPath_ << std::endl;
    std::cout << tmp.url_ << std::endl;
}

// 对热点文件进行判断
zbackup::DataManager *data_ = nullptr;
void test9()
{
    data_ = new zbackup::DataManager();
    zbackup::HotManager hot;
    hot.runModule();
}
int main(int argc, char *argv[])
{
    zbackup::Log::Init();
    // test1(argv[1]);
    //  test2(argv[1]);
    //  test3(argv[1]);
    //  test4(argv[1]);
    //  test5();
    // test6();
    // test7(argv[1]);
    // test8();
    test9();
    return 0;
}