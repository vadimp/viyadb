#include <fstream>
#include <unistd.h>
#include "db/table.h"
#include "db/store.h"
#include "gtest/gtest.h"
#include "db.h"

namespace query = viya::query;

TEST_F(InappEvents, LoadEvents)
{
  auto table = db.GetTable("events");
  table->Load({
    {"US", "purchase", "20141112", "0.1"},
    {"US", "purchase", "20141112", "1.1"}
  });

  EXPECT_EQ(1, table->store()->segments().size());
  EXPECT_EQ(1, table->store()->segments()[0]->size());
}

TEST_F(InappEvents, LoadFromTsv)
{
  auto table = db.GetTable("events");
  std::string fname("InappEvents_LoadFromTsv.tsv");
  std::ofstream out(fname);
  out<<"US\tpurchase\t20141112\t0.1\n";
  out<<"US\tpurchase\t20141112\t1.1\n";
  out<<"US\t\t20141112\t0.3\n";
  out<<"IL\tpurchase\t20141112\t0.0\n";
  out.close();

  util::Config load_conf(
    "{\"file\": \"" + fname + "\","
    " \"type\": \"file\","
    " \"format\": \"tsv\","
    " \"table\": \"" + table->name() + "\"}");
  db.Load(load_conf);
  unlink(fname.c_str());

  EXPECT_EQ(1, table->store()->segments().size());
  EXPECT_EQ(3, table->store()->segments()[0]->size());

  query::MemoryRowOutput output;
  db.Query(
    std::move(util::Config(
        "{\"type\": \"aggregate\","
        " \"table\": \"events\","
        " \"dimensions\": [\"country\"],"
        " \"metrics\": [\"revenue\"],"
        " \"filter\": {\"op\": \"ne\", \"column\": \"country\", \"value\": \"IL\"}}")), output);

  std::vector<query::MemoryRowOutput::Row> expected = {
    {"US", "1.5"}
  };
  EXPECT_EQ(expected, output.rows());
}

TEST_F(InappEvents, LoadFromTsvCols)
{
  auto table = db.GetTable("events");
  std::string fname("InappEvents_LoadFromTsvCols.tsv");
  std::ofstream out(fname);
  out<<"purchase\tUS\t20141112\t1\t0.1\n";
  out<<"purchase\tUS\t20141112\t2\t1.1\n";
  out<<"\tUS\t20141112\t3\t0.3\n";
  out<<"purchase\tIL\t20141112\t5\t0.0\n";
  out.close();

  util::Config load_conf(
    "{\"file\": \"" + fname + "\","
    " \"type\": \"file\","
    " \"columns\": [\"event_name\", \"country\", \"install_time\", \"index\", \"revenue\"],"
    " \"format\": \"tsv\","
    " \"table\": \"" + table->name() + "\"}");
  db.Load(load_conf);
  unlink(fname.c_str());

  EXPECT_EQ(1, table->store()->segments().size());
  EXPECT_EQ(3, table->store()->segments()[0]->size());

  query::MemoryRowOutput output;
  db.Query(
    std::move(util::Config(
        "{\"type\": \"aggregate\","
        " \"table\": \"events\","
        " \"dimensions\": [\"country\"],"
        " \"metrics\": [\"revenue\"],"
        " \"filter\": {\"op\": \"ne\", \"column\": \"country\", \"value\": \"IL\"}}")), output);

  std::vector<query::MemoryRowOutput::Row> expected = {
    {"US", "1.5"}
  };
  EXPECT_EQ(expected, output.rows());
}

