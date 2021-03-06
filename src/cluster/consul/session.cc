#include <stdexcept>
#include <json.hpp>
#include <cpr/cpr.h>
#include <glog/logging.h>
#include "cluster/consul/consul.h"

namespace viya {
namespace cluster {

using json = nlohmann::json;

Session::Session(const Consul& consul, const std::string& name,
                 std::function<void(const Session&)> on_create, uint32_t ttl_sec):
  consul_(consul),name_(name),on_create_(on_create),ttl_sec_(ttl_sec),repeat_(nullptr) {

  Create();

  repeat_ = std::make_unique<util::Repeat>((ttl_sec_ * 2000L) / 3, [this]() {
    Renew();
  });
}

Session::~Session() {
  repeat_.reset();
  Destroy();
}

void Session::Destroy() {
  LOG(INFO)<<"Destroying session '"<<id_<<"'";
  cpr::Put(
    cpr::Url { consul_.url() + "/v1/session/destroy/" + id_ }
  );
}

void Session::Create() {
  json data = {
    { "Name", name_ },
    { "Checks", json::array() },
    { "LockDelay", "0s" },
    { "Behavior", "delete" },
    { "TTL", std::to_string(ttl_sec_) + "s" }
  };

  LOG(INFO)<<"Creating new session '"<<name_<<"' with TTL="<<ttl_sec_<<"sec";
  auto r = cpr::Put(
    cpr::Url { consul_.url() + "/v1/session/create" },
    cpr::Body { data.dump() },
    cpr::Header {{ "Content-Type", "application/json" }}
  );
  if (r.status_code != 200) {
    if (r.status_code == 0) {
      throw std::runtime_error("Can't contact Consul (host is unreachable)");
    }
    throw std::runtime_error("Can't register new session (" + r.text + ")");
  }

  json response = json::parse(r.text);
  id_ = response["ID"];
  LOG(INFO)<<"Created new session: "<<id_;

  if (on_create_) {
    on_create_(const_cast<const Session&>(*this));
  }
}

void Session::Renew() {
  DLOG(INFO)<<"Renewing session '"<<id_<<"'";
  auto r = cpr::Put(
    cpr::Url { consul_.url() + "/v1/session/renew/" + id_ }
  );

  if (r.status_code == 0) {
    LOG(WARNING)<<"Can't contact Consul (host is unreachable)";
  }
  else if (r.status_code == 404) {
    LOG(WARNING)<<"Session '"<<id_<<"' was invalidated externally";
    Create();
  }
}

bool Session::EphemeralKey(const std::string& key, const std::string& value) const {
  std::string target_key = consul_.prefix() + "/" + key;
  DLOG(INFO)<<"Acquiring lock on key '"<<target_key<<"' for session '"<<id_;

  auto r = cpr::Put(
    cpr::Url { consul_.url() + "/v1/kv/" + target_key },
    cpr::Body { value },
    cpr::Parameters {{ "acquire", id_ }},
    cpr::Header {{ "Content-Type", "application/json" }}
  );

  if (r.status_code != 200) {
    if (r.status_code == 0) {
      throw std::runtime_error("Can't contact Consul (host is unreachable)");
    }
    throw std::runtime_error("Can't acquire lock on key (" + r.text + ")");
  }
  return r.text.compare(0, 4, "true") == 0;
}

}}
