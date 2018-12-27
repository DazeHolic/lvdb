#include "lvdb/auto.h"

extern "C"
{
#include "timeout.h"
}

namespace lv
{
	Auto_Save_Manager::Auto_Save_Manager()
	{
		timeout_error_t e;
		timers = timeouts_open(600, &e);
	}


	void Auto_Save_Manager::add(Auto_Save_Object* obj)
	{
		timeout *t = (timeout *)obj->timer();
		timeouts_add((timeouts*)(timers), t, obj->priority());
	}

	void Auto_Save_Manager::tick(int t)
	{
		timeouts* tos = (timeouts*)(timers);
		timeout* to(NULL);
		timeouts_update(tos, t);
		while (NULL != (to = timeouts_get(tos))) {
			Auto_Save_Object* o = (Auto_Save_Object*)(to->callback.arg);
			if (o)
				o->save();
		}
	}


	Auto_Save_Object::Auto_Save_Object() : 
		am_(0),
		priority_(1), 
		dirty_(false),
		saving_(false)
	{
		timeout *t = new timeout;
		timeout_init(t, 0);
		t->callback.fn = 0;
		t->callback.arg = this;
		timer_ = t;
	}

	Auto_Save_Object::~Auto_Save_Object()
	{
		delete (timeout*)(timer_);
	}

}