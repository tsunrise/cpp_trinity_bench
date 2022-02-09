#ifndef MD_TRIE_TRIE_NODE_H
#define MD_TRIE_TRIE_NODE_H

#include "defs.h"
#include <cstdlib>
#include "tree_block.h"
#include <sys/mman.h>

template<dimension_t DIMENSION>
class trie_node {
    
public:

    explicit trie_node(bool is_leaf, dimension_t num_dimensions) {
        
        // is_leaf_ = is_leaf;
        if (!is_leaf){
            trie_or_treeblock_ptr_ = (trie_node<DIMENSION> **)calloc(sizeof(trie_node<DIMENSION> *), 1 << num_dimensions);
        }
    }

    inline trie_node<DIMENSION> *get_child(morton_t symbol) {

        auto trie_ptr = (trie_node<DIMENSION> **) trie_or_treeblock_ptr_;
        return trie_ptr[symbol];
    }

    inline void set_child(morton_t symbol, trie_node *node) {
        auto trie_ptr = (trie_node<DIMENSION> **) trie_or_treeblock_ptr_;
        trie_ptr[symbol] = node;
    }

    inline tree_block<DIMENSION> *get_block() const {
        // if (!is_leaf_)
        //     return nullptr;
        return (tree_block<DIMENSION> *)trie_or_treeblock_ptr_;
    }

    inline void set_block(tree_block<DIMENSION> *block) {

        trie_or_treeblock_ptr_ = block;
        // is_leaf_ = true;
    }

    void get_node_path(level_t level, std::vector<morton_t> &node_path){

        if (parent_trie_node_){
            node_path[level - 1] = parent_symbol_;
            parent_trie_node_->get_node_path(level - 1, node_path);
        }
    }

    trie_node<DIMENSION> *get_parent_trie_node(){

        return parent_trie_node_;
    }

    void set_parent_trie_node(trie_node<DIMENSION> *node){

        parent_trie_node_ = node;
    }

    morton_t get_parent_symbol(){
        return parent_symbol_;
    }

    void set_parent_symbol(morton_t symbol){

        parent_symbol_ = symbol;
    }

    // bool is_leaf(){
    //     return is_leaf_;
    // }

    uint64_t size(level_t num_children, bool is_leaf) {

        // uint64_t total_size = sizeof(is_leaf_) + sizeof(trie_or_treeblock_ptr_); 
        uint64_t total_size = sizeof(trie_or_treeblock_ptr_); 
        total_size += sizeof(parent_trie_node_); // parent_trie_node_
        total_size += sizeof(parent_symbol_); // parent_symbol_

        // if (!is_leaf_)
        if (is_leaf)
            total_size += sizeof(trie_node<DIMENSION> *) * num_children;

        return total_size;
    }


    virtual size_t Serialize(std::ostream& out, level_t num_children, bool is_leaf) {

        size_t out_size = 0; 

        // void *trie_or_treeblock_ptr_ = NULL;
        if (is_leaf && false) {

            for (uint64_t i = 0; i < num_children; i++) {
                out.write(reinterpret_cast<const char *>(&((trie_node<DIMENSION> **) trie_or_treeblock_ptr_)[i]), sizeof(trie_node<DIMENSION> *));
                out_size += sizeof(trie_node<DIMENSION> *);
            }

            // out.write(reinterpret_cast<const char *>(trie_or_treeblock_ptr_), sizeof(trie_node<DIMENSION> *) * num_children);
            // out_size += sizeof(trie_node<DIMENSION> *) * num_children;    
        }
        else {
            out.write(reinterpret_cast<const char *>(&trie_or_treeblock_ptr_), sizeof(trie_or_treeblock_ptr_));
            out_size += sizeof(trie_or_treeblock_ptr_);
        }

        // trie_node *parent_trie_node_;
        out.write(reinterpret_cast<const char *>(&parent_trie_node_), sizeof(parent_trie_node_));
        out_size += sizeof(parent_trie_node_);

        // symbol_t parent_symbol_ = 0; 
        out.write(reinterpret_cast<const char *>(&parent_symbol_), sizeof(parent_symbol_));
        out_size += sizeof(parent_symbol_);       


        return out_size;
    } 

    virtual size_t Deserialize(std::istream& in, level_t num_children, bool is_leaf) {

        size_t in_size = 0;


        // void *trie_or_treeblock_ptr_ = NULL;
        if (is_leaf && false) {


            trie_or_treeblock_ptr_ = static_cast<trie_node<DIMENSION> **>(malloc(sizeof(trie_node<DIMENSION> *) * num_children));

            for (uint64_t i = 0; i < num_children; i++) {
                in.read(reinterpret_cast<char *>(&((trie_node<DIMENSION> **) trie_or_treeblock_ptr_)[i]), sizeof(trie_node<DIMENSION> *));
                in_size += sizeof(trie_node<DIMENSION> *);
            }

            // in.read(reinterpret_cast<char *>(trie_or_treeblock_ptr_), sizeof(trie_node<DIMENSION> *) * num_children);
            // in_size += sizeof(trie_node<DIMENSION> *) * num_children;    
        }
        else {
            in.read(reinterpret_cast<char *>(&trie_or_treeblock_ptr_), sizeof(trie_or_treeblock_ptr_));
            in_size += sizeof(trie_or_treeblock_ptr_);
        }

        // trie_node *parent_trie_node_;
        in.read(reinterpret_cast<char *>(&parent_trie_node_), sizeof(parent_trie_node_));
        in_size += sizeof(parent_trie_node_);

        // symbol_t parent_symbol_ = 0; 
        in.read(reinterpret_cast<char *>(&parent_symbol_), sizeof(parent_symbol_));
        in_size += sizeof(parent_symbol_);           


        return in_size;
    } 

private:
    // bool is_leaf_ = false;
    void *trie_or_treeblock_ptr_ = NULL;
    trie_node<DIMENSION> *parent_trie_node_ = NULL; 
    morton_t parent_symbol_ = 0; 
};

#endif //MD_TRIE_TRIE_NODE_H
