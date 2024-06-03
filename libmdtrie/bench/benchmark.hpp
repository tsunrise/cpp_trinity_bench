#ifndef MdTrieBench_H
#define MdTrieBench_H

#include "common.hpp"
#include "trie.h"
#include <climits>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

template <dimension_t DIMENSION>
class MdTrieBench
{
public:
  MdTrieBench(md_trie<DIMENSION> *mdtrie) { mdtrie_ = mdtrie; };

  void insert(std::string data_addr,
              std::string outfile_name,
              point_t total_points_count,
              std::vector<int32_t> (*parse_line)(std::string line))
  {
    const auto _ = outfile_name;
    std::ifstream infile(data_addr);
    TimeStamp start = 0, diff = 0;
    point_t n_points = 0;
    point_t has_skipped = 0;
    data_point<DIMENSION> leaf_point;

    /**
     * Insertion
     */

    std::string line;
    while (std::getline(infile, line))
    {
      if (has_skipped < skip_size_count)
      {
        has_skipped++;
        continue;
      }

      std::vector<int32_t> vect = parse_line(line);
      for (dimension_t i = 0; i < DIMENSION; i++)
      {
        leaf_point.set_coordinate(i, vect[i]);
      }
      start = GetTimestamp();
      mdtrie_->insert_trie(&leaf_point, n_points, p_key_to_treeblock_compact);
      TimeStamp latency = GetTimestamp() - start;
      diff += latency;
      n_points++;

      if (n_points > total_points_count - POINTS_TO_INSERT)
        insertion_latency_vect_.push_back(latency + SERVER_TO_SERVER_IN_NS);

      if (n_points == total_points_count)
        break;

      if (n_points % (total_points_count / 100) == 0)
        std::cerr << n_points << " out of " << total_points_count << std::endl;
    }

    // std::cout << "[CPP] Insertion time: " << (float)diff / n_points << "us" << std::endl;
    infile.close();
  }

  void lookup(std::string outfile_name)
  {

    // std::cout << "Running lookups" << std::endl;
    TimeStamp cumulative = 0, start = 0;

    for (point_t i = 0; i < POINTS_TO_LOOKUP; i++)
    {
      start = GetTimestamp();
      mdtrie_->lookup_trie(i, p_key_to_treeblock_compact);
      TimeStamp temp_diff = GetTimestamp() - start;
      cumulative += temp_diff;
      lookup_latency_vect_.push_back(temp_diff + SERVER_TO_SERVER_IN_NS);
    }
    flush_vector_to_file(lookup_latency_vect_,
                         results_folder_addr +
                             outfile_name);
    // std::cout << "[CPP] Lookup time: "
    //           << (float)cumulative / POINTS_TO_LOOKUP
    //           << " us, sample size="
    //           << POINTS_TO_LOOKUP
    //           << std::endl;
  }

  void range_search(std::string query_addr,
                    std::string outfile_name,
                    void (*get_query)(std::string,
                                      data_point<DIMENSION> *,
                                      data_point<DIMENSION> *))
  {

    // std::cout << "Running range queries" << std::endl;
    std::ifstream file(query_addr);
    std::ofstream outfile(results_folder_addr +
                          outfile_name);
    TimeStamp diff = 0, start = 0;
    TimeStamp cumulative = 0;
    for (unsigned long i = 0; i < POINTS_TO_RANGE_QUERY; i++)
    {

      std::vector<int32_t> found_points;
      data_point<DIMENSION> start_range;
      data_point<DIMENSION> end_range;

      std::string line;
      std::getline(file, line);
      get_query(line, &start_range, &end_range);

      start = GetTimestamp();
      mdtrie_->range_search_trie(
          &start_range, &end_range, mdtrie_->root(), 0, found_points);
      diff = GetTimestamp() - start;
      cumulative += diff;
      found_points.clear();
    }

    // std::cout << "[CPP] Range search time: "
    //           << (float)cumulative / POINTS_TO_RANGE_QUERY
    //           << " us, sample size="
    //           << POINTS_TO_RANGE_QUERY
    //           << std::endl;
  }

  void get_storage(std::string outfile_name)
  {

    uint64_t size = mdtrie_->size(p_key_to_treeblock_compact);
    // convert to mb
    double size_mb = (double)size / (1024 * 1024);
    // std::cout << "[CPP] Memory used: " << size_mb << " MB" << std::endl;
    flush_string_to_file(
        std::to_string(size) + "," + std::to_string(total_points_count),
        results_folder_addr + outfile_name);
  }

protected:
  std::vector<TimeStamp> insertion_latency_vect_;
  std::vector<TimeStamp> lookup_latency_vect_;
  md_trie<DIMENSION> *mdtrie_;
};

#endif // MdTrieBench_H
