#ifndef BWCNC_CONCURRENT_H__
#define BWCNC_CONCURRENT_H__

//#include <stdio.h>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <iostream>
#include "functions.h"
//#include "boundingbox.h"

namespace BWCNC {

class PartContext;
class Part;
class Command;

// Honor-system semaphore
class HSSem
{
public:
    HSSem( unsigned limit = 0 ) : max_hold(limit) {}
	bool grab();
	bool drop();
	void waitfor_el( const unsigned holdvalue );
  //bool lock() { std::lock_guard<std::mutex> g(m); locked = true; }
  //bool unlock() { std::lock_guard<std::mutex> g(m); locked = false; }

protected:
    const unsigned max_hold;
    unsigned held = 0;
    bool locked = false;
    std::mutex m;
    std::condition_variable cv;;
};


class ShareableWorkProcessor
{
public:
    ShareableWorkProcessor(){}
    virtual ~ShareableWorkProcessor(){}
    virtual bool do_work( BWCNC::Part * ) = 0;
};

class ShareableWorkQueue
{
protected:
    bool m_iterator_isset = false;
    bool m_iterator_isend = false;

    std::vector<BWCNC::Part *> & v;
    std::vector<BWCNC::Part *>::iterator v_it;
    std::mutex v_lock;

public:
    ShareableWorkQueue() = delete; // no default constructor
    ShareableWorkQueue( std::vector<BWCNC::Part *> & vec ) : v(vec) {}
    virtual ~ShareableWorkQueue(){}
    void reset() { m_iterator_isset = m_iterator_isend = false; }

    BWCNC::Part * next();
};

class ShareableWorkQueueProcessor;

class QueueProcessorThread
{
private:
    friend class ShareableWorkQueueProcessor;

protected:
    std::condition_variable notifier_cv;
    std::mutex m;
    bool work_available = false;
    bool work_completed = false;
    bool wait_for_work  = true;
    bool time_to_wake   = false;
    bool end_work_now   = false;
    std::thread * hdl   = nullptr;

    ShareableWorkQueue * worque = nullptr;
    ShareableWorkProcessor * workprocessor = nullptr;
    std::shared_ptr<BWCNC::HSSem> worker_sem;

public:
    QueueProcessorThread() {}
    virtual ~QueueProcessorThread(){ if(hdl) delete hdl; }

    int thread_main();
    void setup_job( ShareableWorkQueue * queue, ShareableWorkProcessor * queueproc, std::shared_ptr<BWCNC::HSSem> sem );

    void teardown_job();
    void begin_work();
    void terminate_thread();
    void interrupt_work();
    bool work_is_complete() { return work_completed; }
};


class ShareableWorkQueueProcessor
{
public:
    ShareableWorkQueueProcessor(){}
    virtual ~ShareableWorkQueueProcessor(){}

    unsigned start_threads( int thread_count = -1 );
    void terminate_threads();
    void run_shareable_job( ShareableWorkQueue * queue, ShareableWorkProcessor * queueproc, bool wait4complete = true );
    bool shareable_job_isdone();
    unsigned threads_running();
};

class ShareableWorkProcessor_pdt_processor : public virtual ShareableWorkProcessor
{
protected:
    position_dependent_transform_t * t;

public:
    ShareableWorkProcessor_pdt_processor() = delete;
    ShareableWorkProcessor_pdt_processor( pdt_t * pdt ) : t(pdt) {}
    virtual ~ShareableWorkProcessor_pdt_processor(){}
    virtual bool do_work( BWCNC::Part * p );
};

class ShareableWorkProcessor_transform : public virtual ShareableWorkProcessor
{
protected:
    const Eigen::Matrix3d & tform_mat;

public:
    ShareableWorkProcessor_transform() = delete;
    ShareableWorkProcessor_transform( const Eigen::Matrix3d & mat ) : tform_mat(mat) {}
    virtual ~ShareableWorkProcessor_transform(){}
    virtual bool do_work( BWCNC::Part * p );
};

class ShareableWorkProcessor_translate : public virtual ShareableWorkProcessor
{
protected:
    const Eigen::Vector3d & vec;

public:
    ShareableWorkProcessor_translate() = delete;
    ShareableWorkProcessor_translate( const Eigen::Vector3d & offset ) : vec(offset) {}
    virtual ~ShareableWorkProcessor_translate(){}
    virtual bool do_work( BWCNC::Part * p );
};

};

#endif

