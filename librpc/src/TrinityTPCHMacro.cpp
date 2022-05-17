#include <iostream>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "MDTrieShardClient.h"
#include "TrinityBenchShared.h"
#include "trie.h"
#include <future>
#include <atomic>
#include <tuple>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <istream>
#include <sys/stat.h>
#include <sys/mman.h>

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
const int DIMENSION = 9;
const int shard_num = 30;
const int client_num = 64;

int main(){

    const char *filepath = "/mntData/orders_lineitem_merged_indexed.csv";
    int fd = open(filepath, O_RDONLY, (mode_t)0600);

    struct stat fileInfo;
    if (fstat(fd, &fileInfo) == -1)
    {
        perror("Error getting the file size");
        exit(EXIT_FAILURE);
    }
    
    char *map;
    map = static_cast<char*>(mmap(0, fileInfo.st_size, PROT_READ, MAP_SHARED, fd, 0));
    if (map == MAP_FAILED)
    {
        close(fd);
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
    }   
    /*
    char* str_start = map;
    char* str_end;
    uint32_t line_num = 0;
    for (long i = 0; i <= fileInfo.st_size; ++i) {
        if (map[i] == '\n') {
            str_end = map + i;
            {
                // C style tokenizing of the string str_start to str_end
                // If you want, you can build a std::string like:
                // std::string line(str_start,str_end);
                // but note that this implies a memory copy.
                line_num ++;
                if (line_num % 1000000 == 0) {
                  cerr << "current_line: " << line_num << endl;
                }
            }
            str_start = map + i + 1;
        }
    }

    return 0;

    FILE * ifile = fopen("/mntData/orders_lineitem_merged_indexed.csv", "r");
    size_t linesz = 0L;
    char * line = nullptr;

    for (uint32_t current_line = 0; current_line < 3000028242; current_line++){

      if (current_line % 1000000 == 0) {
        cerr << "current_line: " << current_line << "," << linesz << endl;
      }
      getline(&line, &linesz, ifile);
    }
    free(line);

    return 0;
    */


    std::vector<std::string> server_ips = {"10.254.254.153", "10.254.254.209", "10.254.254.229", "10.254.254.253", "10.254.254.249"};
    const char *file_address = "/mntData/orders_lineitem_merged_indexed.csv";
    
    total_points_count = 3000028242;

    auto client = MDTrieClient(server_ips, shard_num);
    client_to_server_vect.resize(total_points_count);

    for (unsigned int i = 0; i < server_ips.size(); ++i) {
      for (int j = 0; j < shard_num; j++){
        server_to_client.push_back({});
      }
    }
    if (!client.ping(2)){
        std::cerr << "Server setting wrong!" << std::endl;
        exit(-1);
    }

    /** 
        Insert all points
    */

    TimeStamp start, diff;
    uint32_t throughput;

    start = GetTimestamp();
    // throughput = total_client_insert(file_address, shard_num, client_num, server_ips);
    throughput = total_client_insert_mmap(map, shard_num, client_num, server_ips);
    diff = GetTimestamp() - start;

    cout << "Insertion Throughput (pt / seconds): " << throughput << endl;
    cout << "End-to-end Latency (us): " << diff << endl;
    cout << "Storage: " << client.get_size() << endl;

    // Range Search
    // [QUANTITY, EXTENDEDPRICE, DISCOUNT, TAX, SHIPDATE, COMMITDATE, RECEIPTDATE, TOTALPRICE, ORDERDATE]
    std::vector<int32_t> max_values = {50, 10494950, 10, 8, 19981201, 19981031, 19981231, 59591284, 19980802};
    std::vector<int32_t> min_values = {1, 90001, 0, 0, 19920102, 19920131, 19920103, 81602, 19920101};

    const char *query_address = "/proj/trinity-PG0/Trinity/baselines/clickhouse/tpch_queries_converted";
    std::ifstream file(query_address);

    for (int i = 0; i < 10; i ++) {

      std::vector<int32_t> found_points;
      std::vector<int32_t> start_range = min_values;
      std::vector<int32_t> end_range = max_values;

      std::string line;
      std::getline(file, line);
      std::stringstream ss(line);
      // Example: 0,-1,24,2,5,7,4,19943347,19950101
      while (ss.good()) {

        std::string index_str;
        std::getline(ss, index_str, ',');

        std::string start_range_str;
        std::getline(ss, start_range_str, ',');
        std::string end_range_str;
        std::getline(ss, end_range_str, ',');

        if (start_range_str != "-1") {
          start_range[static_cast<int32_t>(std::stoul(index_str))] = static_cast<int32_t>(std::stoul(start_range_str));
        }
        if (end_range_str != "-1") {
          end_range[static_cast<int32_t>(std::stoul(index_str))] = static_cast<int32_t>(std::stoul(end_range_str));
        }
      }

      start = GetTimestamp();
      client.range_search_trie(found_points, start_range, end_range);
      diff = GetTimestamp() - start;
      std::cout << "Query " << i << " end to end latency: " << diff << std::endl;  
      std::cout << "Found points count: " << found_points.size() << std::endl;
    }
}

/**   
    Insertion Throughput (pt / seconds): 958917
    End-to-end Latency (us): 1312271436
    Storage: 50205921310
    Query 1 end to end latency: 10609785
    Found points count: 23892962
    Primary Key Lookup Throughput (pt / seconds): 759078
*/