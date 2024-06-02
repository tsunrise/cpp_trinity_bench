
#ifndef TrinityCommon_H
#define TrinityCommon_H

#include "trie.h"
#include <climits>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

#define GITHUB_DIMENSION 10

#define SERVER_TO_SERVER_IN_NS 92

// #define TEST_STORAGE

enum
{
  TPCH = 1,
  GITHUB = 2,
  NYC = 3,
};

std::string ROOT_DIR = "/proj/Trinity/";
std::string GITHUB_DATA_ADDR = ROOT_DIR + "datasets/github.csv";

std::string GITHUB_QUERY_ADDR = ROOT_DIR + "queries/github_query";

unsigned int skip_size_count = 0;
/* Because it results in otherwise OOM for other benchmarks. */
bool is_microbenchmark = false;

enum
{
  OPTIMIZATION_SM = 0, /* Default*/
  OPTIMIZATION_B = 1,
  OPTIMIZATION_CN = 2,
  OPTIMIZATION_GM = 3,
};

level_t max_depth = 32;
level_t trie_depth = 6;
preorder_t max_tree_node = 512;
point_t POINTS_TO_INSERT = 1000;
point_t POINTS_TO_LOOKUP = 1000;
point_t POINTS_TO_RANGE_QUERY = 1000;
std::string results_folder_addr = "/proj/Trinity/results/";
std::string identification_string = "";
int optimization_code = OPTIMIZATION_SM;
std::string optimization = "SM";
float selectivity_upper = 0.0015;
float selectivity_lower = 0.0005;


/* [QUANTITY, EXTENDEDPRICE, DISCOUNT, TAX, SHIPDATE, COMMITDATE, RECEIPTDATE,
 * TOTALPRICE, ORDERDATE] */
std::vector<int32_t> github_max_values = {7451541, 737170, 262926, 354850,
                                          379379, 3097263, 703341, 8745,
                                          20201206, 20201206};
std::vector<int32_t> github_min_values = {1, 1, 0, 0, 0,
                                          0, 0, 0, 20110211, 20110211};

int gen_rand(int start, int end)
{
  return start + (std::rand() % (end - start + 1));
}


void use_github_setting(int dimensions, int _total_points_count)
{

  std::vector<level_t> bit_widths = {
      24, 24, 24, 24, 24, 24, 24, 16, 24, 24}; // 10 Dimensions;
  std::vector<level_t> start_bits = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // 10 Dimensions;
  total_points_count = _total_points_count;
  is_collapsed_node_exp = false;

  start_bits.resize(dimensions);
  bit_widths.resize(dimensions);

  trie_depth = 6;
  max_depth = 24;
  no_dynamic_sizing = true;
  max_tree_node = 512;

  create_level_to_num_children(bit_widths, start_bits, max_depth);
}

void flush_vector_to_file(std::vector<TimeStamp> vect, std::string filename)
{

  std::ofstream outFile(filename);
  for (const auto &e : vect)
    outFile << std::to_string(e) << "\n";
}

void flush_string_to_file(std::string str, std::string filename)
{

  std::ofstream outFile(filename);
  outFile << str << "\n";
}

template <dimension_t DIMENSION>
void get_query_github(std::string line,
                      data_point<DIMENSION> *start_range,
                      data_point<DIMENSION> *end_range)
{

  for (dimension_t i = 0; i < DIMENSION; i++)
  {
    start_range->set_coordinate(i, github_min_values[i]);
    end_range->set_coordinate(i, github_max_values[i]);
  }
  std::stringstream ss(line);
  while (ss.good())
  {

    std::string index_str;
    std::getline(ss, index_str, ',');

    std::string start_range_str;
    std::getline(ss, start_range_str, ',');
    std::string end_range_str;
    std::getline(ss, end_range_str, ',');

    dimension_t index = std::stoul(index_str);
    if (index > 10)
      index -= 3;

    if (start_range_str != "-1" && index < DIMENSION)
    {
      start_range->set_coordinate(index, std::stoul(start_range_str));
    }
    if (end_range_str != "-1" && index < DIMENSION)
    {
      end_range->set_coordinate(index, std::stoul(end_range_str));
    }
  }

  for (dimension_t i = 0; i < GITHUB_DIMENSION; i++)
  {
    if (i >= 8)
    {
      start_range->set_coordinate(i, start_range->get_coordinate(i) - 20110000);
      end_range->set_coordinate(i, end_range->get_coordinate(i) - 20110000);
    }
  }
}

#endif // TrinityCommon_H
