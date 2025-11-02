
#ifndef TERNARYSEARCHTREE_H
#define TERNARYSEARCHTREE_H
#include <string>
#include <vector>

class TreeNode {
public:
    char data;
    bool marked = false;
    TreeNode* lChild;
    TreeNode* mChild;
    TreeNode* rChild;
    TreeNode(char data);
    TreeNode(bool marked);
    TreeNode(char data, bool marked);
    TreeNode(char data, TreeNode* lChild, TreeNode* mChild, TreeNode* rChild);
};
class TernarySearchTree {
private:
    TreeNode* root;

    void traverseTST(TreeNode* root, char* buffer, int depth);
    TreeNode* insertHelper(TreeNode* curr, const std::string& key, int keyIndex);
public:
    TernarySearchTree();
    ~TernarySearchTree();

    void insert(const std::string& key);
    void prefixHelper(TreeNode* node, std::string current, std::vector<std::string>& results) const;
    std::vector<std::string> prefixSearch(const std::string& prefix) const;
    void printTST();
};

#endif 