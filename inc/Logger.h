#ifndef _LOGGER_H_
#define _LOGGER_H_

class Logger
{
private:
	Logger();
	bool notifications_available;
	
	public:
	enum log_type {NOTIFY,STATUS};
	void log(log_type,const char *format, ...);
	static Logger & getInstance();
	
};

#endif