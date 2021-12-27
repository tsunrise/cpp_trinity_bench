#include <iostream>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "MDTrieShardClient.h"
#include "TrinityBenchShared.h"
#include "trie.h"
#include <tqdm.h>
#include <future>
#include <atomic>
#include <tuple>
#include <iostream>
#include <fstream>

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
const int DIMENSION = 9;

vector<vector <int32_t>> *get_data_vector(std::vector<int32_t> &max_values, std::vector<int32_t> &min_values){

/** 
    Get data from the TPC-H dataset stored in a vector
*/

  std::ifstream infile("/home/ziming/tpch-dbgen/tpch_20/orders_lineitem_merged_inner.csv");

  std::string line;
  std::getline(infile, line);

  tqdm bar;
  n_leaves_t n_points = 0;
  n_leaves_t n_lines = 110418170;
  auto data_vector = new vector<vector <int32_t>>;

  while (std::getline(infile, line))
  {
      bar.progress(n_points, n_lines);
      std::stringstream ss(line);
      vector <int32_t> point(DIMENSION, 0);

      // Parse string by ","
      int leaf_point_index = 0;
      int index = -1;

      // Kept indexes: 
      // [4, 5, 6, 7, 10, 11, 12, 16, 17]
      // [QUANTITY, EXTENDEDPRICE, DISCOUNT, TAX, SHIPDATE, COMMITDATE, RECEIPTDATE, TOTALPRICE, ORDERDATE]
      while (ss.good())
      {
          index ++;
          std::string substr;
          std::getline(ss, substr, ',');
      
          int32_t num;
          if (index == 5 || index == 6 || index == 7 || index == 16) // float with 2dp
          {
              num = static_cast<int32_t>(std::stof(substr) * 100);
          }
          else if (index == 10 || index == 11 || index == 12 || index == 17) //yy-mm-dd
          {
              substr.erase(std::remove(substr.begin(), substr.end(), '-'), substr.end());
              num = static_cast<int32_t>(std::stoul(substr));
          }
          else if (index == 8 || index == 9 || index == 13 || index == 15 || index == 18) //skip text
              continue;
          else if (index == 0 || index == 1 || index == 2 || index == 14) // secondary keys
              continue;
          else if (index == 19) // all 0
              continue;
          else if (index == 3) // lineitem
              continue;
          else
              num = static_cast<int32_t>(std::stoul(substr));

      
          point[leaf_point_index] = num;
          leaf_point_index++;
      }
      
      if (n_points == n_lines)
          break;

      data_vector->push_back(point);

      for (dimension_t i = 0; i < DIMENSION; i++){
          if (point[i] > max_values[i])
              max_values[i] = point[i];
          if (point[i] < min_values[i])
              min_values[i] = point[i];         
      }    
      n_points ++;
  }
  bar.finish();
  return data_vector;
}



int main(){

    std::vector<std::string> server_ips = {"172.28.229.152", "172.28.229.153", "172.28.229.151", "172.28.229.149", "172.29.249.30"};

    int shard_num = 48;
    int client_num = 128;
    auto client = MDTrieClient(server_ips, shard_num);
    if (!client.ping(2)){
        std::cerr << "Server setting wrong!" << std::endl;
        exit(-1);
    }

    TimeStamp start, diff;

    /** 
        Insert all points
    */

    std::tuple<uint32_t, float> return_tuple;
    uint32_t throughput;
    float latency;

    std::vector<int32_t> max_values(DIMENSION, 0);
    std::vector<int32_t> min_values(DIMENSION, 2147483647);
    vector<vector <int32_t>> *data_vector = get_data_vector(max_values, min_values);

    start = GetTimestamp();
    return_tuple = total_client_insert(data_vector, client_num, server_ips);
    throughput = std::get<0>(return_tuple);
    latency = std::get<1>(return_tuple);
    diff = GetTimestamp() - start;

    cout << "Insertion Throughput measured from thread (pt / seconds): " << throughput << endl;
    cout << "Throughput measured from end-to-end Laatency: " << ((float) total_points_count / diff) * 1000000 << endl;
    cout << "Latency (us): " << latency << endl;

    /**   
        Point Lookup given primary key
    */

    start = GetTimestamp();
    return_tuple = total_client_lookup(data_vector, client_num, server_ips);
    throughput = std::get<0>(return_tuple);
    latency = std::get<1>(return_tuple);

    diff = GetTimestamp() - start;
    cout << "Primary Key Lookup Throughput measured from thread (pt / seconds): " << throughput << endl;
    cout << "Latency (us): " << latency << endl;
    cout << "Throughput measured from end-to-end Laatency: " << ((float) total_points_count / diff) * 1000000 << endl;

    delete data_vector;
    return 0;
}