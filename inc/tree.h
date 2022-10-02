#ifndef __TREE_H__ 
#define __TREE_H__
// Define a binary tree with <Key, Value> Pair search Function
#include "global.h"
#include "debug.h"


struct myTree*new_tree(int key, void *data);
struct myTree *find_key(struct myTree *root, int key);
void * get_value(struct myTree *root, int key);
void free_tree(myTree* root);
struct myTree *insert_tree(struct myTree *node, int key, void *value);
#endif // !__TREE_H__
