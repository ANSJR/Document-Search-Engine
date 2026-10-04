/*
 * TestTernarySearchTree.cpp
 *
 * The TST is the prefix-expansion side of search, and it is the one component
 * that hand-manages memory. Two things matter: countWords() must stay in lock
 * step with the inverted index (Indexer inserts on first sight and deletes when
 * a term's postings go empty), and deleteTerm() must not orphan or double-free
 * nodes on shared prefixes.
 *
 * Build this file under -fsanitize=address to get real value out of the
 * delete-heavy tests. See tests/NOTES.md for the make target.
 */

#include <gtest/gtest.h>

#include "TernarySearchTree.h"

#include <algorithm>
#include <string>
#include <vector>

namespace {

std::vector<std::string> sorted(std::vector<std::string> v) {
    std::sort(v.begin(), v.end());
    return v;
}

TEST(TernarySearchTree, EmptyTreeHasNoWords) {
    TernarySearchTree tst;
    EXPECT_EQ(tst.countWords(), 0);
    EXPECT_TRUE(tst.prefixSearch("anything").empty());
}

TEST(TernarySearchTree, CountWordsMatchesDistinctInserts) {
    TernarySearchTree tst;
    for (const char* const w : {"apple", "apply", "apt", "banana", "band"}) {
        tst.insert(w);
    }
    EXPECT_EQ(tst.countWords(), 5);
}

TEST(TernarySearchTree, DuplicateInsertsDoNotInflateCount) {
    // Indexer calls insert() only on try_emplace success, but the TST should be
    // idempotent regardless -- the serializer's load path merges shards and can
    // re-offer the same term.
    TernarySearchTree tst;
    tst.insert("apple");
    tst.insert("apple");
    tst.insert("apple");
    EXPECT_EQ(tst.countWords(), 1);
    EXPECT_EQ(tst.prefixSearch("app"), (std::vector<std::string>{"apple"}));
}

TEST(TernarySearchTree, SingleCharacterWordsWork) {
    TernarySearchTree tst;
    tst.insert("a");
    tst.insert("i");
    EXPECT_EQ(tst.countWords(), 2);
    EXPECT_EQ(tst.prefixSearch("a"), (std::vector<std::string>{"a"}));
}

TEST(TernarySearchTree, PrefixSearchReturnsEveryDescendant) {
    TernarySearchTree tst;
    for (const char* const w : {"apple", "apply", "apt", "banana"}) tst.insert(w);
    EXPECT_EQ(sorted(tst.prefixSearch("ap")),
              (std::vector<std::string>{"apple", "apply", "apt"}));
}

TEST(TernarySearchTree, PrefixSearchIncludesAnExactMatch) {
    // A query of "app" must return "app" itself, not just its extensions --
    // otherwise a complete word that happens to prefix another disappears from
    // results the moment the longer word is indexed.
    TernarySearchTree tst;
    tst.insert("app");
    tst.insert("apple");
    EXPECT_EQ(sorted(tst.prefixSearch("app")),
              (std::vector<std::string>{"app", "apple"}));
}

TEST(TernarySearchTree, PrefixSearchMissReturnsEmpty) {
    TernarySearchTree tst;
    tst.insert("apple");
    EXPECT_TRUE(tst.prefixSearch("zebra").empty());
    EXPECT_TRUE(tst.prefixSearch("applesauce").empty());
}

TEST(TernarySearchTree, PrefixSearchDoesNotMatchOnUnmarkedInteriorNodes) {
    // "ap" is a path through the tree but was never inserted as a word.
    TernarySearchTree tst;
    tst.insert("apple");
    const auto results = tst.prefixSearch("a");
    EXPECT_EQ(results, (std::vector<std::string>{"apple"}));
}

TEST(TernarySearchTree, DeleteTermRemovesOnlyThatWord) {
    TernarySearchTree tst;
    for (const char* const w : {"apple", "apply", "apt"}) tst.insert(w);
    tst.deleteTerm("apply");
    EXPECT_EQ(tst.countWords(), 2);
    EXPECT_EQ(sorted(tst.prefixSearch("ap")),
              (std::vector<std::string>{"apple", "apt"}));
}

TEST(TernarySearchTree, DeletingAPrefixWordKeepsTheLongerWord) {
    // The node for "app" is still on the path to "apple", so it must be
    // unmarked rather than freed.
    TernarySearchTree tst;
    tst.insert("app");
    tst.insert("apple");
    tst.deleteTerm("app");
    EXPECT_EQ(tst.countWords(), 1);
    EXPECT_EQ(tst.prefixSearch("app"), (std::vector<std::string>{"apple"}));
}

TEST(TernarySearchTree, DeletingALongerWordKeepsThePrefixWord) {
    TernarySearchTree tst;
    tst.insert("app");
    tst.insert("apple");
    tst.deleteTerm("apple");
    EXPECT_EQ(tst.countWords(), 1);
    EXPECT_EQ(tst.prefixSearch("app"), (std::vector<std::string>{"app"}));
}

TEST(TernarySearchTree, DeleteMissingTermIsANoop) {
    TernarySearchTree tst;
    tst.insert("apple");
    tst.deleteTerm("banana");
    tst.deleteTerm("appl");
    tst.deleteTerm("applesauce");
    tst.deleteTerm("");
    EXPECT_EQ(tst.countWords(), 1);
    EXPECT_EQ(tst.prefixSearch("app"), (std::vector<std::string>{"apple"}));
}

TEST(TernarySearchTree, DeletingEveryWordEmptiesTheTree) {
    TernarySearchTree tst;
    for (const char* const w : {"apple", "apply", "apt", "banana"}) tst.insert(w);
    for (const char* const w : {"apple", "apply", "apt", "banana"}) tst.deleteTerm(w);
    EXPECT_EQ(tst.countWords(), 0);
    EXPECT_TRUE(tst.prefixSearch("a").empty());
    EXPECT_TRUE(tst.prefixSearch("b").empty());
}

TEST(TernarySearchTree, InsertDeleteReinsertRoundTrips) {
    // This is exactly what a re-index does: removeFileFromIndex() drops the
    // term, then the rebuild puts it straight back.
    TernarySearchTree tst;
    tst.insert("gamma");
    tst.deleteTerm("gamma");
    EXPECT_EQ(tst.countWords(), 0);
    tst.insert("gamma");
    EXPECT_EQ(tst.countWords(), 1);
    EXPECT_EQ(tst.prefixSearch("gam"), (std::vector<std::string>{"gamma"}));
}

TEST(TernarySearchTree, SurvivesManyInsertsAndInterleavedDeletes) {
    // 1000 three-letter words across a dense keyspace, then half deleted.
    // Under ASan this is the test that catches an orphaned or double-freed node
    // in deleteHelper().
    TernarySearchTree tst;
    std::vector<std::string> keys;
    for (char a = 'a'; a <= 'j'; ++a) {
        for (char b = 'a'; b <= 'j'; ++b) {
            for (char c = 'a'; c <= 'j'; ++c) {
                keys.push_back(std::string{a, b, c});
            }
        }
    }
    for (const auto& key : keys) tst.insert(key);
    ASSERT_EQ(tst.countWords(), static_cast<int>(keys.size()));

    for (std::size_t i = 0; i < keys.size(); i += 2) tst.deleteTerm(keys[i]);
    EXPECT_EQ(tst.countWords(), static_cast<int>(keys.size() / 2));

    for (std::size_t i = 1; i < keys.size(); i += 2) {
        EXPECT_FALSE(tst.prefixSearch(keys[i]).empty()) << "lost surviving key " << keys[i];
    }
}

// An empty prefix has no defined behaviour today. Pick one -- "every word" is
// the usual choice, since it makes prefixSearch("") a cheap full dump for the
// /index/Health endpoint -- then enable this with the matching expectation.
TEST(TernarySearchTree, DISABLED_EmptyPrefixReturnsEveryWord) {
    TernarySearchTree tst;
    tst.insert("alpha");
    tst.insert("beta");
    EXPECT_EQ(sorted(tst.prefixSearch("")), (std::vector<std::string>{"alpha", "beta"}));
}

}  // namespace