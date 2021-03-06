#ifndef VIYA_TESTING_DB_H_
#define VIYA_TESTING_DB_H_

#include "gtest/gtest.h"
#include "util/config.h"
#include "db/database.h"

namespace db = viya::db;
namespace util = viya::util;

class InappEvents : public testing::Test {
  protected:
    InappEvents()
      :db(std::move(util::Config(
              "{\"tables\": [{\"name\": \"events\","
              "               \"dimensions\": [{\"name\": \"country\"},"
              "                                {\"name\": \"event_name\", \"length\": 20},"
              "                                {\"name\": \"install_time\", \"type\": \"numeric\"}],"
              "               \"metrics\": [{\"name\": \"count\", \"type\": \"count\"},"
              "                             {\"name\": \"revenue\", \"type\": \"double_sum\"}]}]}"))) {}
    db::Database db;
};

class LiteEvents : public testing::Test {
  protected:
    LiteEvents()
      :db(std::move(util::Config(
              "{\"tables\": [{\"name\": \"events\","
              "               \"dimensions\": [{\"name\": \"time\", \"type\": \"numeric\", \"max\": 4000000},"
              "                                {\"name\": \"dummy\", \"max\": 1}],"
              "               \"metrics\": [{\"name\": \"count\", \"type\": \"count\"}]}]}"))) {}
    db::Database db;
};

class LowCardColumn : public testing::Test {
  protected:
    LowCardColumn()
      :db(std::move(util::Config(
              "{\"tables\": [{\"name\": \"events\","
              "               \"dimensions\": [{\"name\": \"http_method\", \"cardinality\": 5}],"
              "               \"metrics\": [{\"name\": \"count\", \"type\": \"count\"}]}]}"))) {}
    db::Database db;
};

class CardinalityGuard : public testing::Test {
  protected:
    CardinalityGuard()
      :db(std::move(util::Config(
              "{\"tables\": [{\"name\": \"events\","
              "               \"dimensions\": [{\"name\": \"device_id\"},"
              "                                {\"name\": \"event_name\","
              "                                 \"cardinality_guard\": {\"dimensions\": [\"device_id\"], \"limit\": 3}}],"
              "               \"metrics\": [{\"name\": \"count\", \"type\": \"count\"}]}]}"))) {}
    db::Database db;
};

class UserEvents : public testing::Test {
  protected:
    UserEvents()
      :db(std::move(util::Config(
              "{\"tables\": [{\"name\": \"events\","
              "               \"dimensions\": [{\"name\": \"country\"},"
              "                                {\"name\": \"event_name\"},"
              "                                {\"name\": \"time\", \"type\": \"numeric\"}],"
              "               \"metrics\": [{\"name\": \"user_id\", \"type\": \"bitset\"}]}]}"))) {}
    db::Database db;
};

#endif // VIYA_TESTING_DB_H_
