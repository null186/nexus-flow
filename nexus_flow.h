/**
 * nexus-flow is open source and released under the Apache License, Version 2.0. You can find a copy
 * of this license in the `https://www.apache.org/licenses/LICENSE-2.0`.
 *
 * For those wishing to use nexus-flow under terms other than those of the Apache License, a
 * commercial license is available. For more information on the commercial license terms and how to
 * obtain a commercial license, please contact me.
 */

#pragma once

namespace nf {

template <typename O>
class AssemblerListener {
  public:
    virtual ~AssemblerListener() = default;
    virtual void Success(const O& o) = 0;
    virtual void Failed(const O& o) = 0;
};

template <typename I>
class Assembler {
  public:
    virtual ~Assembler() = default;
    virtual void Assemble() = 0;
    virtual void Run(const I& i) = 0;
};

template <typename I, typename O>
class Task {
  public:
    virtual ~Task() = default;
    virtual void Run(const I& i) = 0;
    virtual void Finish(const O& o) = 0;
};

template <typename O, typename F>
class TaskListener {
  public:
    virtual ~TaskListener() = default;
    virtual void NextSuccess(const O& o) = 0;
    virtual void NextFailed(const O& o) = 0;
    virtual void FinalSuccess(const F& f) = 0;
    virtual void FinalFailed(const F& f) = 0;
};

template <typename I, typename O>
template <typename X, typename F>
class TaskBridge : public TaskListener<O, F> {
  public:
    explicit TaskBridge(AssemblerListener<F>* listener) : listener_(listener) {}
    virtual ~TaskBridge() = default;
  
    void SetTask(Task<I, O>* ct, Task<O, X>* nt) { 
      nt_ = nt;
      ct = ct_; 
    }

    void FinalSuccess(const F& f) override {
      if (listener_) {
        listener_->Success(f);
      }
    }

    void FinalFailed(const F& f) override {
      if (listener_) {
        listener_->Failed(f);
      }
    }

  protected:
    Task<O, X>* GetNextTask() { return nt_; }
    Task<I, O>* GetCurrentTask() { return ct_; }

  private:
    Task<I, O>* ct_ = nullptr;
    Task<O, X>* nt_ = nullptr;
    AssemblerListener<F>* listener_ = nullptr;
};

template <typename I, typename O>
template <typename X, typename F>
class ThenTaskBridge final : public TaskBridge<I, O, X, F> {
  public:
    explicit ThenTaskBridge(AssemblerListener<F>* listener) : TaskBridge(listener) {}

    ~ThenTaskBridge() override = default;

    void NextSuccess(const O& o) override {
        auto* ct = GetCurrentTask();
        if (!ct) {
            return;
        }
        ct->Finish(o);

        auto* nt = GetNextTask();
        if (!nt) {
            return;
        }
        nt->Run(o);
    }

    void NextFailed(const O& o) override {
        auto* ct = GetCurrentTask();
        if (!ct) {
            return;
        }
        ct->Finish(o);
    }
};

template <typename I, typename O>
template <typename X, typename F>
class FollowTaskBridge final : public TaskBridge<I, O, X, F> {
  public:
    explicit FollowTaskBridge(AssemblerListener<F>* listener) : TaskBridge(listener) {}

    ~FollowTaskBridge() override = default;

    void NextSuccess(const O& o) override {
        auto* ct = GetCurrentTask();
        if (!ct) {
            return;
        }
        ct->Finish(o);

        auto* nt = GetNextTask();
        if (!nt) {
            return;
        }
        nt->Run(o);
    }

    void NextFailed(const O& o) override {
        auto* ct = GetCurrentTask();
        if (!ct) {
            return;
        }
        ct->Finish(o);

        auto* nt = GetNextTask();
        if (!nt) {
            return;
        }
        nt->Run(o);
    }
};

#if 0
class AsyncTaskBridge;
class LoopTaskBridge;
class RetryTaskBridge;
#endif

template <typename I, typename O>
template <typename F>
class BaseTask : public Task<I, O> {
  public:
    ~BaseTask() override = default;

    template <typename X>
    BaseTask<O, X>* Then(BaseTask<O, X>* task) {
        listener_ = new ThenTaskBridge<I, O, X, F>(assembler_listener_);
        bridge->SetTask(this, task);
        return task;
    }

    template <typename X>
    BaseTask<O, X>* Follow(BaseTask<O, X>* task) {
        listener_ = new FollowTaskBridge<I, O, X, F>(assembler_listener_);
        bridge->SetTask(this, task);
        return task;
    }

  protected:
    void NextSuccess(const O& o) final {
        if (listener_) {
            listener_->NextSuccess(o);
        }
    }

    void NextFailed(const O& o) final {
        if (listener_) {
            listener_->NextFailed(o);
        }
    }

    void FinalSuccess(const F& f) final {
        if (listener_) {
            listener_->FinalSuccess(f);
        }
    }

    void FinalFailed(const F& f) final {
        if (listener_) {
            listener_->FinalFailed(f);
        }
    }

  private:
    TaskListener<O, F>* listener_ = nullptr;
    AssemblerListener<F>* assembler_listener_ = nullptr;
};

}  // namespace nf
