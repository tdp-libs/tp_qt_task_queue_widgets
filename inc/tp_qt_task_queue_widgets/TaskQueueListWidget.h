#ifndef tp_qt_task_queue_widgets_TaskQueueListWidget_h
#define tp_qt_task_queue_widgets_TaskQueueListWidget_h

#include "tp_qt_task_queue_widgets/Globals.h"

#include <QWidget>

namespace tp_task_queue
{
class TaskQueue;
}

namespace tp_qt_task_queue_widgets
{

//##################################################################################################
//! Displays the active tasks in a list with progress bars and time to next run.
/*!

*/
class TP_QT_TASK_QUEUE_WIDGETS_SHARED_EXPORT TaskQueueListWidget: public QWidget
{
  Q_OBJECT
public:
  //################################################################################################
  TaskQueueListWidget(tp_task_queue::TaskQueue* taskQueue, QWidget* parent = nullptr);

  //################################################################################################
  virtual ~TaskQueueListWidget();

  //################################################################################################
  std::vector<int64_t> selectedTasks();

private:
  struct Private;
  Private* d;
  friend struct Private;
};

}

#endif
