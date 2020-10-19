// 2006.03.20
// Configuration File Manager
// Supporting real time configuration
// 2006.04.02 - "yes", "true" changes "1", "no", "false" changes "0"
// 2006.04.02 - name changes to lower case and trim all spaces(' ','\t','\v','\r','\n')
// 2006.05.08 - add method isExist()
// 2006.05.15 - do not change original value "yes", "true", "no", "false" to 0 or 1
// 2012.12.21 - using boost shared_mutex for multi-threading use

#include <iostream>
#include <fstream>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include "config.h"


using namespace std;

const string separators = "|";					  // section|key
const int CHECK_INTERVAL = 30;					  // seconds


Config::Config() : checkInterval_(CHECK_INTERVAL), check_sharp_only_1st_col_(false), file_(0)
{
}


Config::Config(const char *fName, bool check_sharp_only_1st_col) 
	: checkInterval_(CHECK_INTERVAL), check_sharp_only_1st_col_(check_sharp_only_1st_col)
{
	file_ = strdup(fName);
	readConfig();
}


Config::~Config()
{
	if (file_)
		free(file_);

	{
#ifdef _REENTRANT
		slock.lock();
#endif
		if (!m_cfg_.empty())
			m_cfg_.clear();
#ifdef _REENTRANT
		slock.unlock();
#endif
	}
}


bool Config::init(const char *fName, bool check_sharp_only_1st_col)
{
	if (file_)
	{
		free(file_);
	}
	
	file_ = strdup(fName);
	check_sharp_only_1st_col_ = check_sharp_only_1st_col;

	return readConfig();
}


void Config::ltrim(string &text)
{
	if (text.empty())
	{	
		return;
	}
	unsigned i = 0;

	for (i = 0; i < text.size(); i++)
	{
		if (!isspace(text.at(i)))
			break;
	}

	if (i > 0)
	{
		text = text.substr(i);
	}
}


void Config::rtrim(string &text)
{
	int dec = 0;

	for (int i = text.length() - 1; i >= 0; --i)
	{
		if (isspace(text.at(i)))
		{	
			dec++;
		}	
		else
		{		
			break;
		}
	}
	if (dec > 0)
	{
		text = text.substr(0, text.length() - dec);
	}
}


void Config::trim_all_and_lower(string &s)
{
	int n;
#if 0
	while ((n = s.find_first_of(" \t\n\r\n")) >= 0)
#else
	while ((n = s.find_first_of(" _-\t\n\r\n")) >= 0)
#endif
			s.replace(n, 1, "");

	n = s.length();
	for (int i = 0; i < n; ++i)
	{
		s.at(i) = tolower(s.at(i));
	}
}


bool Config::readConfig(void)
{
	ifstream cfFile;
	string buf, key, new_key, value;
	string section;
	int stop;
	//const char* file_ ="/home/sioc/HD_Project/config/hldsagent.conf";
	cfFile.open(file_, ios::in);

	if (!cfFile.is_open())
	{   
		return false;
	}
	opened_tv_ = last_view_ = time(NULL);
	{
#ifdef _REENTRANT
		slock.lock();
#endif

		if (!m_cfg_.empty())
		{
			m_cfg_.clear();
		}

#ifdef _REENTRANT
		slock.unlock();
#endif
	}

	while (getline(cfFile, buf))
	{
		trim(buf);

		if (buf.empty())
		{	
			continue;
		}
		if (buf.at(0) == '#' or buf.at(0) == ';')
		{
			continue;
		}
		if (buf.at(0) == '[')
		{
			if ((stop = buf.find("]")) > 0)
			{
				section = buf.substr(1, stop - 1);
#if 0
				trim(section);
#else
				trim_all_and_lower(section);
#endif
			}
			continue;
		}

		if ((stop = buf.find("=")) > 0)
		{
			key = buf.substr(0, stop);
			value = buf.substr(stop + 1);
		}
		else
		{
			continue;
		}
#if 0
		rtrim(key);
#else
		trim_all_and_lower(key);
#endif
		if (!check_sharp_only_1st_col_)
		{
			if ((stop = value.find_first_of("#")))
			{
				value = value.substr(0, stop);
			}
		}

		trim(value);

#if 0
		if (!strcasecmp(value.data(), "yes") or !strcasecmp(value.data(), "true"))
			value = "1";
		else if (!strcasecmp(value.data(), "no") or !strcasecmp(value.data(), "false"))
			value = "0";
#endif

		new_key = section + separators + key;

		{
#ifdef _REENTRANT
			slock.lock();
#endif
			if (m_cfg_.count(new_key) != 0)
			{
				cerr << " - [Config] collision of key: [" << section << "]::" << key << endl;
				continue;
			}

			m_cfg_.insert(map<string, string>::value_type(new_key, value));
//			m_cfg_[new_key] = value;
#ifdef _REENTRANT
			slock.unlock();
#endif
		}
	}

	cfFile.close();
	return true;
}


struct my_config_debug {
	void operator() (const pair<std::string, std::string> &i) { int stop = i.first.find(separators); cout << "[" << i.first.substr(0, stop) << "]\t" << i.first.substr(stop + 1) << " -> " << i.second << endl; };
} debug_config;



