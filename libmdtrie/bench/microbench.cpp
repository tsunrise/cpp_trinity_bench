#include "benchmark.hpp"
#include "common.hpp"
#include "parser.hpp"
#include "trie.h"
#include <climits>
#include <ostream>
#include <sys/time.h>
#include <unistd.h>

void github_bench(void)
{

  use_github_setting(GITHUB_DIMENSION, micro_github_size);
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
  bench.get_storage(folder_name + "github_storage" + identification_string);
  bench.lookup(folder_name + "github_lookup" + identification_string);
  bench.range_search(GITHUB_QUERY_ADDR,
                     folder_name + "github_query" + identification_string,
                     get_query_github<GITHUB_DIMENSION>);
}

int main(int argc, char *argv[])
{

  std::string argvalue;
  optimization_code = OPTIMIZATION_SM;
  int arg;
  int sensitivity_dimensions = -1;
  int treeblock_size = -1;
  is_microbenchmark = true;

  while ((arg = getopt(argc, argv, "b:o:d:t:")) != -1)
    switch (arg)
    {
    case 'b':
      argvalue = std::string(optarg);
      break;
    case 't':
      treeblock_size = atoi(optarg);
      break;
    case 'o':
      optimization = std::string(optarg);
      if (optimization == "SM")
        optimization_code = OPTIMIZATION_SM;
      if (optimization == "B")
        optimization_code = OPTIMIZATION_B;
      if (optimization == "CN")
        optimization_code = OPTIMIZATION_CN;
      if (optimization == "GM")
        optimization_code = OPTIMIZATION_GM;
      break;
    case 'd':
      sensitivity_dimensions = atoi(optarg);
      break;
    default:
      abort();
    }

  std::cout << "benchmark: " << argvalue << ", optimization: " << optimization
            << ", dimensions: " << sensitivity_dimensions
            << ", treeblock_size: " << treeblock_size << std::endl;
  if (argvalue == "github")
    github_bench();
  else
    std::cout << "Unrecognized benchmark: " << argvalue << std::endl;
}