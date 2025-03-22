#include "util.hpp"
#include "data_manage.hpp"
#include "client.hpp"

#define BACKUP_FILE "./backup.dat"
#define BACKUP_DIR "./backup/"
int main()
{
    zbackup::Log::Init(zlog::LogLevel::value::WARNING);
	zbackup::Client backup(BACKUP_DIR, BACKUP_FILE);
	backup.run();
	return 0;
}