#include "trie.h"
#include <unistd.h>
#include <sys/time.h>
#include <climits>
#include <tqdm.h>
#include <vector>
#include <iostream>
#include <fstream>

const int DIMENSION = 6; // Max: 6
level_t TRIE_DEPTH = 10;
uint32_t TREEBLOCK_SIZE = 1024;
std::ofstream myfile;

void run_bench(level_t max_depth, level_t trie_depth, preorder_t max_tree_node, std::vector<level_t> dimension_bits){

    create_level_to_num_children(dimension_bits, max_depth);
    auto *found_points = new point_array();
    auto *all_points = new std::vector<data_point>();
    all_points_ptr = all_points;

    auto *mdtrie = new md_trie(max_depth, trie_depth, max_tree_node);
    auto *leaf_point = new data_point();

    char *line = nullptr;
    size_t len = 0;
    ssize_t read;
    FILE *fp = fopen("../libmdtrie/bench/data/sample_shuf.txt", "r");
    std::ofstream writefile;
    writefile.open("filesystem.csv");
    // If the file cannot be open
    if (fp == nullptr)
    {
        fprintf(stderr, "file not found\n");
        exit(EXIT_FAILURE);
    }
    
    n_leaves_t n_points = 0;
    uint64_t max[DIMENSION];
    uint64_t min[DIMENSION];
    n_leaves_t n_lines = 14583357;
    total_points_count = n_lines;

    tqdm bar;
    TimeStamp start, diff;
    diff = 0;
    
    while ((read = getline(&line, &len, fp)) != -1)
    {
        bar.progress(n_points, n_lines);
        char *token = strtok(line, " ");
        char *ptr;

        for (uint8_t i = 1; i <= 2; i ++){
            token = strtok(nullptr, " ");
        }

        for (dimension_t i = 0; i < DIMENSION; i++){
            token = strtok(nullptr, " ");
            if (i != DIMENSION - 1)
                writefile << token << ",";
            else
                writefile << token << "\n";
            leaf_point->set_coordinate(i, strtoul(token, &ptr, 10));
        }

        for (dimension_t i = 0; i < DIMENSION; i++){
            
            if (n_points == 0){
                max[i] = leaf_point->get_coordinate(i);
                min[i] = leaf_point->get_coordinate(i);
            }
            else {
                if (leaf_point->get_coordinate(i) > max[i]){
                    max[i] = leaf_point->get_coordinate(i);
                }
                if (leaf_point->get_coordinate(i) < min[i]){
                    min[i] = leaf_point->get_coordinate(i);
                }
            }          
        }

        (*all_points).push_back((*leaf_point));
        start = GetTimestamp();
        mdtrie->insert_trie(leaf_point, n_points);
        diff += GetTimestamp() - start;
        n_points ++;
    }
    // raise(SIGINT);
    bar.finish();

    myfile << "Insertion Latency: " << (float) diff / n_lines << std::endl;
    myfile << "mdtrie storage: " << mdtrie->size() << std::endl;
    myfile << "trie_size: " << trie_size << std::endl;
    myfile << "p_key_to_treeblock_compact_size: " << p_key_to_treeblock_compact_size << std::endl;
    myfile << "total_treeblock_num: " << total_treeblock_num << std::endl;
    myfile << "treeblock_primary_pointer_size: " << treeblock_primary_pointer_size << std::endl;
    myfile << "treeblock_primary_size: " << treeblock_primary_size << std::endl;
    myfile << "treeblock_nodes_size: " << treeblock_nodes_size << std::endl;

    
    tqdm bar2;
    TimeStamp check_diff = 0;
    uint64_t count_matched = 0;
    for (uint64_t i = 0; i < n_lines; i++){
        bar2.progress(i, n_lines);
        auto check_point = (*all_points)[i];

        bool found = true;
        if (check_point.get_coordinate(1) > 1400000000 || check_point.get_coordinate(1) < 1399000000)
            found  = false;
        if (check_point.get_coordinate(0) > 1400000000 || check_point.get_coordinate(0) < 1000000000)
            found  = false;
        if (check_point.get_coordinate(4) > 100000 || check_point.get_coordinate(4) < 1000)
            found = false;

        if (found)
            count_matched ++;
        // start = GetTimestamp();
        // if (!mdtrie->check(&check_point)){
        //     raise(SIGINT);
        // } 
        // check_diff += GetTimestamp() - start;  
    }
    bar2.finish();
    std::cout << "count: " << count_matched << std::endl;
    myfile << "Average time to check one point: " << (float) check_diff / n_lines << std::endl;

    


    data_point start_range_macro;
    data_point end_range_macro;  
    // [ "create_time,modify_time,access_time,change_time,owner_id,group_id"]

    for (dimension_t i = 0; i < 6; i++){
        start_range_macro.set_coordinate(i, min[i]);
        end_range_macro.set_coordinate(i, max[i]);

        if (i == 1){
            start_range_macro.set_coordinate(i, 1399000000);  //EXTENDEDPRICE <= 100000
            end_range_macro.set_coordinate(i, 1400000000);
        }
        if (i == 0)
        {
            start_range_macro.set_coordinate(i, 1000000000);  // TOTALPRICE >= 50000 (2dp)
            end_range_macro.set_coordinate(i, 1400000000);
        }
        if (i == 4){
            start_range_macro.set_coordinate(i, 1000);  // DISCOUNT >= 0.05
            end_range_macro.set_coordinate(i, 100000);
        }
    }
    point_array found_points_macro;
    start = GetTimestamp();
    mdtrie->range_search_trie(&start_range_macro, &end_range_macro, mdtrie->root(), 0, &found_points_macro);
    diff = GetTimestamp() - start;

    std::cout << "found points macro" << found_points_macro.size() << std::endl;
    // std::cout << "found points size: " << found_points->size() << std::endl;
    // std::cout << "Range Search Latency 1: " << (float) diff / found_points.size() << std::endl;
    std::cout << "Range Search end to end latency 1: " << diff << std::endl;    

    exit(0);
    line = nullptr;
    len = 0;
    fp = fopen("/home/ziming/phtree-cpp/build/filesys_phtree_queries_1000.csv", "r");
    int count = 0;
    diff = 0;
    std::ofstream file_range_search("filesys_mdtrie_queries_1000.csv");

    while ((read = getline(&line, &len, fp)) != -1)
    {
        data_point start_range;
        data_point end_range;      
        char *ptr;

        char *token = strtok(line, ","); // id
        token = strtok(nullptr, ",");
        token = strtok(nullptr, ",");

        for (dimension_t i = 0; i < DATA_DIMENSION; i++){
            token = strtok(nullptr, ","); // id
            start_range.set_coordinate(i, strtoul(token, &ptr, 10));
            token = strtok(nullptr, ",");
            end_range.set_coordinate(i, strtoul(token, &ptr, 10));
        }

        int present_pt_count = 0;
        for (unsigned int i = 0; i < all_points->size(); i++){
            bool match = true;
            for (dimension_t j = 0; j < DATA_DIMENSION; j++){
                if ( (*all_points)[i].get_coordinate(j) < start_range.get_coordinate(j) || (*all_points)[i].get_coordinate(j) > end_range.get_coordinate(j)){
                    match = false;
                    break;
                }
            }
            if (match){
                present_pt_count ++;
            }
        }   
        std::cout << "present point count: " << present_pt_count << std::endl;

        point_array found_points_temp;
        start = GetTimestamp();
        mdtrie->range_search_trie(&start_range, &end_range, mdtrie->root(), 0, &found_points_temp);
        TimeStamp temp_diff =  GetTimestamp() - start; 
        diff += temp_diff;

        count ++;   
        std::cout << "found_points_temp.size: " << primary_key_vector.size() << std::endl; 
        std::cout << "diff: " << temp_diff << std::endl;
        file_range_search << primary_key_vector.size() << "," << temp_diff << std::endl; 
        primary_key_vector.clear();
    }
    std::cout << "average query latency: " << (float) diff / count << std::endl;    

    exit(0);

    auto *start_range = new data_point();
    auto *end_range = new data_point();

    
    int itr = 0;
    std::ofstream file("range_search_filesystem.csv", std::ios_base::app);
    srand(time(NULL));

    tqdm bar3;
    while (itr < 600){
        bar3.progress(itr, 600);

        for (int j = 0; j < DIMENSION; j++){
            start_range->set_coordinate(j, min[j] + (max[j] - min[j] + 1) / 10 * (rand() % 10));
            end_range->set_coordinate(j, start_range->get_coordinate(j) + (max[j] - start_range->get_coordinate(j) + 1) / 10 * (rand() % 10));
        }

        auto *found_points_temp = new point_array();
        start = GetTimestamp();
        mdtrie->range_search_trie(start_range, end_range, mdtrie->root(), 0, found_points_temp);
        diff = GetTimestamp() - start;

        if (found_points_temp->size() >= 1000){
            file << found_points_temp->size() << "," << diff << "," << std::endl;
            itr ++;
        }
    }
    bar3.finish();

    for (dimension_t i = 0; i < DIMENSION; i++){
        start_range->set_coordinate(i, min[i]);
        end_range->set_coordinate(i, max[i]);
    }

    start = GetTimestamp();
    mdtrie->range_search_trie(start_range, end_range, mdtrie->root(), 0, found_points);
    diff = GetTimestamp() - start;

    myfile << "found_pts size: " << found_points->size() << std::endl;
    myfile << "Range Search Latency: " << (float) diff / found_points->size() << std::endl;


    n_leaves_t found_points_size = found_points->size();
    TimeStamp diff_primary = 0;

    n_leaves_t checked_points_size = 0;
    tqdm bar4;
    for (n_leaves_t i = 0; i < found_points_size; i += 5){
        checked_points_size++;

        bar4.progress(i, found_points_size);
        data_point *point = found_points->at(i);
        n_leaves_t returned_primary_key = point->read_primary();

        symbol_t *node_path_from_primary = (symbol_t *)malloc((max_depth + 1) * sizeof(symbol_t));

        tree_block *t_ptr = (tree_block *) (p_key_to_treeblock_compact.At(returned_primary_key));
        
        start = GetTimestamp();

        symbol_t parent_symbol_from_primary = t_ptr->get_node_path_primary_key(returned_primary_key, node_path_from_primary);
        node_path_from_primary[max_depth - 1] = parent_symbol_from_primary;

        data_point *returned_coordinates = t_ptr->node_path_to_coordinates(node_path_from_primary, DIMENSION);

        diff_primary += GetTimestamp() - start;

        for (dimension_t j = 0; j < DIMENSION; j++){
            if (returned_coordinates->get_coordinate(j) != point->get_coordinate(j)){
                raise(SIGINT);
            }
        }    
        auto correct_point = (* all_points)[returned_primary_key];
        for (dimension_t j = 0; j < DIMENSION; j++){
            if (returned_coordinates->get_coordinate(j) != correct_point.get_coordinate(j)){
                raise(SIGINT);
            }
        }           
        free(node_path_from_primary);
    }
    bar4.finish();
    myfile << "Lookup Latency: " << (float) diff_primary / checked_points_size << std::endl;
}

int main() {

    TREEBLOCK_SIZE = 512;
    TRIE_DEPTH = 10;
    myfile.open("filesystem_benchmark_" + std::to_string(DIMENSION) + "_" + std::to_string(TRIE_DEPTH) + "_" + std::to_string(TREEBLOCK_SIZE) + ".txt");
    std::vector<level_t> dimension_bits = {32, 32, 32, 32, 24, 24};

    is_osm = false;
    myfile << "dimension: " << DIMENSION << std::endl;
    myfile << "trie depth: " << TRIE_DEPTH << std::endl;
    myfile << "treeblock sizes: " << TREEBLOCK_SIZE << std::endl;
    run_bench(32, TRIE_DEPTH, TREEBLOCK_SIZE, dimension_bits);
    myfile << std::endl;
}