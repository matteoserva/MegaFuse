#ifndef _EVENTS_HANDLER_H_
#define _EVENTS_HANDLER_H_
#include <map>
#include <mutex>
#include <condition_variable>


struct EventResult
{
	int result;
	std::string value;
	
};

class EventsHandler
{

	public:
	enum event{NONE,LOGIN_RESULT,USERS_UPDATED,NODE_UPDATED,NODE_REMOVED,UNLINK_RESULT,UPLOAD_COMPLETE};
	
	std::map<event,int> listeners;
	
	void addListener(event);
	void removeListener(event);
	EventResult waitEvent(event);
	int notifyEvent(event,int r=0,std::string v="");
	
	private:
	std::mutex listeners_mutex;
	std::mutex notify_mutex;
	int clients_notified;
	std::condition_variable cv_notify;
	std::mutex cvm_notify;
	int countListeners(event e);
	
	std::condition_variable cv_listener;
	std::mutex cvm_listener;
	
	int event_result;
	std::string event_value;
	event event_received;
	
};
class EventsListener
{
	EventsHandler &eh;
	EventsHandler::event e;
	public:
	EventsListener(EventsHandler &eh,EventsHandler::event e);
	EventResult waitEvent();
	~EventsListener();
	
};
#endif