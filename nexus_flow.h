/**
 * nexus-flow is open source and released under the Apache License, Version 2.0.
 * You can find a copy of this license in the
 * `https://www.apache.org/licenses/LICENSE-2.0`.
 *
 * For those wishing to use nexus-flow under terms other than those of the
 * Apache License, a commercial license is available. For more information on
 * the commercial license terms and how to obtain a commercial license, please
 * contact me.
 */

#pragma once

namespace nf {

template <typename F>
class FinalListener {
 public:
  virtual ~FinalListener() = default;
  virtual void Success(const F& f) = 0;
  virtual void Failed(const F& f) = 0;
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

template <typename I, typename O, typename X, typename F>
class TaskBridge : public TaskListener<O, F> {
 public:
  TaskBridge(Task<I, O>* ct, Task<O, X>* nt,
             std::shared_ptr<FinalListener<F>> fl)
      : ct_(ct), nt_(nt), fl_(fl) {}
  ~TaskBridge() override = default;

  void NextSuccess(const O& o) override {
    if (!ct_) {
      return;
    }
    ct_->Finish(o);

    if (!nt_) {
      return;
    }
    nt_->Run(o);
  }

  void FinalSuccess(const F& f) override {
    if (fl_) {
      fl_->Success(f);
    }
  }

  void FinalFailed(const F& f) override {
    if (fl_) {
      fl_->Failed(f);
    }
  }

 protected:
  Task<I, O>* GetCurrentTask() { return ct_; }
  Task<O, X>* GetNextTask() { return nt_; }

 private:
  Task<I, O>* ct_ = nullptr;
  Task<O, X>* nt_ = nullptr;
  std::shared_ptr<FinalListener<F>> fl_;
};

template <typename I, typename O, typename X, typename F>
class ThenTaskBridge final : public TaskBridge<I, O, X, F> {
 public:
  ThenTaskBridge(Task<I, O>* ct, Task<O, X>* nt,
                 std::shared_ptr<FinalListener<F>> fl)
      : TaskBridge<I, O, X, F>(ct, nt, fl) {}
  ~ThenTaskBridge() override = default;

  void NextFailed(const O& o) override {
    auto ct = TaskBridge<I, O, X, F>::GetCurrentTask();
    if (!ct) {
      return;
    }
    ct->Finish(o);
  }
};

template <typename I, typename O, typename X, typename F>
class FollowTaskBridge final : public TaskBridge<I, O, X, F> {
 public:
  FollowTaskBridge(Task<I, O>* ct, Task<O, X>* nt,
                   std::shared_ptr<FinalListener<F>> fl)
      : TaskBridge<I, O, X, F>(ct, nt, fl) {}
  ~FollowTaskBridge() override = default;

  void NextFailed(const O& o) override {
    TaskBridge<I, O, X, F>::NextSuccess(o);
  }
};

template <typename I, typename O, typename F>
class BaseTask : public Task<I, O> {
 public:
  ~BaseTask() override = default;

  template <typename X>
  BaseTask<O, X, F>* Then(BaseTask<O, X, F>* nt) {
    auto bridge = std::make_unique<ThenTaskBridge<I, O, X, F>>(this, nt, fl_);
    l_ = std::move(bridge);
    return nt;
  }

  template <typename X>
  BaseTask<O, X, F>* Follow(BaseTask<O, X, F>* nt) {
    auto bridge = std::make_unique<FollowTaskBridge<I, O, X, F>>(this, nt, fl_);
    l_ = std::move(bridge);
    return nt;
  }

  void SetFinalListener(std::shared_ptr<FinalListener<F>> fl) { fl_ = fl; }

 protected:
  void NextSuccess(const O& o) {
    if (l_) {
      l_->NextSuccess(o);
    }
  }

  void NextFailed(const O& o) {
    if (l_) {
      l_->NextFailed(o);
    }
  }

  void FinalSuccess(const F& f) {
    if (l_) {
      l_->FinalSuccess(f);
    }
  }

  void FinalFailed(const F& f) {
    if (l_) {
      l_->FinalFailed(f);
    }
  }

 private:
  std::unique_ptr<TaskListener<O, F>> l_;
  std::shared_ptr<FinalListener<F>> fl_;
};

}  // namespace nf
