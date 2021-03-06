#include <chrono>
#include <json.hpp>
#include <glog/logging.h>
#include "cluster/worker.h"
#include "util/hostname.h"

namespace viya {
namespace cluster {

using json = nlohmann::json;

Worker::Worker(const util::Config& config):config_(config),consul_(config) {
  if (consul_.Enabled()) {
    session_ = consul_.CreateSession(std::string("viyadb-worker"), [this](const Session& session) {
      CreateKey(session);
    });
  }
}

void Worker::CreateKey(const Session& session) const {
  std::string hostname = util::get_hostname();
  std::ostringstream key;
  key<<config_.str("cluster_id")<<"/nodes/workers/"<<hostname<<":"<<std::to_string(config_.num("http_port"));

  json data = json({});
  if (config_.exists("rack_id")) {
    data["rack_id"] = config_.str("rack_id");
  }
  if (config_.exists("cpu_list")) {
    data["cpu_list"] = config_.numlist("cpu_list");
  }
  data["http_port"] = config_.num("http_port");
  data["hostname"] = hostname;

  while (!session.EphemeralKey(key.str(), data.dump())) {
    LOG(WARNING)<<"The worker key is still locked by the previous process... waiting";
    std::this_thread::sleep_for(std::chrono::milliseconds(10000));
  }
}

}}
