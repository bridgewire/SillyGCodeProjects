#include "concurrent.h"
#include "part.h"

// Honor-system semaphore
bool BWCNC::HSSem::grab()
{
    std::unique_lock<std::mutex> lk(m);
    if( locked || (max_hold > 0 && held >= max_hold) )
        return false;

    held++;

    lk.unlock();
    cv.notify_all();

    return true;
}

bool BWCNC::HSSem::drop()
{
    std::unique_lock<std::mutex> lk(m);
    if( locked || held < 1 )
        return false;

    held--;

    lk.unlock();
    cv.notify_all();

    return true;
}

// wait for held <= holdvalue
void BWCNC::HSSem::waitfor_el( const unsigned holdvalue )
{
    std::unique_lock<std::mutex> lk(m);
    if( held <= holdvalue )
        return;
    cv.wait(lk, [this,holdvalue]{ return (this->held <= holdvalue); });
};

////////////////////////////////////////////////////////////////////////////////////////////////

BWCNC::Part * BWCNC::ShareableWorkQueue::next()
{
    std::lock_guard<std::mutex> g(v_lock);
    BWCNC::Part * p = nullptr;

    if( ! m_iterator_isend )
    {
        if( m_iterator_isset )
            v_it++;
        else
        {
            v_it = v.begin();
            m_iterator_isset = true;
        }

        m_iterator_isend = (v_it == v.end());
        if( ! m_iterator_isend )
            p = *v_it;
    }

    return p;
}


////////////////////////////////////////////////////////////////////////////////////////////////

int BWCNC::QueueProcessorThread::thread_main()
{
    //std::cout << hdl->get_id() << ": thread_main started\n";

    std::unique_lock<std::mutex> lk(m);
    while( wait_for_work )
    {
        //std::cout << "entering wait\n";
        notifier_cv.wait(lk, [this]{ return this->time_to_wake; }); // lk is unlocked during waiting sleep, but is otherwise
                                                                    // locked ... until thread_main ends
        //std::cout << hdl->get_id() << ": received notify\n" << std::flush;

        //std::cout << "exited wait ..  work_available:" << work_available << " workprocessor:" << workprocessor << " worque:" <<  worque << "\n";

        if( work_available && workprocessor && worque )             // make sure there's something to do before trying to do it.
            while(   ! end_work_now                                 // do the work unless interrupt
                  && workprocessor->do_work( worque->next() ) )     // and until do_work() says there's no more work to do.
            {  /* std::cout << "looping\n" */ ;   }


        //std::cout << hdl->get_id() << ": work is complete\n";

        time_to_wake   = false;
        work_completed = true;
        work_available = false;                                      // re-initialize vars before waiting, again, for the next batch
        end_work_now = false;
        worque = nullptr;
        workprocessor = nullptr;

        if( worker_sem )
        {
            worker_sem->drop();
            worker_sem = nullptr;
        }
    }

    //std::cout << hdl->get_id() << ": thread_main terminating\n";

    return 0;
}

void BWCNC::QueueProcessorThread::setup_job( ShareableWorkQueue * queue, ShareableWorkProcessor * queueproc, std::shared_ptr<HSSem> sem )
{
    std::unique_lock<std::mutex> lk(m);
    //std::cout << "delivering work to thread, id:" << hdl->get_id() << "\n";

    worque = queue;
    workprocessor = queueproc;
    work_available = true;
    work_completed = false;
    worker_sem = sem;
}

void BWCNC::QueueProcessorThread::teardown_job()
{
    std::unique_lock<std::mutex> lk(m);
    //std::cout << "trearingdn work in thread, id:" << hdl->get_id() << "\n";
    worque = nullptr;
    workprocessor = nullptr;
    work_available = false;
    work_completed = false;
}

void BWCNC::QueueProcessorThread::begin_work()
{
    if( ! hdl ) return;

    std::unique_lock<std::mutex> lk(m);
    //std::cout << "initiating work in thread, id:" << hdl->get_id() << "\n";

    if( worque && workprocessor )
        time_to_wake = true;

    lk.unlock();

    if( worker_sem ) worker_sem->grab();

    notifier_cv.notify_one();
}

