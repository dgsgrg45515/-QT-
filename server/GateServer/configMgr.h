#pragma once
#include "const.h"
struct SectionInfo
{
	SectionInfo() {}
	SectionInfo(const SectionInfo&src) {
		_section_datas = src._section_datas;
	}
	SectionInfo& operator=(const SectionInfo&src) {
		if (&src==this)
		{
			return *this;
		}
		this->_section_datas = src._section_datas;
		return *this; 
	}
	~SectionInfo() { _section_datas.clear(); }
	std::map<std::string, std::string> _section_datas;
	std::string operator[](const std::string& key) const {
		if (_section_datas.find(key)==_section_datas.end())
		{
			return "";
		}
		return _section_datas.at(key);
	}
};
class configMgr
{
public:
	~configMgr() {
		_config_map.clear();
	}
	SectionInfo operator[](const std::string& section) const {
		if (_config_map.find(section)==_config_map.end())
		{
			return SectionInfo();
		}
		return _config_map.at(section);
	}
	static configMgr& Inst() {
		static configMgr cfg_mgr;
		return cfg_mgr;
	}
	configMgr(const configMgr&) = delete;
	configMgr& operator=(const configMgr&) = delete;
	
private:
	configMgr();
	std :: map<std::string, SectionInfo> _config_map;
};

