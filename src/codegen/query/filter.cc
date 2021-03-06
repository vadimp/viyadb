#include <algorithm>
#include <ctime>
#include <cstring>
#include "db/dictionary.h"
#include "codegen/query/filter.h"

namespace viya {
namespace codegen {

void FilterArgsPacker::Visit(const query::RelOpFilter* filter) {
  ValueDecoder value_decoder(filter->value());
  filter->column()->Accept(value_decoder);
  args_.push_back(value_decoder.decoded_value());
}

void FilterArgsPacker::Visit(const query::InFilter* filter) {
  auto column = filter->column();
  auto & values = filter->values();
  for(size_t i = 0; i < values.size(); ++i) {
    ValueDecoder value_decoder(values[i]);
    column->Accept(value_decoder);
    args_.push_back(value_decoder.decoded_value());
  }
}

void FilterArgsPacker::Visit(const query::CompositeFilter* filter) {
  for (auto f : filter->filters()) {
    f->Accept(*this);
  }
}

void FilterArgsPacker::Visit(const query::NotFilter* filter) {
  filter->filter()->Accept(*this);
}

void ArgsUnpacker::UnpackArg(const db::Column* column) {
  auto type = column->num_type().cpp_type();
  auto arg_idx = std::to_string(argidx_);
  code_<<type<<" farg"<<arg_idx<<" = fargs["<<arg_idx<<"].get_"<<type<<"();\n";
  ++argidx_;
}

void ArgsUnpacker::Visit(const query::RelOpFilter* filter) {
  UnpackArg(filter->column());
}

void ArgsUnpacker::Visit(const query::InFilter* filter) {
  for(size_t i = 0; i < filter->values().size(); ++i) {
    UnpackArg(filter->column());
  }
}

void ArgsUnpacker::Visit(const query::CompositeFilter* filter) {
  for (auto f : filter->filters()) {
    f->Accept(*this);
  }
}

void ArgsUnpacker::Visit(const query::NotFilter* filter) {
  filter->filter()->Accept(*this);
}

void ValueDecoder::Visit(const db::StrDimension* dimension) {
  decoded_value_ = dimension->dict()->Decode(value_);
}

void ValueDecoder::Visit(const db::NumDimension* dimension) {
  decoded_value_ = dimension->num_type().Parse(value_);
}

void ValueDecoder::Visit(const db::TimeDimension* dimension) {
  if (std::all_of(value_.begin(), value_.end(), ::isdigit)) {
    decoded_value_ = dimension->num_type().Parse(value_);
  } else {
    uint32_t multiplier = dimension->micro_precision() ? 1000000L : 1L;
    uint64_t ts = 0;
    std::tm tm;

    const char* r = strptime(value_.c_str(), "%Y-%m-%d %T", &tm);
    if (r != nullptr && *r == '\0') {
      ts = timegm(&tm) * multiplier;
    } else {
      if (r != nullptr && dimension->micro_precision() && *r == '.') {
        ts = timegm(&tm) * multiplier + std::stoul(r);
      } else {
        std::memset(&tm, 0, sizeof(tm));
        r = strptime(value_.c_str(), "%Y-%m-%d", &tm);
        if (r != nullptr && *r == '\0') {
          ts = timegm(&tm) * multiplier;
        }
      }
    }
    if (ts > 0) {
      decoded_value_ = dimension->micro_precision() ? db::AnyNum(ts) : db::AnyNum((uint32_t)ts);
    } else {
      throw std::invalid_argument("Unrecognized time format: " + value_);
    }
  }
}

void ValueDecoder::Visit(const db::BoolDimension* dimension __attribute__((unused))) {
  decoded_value_ = value_ == "true";
}

void ValueDecoder::Visit(const db::ValueMetric* metric) {
  decoded_value_ = metric->num_type().Parse(value_);
}

void ValueDecoder::Visit(const db::BitsetMetric* metric) {
  decoded_value_ = metric->num_type().Parse(value_);
}

void ComparisonBuilder::Visit(const query::RelOpFilter* filter) {
  code_<<"("
    <<(filter->column()->type() == db::Column::Type::DIMENSION ? "tuple_dims" : "tuple_metrics")
    <<"._"<<std::to_string(filter->column()->index())
    <<filter->opstr()<<"farg"<<std::to_string(argidx_++)
    <<")";
}

void ComparisonBuilder::Visit(const query::InFilter* filter) {
  auto dim_idx = std::to_string(filter->column()->index());
  code_<<"(";
  auto struct_name = filter->column()->type() == db::Column::Type::DIMENSION ? "tuple_dims" : "tuple_metrics";
  for(size_t i = 0; i < filter->values().size(); ++i) {
    if (i > 0) {
      code_<<" | ";
    }
    code_<<struct_name<<"._"<<dim_idx<<"==farg"<<std::to_string(argidx_++);
  }
  code_<<")";
}

void ComparisonBuilder::Visit(const query::CompositeFilter* filter) {
  auto filters = filter->filters();
  auto filters_num = filters.size();
  std::string op = filter->op() == query::CompositeFilter::Operator::AND ? " & " : " | ";
  code_<<"(";
  for(size_t i = 0; i < filters_num; ++i) {
    if (i > 0) {
      code_<<op;
    }
    filters[i]->Accept(*this);
  }
  code_<<")";
}

void ComparisonBuilder::Visit(const query::NotFilter* filter) {
  code_<<"!";
  filter->filter()->Accept(*this);
}

void SegmentSkipBuilder::Visit(const query::RelOpFilter* filter) {
  bool applied = false;
  auto arg_idx = std::to_string(argidx_++);

  if (filter->column()->type() == db::Column::Type::DIMENSION) {
    auto dim = static_cast<const db::Dimension*>(filter->column());

    if (dim->dim_type() == db::Dimension::DimType::NUMERIC
        || dim->dim_type() == db::Dimension::DimType::TIME) {

      auto dim_idx = std::to_string(dim->index());
      switch (filter->op()) {
        case query::RelOpFilter::Operator::EQUAL:
          code_<<"((segment->stats.dmin"<<dim_idx<<"<=farg"<<arg_idx<<") & "
            <<"(segment->stats.dmax"<<dim_idx<<">=farg"<<arg_idx<<"))";
          break;
        case query::RelOpFilter::Operator::LESS:
        case query::RelOpFilter::Operator::LESS_EQUAL:
          code_<<"(segment->stats.dmin"<<dim_idx<<"<=farg"<<arg_idx<<")";
          break;
        case query::RelOpFilter::Operator::GREATER:
        case query::RelOpFilter::Operator::GREATER_EQUAL:
          code_<<"(segment->stats.dmax"<<dim_idx<<">=farg"<<arg_idx<<")";
          break;
        default:
          throw std::runtime_error("Unsupported operator");
      }
      applied = true;
    }
  }
  if (!applied) {
    code_<<"1";
  }
}

void SegmentSkipBuilder::Visit(const query::InFilter* filter) {
  bool applied = false;

  if (filter->column()->type() == db::Column::Type::DIMENSION) {
    auto dim = static_cast<const db::Dimension*>(filter->column());

    if (dim->dim_type() == db::Dimension::DimType::NUMERIC
        || dim->dim_type() == db::Dimension::DimType::TIME) {

      auto dim_idx = filter->column()->index();
      code_<<"(";
      for(size_t i = 0; i < filter->values().size(); ++i) {
        auto arg_idx = std::to_string(argidx_++);
        if (i > 0) {
          code_<<" | ";
        }
        code_<<"(segment->stats.dmin"<<dim_idx<<"<=farg"<<arg_idx<<" & "
          <<"segment->stats.dmax"<<dim_idx<<">=farg"<<arg_idx<<")";
      }
      code_<<")";
      applied = true;
    }
  }
  if (!applied) {
    for(size_t i = 0; i < filter->values().size(); ++i) {
      ++argidx_;
    }
    code_<<"1";
  }
}

Code FilterArgsUnpack::GenerateCode() const {
  Code code;
  ArgsUnpacker b(code);
  filter_->Accept(b);
  return code;
}

Code SegmentSkip::GenerateCode() const {
  Code code;
  SegmentSkipBuilder b(code);
  filter_->Accept(b);
  return code;
}

Code FilterComparison::GenerateCode() const {
  Code code;
  ComparisonBuilder b(code);
  filter_->Accept(b);
  return code;
}

}}
