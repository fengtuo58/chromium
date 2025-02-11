// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICE_WORKER_SERVICE_WORKER_EVENT_QUEUE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICE_WORKER_SERVICE_WORKER_EVENT_QUEUE_H_

#include <set>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_event_status.mojom-blink-forward.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/wtf/deque.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"

namespace base {

class TickClock;

}  // namespace base

namespace blink {

// ServiceWorkerEventQueue manages two types of timeouts: the long standing
// event timeout and the idle timeout.
//
// 1) Event timeout: when an event starts, StartEvent() records the expiration
// time of the event (kEventTimeout). If EndEvent() has not been called within
// the timeout time, |abort_callback| passed to StartEvent() is called with
// status TIMEOUT. Also, |zero_idle_timer_delay_| is set to true to shut down
// the worker as soon as possible since the worker may have gone into bad state.
// 2) Idle timeout: when a certain time has passed (kIdleDelay) since all of
// events have ended, ServiceWorkerEventQueue calls the |idle_callback|.
// |idle_callback| will be continuously called at a certain interval
// (kUpdateInterval) until the next event starts.
//
// The lifetime of ServiceWorkerEventQueue is the same with the worker
// thread. If ServiceWorkerEventQueue is destructed while there are inflight
// events, all |abort_callback|s will be immediately called with status ABORTED.
class MODULES_EXPORT ServiceWorkerEventQueue {
 public:
  // A token to keep the event queue from going into the idle state if any of
  // them are alive.
  class MODULES_EXPORT StayAwakeToken {
   public:
    explicit StayAwakeToken(base::WeakPtr<ServiceWorkerEventQueue> event_queue);
    ~StayAwakeToken();

   private:
    base::WeakPtr<ServiceWorkerEventQueue> event_queue_;
  };

  using AbortCallback =
      base::OnceCallback<void(int /* event_id */,
                              mojom::blink::ServiceWorkerEventStatus)>;

  explicit ServiceWorkerEventQueue(base::RepeatingClosure idle_callback);
  // For testing.
  ServiceWorkerEventQueue(base::RepeatingClosure idle_callback,
                          const base::TickClock* tick_clock);
  ~ServiceWorkerEventQueue();

  // Starts the event_queue. This may also update |idle_time_| if there was no
  // activities (i.e., StartEvent()/EndEvent() or StayAwakeToken creation)
  // on the event_queue before.
  void Start();

  using StartCallback = base::OnceCallback<void(int /* event_id */)>;

  // Represents an event dispatch task, which can be queued into |task_queue_|.
  struct MODULES_EXPORT Task {
    enum class Type {
      // A normal task is pushed to the end of the task queue and triggers
      // processing of the queue.
      Normal,
      // A pending task, which does not start until a normal task is
      // pushed. A pending task should be used if the idle timeout
      // occurred (did_idle_timeout() returns true). In practice, the
      // caller pushes pending tasks after the service worker
      // requested the browser to terminate it due to idleness. These
      // tasks run once PushTask() is called due to a new event from
      // the browser, signalling that the browser decided not to
      // terminate the worker.
      Pending,
    };

    Task(Type type,
         StartCallback start_callback,
         AbortCallback abort_callback,
         base::Optional<base::TimeDelta> custom_timeout);
    ~Task();
    Type type;
    // Callback which is run when the event_queue starts this task. The
    // callback receives |event_id|, which is the result of
    // StartEvent(). When an event finishes, EndEvent() should be
    // called with the given |event_id|.
    StartCallback start_callback;
    // Callback which is run when the event_queue aborts a started task.
    AbortCallback abort_callback;
    // The custom timeout value.
    base::Optional<base::TimeDelta> custom_timeout;
  };

  void EndEvent(int event_id);

  // Push the task to the queue, and tasks in the queue can run synchronously.
  // See also Task's comment.
  void PushTask(std::unique_ptr<Task> task);

  // Returns true if |event_id| was started and hasn't ended.
  bool HasEvent(int event_id) const;

