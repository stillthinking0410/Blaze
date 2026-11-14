#pragma once

#include <drogon/drogon.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ConfigManager.hpp"
#include "SortingController.hpp"

class ApiServer {
  struct WorkerRecord {
    std::thread thread;
    std::atomic_bool stopRequested{false};
    std::atomic_bool finished{false};
  };

 public:
  ApiServer(ConfigManager& configManager, SortingController& sortingController);

  ~ApiServer() {
    std::vector<std::unique_ptr<WorkerRecord>> workers;

    {
      std::lock_guard<std::mutex> lock(workerMutex_);
      shuttingDown_.store(true);

      for (const auto& worker : workers_) {
        worker->stopRequested.store(true);
      }

      workers.swap(workers_);
    }

    for (const auto& worker : workers) {
      if (worker->thread.joinable()) {
        worker->thread.join();
      }
    }
  }

  void start(const std::string& host, uint16_t port);

 private:
  void registerRoutes();

  ConfigManager& configManager_;
  SortingController& sortingController_;

  std::atomic_bool shuttingDown_{false};
  std::mutex workerMutex_;
  std::vector<std::unique_ptr<WorkerRecord>> workers_;
};