void Config::debugPrint(void)
{
	{
#ifdef _REENTRANT
		slock.lock_shared();
#endif

		for_each(m_cfg_.begin(), m_cfg_.end(), debug_config);

#ifdef _REENTRANT
		slock.unlock_shared();
#endif
	}
}


bool Config::changeFile(const char *File)
{
	return init(File);
}


// this method must be private
bool Config::isFileModified(void)
{
	if ((time(NULL) - last_view_) <= checkInterval_)
	{
		return false;
	}
	last_view_ = time(NULL);
	struct stat buf;
	if (lstat(file_, &buf) < 0)
	{
		return false;
	}
	return (buf.st_mtime > opened_tv_);
}

void Config::doRealTimeConfig(void)
{
	if (isFileModified())
	{
		readConfig();
	}
}


bool Config::reload(void)
{
	return readConfig();
}


bool Config::isExist(const char *section, const char *key)
{
	string search = section + separators + key;
	trim_all_and_lower(search);
	CFG_Map::const_iterator itr;
	{
#ifdef _REENTRANT
		slock.lock_shared();
#endif

		itr = m_cfg_.find(search);

#ifdef _REENTRANT
		slock.unlock_shared();
#endif
	}

	return itr != m_cfg_.end();
}


bool Config::findVal(const string text, string &return_string)
{
	string s = text;
	trim_all_and_lower(s);
	CFG_Map::const_iterator itr;
	{
#ifdef _REENTRANT
		slock.lock_shared();
#endif

		itr = m_cfg_.find(s);
		if (itr != m_cfg_.end())
			return_string = (*itr).second;

#ifdef _REENTRANT
		slock.unlock_shared();
#endif
	}

	return itr != m_cfg_.end();
}


int Config::getiValue(const char *section, const char *key, const int default_val)
{
	string search = section + separators + key;
	string val;

	if  (!findVal(search, val))
	{	
		return default_val;
	}
	string Val;

	if (checkVal(section, val, Val))
	{
		if (!strncasecmp(Val.c_str(), "0x", 2))
		{
			return strtol(Val.c_str(), (char **)NULL, 16);
		}
		else
		{
			return strtol(Val.c_str(), (char **)NULL, 10);

		}	
	}

	if (!strncasecmp(val.c_str(), "0x", 2))
	{
		return strtol(val.c_str(), (char **)NULL, 16);
	}
	else
	{
		return strtol(val.c_str(), (char **)NULL, 10);
	}
}


int Config::getiValueRealTime(const char *section, const char *key, const int default_val)
{
	if (isFileModified())
	{
		reload();
	}
	return getiValue(section, key, default_val);
}


const string varStart   = "${";
const string varEnd     = "}";
const string varDelim   = ".";

bool Config::checkVal(const string section, const string val, string &return_val)
{
	int start = val.find(varStart);
	if (start < 0)
	{
		return false;
	}

	int end = val.find (varEnd);
	if ((end <= 0) or (end < (start + (int)varStart.length())))
	{	
		return false;
	}
	int offset = start + varStart.length();
	string varName = val.substr(offset, end - offset);
	string search;
	int stop;
	if ((stop = varName.find(varDelim)) >= 0)
	{
		if (stop == 0)
		{
			search = section  + separators + varName.substr(varDelim.length());
		}
		else
		{
			search = varName.substr(0, stop) + separators + varName.substr(stop + varDelim.length());
		}
	}
	else
	{
		search = section + separators + varName;
	}
	string here;
	if (!findVal(search, here))
	{
		return false;
	}

	string aa;
	if (checkVal(section, here, aa))
	{
		here = aa;
	}
	if (start == 0)
	{
		return_val = here + val.substr(end + varEnd.length());
	}
	else
	{
		return_val = val.substr(0, start) + here + val.substr(end + varEnd.length());
	}
	return true;
}


bool Config::getValue(const char *section, const char *key, char *return_val, const char *default_val)
{
	string search = section + separators + key;
	string val;

	if (!findVal(search, val))
	{
		strcpy(return_val, default_val);
		return false;
	}

	string Val;

	if (checkVal(section, val, Val))
	{
		strcpy(return_val, Val.c_str());
	}
	else
	{
		strcpy(return_val, val.c_str());
	}
	return true;
}


bool Config::getValueRealTime(const char *section, const char *key, char *return_val, const char *default_val)
{
	if (isFileModified())
	{
		reload();
	}
	return getValue(section, key, return_val, default_val);
}

string Config::str(const char *section, const char *key, const char *default_val)
{
	string search = section + separators + key;
	string val;

	if  (!findVal(search, val))
	{
		return default_val;
	}
	string Val;
	return checkVal(section, val, Val) ? Val : val;
}

bool Config::boolean(const char *s, const char *k, const bool default_val)
{
	string search = s + separators + k;
	string val;

	if (!findVal(search, val))
	{	
		return default_val;
	}
	string Val;

	if (checkVal(s, val, Val))
	{	
		return (!strcasecmp(Val.data(), "yes") or !strcasecmp(Val.data(), "true") or !strcmp(Val.data(), "1"));
	}
	else
	{
		return (!strcasecmp(val.data(), "yes") or !strcasecmp(val.data(), "true") or !strcmp(val.data(), "1"));
	}
}

bool Config::booleanRealTime(const char *s, const char *k, const bool default_val)
{
	if (isFileModified())
	{	
		reload();
	}
	return boolean(s, k, default_val);
}
