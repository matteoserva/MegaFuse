#include "EventsHandler.h"


EventsListener::EventsListener(EventsHandler &eh,EventsHandler::event e):eh(eh),e(e)
{
	eh.addListener(e);
}
	EventResult EventsListener::waitEvent()
	{
		return eh.waitEvent(e);
	}
	
EventsListener::~EventsListener()
{
	eh.removeListener(e);
}

	EventResult EventsHandler::waitEvent(event e)
	{
		{
		std::unique_lock<std::mutex> lk(cvm_notify);
		cv_notify.wait(lk, [this,e] {return event_received == e;});
		}
		
		{
				std::lock_guard<std::mutex> lk(cvm_listener);
				clients_notified++;
		}
		cv_listener.notify_one();
		
		return EventResult{event_result,event_value};
	}
	int EventsHandler::countListeners(event e)
	{
		listeners_mutex.lock();
		int l = listeners[e];
		listeners_mutex.unlock();
		return l;
		
	}
	
	int EventsHandler::notifyEvent(event e,int res,std::string v)
	{
		std::lock_guard<std::mutex> lk(notify_mutex);
		listeners_mutex.lock();
		int l = listeners[e];
		listeners_mutex.unlock();
		if (l <= 0)
			return l;
		
		
		{
				std::lock_guard<std::mutex> lk(cvm_notify);
				clients_notified = 0;
				event_received = e;
				event_result = res;
				event_value = v;
		}
		cv_notify.notify_all();
		
		{
			std::unique_lock<std::mutex> lk(cvm_listener);
			cv_listener.wait(lk, [this,l,e] {return clients_notified >= countListeners(e);});
		
		}
		
		return l;
	}
	
	void EventsHandler::addListener(event e)
	{
		listeners_mutex.lock();
		listeners[e]++;
		listeners_mutex.unlock();
	}

	void EventsHandler::removeListener(event e)
	{
		listeners_mutex.lock();
		listeners[e]--;
		listeners_mutex.unlock();

		cv_listener.notify_one();
	}


