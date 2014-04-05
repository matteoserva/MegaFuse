#ifndef _EVENTS_HANDLER_H_
#define _EVENTS_HANDLER_H_
#include <map>
#include <mutex>
#include <set>
#include <condition_variable>
#include <list>


struct EventResult
{
	int result;
	std::string value;
	
};
class EventsListener;
class EventsHandler
{

	public:
	enum event{NONE,TRANSFER_COMPLETE,LOGIN_RESULT,USERS_UPDATED,NODE_UPDATED,NODE_REMOVED,UNLINK_RESULT,UPLOAD_COMPLETE,PUTNODES_RESULT,TRANSFER_UPDATE,TOPEN_RESULT};
	
	void addListener(EventsListener*,event);
	void removeListener(EventsListener*, event);
	int notifyEvent(event,int r=0,std::string v="");
	
private:
	std::mutex events_mutex;
	std::map<event,std::set<EventsListener * > > listeners;
	
};
class EventsListener
{
	EventsHandler &eh;
	EventsHandler::event e;
	
	std::mutex listener_mutex;
	std::condition_variable listener_cvm;
	std::list<EventResult> eventsReceived;
	public:
	EventsListener(EventsHandler &eh,EventsHandler::event e);
	EventResult waitEvent();
	int notifyEvent(EventResult &);
	~EventsListener();
	
};
#endif