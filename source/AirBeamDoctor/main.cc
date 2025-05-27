#include <sys/fcntl.h>

#include <atomic>
#include <chrono>
#include <fstream>
#include <ios>
#include <iostream>
#include <thread>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/log/check.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/log/log_sink.h"
#include "absl/log/log_sink_registry.h"
#include "absl/strings/str_split.h"
#include "macos/bonjour_browse.h"
#include "raop/codec.h"
#include "raop/fifo.h"
#include "raop/raop.h"

ABSL_FLAG(std::string, audio_pcm, "", "Path to the audio PCM file.");
ABSL_FLAG(std::string, log, "", "Log to this file. ");

using namespace AirBeamCore::raop;
using namespace AirBeamCore::macos;

#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#include "absl/log/log_sink.h"

class FileLogSink : public absl::LogSink {
 public:
  explicit FileLogSink(const std::string& filename) {
    absl::AddLogSink(this);

    // Open the log file
    fd_ = open(filename.c_str(), O_WRONLY | O_TRUNC | O_CREAT | O_APPEND,
               0744);  // open or create file for writing
    if (fd_ == -1) {
      std::cerr << "Failed to open log file: " << filename << std::endl;
    }

    if (fd_ != -1) {
      dup2(fd_, STDERR_FILENO);
    } else {
      std::cout << "Failed to open log file for stderr redirection."
                << std::endl;
    }
  }

  ~FileLogSink() override {
    absl::RemoveLogSink(this);
    if (fd_ != -1) {
      close(fd_);
      fd_ = -1;
    }
  }

  void Send(const absl::LogEntry& entry) override {
    std::string_view message = entry.text_message_with_prefix();
    std::cout << message;
    std::cout.flush();

    if (fd_ == -1) return;
    write(fd_, message.data(), message.size());
    fsync(fd_);
  }

 private:
  int fd_ = -1;
};

void TryBonjourBrowse(std::vector<BonjourBrowse::ServiceInfo>& found_services) {
  BonjourBrowse browser;
  auto callback = [&](BonjourBrowse::EventType event_type,
                      const BonjourBrowse::ServiceInfo& service_info) {
    if (event_type == BonjourBrowse::EventType::kServiceOnline) {
      LOG(INFO) << "[Bonjour] Found: " << service_info.name
                << ", IP: " << service_info.ip
                << ", Port: " << service_info.port << std::endl;
      found_services.push_back(service_info);
    } else if (event_type == BonjourBrowse::EventType::kServiceOffline) {
      LOG(INFO) << "[Bonjour] Offline: " << service_info.name << std::endl;
    }
  };
  bool started = browser.startBrowse("_raop._tcp", callback);
  CHECK(started) << "Failed to start Bonjour browsing.";

  std::this_thread::sleep_for(std::chrono::seconds(3));
  browser.stop();

  LOG(INFO) << "Found " << found_services.size() << " services.";
}

void TryChooseService(const std::vector<BonjourBrowse::ServiceInfo>& services,
                      BonjourBrowse::ServiceInfo& chosen) {
  CHECK(services.size() > 0) << "No service found.";

  LOG(INFO) << "Available services:" << std::endl;
  for (size_t i = 0; i < services.size(); ++i) {
    LOG(INFO) << "[" << i << "] " << services[i].name
              << ", IP: " << services[i].ip << ", Port: " << services[i].port
              << std::endl;
  }

  size_t choice;
  while (true) {
    LOG(INFO) << "Choose a service (enter the number): ";
    std::cin >> choice;

    if (std::cin.fail()) {
      std::cin.clear();
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      LOG(INFO) << "Invalid input. Please enter a number." << std::endl;
      continue;
    }

    if (choice < services.size()) {
      chosen = services[choice];
      break;
    } else {
      LOG(INFO) << "Invalid choice. Please enter a number between 0 and "
                << services.size() - 1 << "." << std::endl;
    }
  }

  LOG(INFO) << "choice=" << choice << ", chosen=" << chosen.name;
  LOG(INFO) << "Service chosen: " << chosen.name << ", IP: " << chosen.ip
            << ", Port: " << chosen.port << std::endl;
}

void TryRaop(const BonjourBrowse::ServiceInfo& service,
             const std::string& audio_pcm_path) {
  Raop raop(service.ip, service.port);
  raop.Start();
  raop.SetVolume(30);

  LOG(INFO) << "Service Connected";

  constexpr size_t kFiFOCapacity = 30 * 1024 * 1024;  // 30MB buffer

  ConcurrentByteFIFO fifo(kFiFOCapacity);
  std::ifstream ifs(audio_pcm_path, std::ios::binary);

  CHECK(ifs.is_open()) << "Failed to open file: " << audio_pcm_path;

  std::atomic<bool> write_done = false;
  std::thread producer([&]() {
    char chunk[1024];
    while (ifs.read(chunk, sizeof(chunk)) || ifs.gcount() > 0) {
      fifo.Write(reinterpret_cast<uint8_t*>(chunk), ifs.gcount());
    }

    if (ifs.eof()) {
      LOG(INFO) << "Finished reading file.";
    } else if (ifs.fail()) {
      LOG(ERROR) << "Error reading file.";
    }

    ifs.close();
    write_done.store(true);
  });
  producer.detach();

  RtpAudioPacketChunk chunk, encoded;
  while (!write_done.load() || !fifo.Empty()) {
    size_t size = fifo.Read(chunk.data_, sizeof(chunk.data_),
                            std::chrono::milliseconds(100));
    chunk.len_ = size;
    PCMCodec::Encode(chunk, encoded);
    encoded.len_ = chunk.len_;
    raop.AcceptFrame();
    raop.SendChunk(encoded);
  }

  LOG(INFO) << "Finished sending audio.";
}

int main(int argc, char* argv[]) {
  absl::SetProgramUsageMessage("AirBeamDoctor");
  absl::ParseCommandLine(argc, argv);

  // audio_pcm file should be raw, 16bit, signed-integer, stereo, 44100 sample
  // rate, e.g.:
  // brew install sox
  // play -t raw -b 16 -e signed-integer -c 2 -r 44100 ./resources/audio.pcm
  std::string audio_pcm_path = absl::GetFlag(FLAGS_audio_pcm);
  std::string log_path = absl::GetFlag(FLAGS_log);

  if (audio_pcm_path.empty() || log_path.empty()) {
    LOG(INFO) << "Both --audio_pcm and --log must be specified." << std::endl;
    return 1;
  }

  absl::InitializeLog();
  FileLogSink file_sink(log_path);

  LOG(INFO) << "Begin Doctor";

  int ret = 0;
  std::vector<BonjourBrowse::ServiceInfo> found_services;
  BonjourBrowse::ServiceInfo chosen_service;

  TryBonjourBrowse(found_services);
  TryChooseService(found_services, chosen_service);
  TryRaop(chosen_service, audio_pcm_path);

  return 0;
}