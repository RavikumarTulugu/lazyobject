#ifndef __INC_TASK_H__
#define __INC_TASK_H__
#include <iostream>
#include <unistd.h>
#include <assert.h>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <iostream>
#include <thread>
#include <future>
#include <chrono>

//antkorp task abstraction which provides the execution context to write the thread 
//logic which can invoke member functions.
class Task
{
	static bool _taskLibInited;
	static std::shared_ptr<std::thread> _reaper;
	static bool _reaperAskedToExit;
	static std::mutex _reaperMutex;
	static std::condition_variable _reaperCondVar;
	static Task* _reaperObject;
	static bool _reaperSignalled;

#define START_WAIT_TIME 2  //wait for 2 seconds for start trigger.
	std::shared_ptr<std::thread> _thread; //execution thread which runs the logic 
	std::promise<bool> _runPromise;
	std::future<bool>  _runFuture;
	bool _askedToStop; //exit switch the application logic need to check this periodically and exit on true
	bool _aboutToRun;
	bool _joinDone;
	std::function<void(void)> _thrFunc; //functor which runs the application logic. 

	//send  our this pointer to the reaper where the reaper can delete the object.
	void _informReaper();
	void _run();
	public:
	Task(std::function<void(void)>f); 
	virtual ~Task();
	bool askedToStop();
	void stop();
	void join();
	void start();
	virtual Task *getDerivedObjPtr() = 0;
	static void _reaperLogic();
	static void _initializeTaskLib();
	static void _finalizeTaskLib();
};
#endif
