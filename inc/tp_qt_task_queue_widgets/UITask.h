#ifndef tp_qt_task_queue_widgets_UITask_h
#define tp_qt_task_queue_widgets_UITask_h

#include "tp_qt_task_queue_widgets/Globals.h"

#include <functional>

namespace tp_task_queue
{
class Task;
}

namespace tp_qt_task_queue_widgets
{

//##################################################################################################
//! Execute a task to update the UI.
class TP_QT_TASK_QUEUE_WIDGETS_SHARED_EXPORT UITask
{
public:
  //################################################################################################
  /*!
  \param taskName
  \param performTask Called in a worker thread to perform the task.
  \param taskComplete Called in the UI thread when the task is complete.
  */
  UITask(const std::string& taskName,
         const std::function<void()>& performTask,
         const std::function<void()>& taskComplete);

  //################################################################################################
  virtual ~UITask();

  //################################################################################################
  tp_task_queue::Task* task();

private:
  struct Private;
  Private* d;
  friend struct Private;
};

}

#endif
