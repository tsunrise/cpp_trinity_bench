#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/ThreadFactory.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/TToString.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/transport/TNonblockingServerSocket.h>
#include <thrift/transport/TNonblockingServerTransport.h>

#include <iostream>
#include <stdexcept>
#include <sstream>
#include <future>

#include "MDTrieShard.h"
#include "trie.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

const int DIMENSION = 6; 
const symbol_t NUM_BRANCHES = pow(2, DIMENSION);
const level_t max_depth = 32;
const level_t trie_depth = 10;
const preorder_t max_tree_node = 512;

class MDTrieHandler : public MDTrieShardIf {
public:

  MDTrieHandler(){
    mdtrie_ = new md_trie<DIMENSION, NUM_BRANCHES>(max_depth, trie_depth, max_tree_node);
  };

  void ping() { cout << "ping()" << endl; }

  int32_t add(const int32_t n1, const int32_t n2) {
    cout << "add(" << n1 << ", " << n2 << ")" << endl;
    return n1 + n2;
  }

  bool check(const std::vector<int32_t> & point){

    TimeStamp start = GetTimestamp();
    auto *leaf_point = new data_point<DIMENSION>();

    for (uint8_t i = 0; i < DIMENSION; i++)
      leaf_point->set_coordinate(i, point[i]);
    thrift_vector_time += GetTimestamp() - start;

    start = GetTimestamp();
    bool result = mdtrie_->check(leaf_point, max_depth);
    thrift_inner_function_time += GetTimestamp() - start;

    return result;
  }

  int32_t insert_trie(const std::vector<int32_t> & point, int32_t primary_key){

    inserted_points_ ++;
    TimeStamp start = GetTimestamp();
    auto *leaf_point = new data_point<DIMENSION>();

    for (uint8_t i = 0; i < DIMENSION; i++)
      leaf_point->set_coordinate(i, point[i]);
    
    thrift_vector_time += GetTimestamp() - start;

    start = GetTimestamp();
    mdtrie_->insert_trie(leaf_point, max_depth, primary_key);

    thrift_inner_function_time += GetTimestamp() - start;

    return primary_key;
  }

  void range_search_trie(std::vector<int32_t> & _return, const std::vector<int32_t> & start_range, const std::vector<int32_t> & end_range){
    
    TimeStamp start;
    start = GetTimestamp();
    auto *start_range_point = new data_point<DIMENSION>();

    for (uint8_t i = 0; i < DIMENSION; i++)
      start_range_point->set_coordinate(i, start_range[i]);    

    auto *end_range_point = new data_point<DIMENSION>();

    for (uint8_t i = 0; i < DIMENSION; i++)
      end_range_point->set_coordinate(i, end_range[i]);     

    auto *found_points = new point_array<DIMENSION>();
    thrift_vector_time += GetTimestamp() - start;

    start = GetTimestamp();
    mdtrie_->range_search_trie(start_range_point, end_range_point, mdtrie_->root(), 0, found_points);
    thrift_inner_function_time += GetTimestamp() - start;

    n_leaves_t n_found_points = found_points->size();

    _return.reserve(n_found_points);
    start = GetTimestamp();
    for (n_leaves_t i = 0; i < n_found_points; i++){
      _return.emplace_back(found_points->at(i)->read_primary());
    }
    thrift_vector_time += GetTimestamp() - start;

    // get_throughput(n_found_points);
  }

  void primary_key_lookup(std::vector<int32_t> & _return, const int32_t primary_key){

    _return.reserve(DIMENSION);

    TimeStamp start = GetTimestamp();
    symbol_t *node_path_from_primary = (symbol_t *)malloc((max_depth + 1) * sizeof(symbol_t));
    tree_block<DIMENSION, NUM_BRANCHES> *t_ptr = (tree_block<DIMENSION, NUM_BRANCHES> *) (p_key_to_treeblock_compact.At(primary_key));

    symbol_t parent_symbol_from_primary = t_ptr->get_node_path_primary_key(primary_key, node_path_from_primary);
    node_path_from_primary[max_depth - 1] = parent_symbol_from_primary;

    auto returned_coordinates = t_ptr->node_path_to_coordinates(node_path_from_primary);  

    thrift_inner_function_time += GetTimestamp() - start;
    
    start = GetTimestamp();
    for (uint8_t i = 0; i < DIMENSION; i++){
      _return.emplace_back(returned_coordinates->get_coordinate(i));
    }      
    thrift_vector_time += GetTimestamp() - start;
  }

  void get_throughput(uint32_t count){

    cout << "Throughput: " << ((float) count / thrift_inner_function_time) * 1000000  << endl;    
    thrift_vector_time = 0;
    thrift_inner_function_time = 0;    
  }

  void get_time(){

    cout << "vector time: " << (float) thrift_vector_time  << endl;
    cout << "inner function time: " << (float) thrift_inner_function_time  << endl;
    thrift_vector_time = 0;
    thrift_inner_function_time = 0;

  }

  int32_t get_count(){
    return inserted_points_;
  }

protected:

  md_trie<DIMENSION, NUM_BRANCHES> *mdtrie_; 
  uint64_t inserted_points_ = 0;
};

class MDTrieCloneFactory : virtual public MDTrieShardIfFactory {
 public:
  ~MDTrieCloneFactory() override = default;
  MDTrieShardIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo)
  {
    std::shared_ptr<TSocket> sock = std::dynamic_pointer_cast<TSocket>(connInfo.transport);
    cout << "Incoming connection\n";
    cout << "\tSocketInfo: "  << sock->getSocketInfo() << "\n";
    cout << "\tPeerHost: "    << sock->getPeerHost() << "\n";
    cout << "\tPeerAddress: " << sock->getPeerAddress() << "\n";
    cout << "\tPeerPort: "    << sock->getPeerPort() << "\n";
    return new MDTrieHandler;
  }
  void releaseHandler( MDTrieShardIf* handler) {
    delete handler;
  }
};


class MDTrieServerCoordinator {

public:
    MDTrieServerCoordinator(int port_num) {

      start_server(port_num);
    }

    MDTrieServerCoordinator(int port_num, int server_count) {

        std::vector<std::future<void>> futures;

        for(int i = 0; i < server_count; ++i) {
          futures.push_back(std::async(start_server, port_num + i));
        }

        for(auto &e : futures) {
          e.get();
        }
    }

    static void start_server(int port_num){

        auto handler = std::make_shared<MDTrieHandler>();
        auto processor = std::make_shared<MDTrieShardProcessor>(handler);
        auto socket = std::make_shared<TNonblockingServerSocket>("172.29.249.44", port_num);
        auto server = std::make_shared<TNonblockingServer>(processor, socket);

        cout << "Starting the server..." << endl;
        server->serve();
        cout << "Done." << endl;
    }

private:

};


int main(int argc, char *argv[]){

  if (argc == 2){
    MDTrieServerCoordinator(atoi(argv[1]));
    return 0;
  }

  MDTrieServerCoordinator(9090, 72);
  return 0;
  
}