#include "ThreadPool.h"

ThreadPool::ThreadPool() : m_pool_size(DEFAULT_POOL_SIZE)
{
  #ifdef VERBOSE
  cout << "Constructed ThreadPool of size " << m_pool_size << endl;
  #endif
}

ThreadPool::ThreadPool(int pool_size) : m_pool_size(pool_size)
{
  #ifdef VERBOSE
  cout << "Constructed ThreadPool of size " << m_pool_size << endl;
  #endif
}

ThreadPool::~ThreadPool()
{
  // Release resources
  if (m_pool_state != STOPPED) {
    destroy_threadpool();
  }
}

// We can't pass a member function to pthread_create.
// So created the wrapper function that calls the member function
// we want to run in the thread.
extern "C"
void* start_thread(void* arg)
{
  ThreadPool* tp = (ThreadPool*) arg;
  tp->execute_thread();
  return NULL;
}

int ThreadPool::initialize_threadpool()
{
  // TODO: COnsider lazy loading threads instead of creating all at once
  m_pool_state = STARTED;
  int ret = -1;
  for (int i = 0; i < m_pool_size; i++) {
    pthread_t tid;
    ret = pthread_create(&tid, NULL, start_thread, (void*) this);
    if (ret != 0) {
      cerr << "pthread_create() failed: " << ret << endl;
      return -1;
    }
    m_threads.push_back(tid);
  }
  #ifdef VERBOSE
  cout << m_pool_size << " threads created by the thread pool" << endl;
  #endif

  return 0;
}

int ThreadPool::destroy_threadpool()
{
  // Note: this is not for synchronization, its for thread communication!
  // destroy_threadpool() will only be called from the main thread, yet
  // the modified m_pool_state may not show up to other threads until its 
  // modified in a lock!
  m_task_mutex.lock();
  m_pool_state = STOPPED;
  m_task_mutex.unlock();
  #ifdef VERBOSE
  cout << "Broadcasting STOP signal to all threads..." << endl;
  #endif
  m_task_cond_var.broadcast(); // notify all threads we are shttung down

  int ret = -1;
  for (int i = 0; i < m_pool_size; i++) {
    void* result;
    ret = pthread_join(m_threads[i], &result);
    #ifdef VERBOSE
    cout << "pthread_join() returned " << ret << ": " << strerror(errno) << endl;
    #endif
    m_task_cond_var.broadcast(); // try waking up a bunch of threads that are still waiting
  }
  #ifdef VERBOSE
  cout << m_pool_size << " threads exited from the thread pool" << endl;
  #endif
  return 0;
}

void* ThreadPool::execute_thread()
{
  Task* task = NULL;
  #ifdef VERBOSE
  cout << "Starting thread " << pthread_self() << endl;
  #endif
  while(true) {
    // Try to pick a task
    #ifdef VERBOSE
    cout << "Locking: " << pthread_self() << endl;
    #endif
    m_task_mutex.lock();
    
    // We need to put pthread_cond_wait in a loop for two reasons:
    // 1. There can be spurious wakeups (due to signal/ENITR)
    // 2. When mutex is released for waiting, another thread can be waken up
    //    from a signal/broadcast and that thread can mess up the condition.
    //    So when the current thread wakes up the condition may no longer be
    //    actually true!
    while ((m_pool_state != STOPPED) && (m_tasks.empty())) {
      // Wait until there is a task in the queue
      // Unlock mutex while wait, then lock it back when signaled
      #ifdef VERBOSE
      cout << "Unlocking and waiting: " << pthread_self() << endl;
      #endif
      m_task_cond_var.wait(m_task_mutex.get_mutex_ptr());
      #ifdef VERBOSE
      cout << "Signaled and locking: " << pthread_self() << endl;
      #endif
    }

    // If the thread was woken up to notify process shutdown, return from here
    if (m_pool_state == STOPPED) {
      #ifdef VERBOSE
      cout << "Unlocking and exiting: " << pthread_self() << endl;
      #endif
      m_task_mutex.unlock();
      pthread_exit(NULL);
    }

    task = m_tasks.front();
    m_tasks.pop_front();
    #ifdef VERBOSE
    cout << "Unlocking: " << pthread_self() << endl;
    #endif
    m_task_mutex.unlock();

    // execute the task
    (*task)(); // could also do task->run(arg);
    delete task;
  }
  return NULL;
}

int ThreadPool::add_task(Task* task)
{
  m_task_mutex.lock();

  // TODO: put a limit on how many tasks can be added at most
  m_tasks.push_back(task);

  m_task_cond_var.signal(); // wake up one thread that is waiting for a task to be available

  m_task_mutex.unlock();

  return 0;
}
