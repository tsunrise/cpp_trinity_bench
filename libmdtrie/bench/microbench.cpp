#include "benchmark.hpp"
#include "common.hpp"
#include "parser.hpp"
#include "trie.h"
#include <climits>
#include <ostream>
#include <sys/time.h>
#include <unistd.h>

void github_bench()
{

  use_github_setting(GITHUB_DIMENSION, POINTS_TO_INSERT);

  md_trie<GITHUB_DIMENSION> mdtrie(max_depth, trie_depth, max_tree_node);
  MdTrieBench<GITHUB_DIMENSION> bench(&mdtrie);
  p_key_to_treeblock_compact = new bitmap::CompactPtrVector(total_points_count);
  std::string folder_name = "microbenchmark/";
  if (identification_string != "")
  {
    folder_name = "optimization/";
  }
  bench.insert(GITHUB_DATA_ADDR,
               folder_name + "github_insert" + identification_string,
               total_points_count,
               parse_line_github);
  
  bench.lookup(folder_name + "github_lookup" + identification_string);
  bench.range_search(GITHUB_QUERY_ADDR,
                     folder_name + "github_query" + identification_string,
                     get_query_github<GITHUB_DIMENSION>);
  bench.get_storage(folder_name + "github_storage" + identification_string);
}

int main(int argc, char *argv[])
{

  std::string argvalue;
  optimization_code = OPTIMIZATION_SM;
  int arg;
  is_microbenchmark = true;

  while ((arg = getopt(argc, argv, "m:n:r:")) != -1)
    switch (arg)
    {
    case 'm':
      argvalue = optarg;
      POINTS_TO_INSERT = std::stoi(argvalue);
      break;
    case 'n':
      argvalue = optarg;
      POINTS_TO_LOOKUP = std::stoi(argvalue);
      break;
    case 'r':
      argvalue = optarg;
      POINTS_TO_RANGE_QUERY = std::stoi(argvalue);
      break;
    default:
      abort();
    }

  github_bench();
}