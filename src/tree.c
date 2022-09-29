#include "../inc/tree.h"
// An implementaion of AVL ==> 平衡搜索二叉树
// 

// Calculate height
int height(myTree *N) {
  if (N == NULL)
    return 0;
  return N->height;
}

int max(int a, int b) {
  return (a > b) ? a : b;
}


/**
* @brief Create a new Tree(Node)
*
* @param key
* @param data
*
* @return 
*/
struct myTree*new_tree(int key, void *data) {
  struct myTree *node = (struct myTree*)
    malloc(sizeof(struct myTree));
  node->key = key;
  node->value = data;
  node->left = NULL;
  node->right = NULL;
  node->height = 1;
  return (node);
}

// Right rotate
struct myTree *rightRotate(struct myTree *y) {
  struct myTree *x = y->left;
  struct myTree *T2 = x->right;

  x->right = y;
  y->left = T2;

  y->height = max(height(y->left), height(y->right)) + 1;
  x->height = max(height(x->left), height(x->right)) + 1;

  return x;
}

// Left rotate
struct myTree *leftRotate(struct myTree *x) {
  struct myTree *y = x->right;
  struct myTree *T2 = y->left;

  y->left = x;
  x->right = T2;

  x->height = max(height(x->left), height(x->right)) + 1;
  y->height = max(height(y->left), height(y->right)) + 1;

  return y;
}

// Get the balance factor
int getBalance(struct myTree *N) {
  if (N == NULL)
    return 0;
  return height(N->left) - height(N->right);
}


/**
* @brief Inertion
*
* @param node
* @param key
* @param value
*
* @return 
*/
struct myTree *insert_tree(struct myTree *node, int key, void *value) {
  // Find the correct position to insertmyTree the node and insertmyTree it
  if (node == NULL)
    return (new_tree(key, value));

  if (key < node->key)
    node->left = insert_tree(node->left, key, value);
  else if (key > node->key)
    node->right = insert_tree(node->right, key, value);
  else
    return node;

  // Update the balance factor of each node and
  // Balance the tree
  node->height = 1 + max(height(node->left),
               height(node->right));

  int balance = getBalance(node);
  if (balance > 1 && key < node->left->key)
    return rightRotate(node);

  if (balance < -1 && key > node->right->key)
    return leftRotate(node);

  if (balance > 1 && key > node->left->key) {
    node->left = leftRotate(node->left);
    return rightRotate(node);
  }

  if (balance < -1 && key < node->right->key) {
    node->right = rightRotate(node->right);
    return leftRotate(node);
  }

  return node;
}

struct myTree *minValuemyTree(struct myTree *node) {
  struct myTree *current = node;

  while (current->left != NULL)
    current = current->left;

  return current;
}

/**
* @brief get value with Key
*
* @param root
* @param key
*
* @return 
*/
struct myTree *find_key(struct myTree *root, int key) {
  // Find the node and delete it
  if (root == NULL)
    return root;

  if (key < root->key)
    root->left = find_key(root->left, key);

  else if (key > root->key)
    root->right = find_key(root->right, key);

  else {
    if ((root->left == NULL) || (root->right == NULL)) {
      struct myTree *temp = root->left ? root->left : root->right;

      if (temp == NULL) {
        temp = root;
        root = NULL;
      } else
        *root = *temp;
      free(temp);
    } else {
      struct myTree *temp = minValuemyTree(root->right);

      root->key = temp->key;

      root->right = find_key(root->right, temp->key);
    }
  }

  if (root == NULL)
    return root;

  // Update the balance factor of each node and
  // balance the tree
  root->height = 1 + max(height(root->left),
               height(root->right));

  int balance = getBalance(root);
  if (balance > 1 && getBalance(root->left) >= 0)
    return rightRotate(root);

  if (balance > 1 && getBalance(root->left) < 0) {
    root->left = leftRotate(root->left);
    return rightRotate(root);
  }

  if (balance < -1 && getBalance(root->right) <= 0)
    return leftRotate(root);

  if (balance < -1 && getBalance(root->right) > 0) {
    root->right = rightRotate(root->right);
    return leftRotate(root);
  }

  return root;
}

void free_tree(myTree* root){
  if(root==NULL) return;
  if(root->left!=NULL) free_tree(root->left);
  if(root->right!=NULL) free_tree(root->right);
  if(root->value!=NULL)
    free(root->value);
  free(root);
  return;
}
