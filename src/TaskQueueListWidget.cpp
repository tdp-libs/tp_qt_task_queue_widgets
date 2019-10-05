#include "tp_qt_task_queue_widgets/TaskQueueListWidget.h"

#include "tp_qt_utils/TimerCallback.h"
#include "tp_qt_utils/CrossThreadCallback.h"

#include "tp_task_queue/Task.h"
#include "tp_task_queue/TaskQueue.h"

#include "tp_utils/DebugUtils.h"

#include <QListWidget>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

#include <unordered_set>

namespace tp_qt_task_queue_widgets
{
//##################################################################################################
struct TaskQueueListWidget::Private
{
  TP_NONCOPYABLE(Private);

  TaskQueueListWidget* q;
  tp_task_queue::TaskQueue* taskQueue;

  QListWidget* listWidget{nullptr};

  std::function<void()> statusChangedCallback;
  tp_qt_utils::CrossThreadCallback statusChangedCrossThreadCallback;

  struct TaskDetails_lt
  {
    QListWidgetItem* item        {nullptr};
    QFrame*          itemWidget  {nullptr};
    QLabel*          messageLabel{nullptr};    
    QPushButton*     pauseButton {nullptr};
    QProgressBar*    progressBar {nullptr};
    std::string      taskName;
    std::string      message;
    int              progress    {0};
    bool             complete    {false};
    bool             pauseable   {false};
    bool             paused      {false};
  };

  std::unordered_map<int64_t, TaskDetails_lt> items;

  QIcon pauseIcon{":/tp_qt_icons_technical/pause.png"};
  QIcon playIcon{":/tp_qt_icons_technical/play.png"};

  //################################################################################################
  Private(TaskQueueListWidget* q_, tp_task_queue::TaskQueue* taskQueue_):
    q(q_),
    taskQueue(taskQueue_),
    statusChangedCrossThreadCallback([&](){update();})
  {
    statusChangedCallback = [&](){statusChangedCrossThreadCallback.call();};
    taskQueue->addStatusChangedCallback(&statusChangedCallback);
  }

  //################################################################################################
  ~Private()
  {
    taskQueue->removeStatusChangedCallback(&statusChangedCallback);
  }

