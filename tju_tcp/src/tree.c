
#include "../inc/tree.h"
// An implementaion of AVL ==> 平衡搜索二叉树
// 

struct treeNode*minValuetreeNode(struct treeNode*node, struct treeNode**pre) {
  struct treeNode*current = node;
  while (current->left != NULL){
    *pre = current;
    current = current->left;
  }
  return current;
}

void free_tree_node(struct treeNode* root){
  _debug_("Start Free\n");
  if(root==NULL) return;
  _debug_("Root Not NULL: %d\n" ,root->key);
  if(root->left!=NULL) {_debug_("free Left\n");free_tree_node(root->left);}
  if(root->right!=NULL) {_debug_("free right\n");free_tree_node(root->right);}
  if(root->value!=NULL){free(root->value); root->value = NULL;}
  free(root); root = NULL;
  return;
}
struct myTree* init_tree(){
  struct myTree* ret = malloc(sizeof(struct myTree));
  ret->root = NULL;
  ret->size = 0;
  return ret;
}
// Calculate height
int height(treeNode*N) {
  if (N == NULL)
    return 0;
  return N->height;
}


/**
* @brief Create a new Tree(Node)
*
* @param key
* @param data
*
* @return 
*/
struct treeNode*new_tree_node(int key, void *data) {
  _debug_("New Node: %d\n" , key);
  struct treeNode*node = (struct treeNode*)
    malloc(sizeof(struct treeNode));
  node->key = key;
  node->value = data;
  node->left = NULL;
  node->right = NULL;
  node->height = 1;
  return (node);
}

// Right rotate
struct treeNode *rightRotate(struct treeNode *y) {
  struct treeNode *x = y->left;
  struct treeNode *T2 = x->right;

  x->right = y;
  y->left = T2;

  y->height = max(height(y->left), height(y->right)) + 1;
  x->height = max(height(x->left), height(x->right)) + 1;

  return x;
}

// Left rotate
struct treeNode *leftRotate(struct treeNode *x) {
  struct treeNode *y = x->right;
  struct treeNode *T2 = y->left;

  y->left = x;
  x->right = T2;

  x->height = max(height(x->left), height(x->right)) + 1;
  y->height = max(height(y->left), height(y->right)) + 1;

  return y;
}

