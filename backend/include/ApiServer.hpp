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
    stopSSEWorkers();
  }

  void start(const std::string& host, uint16_t port);
  void shutdown();

 private:
  void registerRoutes();
  void stopSSEWorkers();

  ConfigManager& configManager_;
  SortingController& sortingController_;

  std::atomic_bool shuttingDown_{false};
  std::mutex workerMutex_;
  std::vector<std::unique_ptr<WorkerRecord>> workers_;
};
