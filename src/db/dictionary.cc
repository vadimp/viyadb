#include "db/dictionary.h"

namespace viya {
namespace db {

DimensionDict::DimensionDict(const NumericType& code_type):code_type_(code_type) {
  std::string exceeded_value("__exceeded");
  c2v_.push_back(exceeded_value);

  switch (code_type.size()) {
    case NumericType::Size::_1:
      v2c_ = new DictImpl<uint8_t>(exceeded_value);
      break;
    case NumericType::Size::_2:
      v2c_ = new DictImpl<uint16_t>(exceeded_value);
      break;
    case NumericType::Size::_4:
      v2c_ = new DictImpl<uint32_t>(exceeded_value);
      break;
    case NumericType::Size::_8:
      v2c_ = new DictImpl<uint64_t>(exceeded_value);
      break;
    default:
      throw std::runtime_error("Unsupported dimension code size!");
  }
}

// Using this method should be reduced to non-intensive parts only:
AnyNum DimensionDict::Decode(const std::string& value) {
  AnyNum code;
  lock_.lock_shared();
  switch (code_type_.size()) {
    case NumericType::Size::_1:
      {
        auto typed_dict = reinterpret_cast<DictImpl<uint8_t>*>(v2c_);
        auto it = typed_dict->find(value);
        code = AnyNum((uint8_t)(it != typed_dict->end() ? it->second : UINT8_MAX));
      }
      break;
    case NumericType::Size::_2:
      {
        auto typed_dict = reinterpret_cast<DictImpl<uint16_t>*>(v2c_);
        auto it = typed_dict->find(value);
        code = AnyNum((uint16_t)(it != typed_dict->end() ? it->second : UINT16_MAX));
      }
      break;
    case NumericType::Size::_4:
      {
        auto typed_dict = reinterpret_cast<DictImpl<uint32_t>*>(v2c_);
        auto it = typed_dict->find(value);
        code = AnyNum((uint32_t)(it != typed_dict->end() ? it->second : UINT32_MAX));
      }
      break;
    case NumericType::Size::_8:
      {
        auto typed_dict = reinterpret_cast<DictImpl<uint64_t>*>(v2c_);
        auto it = typed_dict->find(value);
        code = AnyNum((uint64_t)(it != typed_dict->end() ? it->second : UINT64_MAX));
      }
      break;
  }
  lock_.unlock_shared();
  return code;
}

DimensionDict::~DimensionDict() {
  switch (code_type_.size()) {
    case NumericType::Size::_1:
      delete reinterpret_cast<DictImpl<uint8_t>*>(v2c_);
      break;
    case NumericType::Size::_2:
      delete reinterpret_cast<DictImpl<uint16_t>*>(v2c_);
      break;
    case NumericType::Size::_4:
      delete reinterpret_cast<DictImpl<uint32_t>*>(v2c_);
      break;
    case NumericType::Size::_8:
      delete reinterpret_cast<DictImpl<uint64_t>*>(v2c_);
      break;
  }
}

Dictionaries::~Dictionaries() {
  for (auto& it : dicts_) {
    delete it.second;
  }
}

DimensionDict* Dictionaries::GetOrCreate(const std::string& dim_name, const NumericType& code_type) {
  DimensionDict* dict = nullptr;
  lock_.lock();
  auto it = dicts_.find(dim_name);
  if (it == dicts_.end()) {
    dict = new DimensionDict(code_type);
    dicts_.insert(std::make_pair(dim_name, dict));
  } else {
    dict = it->second;
  }
  lock_.unlock();
  return dict;
}

}}
