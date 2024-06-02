
#ifndef TrinityParseFile_H
#define TrinityParseFile_H

#include "common.hpp"
#include <atomic>
#include <fstream>
#include <future>
#include <iostream>
#include <tuple>
#include <vector>

using namespace std;

// Parse one line from TPC-H file.
std::vector<int32_t>
parse_line_github(std::string line)
{

  vector<int32_t> point(GITHUB_DIMENSION, 0);
  int index = -1;
  bool primary_key = true;
  std::string delim = ",";
  auto start = 0U;
  auto end = line.find(delim);
  // int real_index = -1;
  // [id, events_count, authors_count, forks, stars, issues, pushes, pulls,
  // downloads, adds, dels, add_del_ratio, start_date, end_date]
  while (end != std::string::npos)
  {
    std::string substr = line.substr(start, end - start);
    start = end + 1;
    end = line.find(delim, start);

    if (primary_key)
    {
      primary_key = false;
      continue;
    }
    index++;
    point[index] = static_cast<int32_t>(std::stoul(substr));
  }
  index++;
  std::string substr = line.substr(start, end - start);
  point[index] = static_cast<int32_t>(std::stoul(substr));

  for (int i = 0; i < GITHUB_DIMENSION; i++)
  {
    if (i == 8 || i == 9)
    {
      point[i] -= 20110000;
    }
  }

  return point;
}

void update_range_search_range_github(std::vector<int32_t> &start_range,
                                      std::vector<int32_t> &end_range,
                                      std::string line)
{

  std::stringstream ss(line);

  while (ss.good())
  {

    std::string index_str;
    std::getline(ss, index_str, ',');

    std::string start_range_str;
    std::getline(ss, start_range_str, ',');
    std::string end_range_str;
    std::getline(ss, end_range_str, ',');

    int index = std::stoul(index_str);
    if (index > 10)
      index -= 3;

    if (start_range_str != "-1")
    {
      start_range[static_cast<int32_t>(index)] =
          static_cast<int32_t>(std::stoul(start_range_str));
    }
    if (end_range_str != "-1")
    {
      end_range[static_cast<int32_t>(index)] =
          static_cast<int32_t>(std::stoul(end_range_str));
    }
  }

  for (unsigned int i = 0; i < start_range.size(); i++)
  {
    if (i >= 8 || i == 9)
    {
      start_range[i] -= 20110000;
      end_range[i] -= 20110000;
    }
  }
}

#endif // TrinityParseFile_H
