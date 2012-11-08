#include "task.hh"

//static members of the Task class 
//---------------------------------------------------------------------------//
bool Task::_taskLibInited = false;
std::shared_ptr<std::thread> Task::_reaper;
bool Task::_reaperAskedToExit = false;
std::mutex Task::_reaperMutex;
std::condition_variable Task::_reaperCondVar;
Task* Task::_reaperObject = NULL;
bool Task::_reaperSignalled = false;
//---------------------------------------------------------------------------//

//our reaper logic is simple wait on the reaper promise and 
//future for 2 seconds and reap the objects who exited.
void
Task::_reaperLogic()
{
	//std::cerr<<"\nReaper started";
	std::unique_lock<std::mutex> _reaperLock(Task::_reaperMutex);
	while(!_reaperAskedToExit)
	{
		std::cv_status rv = _reaperCondVar.wait_for(_reaperLock, std::chrono::seconds(START_WAIT_TIME));
		if ((rv != std::cv_status::timeout) && Task::_reaperSignalled) {
			Task *tptr = Task::_reaperObject;
			assert(tptr);
			delete tptr;
			Task::_reaperObject = NULL;
			Task::_reaperSignalled = false;
			//std::cerr<<"\nReaper just reaped:"<<tptr;
		}
	}
	//std::cerr<<"\nReaper no more active";
	return;
}

//initialize the akorp book keeping information
void 
Task::_initializeTaskLib()
{
	if (Task::_taskLibInited) return;
	Task::_reaper = std::make_shared<std::thread>(Task::_reaperLogic);
	Task::_taskLibInited = true;
	atexit(Task::_finalizeTaskLib);
	return;
}

void 
Task::_finalizeTaskLib()
{
	if (!Task::_taskLibInited) return;
	Task::_reaperAskedToExit = true;
	Task::_reaper->join();
	Task::_taskLibInited = false;
	return;
}

//inform the reaper to reap our exit status.
void Task::_informReaper()
{
	Task::_reaperObject = getDerivedObjPtr();
	std::unique_lock<std::mutex> _reaperLock(_reaperMutex);
	Task::_reaperSignalled = true;
	Task::_reaperCondVar.notify_one();
	return;
}

//the executor which calls the thread function up on a start trigger.
void Task::_run()
{
	//wait for the start trigger to run on the future.
	std::future_status rv = _runFuture.wait_for(std::chrono::seconds(START_WAIT_TIME));
	if (rv == std::future_status::ready) 
	{
		if (_runFuture.get() == true) 
		{
			_aboutToRun = false;
			_thrFunc();
			if (!_askedToStop) _informReaper();
			return;
		}
	}
	else if (rv == std::future_status::timeout) {
		std::cerr<<"\nTimed out waiting for run trigger.";
		assert(0);
	}
	return;
}

Task::Task(std::function<void(void)>f) : 
	_runFuture(_runPromise.get_future()),
	_askedToStop(false),
	_aboutToRun(true),
	_joinDone(false),
	_thrFunc(f)
{
	if (!Task::_taskLibInited) Task::_initializeTaskLib();
	_thread = std::make_shared<std::thread>(std::bind(&Task::_run, this));
	return;
};

//The thread logic should always check this flag whether its asked to stop or not.
bool Task::askedToStop()
{
	return _askedToStop;
}

//This is always called from another thread context so we need to 
//invoke join. 
Task::~Task()
{
	if(_aboutToRun) _runPromise.set_value(false);
	_askedToStop = true; 
	if(!_joinDone) { _thread->join(); _joinDone = true; }
	return;
}

//method to be called by the other thread to stop us.
void Task::stop()
{
	assert(!_aboutToRun);
	_askedToStop = true; 
	return; 
}

//method to be called by the other thread to wait for us.
void Task::join()
{
	assert(!_joinDone);
	_thread->join();
	_joinDone = true;
	return;
}

//to be called by the object itself when it has done its construction successfully 
//infact this should be the last call in the constructor.
void Task::start()
{
	assert(_aboutToRun);
	_runPromise.set_value(true);
	return;
}

#if 0
//compile with g++-4.7 -g -pthread -std=c++0x task.cc -o task 
class lazyObject  : public Task
{
	int count;
	public:
	lazyObject(int c) :
	Task([&](){ 
		//Logic of the task 
		for (int i = 0 ;i < count ; i++) {
		std::cerr<<"\nneverending nonsense";
		sleep(1);
		}
	}),
	count(c)
	{
		start();
		return; 
	}
	~lazyObject() 
	{
		stop();
		return;
	};

	Task*getDerivedObjPtr() { return this; }
};

int
main(int ac, char **av)
{
	lazyObject *l = new lazyObject(5);
	l->join();
	return 0;
}
#endif
