#pragma once
#include "const.h"
#include <fstream>  
#include <boost/property_tree/ptree.hpp>  
#include <boost/property_tree/ini_parser.hpp>  
#include <map>
#include <iostream>
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
	std::string GetValue(const std::string& key) {
		if (_section_datas.find(key)==_section_datas.end())
		{
			return "";
		}
		return _section_datas[key];
	}
};
class ConfigMgr
{
public:
	~ConfigMgr() {
		_config_map.clear();
	}
	SectionInfo operator[](const std::string& section) {
		if (_config_map.find(section) == _config_map.end()) {
			return SectionInfo();
		}
		return _config_map[section];
	}


	ConfigMgr& operator=(const ConfigMgr& src) {
		if (&src == this) {
			return *this;
		}

		this->_config_map = src._config_map;
	};

	ConfigMgr(const ConfigMgr& src) {
		this->_config_map = src._config_map;
	}

	static ConfigMgr& Inst() {
		static ConfigMgr cfg_mgr;
		return cfg_mgr;
	}

	std::string GetValue(const std::string& section, const std::string& key);
private:
	ConfigMgr();
	std::map<std::string, SectionInfo> _config_map;
};