  //################################################################################################
  void update()
  {
    taskQueue->viewTaskStatus([&](const std::vector<tp_task_queue::TaskStatus>& tasks)
    {
      std::vector<int64_t> existingKeys;
      existingKeys.reserve(items.size());
      for(const auto& i : items)
        existingKeys.push_back(i.first);

      //We put the item widget in a container so that we can reparent it when it is moved in the
      //list. Below c is a sacrificial container that will be desroyed when the item is moved.
      auto addItemWidget = [&](TaskDetails_lt& taskDetails)
      {
        auto c = new QWidget();
        auto l = new QVBoxLayout(c);
        l->setContentsMargins(0, 0, 0, 0);
        l->addWidget(taskDetails.itemWidget);
        listWidget->setItemWidget(taskDetails.item, c);
      };

      for(const tp_task_queue::TaskStatus& taskStatus : tasks)
      {
        tpRemoveOne(existingKeys, taskStatus.taskID);

        TaskDetails_lt& taskDetails = items[taskStatus.taskID];

        if(!taskDetails.item)
        {
          taskDetails.itemWidget = new QFrame();
          taskDetails.itemWidget->setFrameShape(QFrame::Box);
          taskDetails.itemWidget->setDisabled(taskStatus.complete);
          auto itemLayout = new QVBoxLayout(taskDetails.itemWidget);

          auto buttonLayout = new QHBoxLayout();
          buttonLayout->setContentsMargins(0,0,0,0);
          itemLayout->addLayout(buttonLayout);

          auto messageLayout = new QVBoxLayout();
          messageLayout->setContentsMargins(0,0,0,0);
          messageLayout->setSpacing(1);
          buttonLayout->addLayout(messageLayout);

          itemLayout->setContentsMargins(2,2,2,2);
          itemLayout->setSpacing(1);
          messageLayout->addWidget(new QLabel(QString("<b>%1</b>").arg(QString::fromStdString(taskStatus.taskName))));
          taskDetails.messageLabel = new QLabel();
          messageLayout->addWidget(taskDetails.messageLabel);

          buttonLayout->addStretch();

          int btnSz=24;
          int iconSz=16;
          taskDetails.pauseButton = new QPushButton("");
          taskDetails.pauseButton->setIcon(pauseIcon);
          taskDetails.pauseButton->setVisible(taskStatus.pauseable);
          taskDetails.pauseButton->setToolTip("Pause task.");
          taskDetails.pauseButton->setFixedSize(btnSz, btnSz);
          taskDetails.pauseButton->setIconSize(QSize(iconSz, iconSz));
          buttonLayout->addWidget(taskDetails.pauseButton);
          auto taskID = taskStatus.taskID;
          connect(taskDetails.pauseButton, &QPushButton::clicked, [=](){taskQueue->togglePauseTask(taskID);});

          taskDetails.progressBar = new QProgressBar();
          taskDetails.progressBar->setEnabled(taskStatus.progress>=0);
          taskDetails.progressBar->setRange(0, 100);
          itemLayout->addWidget(taskDetails.progressBar);

          taskDetails.item = new QListWidgetItem();
          taskDetails.item->setData(Qt::UserRole, qint64(taskStatus.taskID));
          listWidget->insertItem(0, taskDetails.item);
          addItemWidget(taskDetails);
          taskDetails.item->setSizeHint(QSize(0, 64));
        }

        bool updateTooltip=false;

        if(taskDetails.taskName != taskStatus.taskName)
        {
          taskDetails.taskName = taskStatus.taskName;
          updateTooltip = true;
        }

        if(taskDetails.message != taskStatus.message)
        {
          taskDetails.message = taskStatus.message;
          taskDetails.messageLabel->setText(QString::fromStdString(taskStatus.message));
          updateTooltip = true;
        }

        if(updateTooltip)
          taskDetails.itemWidget->setToolTip(QString("%1. %2").arg(QString::fromStdString(taskStatus.taskName), QString::fromStdString(taskStatus.message)));

        if(taskDetails.progress != taskStatus.progress)
        {
          taskDetails.progress = taskStatus.progress;
          taskDetails.progressBar->setValue(taskStatus.progress);
          taskDetails.progressBar->setEnabled(taskStatus.progress>=0);
        }

        if(taskDetails.complete != taskStatus.complete)
        {
          taskDetails.complete = taskStatus.complete;
          taskDetails.itemWidget->setDisabled(taskStatus.complete);
        }

        if(taskDetails.pauseable != taskStatus.pauseable)
        {
          taskDetails.pauseable = taskStatus.pauseable;
          taskDetails.pauseButton->setVisible(taskStatus.pauseable);
        }

        if(taskDetails.paused != taskStatus.paused)
        {
          taskDetails.paused = taskStatus.paused;
          taskDetails.pauseButton->setIcon(taskStatus.paused?playIcon:pauseIcon);
        }
      }

      for(int64_t taskID : existingKeys)
      {
        TaskDetails_lt& taskDetails = items[taskID];
        delete taskDetails.item;
        items.erase(taskID);
      }

      //Sort the items to have non complete items first
      {
        int c=0;
        for(int r=0; r<listWidget->count(); r++)
        {
          auto item = listWidget->item(r);
          int64_t taskID = item->data(Qt::UserRole).toLongLong();

          TaskDetails_lt& taskDetails = items[taskID];

          if(taskDetails.complete)
            continue;

          if(r!=c)
          {
            taskDetails.itemWidget->setParent(nullptr);
            listWidget->insertItem(c, listWidget->takeItem(r));
            addItemWidget(taskDetails);
          }

          c++;
        }
      }
    });
  }
};

//##################################################################################################
TaskQueueListWidget::TaskQueueListWidget(tp_task_queue::TaskQueue* taskQueue, QWidget* parent):
  QWidget(parent),
  d(new Private(this, taskQueue))
{
  auto l = new QHBoxLayout(this);
  l->setContentsMargins(0, 0, 0, 0);

  d->listWidget = new QListWidget();
  l->addWidget(d->listWidget);

  d->update();
}

//##################################################################################################
TaskQueueListWidget::~TaskQueueListWidget()
{
  delete d;
}

//##################################################################################################
std::vector<int64_t> TaskQueueListWidget::selectedTasks()
{
  std::vector<int64_t> selectedTasks;
  for(QListWidgetItem* item : d->listWidget->selectedItems())
    selectedTasks.push_back(item->data(Qt::UserRole).toLongLong());
  return selectedTasks;
}

}
