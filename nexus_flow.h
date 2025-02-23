/**
 * nexus-flow is open source and released under the Apache License, Version 2.0. You can find a copy
 * of this license in the `https://www.apache.org/licenses/LICENSE-2.0`.
 *
 * For those wishing to use nexus-flow under terms other than those of the Apache License, a
 * commercial license is available. For more information on the commercial license terms and how to
 * obtain a commercial license, please contact me.
 */

#pragma once

#include <chrono>
#include <future>
#include <string>
#include <thread>

#define UNUSED __attribute__((unused))

namespace nf {

class TaskContext {
  public:
    TaskContext() = default;

    ~TaskContext() = default;

    // TODO: Add something.
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
     * Task run.
     */
    virtual void Run() = 0;

    /**
     * Task success.
     *
     * @param param Input param.
     */
    virtual void Success(const O& param) = 0;

    /**
     * Task failed.
     *
     * @param param Input param.
     */
    virtual void Failed(const O& param) = 0;

    /**
     * Task finish.
     */
    virtual void OnFinish(const O& param) = 0;
};

template <typename O>
class TaskListener {
  public:
    TaskListener() = default;

    virtual ~TaskListener() = default;

    /**
     * Task success callback.
     *
     * @param param The input data type of the next task.
     */
    virtual void OnSuccess(const O& param) = 0;

    /**
     * Task failed callback.
     *
     * @param param The input data type of the next task.
     */
    virtual void OnFailed(const O& param) = 0;
};

/**
 * Task bridge.
 *
 * @tparam O The input data type of the current task.
 * @tparam O The output data type of the current task is also the input data type of the next task.
 * @tparam X The output data type of the next task.
 */
template <typename I, typename O, typename X>
class TaskBridge : public TaskListener<O> {
  public:
    TaskBridge() = default;

    ~TaskBridge() override = default;

    void SetNextTask(Task<O, X>* task) { next_task_ = task; }

    Task<O, X>* GetNextTask() { return next_task_; }

    void SetCurrentTask(Task<I, O>* task) { current_task_ = task; }

    Task<I, O>* GetCurrentTask() { return current_task_; }

  private:
    Task<I, O>* current_task_ = nullptr;
    Task<O, X>* next_task_ = nullptr;
};

/**
 * Then mode task bridge.
 * If the current task succeeds, the next task is continued.
 * If the current task fails, the task is not continued.
 */
template <typename I, typename O, typename X>
class ThenTaskBridge final : public TaskBridge<I, O, X> {
  public:
    ThenTaskBridge() = default;

    ~ThenTaskBridge() override = default;

    void OnSuccess(const O& param) override {
        auto* current_task = TaskBridge<I, O, X>::GetCurrentTask();
        if (!current_task) {
            return;
        }
        current_task->OnFinish(param);

        auto* next_task = TaskBridge<I, O, X>::GetNextTask();
        if (!next_task) {
            return;
        }
        next_task->SetParam(param);
        next_task->Run();
    }

    void OnFailed(const O& UNUSED param) override {
        auto* current_task = TaskBridge<I, O, X>::GetCurrentTask();
        if (!current_task) {
            return;
        }
        current_task->OnFinish(param);
    }
};

/**
 * Follow mode task bridge.
 * Continue to execute the next task regardless of whether the current task is successfully
 * executed.
 */
template <typename I, typename O, typename X>
class FollowTaskBridge final : public TaskBridge<I, O, X> {
  public:
    FollowTaskBridge() = default;

    ~FollowTaskBridge() override = default;

    void OnSuccess(const O& param) override {
        auto* current_task = TaskBridge<I, O, X>::GetCurrentTask();
        if (!current_task) {
            return;
        }
        current_task->OnFinish(param);

        auto* next_task = TaskBridge<I, O, X>::GetNextTask();
        if (!next_task) {
            return;
        }
        next_task->SetParam(param);
        next_task->Run();
    }

    void OnFailed(const O& param) override {
        auto* current_task = TaskBridge<I, O, X>::GetCurrentTask();
        if (!current_task) {
            return;
        }
        current_task->OnFinish(param);

        auto* next_task = TaskBridge<I, O, X>::GetNextTask();
        if (!next_task) {
            return;
        }
        next_task->SetParam(param);
        next_task->Run();
    }
};

#if 0
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
  public:
    // TODO: 待实现

  private:
    int max_retries_ = 0;
    int retry_count_ = 0;
    std::chrono::milliseconds delay_ = std::chrono::milliseconds(0);
};
#endif

template <typename I, typename O>
class BaseTask : public Task<I, O> {
  public:
    explicit BaseTask(TaskContext* tc) : task_context_(tc) {}

    ~BaseTask() override = default;

    /**
     * The task is bridged in Then mode.
     *
     * @tparam X The output data type of the next task.
     * @param task Next task.
     * @return Next task.
     */
    template <typename X>
    BaseTask<O, X>* Then(BaseTask<O, X>* task) {
        std::unique_ptr<ThenTaskBridge<I, O, X>> bridge(new ThenTaskBridge<I, O, X>());
        bridge->SetCurrentTask(this);
        bridge->SetNextTask(task);
        listener_ = std::move(bridge);
        return task;
    }

    /**
     * The task is bridged in Follow mode.
     *
     * @tparam X The output data type of the next task.
     * @param task Next task.
     * @return Next task.
     */
    template <typename X>
    BaseTask<O, X>* Follow(BaseTask<O, X>* task) {
        std::unique_ptr<FollowTaskBridge<I, O, X>> bridge(new FollowTaskBridge<I, O, X>());
        bridge->SetCurrentTask(this);
        bridge->SetNextTask(task);
        listener_ = std::move(bridge);
        return task;
    }

    void SetParam(const I& param) final {
        // TODO: 拷贝浪费性能。
        params_ = param;
    }

  protected:
    void Success(const O& param) final {
        if (listener_) {
            listener_->OnSuccess(param);
        }
    }

    void Failed(const O& param) final {
        if (listener_) {
            listener_->OnFailed(param);
        }
    }

  protected:
    TaskContext* UNUSED task_context_ = nullptr;
    I params_;

  private:
    std::unique_ptr<TaskListener<O>> listener_;
};

}  // namespace nf
