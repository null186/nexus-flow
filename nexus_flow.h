/**
 * message-thread is open source and released under the Apache License,
 * Version 2.0. You can find a copy of this license in the
 * `https://www.apache.org/licenses/LICENSE-2.0`.
 *
 * For those wishing to use message-thread under terms other than those of the
 * Apache License, a commercial license is available. For more information on
 * the commercial license terms and how to obtain a commercial license, please
 * contact me.
 */

#pragma once

#include <map>
#include <string>

#define EXPORT __attribute__((visibility("default")))
#define UNUSED __attribute__((unused))

namespace nf {

class TaskContext {
  public:
    TaskContext() = default;

    ~TaskContext() = default;

    [[nodiscard]] const std::string& GetName() const { return name_; }

  private:
    std::string name_;
};

template <typename I, typename O>
class Task {
  public:
    Task() = default;

    virtual ~Task() = default;

    /**
     * Set task param.
     *
     * @param param Task param.
     */
    virtual void SetParam(const I& param) = 0;

    /**
     * Task start.
     */
    virtual void Start() = 0;

    /**
     * Task finish.
     */
    virtual void Finish() = 0;

    /**
     * Task success.
     *
     * @param param Input param.
     */
    virtual void TaskSuccess(const O& param) = 0;

    /**
     * Task failed.
     */
    virtual void TaskFailed(const O& param) = 0;

    /**
     * Task failed.
     */
    virtual void TaskFailed() = 0;
};

template <typename O>
class TaskListener {
  public:
    TaskListener() = default;

    virtual ~TaskListener() = default;

    /**
     * Task success callback.
     * @param param The input data type of the next task.
     */
    virtual void OnSuccess(O param) = 0;

    /**
     * Task failed callback.
     * @param param The input data type of the next task.
     */
    virtual void OnFailed(O param) = 0;

    /**
     * Task failed callback.
     */
    virtual void OnFailed() = 0;
};

/**
 * Task bridge.
 * @tparam O The output data type of the current task is also the input data
 * type of the next task.
 * @tparam X The output data type of the next task.
 */
template <typename O, typename X>
class TaskBridge : public TaskListener<O> {
  public:
    TaskBridge() = default;

    ~TaskBridge() override = default;

    void SetTask(Task<O, X>* task) { next_task_ = task; }

    Task<O, X>* GetTask() { return next_task_; }

  private:
    Task<O, X>* next_task_ = nullptr;
};

/**
 * Then mode task bridge.
 * If the current task succeeds, the next task is continued.
 * If the current task fails, the task is not continued.
 */
template <typename O, typename X>
class ThenTaskBridge final : public TaskBridge<O, X> {
  public:
    ThenTaskBridge() = default;

    ~ThenTaskBridge() override = default;

    void OnSuccess(O param) override {
        auto* next_task = TaskBridge<O, X>::GetTask();
        if (!next_task) {
            return;
        }
        next_task->SetParam(param);
        next_task->Start();
    }

    void OnFailed(O UNUSED param) override {
        // TODO: 临时方案
    }

    void OnFailed() override {
        auto* next_task = TaskBridge<O, X>::GetTask();
        if (!next_task) {
            return;
        }
        next_task->TaskFailed();
    }
};

/**
 * Follow mode task bridge.
 * Continue to execute the next task regardless of whether the current task is
 * successfully executed.
 */
template <typename O, typename X>
class FollowTaskBridge final : public TaskBridge<O, X> {
  public:
    FollowTaskBridge() = default;

    ~FollowTaskBridge() override = default;

    void OnSuccess(O param) override {
        auto* next_task = TaskBridge<O, X>::GetTask();
        if (!next_task) {
            return;
        }
        next_task->SetParam(param);
        next_task->Start();
    }

    void OnFailed(O param) override {
        auto* next_task = TaskBridge<O, X>::GetTask();
        if (!next_task) {
            return;
        }
        next_task->SetParam(param);
        next_task->Start();
    }

    void OnFailed() override {
        // TODO: 临时方案
    }
};

template <typename O, typename X>
class AsyncTaskBridge : public TaskBridge<O, X> {
    // TODO: 待实现
};

template <typename O, typename X>
class LoopTaskBridge : public TaskBridge<O, X> {
    // TODO: 待实现
};

template <typename O, typename X>
class RetryTaskBridge : public TaskBridge<O, X> {
    // TODO: 待实现
};

template <typename I, typename O>
class BaseTask : public Task<I, O> {
  public:
    explicit BaseTask(TaskContext* tc) : task_context_(tc) {}

    ~BaseTask() override {
        if (listener_) {
            delete listener_;
            listener_ = nullptr;
        }
    }

    /**
     * The task is bridged in Then mode.
     * @tparam X The output data type of the next task.
     * @param task Next task.
     * @return Next task.
     */
    template <typename X>
    BaseTask<O, X>* Then(BaseTask<O, X>* task) {
        auto* bridge_ = new ThenTaskBridge<O, X>();
        bridge_->SetTask(task);
        listener_ = bridge_;
        return task;
    }

    /**
     * The task is bridged in Follow mode.
     * @tparam X The output data type of the next task.
     * @param task Next task.
     * @return Next task.
     */
    template <typename X>
    BaseTask<O, X>* Follow(BaseTask<O, X>* task) {
        auto* bridge_ = new FollowTaskBridge<O, X>();
        bridge_->SetTask(task);
        listener_ = bridge_;
        return task;
    }

    void SetParam(const I& param) override {
        // TODO: 拷贝多次浪费性能。
        params_ = param;
    }

    void TaskSuccess(const O& param) override {
        if (listener_) {
            listener_->OnSuccess(param);
        }
    }

    void TaskFailed(const O& param) override {
        if (listener_) {
            listener_->OnFailed(param);
        }
    }

    void TaskFailed() override {
        if (listener_) {
            listener_->OnFailed();
        }
    }

  protected:
    TaskContext* task_context_ = nullptr;
    I params_;

  private:
    TaskListener<O>* listener_ = nullptr;
};

}  // namespace nf
