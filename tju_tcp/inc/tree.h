#ifndef __TREE_H__ 
#define __TREE_H__
// Define a binary tree with <Key, Value> Pair search Function
#include "global.h"
#include "debug.h"
// Create Node
typedef struct treeNode{
  int key;
  void *value;
  struct treeNode*left;
  struct treeNode*right;
  int height;
}treeNode;
typedef struct myTree{
  struct treeNode *root;
  uint32_t size;
}myTree;
tju_packet_t* get_value(struct myTree *root, int key);
void free_tree(myTree* root);
void insert_key_value(myTree *node, int key, void *value);
void print_tree(myTree *root);
struct myTree* init_tree();
void remove_blow(struct treeNode* root, int key);
#endif // !__TREE_H__
