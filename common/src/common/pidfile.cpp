#include "pidfile.h"
#include <fstream>
#include <boost/format.hpp>

void PidFile::reg(const std::string &proc)
{
	fname = boost::str(boost::format("%s/zetavu/pid/%s.pid") % getenv("HOME") % proc);
	std::ofstream o(fname.c_str());
	o << getpid();
	o.close();
}


void PidFile::reg(const std::string &proc, int pid)
{
    fname = boost::str(boost::format("%s/zetavu/pid/%s.pid") % getenv("HOME") % proc);
    std::ofstream o(fname.c_str());
    o << pid;
    o.close();
}

void PidFile::dereg()
{
	unlink(fname.c_str());
}
