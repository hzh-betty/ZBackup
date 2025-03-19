#include "../util.hpp"
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

int main(int argc, char *argv[])
{
    zbackup::Log::Init();
    // test1(argv[1]);
    // test2(argv[1]);
    test3(argv[1]);
    // test4(argv[1]);
    return 0;
}