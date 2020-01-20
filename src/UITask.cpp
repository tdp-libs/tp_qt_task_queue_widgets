#include "tp_qt_task_queue_widgets/UITask.h"

#include "tp_qt_utils/CrossThreadCallback.h"

#include "tp_task_queue/Task.h"

#include "tp_utils/MutexUtils.h"

#include <memory>

namespace tp_qt_task_queue_widgets
{

namespace
{
//##################################################################################################
struct Handle_lt
{
  TPMutex mutex{TPM};
  tp_qt_utils::CrossThreadCallback* callback{nullptr};

  //################################################################################################
  void call()
  {
    TP_MUTEX_LOCKER(mutex);
    if(callback)
      callback->call();
  }
};
}

//##################################################################################################
struct UITask::Private
{
  std::string taskName;
  std::function<void()> performTask;
  tp_qt_utils::CrossThreadCallback callback;

  std::shared_ptr<Handle_lt> handle;

  tp_task_queue::Task* task{nullptr};

  //################################################################################################
  Private(const std::string& taskName_,
          const std::function<void()>& performTask_,
          const std::function<void()>& taskComplete):
    taskName(taskName_),
    performTask(performTask_),
    callback(taskComplete)
  {
    handle = std::make_shared<Handle_lt>();
    handle->callback = &callback;
  }

  //################################################################################################
  ~Private()
  {
    TP_MUTEX_LOCKER(handle->mutex);
    handle->callback = nullptr;
  }
};

//##################################################################################################
UITask::UITask(const std::string& taskName,
               const std::function<void()>& performTask,
               const std::function<void()>& taskComplete):
  d(new Private(taskName, performTask, taskComplete))
{

}

//##################################################################################################
UITask::~UITask()
{
  delete d;
}

//##################################################################################################
tp_task_queue::Task* UITask::task()
{
  if(!d->task)
  {
    auto handle      = d->handle;
    auto performTask = d->performTask;
    d->task = new tp_task_queue::Task(d->taskName, [handle, performTask](tp_task_queue::Task& task)
    {
      TP_UNUSED(task);

      {
        TP_MUTEX_LOCKER(handle->mutex);
        if(!handle->callback)
          return tp_task_queue::RunAgain::No;
      }

      performTask();
      handle->call();

      return tp_task_queue::RunAgain::No;
    });
  }

  return d->task;
}

}
