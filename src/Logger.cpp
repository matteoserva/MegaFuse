#include "Logger.h"
#include <stdlib.h>
#include <stdarg.h>  
#include <stdio.h>
#include <time.h>  
Logger::Logger():notifications_available(false)
{
	if(!system("which notify-send"))
		notifications_available=true;
}

Logger & Logger::getInstance()
{
	static Logger logger;
	return logger;
}

void Logger::log(log_type type,const char *format, ...)
{
	char buffer[512];
	va_list args;
	va_start(args, format);
	vsprintf(buffer,format, args);
	va_end(args);
	
	if(notifications_available && type == NOTIFY)
	{
		char notify_buffer[512];
		sprintf(notify_buffer,"notify-send \"%s\"",buffer);
		system(notify_buffer);
	}
	
	char time_buf[100];
	time_t rawtime;
	struct tm * timeinfo;

	time (&rawtime);
	timeinfo = localtime (&rawtime);
	strftime (time_buf, 256, "%X", timeinfo);
	printf("[%s] %s\n",time_buf,buffer);
	
	
}