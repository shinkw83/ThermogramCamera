#ifndef 	__PID_FILE_H__
#define 	__PID_FILE_H__

#include <string>

class PidFile{

	std::string fname;

	public:
		PidFile() {}
		~PidFile() {}

		void reg(const std::string &);
		void reg(const std::string &, int);
        void dereg();
};


#endif
