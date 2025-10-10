/**
 * nexus-flow is open source and released under the Apache License, Version 2.0.
 * You can find a copy of this license in the
 * `https:
 *
 * For those wishing to use nexus-flow under terms other than those of the
 * Apache License, a commercial license is available. For more information on
 * the commercial license terms and how to obtain a commercial license, please
 * contact me.
 */

#include <iostream>

#include "nexus_flow.h"

struct OrderInfo {
  long order_id = 0;
  int item_count = 0;
  double total_amount = 0.0;
  bool inventory_reserved = false;
  std::string user_id;
  std::string stock_status = "PENDING";
};

struct FlowResult {
  long order_id = 0;
  bool success = false;
  std::string status;
  std::string detail;
};

class OrderFlowListener final : public nf::FinalListener<FlowResult> {
 public:
  void Success(const FlowResult& f) override {
    std::cout << "\n--- Order Flow Completed (Success) ---\n";
    std::cout << "Order ID: " << f.order_id << std::endl;
    std::cout << "Status: " << f.status << std::endl;
    std::cout << "Detail: " << f.detail << std::endl;
    std::cout << "--------------------------------------\n";
  }

  void Failed(const FlowResult& f) override {
    std::cout << "\n--- Order Flow Completed (Failed) ---\n";
    std::cout << "Order ID: " << f.order_id << std::endl;
    std::cout << "Status: " << f.status << std::endl;
    std::cout << "Error Detail: " << f.detail << std::endl;
    std::cout << "-------------------------------------\n";
  }
};

class QueryOrder final : public nf::BaseTask<long, OrderInfo, FlowResult> {
 public:
  void Run(const long& order_id) override {
    std::cout << " Run: Querying order " << order_id << std::endl;

    if (order_id % 2 != 0) {
      std::cout << " Simulating FAILED: Order not found." << std::endl;
      NextFailed({});

    } else {
      OrderInfo info;
      info.order_id = order_id;
      info.user_id = "User_" + std::to_string(order_id / 100);
      info.item_count = 5;
      info.total_amount = 120.50;
      std::cout << " SUCCESS: Order found. Total: " << info.total_amount
                << std::endl;
      NextSuccess(info);
    }
  }

  void Finish(const OrderInfo& o) override {
    std::cout << " Finish: Order query completed." << std::endl;
  }
};

class ReserveInventory final
    : public nf::BaseTask<OrderInfo, OrderInfo, FlowResult> {
 public:
  void Run(const OrderInfo& i) override {
    OrderInfo o = i;
    std::cout << " Run: Attempting inventory reservation for Order "
              << o.order_id << std::endl;

    if (o.total_amount > 100.0) {
      o.stock_status = "FAILED_OUT_OF_STOCK";
      o.inventory_reserved = false;
      std::cout << " FAILED: Inventory insufficient." << std::endl;
      NextFailed(o);
    } else {
      o.stock_status = "RESERVED";
      o.inventory_reserved = true;
      std::cout << " SUCCESS: Inventory reserved." << std::endl;
      NextSuccess(o);
    }
  }

  void Finish(const OrderInfo& o) override {
    std::cout << " Finish: Inventory task completed." << std::endl;
  }
};

class RiskCheck final : public nf::BaseTask<OrderInfo, FlowResult, FlowResult> {
 public:
  void Run(const OrderInfo& i) override {
    std::cout << " Run: Executing risk check for Order " << i.order_id
              << std::endl;

    FlowResult f;
    f.order_id = i.order_id;

    if (!i.inventory_reserved && i.stock_status == "FAILED_OUT_OF_STOCK") {
      f.success = false;
      f.status = "FAILED_INVENTORY";
      f.detail = "Risk check passed, but inventory reservation failed.";
      std::cout << " Conclusion: Inventory failed. Rejecting flow."
                << std::endl;
      FinalFailed(f);
    } else {
      f.success = true;
      f.status = "COMPLETED";
      f.detail = "Inventory reserved and risk check passed. Ready for payment.";
      std::cout << " Conclusion: Process ready for payment." << std::endl;
      FinalSuccess(f);
    }
  }

  void Finish(const FlowResult& o) override {
    std::cout << " Finish: Final risk check completed." << std::endl;
  }
};

class OrderFlowAssembler final : public nf::Assembler<long> {
 public:
  OrderFlowAssembler() {
    final_listener_ = std::make_shared<OrderFlowListener>();
  }

  void Assemble() override {
    task_a_ = std::make_shared<QueryOrder>();
    task_b_ = std::make_shared<ReserveInventory>();
    task_c_ = std::make_shared<RiskCheck>();

    task_a_->SetFinalListener(final_listener_);
    task_b_->SetFinalListener(final_listener_);
    task_c_->SetFinalListener(final_listener_);

    task_a_->Then<OrderInfo>(task_b_)->Follow<FlowResult>(task_c_);
  }

  void Run(const long& initial_input) override {
    if (!task_a_) {
      std::cerr << "Error: Assembly not run. Call Assemble() first."
                << std::endl;
      return;
    }
    std::cout << "\n===== Flow Started (Order ID: " << initial_input
              << ") =====" << std::endl;
    task_a_->Run(initial_input);
  }

 private:
  std::shared_ptr<QueryOrder> task_a_;
  std::shared_ptr<ReserveInventory> task_b_;
  std::shared_ptr<RiskCheck> task_c_;
  std::shared_ptr<OrderFlowListener> final_listener_;
};

int main() {
  OrderFlowAssembler assembler;
  assembler.Assemble();

  std::cout << "\n\n--- TEST CASE 1: Full Success (Order ID 100) ---\n";
  assembler.Run(100);

  std::cout << "\n\n--- TEST CASE 2: Early Then Failure (Order ID 101) ---\n";
  assembler.Run(101);

  std::cout << "\n\n--- TEST CASE 3: Follow Path Failure (Order ID 200) ---\n";
  assembler.Run(200);

  return 0;
}
