/**
 * message-thread is open source and released under the Apache License, Version 2.0.
 * You can find a copy of this license in the `https://www.apache.org/licenses/LICENSE-2.0`.
 *
 * For those wishing to use message-thread under terms other than those of the Apache License, a
 * commercial license is available. For more information on the commercial license terms and how to
 * obtain a commercial license, please contact me.
 */

#include <iostream>

#include "nexus_flow.h"

namespace nf {

struct CustomParam {
    std::string s = "Hello World!";
};

class FirstTask final : public BaseTask<CustomParam, int> {
  public:
    explicit FirstTask(TaskContext* tc) : BaseTask<CustomParam, int>(tc) {}

    ~FirstTask() override = default;

    void Start() override {
        std::cout << "First task start." << std::endl;
        if (params_.s == "Hello World!") {
            TaskSuccess(1);
        } else {
            TaskFailed(0);
        }
    }

    void Finish() override {}

    void TaskSuccess(const int& param) override { BaseTask<CustomParam, int>::TaskSuccess(param); }

    void TaskFailed(const int& param) override { BaseTask<CustomParam, int>::TaskFailed(param); }
};

class SecondTask final : public BaseTask<int, std::string> {
  public:
    explicit SecondTask(TaskContext* tc) : BaseTask<int, std::string>(tc) {}

    ~SecondTask() override = default;

    void Start() override { std::cout << "Second task start." << std::endl; }

    void Finish() override {}

    void TaskSuccess(const std::string& param) override {
        BaseTask<int, std::string>::TaskSuccess(param);
    }

    void TaskFailed(const std::string& param) override {
        BaseTask<int, std::string>::TaskFailed(param);
    }
};

class AssemblerTask : public BaseTask<CustomParam, CustomParam> {
  public:
    explicit AssemblerTask(TaskContext* tc) : BaseTask<CustomParam, CustomParam>(tc) {
        first_task_ = new FirstTask(tc);
        second_task_ = new SecondTask(tc);
    }

    ~AssemblerTask() override { delete first_task_; }

    void Assembler() {
        BaseTask<CustomParam, CustomParam>::Follow(first_task_)->Then(second_task_);
    }

    void SetParam(const CustomParam& param) override {
        BaseTask<CustomParam, CustomParam>::SetParam(param);
    }

    void Start() override {
        BaseTask<CustomParam, CustomParam>::TaskFailed(BaseTask<CustomParam, CustomParam>::params_);
        BaseTask<CustomParam, CustomParam>::TaskSuccess(
                BaseTask<CustomParam, CustomParam>::params_);
    }

    void Finish() override {
        // TODO
    }

  private:
    FirstTask* first_task_ = nullptr;
    SecondTask* second_task_ = nullptr;
};

}  // namespace nf

int main() {
    nf::CustomParam param;
    nf::TaskContext tk;
    auto at = nf::AssemblerTask(&tk);
    at.Assembler();
    at.SetParam(param);
    at.Start();

    return 0;
}
