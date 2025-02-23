/**
 * nexus-flow is open source and released under the Apache License, Version 2.0. You can find a copy
 * of this license in the `https://www.apache.org/licenses/LICENSE-2.0`.
 *
 * For those wishing to use nexus-flow under terms other than those of the Apache License, a
 * commercial license is available. For more information on the commercial license terms and how to
 * obtain a commercial license, please contact me.
 */

#include <iostream>

#include "nexus_flow.h"

namespace nf {

struct TestParam {
    std::string success = "Task success";
    std::string failed = "Task failed";
};

class FirstTask final : public BaseTask<TestParam, std::string> {
  public:
    explicit FirstTask(TaskContext* tc) : BaseTask<TestParam, std::string>(tc) {}

    ~FirstTask() override = default;

    void Run() override {
        std::cout << "First task start." << std::endl;
        BaseTask<TestParam, std::string>::Success(params_.success);
    }

    void OnFinish(const std::string& param) override {
        std::cout << "First task finish, string is " << param << std::endl;
    }
};

class SecondTask final : public BaseTask<std::string, int> {
  public:
    explicit SecondTask(TaskContext* tc) : BaseTask<std::string, int>(tc) {}

    ~SecondTask() override = default;

    void Run() override {
        std::cout << "Second task start." << std::endl;
        if (params_ == "Task success") {
            BaseTask<std::string, int>::Success(1);
        } else {
            BaseTask<std::string, int>::Failed(0);
        }
    }

    void OnFinish(const int& param) override {
        std::cout << "Second task finish, value is " << param << std::endl;
    }
};

class ThirdTask final : public BaseTask<int, bool> {
  public:
    explicit ThirdTask(TaskContext* tc) : BaseTask<int, bool>(tc) {}

    ~ThirdTask() override = default;

    void Run() override {
        std::cout << "Third task start." << std::endl;
        if (params_ == 1) {
            BaseTask<int, bool>::Success(true);
        } else {
            BaseTask<int, bool>::Failed(false);
        }
    }

    void OnFinish(const bool& param) override {
        std::cout << "Third task finish, bool is " << param << std::endl;
    }
};

}  // namespace nf

int main() {
    nf::TaskContext tc;
    auto* first_task = new nf::FirstTask(&tc);
    auto* second_task = new nf::SecondTask(&tc);
    auto* third_task = new nf::ThirdTask(&tc);

    nf::TestParam param;
    first_task->SetParam(param);
    first_task->Follow(second_task)->Then(third_task);
    first_task->Run();

    delete third_task;
    delete second_task;
    delete first_task;

    return 0;
}