// Get the balance factor
int getBalance(struct treeNode *N) {
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
struct treeNode *insert_tree(struct treeNode *node, int key, void *value) {
  // Find the correct position to inserttreeNode the node and inserttreeNode it
  if (node == NULL){
    return (new_tree_node(key, value));
  }

  if (key < node->key){
    if(node->left==NULL){
      node->left =  insert_tree(node->left, key, value);
    }else{
      node->left = insert_tree(node->left, key, value);
    }
  }else if (key > node->key){
    if(node->right==NULL){
      node->right =  insert_tree(node->right, key, value);
    }else{
      node->right= insert_tree(node->right, key, value);
    }
  }else return node;

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

void insert_key_value(myTree *tree, int key, void *value){
  tree->root = insert_tree(tree->root, key, value);
  tree->size ++;
  print_tree(tree);
  return ;
}


// struct treeNode *remove_key_below(struct treeNode *root, int key) {
//   _debug_("FIND KEY\n");
//   // Find the node and delete it
//   if (root == NULL){
//     _debug_("root null\n");
//     return root;
//   }
//   _debug_("rootk:%d <-> key:%d\n",root->key, key);
//
//   if (key < root->key)
//     root->left = remove_key_below(root->left, key);
//   else {
//       // WHEN key >= root->key ==> Remove All 
//       _debug_("Found key: %d\n" ,key);
//       if ((root->left == NULL) || (root->right == NULL)) {
//         struct treeNode *temp = root->left ? root->left : root->right;
//
//         if (temp == NULL) {
//           temp = root;
//           root = NULL;
//         } else
//         *root = *temp;
//         free_tree_node(temp);
//       } else {
//         struct treeNode *temp = minValuetreeNode(root->right);
//
//         root->key = temp->key;
//
//         root->right = remove_key_below(root->right, temp->key);
//       }
//     }
//
//   if (root == NULL)
//     return root;
//
//   // Update the balance factor of each node and
//   // balance the tree
//   root->height = 1 + max(height(root->left),
//                          height(root->right));
//
//   int balance = getBalance(root);
//   if (balance > 1 && getBalance(root->left) >= 0)
//     return rightRotate(root);
//
//
//   if (balance > 1 && getBalance(root->left) < 0) {
//     root->left = leftRotate(root->left);
//     return rightRotate(root);
//   }
//
//   if (balance < -1 && getBalance(root->right) <= 0)
//     return leftRotate(root);
//
//   if (balance < -1 && getBalance(root->right) > 0) {
//     root->right = rightRotate(root->right);
//     return leftRotate(root);
//   }
//
//   return root;
// }
//
// void remove_blow(struct treeNode* root, int key){
//   // Recursively Remove ALl Nodes with key less than key
//   struct treeNode *ret = remove_key_below(root, key);
//   while(ret!=NULL){
//     _debug_("Recursively Remove Node, key: %d <-> %d\n" ,key,ret->key);
//     ret = remove_key_below(root, key);
//   }
//   return ;
// }


/**
* @brief get value with Key
*
* @param root
* @param key
*
* @return 
*/
struct treeNode *find_key(struct treeNode *root, int key, struct treeNode** ret, int flag) {
  if (root == NULL){
    return root;
  }
  // _debug_("rootk:%d <-> key:%d\n",root->key, key);
  if (key < root->key){
    root->left = find_key(root->left, key, ret, flag);
  }else if (key > root->key){
    root->right = find_key(root->right, key, ret, flag);
  }else {
    _debug_("Found key: %d\n" ,key);
    if ((root->left == NULL) || (root->right == NULL)) {
      if(root->left == NULL && root->right==NULL){
      }else if(root->left == NULL){
      }else{
      }
      if(flag){
        if(*ret == NULL) *ret = malloc(sizeof(treeNode));
        memcpy(*ret, root, sizeof(treeNode));
      }
      // WARN: Potential Memory Leaking
      struct treeNode *temp = root->left ? root->left : root->right;
      if (temp == NULL) {
        temp = root;
        root = NULL;
      } else{
        *root = *temp;
      }
    } else {
      struct treeNode *pre;
      struct treeNode *temp = minValuetreeNode(root->right, &pre);

      if(pre!=NULL){
        pre->left = NULL; // TODO: check 这个去除的对不对
      }
      if(flag){
        if(*ret == NULL) *ret = malloc(sizeof(treeNode));
        memcpy(*ret, root, sizeof(treeNode));
      }
      root->key = temp->key;
      memcpy(root->value, temp->value, sizeof(tju_packet_t));
      root->right = find_key(root->right, temp->key, ret, 0); // 关闭内容拷贝

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

// Print the tree
void printTree(treeNode*root, char indent[], int last) {
  char tmp[1000];
  memset(tmp, 0, sizeof(tmp));
  memcpy(tmp, indent, strlen(indent));
  if (root != NULL) {
    printf("%s",indent);
    if (last) {
      printf("R----");
      strcat(tmp, "   ");
    } else {
      printf("L----");
      strcat(tmp, "|  ");
    }
    printf("%d\n" ,root->key);
    printTree(root->left, tmp, FALSE);
    printTree(root->right, tmp, TRUE);
  }
}

void print_tree(myTree *root){
  if(!DEBUG_FLAG) return;
  if(1){
    _debug_("Current Tree Number %d\n",root->size);
    return ;
  }else{

    char tmp[10000];
    memset(tmp, 0, sizeof(tmp));
    tmp[0] = '\t';
    _debug_line_("Begin Tree Print");
    printTree(root->root, tmp, TRUE);
    _debug_line_("End Tree Print");
  }
}

tju_packet_t* get_value(struct myTree *root, int key){
  treeNode* ret = NULL;
  root->root = find_key(root->root, key, &ret, 1);
  if(ret==NULL){
    return NULL;
  }else{
    root->size --;
    return ret->value;
  }
}


void free_tree(struct myTree* node){
  free_tree_node(node->root);
  free(node);
  node = NULL;
}
