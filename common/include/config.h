#ifndef	__CONFIG_H__
#define	__CONFIG_H__


#include <string>
#include <map>

#ifdef _REENTRANT
	#include <boost/thread/shared_mutex.hpp>
#endif


typedef	std::map<std::string, std::string> CFG_Map;


class Config {
	private:
		CFG_Map m_cfg_;
		int checkInterval_;
		bool check_sharp_only_1st_col_;
		time_t opened_tv_, last_view_;
		char *file_;
#ifdef	_REENTRANT
		boost::shared_mutex slock;
#endif

		void ltrim(std::string &);
		void rtrim(std::string &);
		inline void trim(std::string &t) { ltrim(t); rtrim(t); }
		void trim_all_and_lower(std::string &);
		bool readConfig(void);
		bool findVal(const std::string text, std::string &return_string);
		bool checkVal(const std::string, const std::string, std::string &);
		bool isFileModified(void);
		bool reload(void);
		
	public:
		Config();
		Config(const char *, bool check_sharp_only_1st_col = false);
		~Config();
		bool init(const char *, bool check_sharp_only_1st_col = false);
		void debugPrint(void);
		bool changeFile(const char *File);
		void doRealTimeConfig(void);
		void setInterval(const int intvl) { checkInterval_ = intvl; }

		int getiValue(const char *section, const char *key, const int default_val = 0);
		int getiValueRealTime(const char *section, const char *key, const int default_val = 0);
		bool getValue(const char *section, const char *key, char *return_val, const char *default_val = "");
		bool getValueRealTime(const char *section, const char *key, char *return_val, const char *default_val= "");

		bool boolean(const char *s, const char *k, const bool default_val = false);
		bool booleanRealTime(const char *s, const char *k, const bool default_val = false);

		std::string str(const char *, const char *, const char *default_val = "");

		bool isExist(const char *, const char *);
};
#endif