void BWCNC::QueueProcessorThread::terminate_thread()
{
    if( ! hdl ) return;

    std::unique_lock<std::mutex> lk(m);
    //std::cout << "delivering term to thread, id:" << hdl->get_id() << "\n";

    time_to_wake   = true;
    work_available = false;
    work_completed = false;
    wait_for_work  = false;

    lk.unlock();
    notifier_cv.notify_one();

    hdl->join();
}

void BWCNC::QueueProcessorThread::interrupt_work() { end_work_now = true; }

////////////////////////////////////////////////////////////////////////////////////////////////

static unsigned threads_operating = 0;
static std::vector<BWCNC::QueueProcessorThread *> thread_list;
static std::mutex thread_list_mutex;
static std::condition_variable thread_list_mutex_cv;
static bool thread_list_wake = false;

unsigned BWCNC::ShareableWorkQueueProcessor::threads_running() { return threads_operating; }


void BWCNC::ShareableWorkQueueProcessor::terminate_threads()
{
    for( auto t : thread_list )
        t->terminate_thread();

    for( unsigned i = 0; i < thread_list.size(); i++ )
    {
        delete thread_list[i];
        thread_list[i] = nullptr;
    }

    thread_list.clear();
}

static int worker_thread_main();

unsigned BWCNC::ShareableWorkQueueProcessor::start_threads( int thread_count )
{
    if( threads_operating > 0 )
        terminate_threads();

    if( thread_count < 0 )
        thread_count = std::thread::hardware_concurrency();

    if( thread_count > 0 )
    {
        if( (unsigned)thread_count > 4 * std::thread::hardware_concurrency() )
            thread_count = 4 * std::thread::hardware_concurrency();

        for( int i = 0; i < thread_count; i++ )
        {
            std::unique_lock<std::mutex> thread_list_lock(thread_list_mutex);
            thread_list_wake = false;
            std::thread * hdl = new std::thread( worker_thread_main );
            thread_list_mutex_cv.wait(thread_list_lock, []{ return thread_list_wake; });
            thread_list[threads_operating-1]->hdl = hdl;
        }
    }
    return threads_operating;
}

void BWCNC::ShareableWorkQueueProcessor::run_shareable_job( BWCNC::ShareableWorkQueue * queue, BWCNC::ShareableWorkProcessor * queueproc, bool wait4done )
{
    std::shared_ptr<HSSem> sem_ptr =  wait4done ? std::make_shared<HSSem>() : nullptr;

    for( unsigned i = 0; i < thread_list.size(); i++ )
    {
        thread_list[i]->setup_job( queue, queueproc, sem_ptr );
        thread_list[i]->begin_work();
    }

    if( wait4done )
        sem_ptr->waitfor_el(0);
}

bool BWCNC::ShareableWorkQueueProcessor::shareable_job_isdone()
{
    for( unsigned i = 0; i < thread_list.size(); i++ )
    {
        if( ! thread_list[i]->work_is_complete() )
            return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
static int worker_thread_main()
{
    std::unique_lock<std::mutex> thread_list_lock(thread_list_mutex);
    BWCNC::QueueProcessorThread * t = new BWCNC::QueueProcessorThread();

    if( t )
    {
        thread_list.push_back( t );
        threads_operating++;
    }

    thread_list_wake = true;
    thread_list_lock.unlock();
    thread_list_mutex_cv.notify_one();

    return t->thread_main();
}

////////////////////////////////////////////////////////////////////////////////////////////////

bool BWCNC::ShareableWorkProcessor_pdt_processor::do_work( BWCNC::Part * p )
{
    if( p )
    {
      //printf("p");
        p->pos_dep_tform( t );
        return true;
    }
    return false;
}

bool BWCNC::ShareableWorkProcessor_transform::do_work( BWCNC::Part * p )
{
    if( p )
    {
      //printf("t");
        p->transform( tform_mat );
        return true;
    }
    return false;
}

bool BWCNC::ShareableWorkProcessor_translate::do_work( BWCNC::Part * p )
{
    if( p )
    {
      //printf("s");
        p->translate( vec );
        return true;
    }
    return false;
}



