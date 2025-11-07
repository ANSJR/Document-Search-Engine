
#include <../include/TernarySearchTree.h>
#include <iostream>
#include <iomanip>
#include <vector>



TreeNode::TreeNode(char data) : data(data), marked(false), lChild(nullptr), mChild(nullptr), rChild(nullptr) {}
TreeNode::TreeNode(bool marked) : marked(marked), lChild(nullptr), mChild(nullptr), rChild(nullptr) {}
TreeNode::TreeNode(char data, bool marked) : data(data), marked(false), lChild(nullptr), mChild(nullptr), rChild(nullptr) {}
TreeNode::TreeNode(char data, TreeNode* lChild, TreeNode* mChild, TreeNode* rChild) : data(data), marked(false), lChild(lChild), mChild(mChild), rChild(rChild) {}


TernarySearchTree::TernarySearchTree() {
    root = nullptr;
}
TernarySearchTree::~TernarySearchTree() = default;

void TernarySearchTree::insert(const std::string& key) {
    if (key.empty()) {return;}
    root = insertHelper(root, key, 0);
}
TreeNode* TernarySearchTree::insertHelper(TreeNode* curr, const std::string& key, int keyIndex) {
    if (curr == nullptr) {
        curr = new TreeNode(key[keyIndex]);
    }
    if (curr->data > key[keyIndex]) {
        curr->lChild = insertHelper(curr->lChild, key, keyIndex);
    }
    else if (curr->data < key[keyIndex]) {
        curr->rChild = insertHelper(curr->rChild, key, keyIndex);
    }
    else { 
        if (keyIndex + 1 < key.size())
            curr->mChild = insertHelper(curr->mChild, key, keyIndex + 1);
        else
            curr->marked = true;
    }
    return curr;
}

void TernarySearchTree::prefixHelper(TreeNode* node, std::string current, std::vector<std::string>& results) const {
    if (!node) return;

    prefixHelper(node->lChild, current, results);
    std::string nextCurrent = current + node->data;  // Append current node's char
    if (node->marked) 
        results.push_back(nextCurrent);

    prefixHelper(node->mChild, nextCurrent, results);
    prefixHelper(node->rChild, current, results);
}


std::vector<std::string> TernarySearchTree::prefixSearch(const std::string& prefix) const {
    std::vector<std::string> results;
    if (!root || prefix.empty()) return results;

    TreeNode* node = root;
    int index = 0;

    while (node) {
        char c = prefix[index];
        if (c < node->data) node = node->lChild;
        else if (c > node->data) node = node->rChild;
        else {
            index++;
            if (index == prefix.size())
                break; 
            node = node->mChild;
        }
    }

    if (!node) return results;

    if (node->marked)   // If the final prefix node is a complete word, add it
        results.push_back(prefix);

    prefixHelper(node->mChild, prefix, results); 
    // Print results for debugging
    for (const auto& res : results) {
        std::cout << "Prefix match found: " << res << std::endl;
    }
    return results;
}



void TernarySearchTree::traverseTST(TreeNode* root, char* buffer, int depth)
{
    if (root) {
        traverseTST(root->lChild, buffer, depth);
        buffer[depth] = root->data;
        if (root->marked) {
            buffer[depth + 1] = '\0';
            std::cout << buffer << std::endl;
        }
        traverseTST(root->mChild, buffer, depth + 1);
        traverseTST(root->rChild, buffer, depth);
    }
}
void TernarySearchTree::printTST()
{
    int MAX = 200;
    char buffer[MAX];
    traverseTST(root, buffer, 0);
}