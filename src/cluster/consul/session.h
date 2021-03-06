#ifndef VIYA_CLUSTER_SESSION_H_
#define VIYA_CLUSTER_SESSION_H_

#include <memory>
#include <functional>
#include "util/schedule.h"

namespace viya {
namespace cluster {

class Consul;

class Session {
  public:
    Session(const Consul& consul, const std::string& name,
        std::function<void(const Session&)> on_create, uint32_t ttl_sec);
    Session(const Session& other) = delete;
    ~Session();

    void Renew();
    bool EphemeralKey(const std::string& key, const std::string& value) const;

    const std::string& id() const { return id_; }

  private:
    void Destroy();
    void Create();

  private:
    const Consul& consul_;
    const std::string name_;
    std::function<void(const Session&)> on_create_;
    const uint32_t ttl_sec_;
    std::string id_;
    std::unique_ptr<util::Repeat> repeat_;
};

}}

#endif // VIYA_CLUSTER_SESSION_H_
