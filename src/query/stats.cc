#include <glog/logging.h>
#include <sstream>
#include "query/stats.h"
#include "util/statsd.h"

namespace viya {
namespace query {

void QueryStats::OnBegin(const std::string& query_type, const std::string& table) {
  query_type_ = query_type;
  table_ = table;
  begin_work_ = cr::steady_clock::now();
}

void QueryStats::OnCompile() {
  compile_time = cr::steady_clock::now() - begin_work_;
}

void QueryStats::OnEnd() {
  whole_time = cr::steady_clock::now() - begin_work_;
  auto query_time = cr::duration_cast<cr::milliseconds>(whole_time).count();

  LOG(INFO)<<"Query time "<<query_time<<" ms ("
    <<"ss="<<std::to_string(scanned_segments)
    <<",sr="<<std::to_string(scanned_recs)
    <<",ar="<<std::to_string(aggregated_recs)
    <<",or="<<std::to_string(output_recs)
    <<")"<<std::endl;

  std::ostringstream prefix;
  prefix<<"query."<<query_type_<<"."<<table_<<".";
  statsd_.Timing(prefix.str() + "time", query_time);
  statsd_.Count(prefix.str() + "rows", output_recs);
}

}}