  // Creates a StayAwakeToken to ensure that the idle timer won't be triggered
  // while any of these are alive.
  std::unique_ptr<StayAwakeToken> CreateStayAwakeToken();

  // Sets the |zero_idle_timer_delay_| to true and triggers the idle callback if
  // there are not inflight events. If there are, the callback will be called
  // next time when the set of inflight events becomes empty in EndEvent().
  void SetIdleTimerDelayToZero();

  // Returns true if the timer thinks no events ran for a while, and has
  // triggered the |idle_callback| passed to the constructor. It'll be reset to
  // false again when StartEvent() is called.
  bool did_idle_timeout() const { return did_idle_timeout_; }

  // Idle timeout duration since the last event has finished.
  static constexpr base::TimeDelta kIdleDelay =
      base::TimeDelta::FromSeconds(30);
  // Duration of the long standing event timeout since StartEvent() has been
  // called.
  static constexpr base::TimeDelta kEventTimeout =
      base::TimeDelta::FromMinutes(5);
  // ServiceWorkerEventQueue periodically updates the timeout state by
  // kUpdateInterval.
  static constexpr base::TimeDelta kUpdateInterval =
      base::TimeDelta::FromSeconds(30);

 private:
  // StartEvent() should be called at the beginning of an event. It returns an
  // event id, which is unique among threads in the same process.
  // The event id should be passed to EndEvent() when the event has finished.
  int StartEvent(AbortCallback abort_callback, base::TimeDelta timeout);

  // Updates the internal states and fires timeout callbacks if any.
  void UpdateStatus();

  // Triggers idle timer if |zero_idle_timer_delay_| is true. Returns true if
  // the idle callback is called.
  bool MaybeTriggerIdleTimer();

  // Sets the |idle_time_| and maybe calls |idle_callback_| immediately if the
  // timeout delay is set to zero.
  void OnNoInflightEvent();

  // Returns true if there are running events.
  bool HasInflightEvent() const;

  // Processes all tasks in |task_queue_|.
  void ProcessTasks();

  // Start a single task.
  void StartTask(std::unique_ptr<Task> task);

  struct EventInfo {
    EventInfo(base::TimeTicks expiration_time,
              base::OnceCallback<void(mojom::blink::ServiceWorkerEventStatus)>
                  abort_callback);
    ~EventInfo();
    const base::TimeTicks expiration_time;
    base::OnceCallback<void(mojom::blink::ServiceWorkerEventStatus)>
        abort_callback;
  };

  // For long standing event timeouts. This is used to look up an EventInfo
  // by event id.
  HashMap<int /* event_id */, std::unique_ptr<EventInfo>> id_event_map_;

  // For idle timeouts. The time the service worker started being considered
  // idle. This time is null if there are any inflight events.
  base::TimeTicks idle_time_;

  // Set to true if the idle callback should be fired immediately after all
  // inflight events finish.
  bool zero_idle_timer_delay_ = false;

  // For idle timeouts. Invoked when UpdateStatus() is called after
  // |idle_time_|.
  base::RepeatingClosure idle_callback_;

  // Set to true once |idle_callback_| has been invoked. Set to false when
  // StartEvent() is called.
  bool did_idle_timeout_ = false;

  // Tasks that are to be run in the next ProcessTasks() call.
  Deque<std::unique_ptr<Task>> task_queue_;

  // Set to true during running ProcessTasks(). This is used for avoiding to
  // invoke |idle_callback_| or to re-enter ProcessTasks() when calling
  // ProcessTasks().
  bool processing_tasks_ = false;

  // The number of the living StayAwakeToken. See also class comments.
  int num_of_stay_awake_tokens_ = 0;

  // |timer_| invokes UpdateEventStatus() periodically.
  base::RepeatingTimer timer_;

  // |tick_clock_| outlives |this|.
  const base::TickClock* const tick_clock_;

  bool in_dtor_ = false;

  base::WeakPtrFactory<ServiceWorkerEventQueue> weak_factory_{this};
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_SERVICE_WORKER_SERVICE_WORKER_EVENT_QUEUE_H_
