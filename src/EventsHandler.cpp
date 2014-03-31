#include "EventsHandler.h"


EventsListener::EventsListener(EventsHandler &eh,EventsHandler::event e):eh(eh),e(e)
{
	eh.addListener(this,e);
}
EventResult EventsListener::waitEvent()
{
	std::unique_lock<std::mutex> lk(listener_mutex);
	while(!eventsReceived.size())
		listener_cvm.wait(lk);
	
	EventResult ev = eventsReceived.front();
	eventsReceived.pop_front();
	return ev;
}

EventsListener::~EventsListener()
{
	eh.removeListener(this,e);
}


int EventsListener::notifyEvent(EventResult &er)
{
	{
		
	std::lock_guard<std::mutex> lk(listener_mutex);
	eventsReceived.push_back(er);
	}
	listener_cvm.notify_one();
	
	
	return 0;
}

int EventsHandler::notifyEvent(event e,int res,std::string v)
{
	std::lock_guard<std::mutex> lk(events_mutex);
	
	EventResult er{res,v};
	for(EventsListener * it : listeners[e])
		it->notifyEvent(er);
	return 0;
}

void EventsHandler::addListener(EventsListener* l, event e)
{
	std::lock_guard<std::mutex> lk(events_mutex);
	listeners[e].insert(l);
}

void EventsHandler::removeListener(EventsListener* l,event e)
{
	std::lock_guard<std::mutex> lk(events_mutex);
	listeners[e].erase(l);

}
