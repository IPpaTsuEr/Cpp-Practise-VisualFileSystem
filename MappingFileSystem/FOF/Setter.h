#pragma once
#include "Base.h"
#include<map>
#include<type_traits>
class Setter {
	std::map<std::string, std::string> sets;
public:
	template<typename T>
	T Convert(std::string key,bool * result) {
		std::stringstream s;
		auto itr = sets.find(key);
		if (itr != sets.end()) {
			s << itr->second;
			*result = true;
		}
		else {
			s << "";
			*result = false;
		}
		T t;
		if (std::is_same<T, bool>::value) {
			s >> std::boolalpha >> t;
		}
		else s >> t;
		return t;
	}

	bool Have(std::string key) {
		auto e = sets.find(key);
		if (e != sets.end())return true;
		return false;
	}

	void Put(std::string key, std::string value) {
		auto mp = std::make_pair(key, value);
		sets.insert(mp);
		//sets.insert(std::make_pair("9","6"));
	}

	void Pop(std::string key) {
		sets.erase(key);
	}

	std::map<std::string, std::string>* GetSrc() {
		return &sets;
	}

	void ConvertSetting(Setting setting) {
		sets.insert(std::make_pair("BatchBlockSize", ConvertSize(setting.BatchBlockSize)));
		sets.insert(std::make_pair("HugeFileSize", ConvertSize(setting.HugeFileSize)));
		sets.insert(std::make_pair("SectionSize", ConvertSize(setting.SectionSize)));
		sets.insert(std::make_pair("Size", ConvertSize(setting.VMSize)));
		sets.insert(std::make_pair("Compressable",ConvertBool(setting.Compressable)));
	}
};