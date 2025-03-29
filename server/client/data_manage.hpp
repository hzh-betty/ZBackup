#pragma once
#include <unordered_map>
#include <sstream>
#include "util.hpp"

namespace zbackup
{
	class DataManager
	{
	public:
		DataManager(const std::string &backup_file) : backupFile_(backup_file)
		{
			if (initLoad() == false)
			{
				logger->warn("initLoad backup file[{}] failed", backupFile_);
			}
		}
		bool storage()
		{
			// 1. 获取所有的备份信息
			std::stringstream ss;
			auto it = tables_.begin();
			for (; it != tables_.end(); ++it)
			{
				// 2. 将所有信息进行指定持久化格式的组织
				ss << it->first << " " << it->second << "\n";
			}

			// 3. 持久化存储
			FileUtil fu(backupFile_);
			fu.setContent(ss.str());
			return true;
		}
		int splitStr(const std::string &str, const std::string &sep, std::vector<std::string> *arry)
		{
			int count = 0;
			size_t pos = 0, idx = 0;
			while (true)
			{
				pos = str.find(sep, idx);
				if (pos == std::string::npos)
				{
					break;
				}
				if (pos == idx)
				{
					idx = pos + sep.size();
					continue;
				}
				std::string tmp = str.substr(idx, pos - idx);
				arry->push_back(tmp);
				count++;
				idx = pos + sep.size();
			}

			if (idx < str.size())
			{
				arry->push_back(str.substr(idx));
				count++;
			}
			
			return count;
		}

		bool initLoad()
		{
			// 1. 从文件中读取所有数据
			FileUtil fu(backupFile_);
			std::string body;
			if (fu.exists() == false)
			{
				if (!fu.createFile())
				{
					logger->fatal("backup file[{}] cannot be created", backupFile_);
					return false;
				}
				logger->debug("backup file[{}] be created", backupFile_);
				return true;
			}
			if (fu.getContent(&body) == false)
			{
				logger->error("initLoad file[{}] get content failed", backupFile_);
				return false;
			}

			// 2. 进行数据解析，添加到表中
			std::vector<std::string> arry;
			splitStr(body, "\n", &arry);
			for (auto &a : arry)
			{
				// b.txt b.txt-34657-345636
				std::vector<std::string> tmp;
				splitStr(a, " ", &tmp);
				if (tmp.size() != 2)
				{
					logger->warn("split Str != 2", backupFile_);
					continue;
				}
				tables_[tmp[0]] = tmp[1];
			}
			return true;
		}

		bool insert(const std::string &key, const std::string &val)
		{
			tables_[key] = val;
			storage();
			return true;
		}

		bool update(const std::string &key, const std::string &val)
		{
			tables_[key] = val;
			storage();
			return true;
		}
		bool getOneByKey(const std::string &key, std::string *val)
		{
			auto it = tables_.find(key);
			if (it == tables_.end())
			{
				return false;
			}
			*val = it->second;
			return true;
		}

	private:
		std::string backupFile_; 
		std::unordered_map<std::string, std::string> tables_;
	};
}
