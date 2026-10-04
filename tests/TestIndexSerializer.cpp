/*
 * TestIndexSerializer.cpp
 *
 * The persistence layer exists because the full corpus does not fit in RAM, so
 * "it round-trips" is the whole value proposition. These tests assert that a
 * save/load cycle reproduces not just the postings but everything BM25 reads --
 * token counts and the corpus total -- because an index that survives a restart
 * with different scores is worse than one that fails loudly.
 *
 * Engine writes each shard as <indexBin>/<filename>.bin and points load() at
 * the directory. shardFor() mirrors that, so these tests exercise the same
 * layout production does.
 */

#include <gtest/gtest.h>

#include "IndexSerializer.h"
#include "Indexer.h"
#include "TernarySearchTree.h"
#include "TestHelpers.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

using dse_test::TempDir;

std::filesystem::path shardFor(const std::filesystem::path& indexBin,
                               const std::filesystem::path& source) {
    return (indexBin / (source.filename().string() + ".bin")).lexically_normal();
}

class SerializerTest : public ::testing::Test {
protected:
    TempDir corpus;    // source documents
    TempDir indexBin;  // serialized shards
};

// ---------------------------------------------------------------------- save

TEST_F(SerializerTest, SaveWritesANonEmptyShard) {
    const auto doc = corpus.writeFile("a.txt", "alpha beta beta");
    Indexer indexer;
    TernarySearchTree tst;
    indexer.buildIndex(doc, tst);

    const auto shard = shardFor(indexBin.path(), doc);
    ASSERT_TRUE(IndexSerializer::save(indexer, doc, shard));
    ASSERT_TRUE(std::filesystem::exists(shard));
    EXPECT_GT(std::filesystem::file_size(shard), 0u);
}

TEST_F(SerializerTest, SavingTwiceOverwritesRatherThanAppends) {
    const auto doc = corpus.writeFile("a.txt", "alpha beta beta");
    Indexer indexer;
    TernarySearchTree tst;
    indexer.buildIndex(doc, tst);
    const auto shard = shardFor(indexBin.path(), doc);

    ASSERT_TRUE(IndexSerializer::save(indexer, doc, shard));
    const auto firstSize = std::filesystem::file_size(shard);
    ASSERT_TRUE(IndexSerializer::save(indexer, doc, shard));

    EXPECT_EQ(std::filesystem::file_size(shard), firstSize);
}

// ---------------------------------------------------------------- round trip

TEST_F(SerializerTest, RoundTripRestoresPostingsExactly) {
    const auto a = corpus.writeFile("a.txt", "alpha beta beta");
    const auto b = corpus.writeFile("b.txt", "beta gamma");

    Indexer original;
    TernarySearchTree originalTst;
    original.buildIndex(std::vector<std::filesystem::path>{a, b}, originalTst);
    ASSERT_TRUE(IndexSerializer::save(original, a, shardFor(indexBin.path(), a)));
    ASSERT_TRUE(IndexSerializer::save(original, b, shardFor(indexBin.path(), b)));

    Indexer restored;
    TernarySearchTree restoredTst;
    IndexSerializer::load(restored, restoredTst, indexBin.path());

    EXPECT_TRUE(dse_test::indexesMatch(original.getIndex(), restored.getIndex()));
    EXPECT_EQ(restored.getTotalIndexTerms(), original.getTotalIndexTerms());
    EXPECT_TRUE(restored.filePresent(a));
    EXPECT_TRUE(restored.filePresent(b));
}

TEST_F(SerializerTest, RoundTripRebuildsTheTernarySearchTree) {
    // load() has to repopulate the TST as well as the index, or prefix search
    // comes back empty after a restart while exact search keeps working --
    // which is exactly the kind of bug that only shows up in production.
    const auto a = corpus.writeFile("a.txt", "alpha alphabet alpine");
    Indexer original;
    TernarySearchTree originalTst;
    original.buildIndex(a, originalTst);
    ASSERT_TRUE(IndexSerializer::save(original, a, shardFor(indexBin.path(), a)));

    Indexer restored;
    TernarySearchTree restoredTst;
    IndexSerializer::load(restored, restoredTst, indexBin.path());

    EXPECT_EQ(restoredTst.countWords(), originalTst.countWords());
    EXPECT_EQ(restoredTst.countWords(), restored.getTotalIndexTerms());
    EXPECT_EQ(restoredTst.prefixSearch("alp").size(), 3u);
}

TEST_F(SerializerTest, RoundTripPreservesBm25Inputs) {
    // Scores depend on tokenCount and totalTokensInIndex, which live in
    // FileMetadata rather than in the postings. If those don't survive, search
    // still "works" after a restart but ranks documents differently.
    const auto a = corpus.writeFile("a.txt", "alpha beta beta");
    const auto b = corpus.writeFile("b.txt", "alpha gamma");

    Indexer original;
    TernarySearchTree originalTst;
    original.buildIndex(std::vector<std::filesystem::path>{a, b}, originalTst);
    ASSERT_TRUE(IndexSerializer::save(original, a, shardFor(indexBin.path(), a)));
    ASSERT_TRUE(IndexSerializer::save(original, b, shardFor(indexBin.path(), b)));

    Indexer restored;
    TernarySearchTree restoredTst;
    IndexSerializer::load(restored, restoredTst, indexBin.path());

    EXPECT_NEAR(restored.computeScore(a, "beta"), original.computeScore(a, "beta"), 1e-12);
    EXPECT_NEAR(restored.computeScore(a, "alpha"), original.computeScore(a, "alpha"), 1e-12);
    EXPECT_NEAR(restored.computeScore(b, "alpha"), original.computeScore(b, "alpha"), 1e-12);
}

TEST_F(SerializerTest, RoundTripPreservesGeneration) {
    // If generation resets to 0 or 1 on load, the serializer worker's stale-job
    // check compares against the wrong baseline after every restart.
    const auto a = corpus.writeFile("a.txt", "alpha");
    Indexer original;
    TernarySearchTree originalTst;
    original.buildIndex(a, originalTst);
    original.buildIndex(a, originalTst);
    ASSERT_EQ(original.getFileGen(a), 2u);
    ASSERT_TRUE(IndexSerializer::save(original, a, shardFor(indexBin.path(), a)));

    Indexer restored;
    TernarySearchTree restoredTst;
    IndexSerializer::load(restored, restoredTst, indexBin.path());

    EXPECT_EQ(restored.getFileGen(a), 2u);
}

TEST_F(SerializerTest, RoundTripSurvivesUnicodeAndPunctuationInContent) {
    const std::string curly = std::string("\xE2\x80\x99");
    const auto a = corpus.writeFile("a.txt", "don" + curly + "t stop well-known things");

    Indexer original;
    TernarySearchTree originalTst;
    original.buildIndex(a, originalTst);
    ASSERT_TRUE(IndexSerializer::save(original, a, shardFor(indexBin.path(), a)));

    Indexer restored;
    TernarySearchTree restoredTst;
    IndexSerializer::load(restored, restoredTst, indexBin.path());

    EXPECT_TRUE(dse_test::indexesMatch(original.getIndex(), restored.getIndex()));
    EXPECT_EQ(restored.getIndex().count("don't"), 1u);
    EXPECT_EQ(restored.getIndex().count("well-known"), 1u);
}

TEST_F(SerializerTest, LoadingAnEmptyDirectoryLeavesEverythingEmpty) {
    Indexer restored;
    TernarySearchTree restoredTst;
    IndexSerializer::load(restored, restoredTst, indexBin.path());

    EXPECT_EQ(restored.getTotalIndexTerms(), 0);
    EXPECT_EQ(restoredTst.countWords(), 0);
}

TEST_F(SerializerTest, LoadIgnoresNonBinFilesInTheDirectory) {
    // Engine's indexBin is a plain directory a user can point anywhere. A
    // stray README or .DS_Store must not be parsed as a shard.
    const auto a = corpus.writeFile("a.txt", "alpha");
    Indexer original;
    TernarySearchTree originalTst;
    original.buildIndex(a, originalTst);
    ASSERT_TRUE(IndexSerializer::save(original, a, shardFor(indexBin.path(), a)));
    { std::ofstream readme(indexBin.path() / "README.md"); readme << "not a shard"; }

    Indexer restored;
    TernarySearchTree restoredTst;
    IndexSerializer::load(restored, restoredTst, indexBin.path());

    EXPECT_EQ(restored.getTotalIndexTerms(), 1);
    EXPECT_TRUE(restored.filePresent(a));
}

TEST_F(SerializerTest, LoadingManyShardsRebuildsTheWholeCorpus) {
    // Exercises the parallel load path across more shards than cores.
    Indexer original;
    TernarySearchTree originalTst;
    std::vector<std::filesystem::path> files;
    for (int i = 0; i < 24; ++i) {
        files.push_back(corpus.writeFile("doc" + std::to_string(i) + ".txt",
                                         "shared term" + std::to_string(i) + " filler filler"));
    }
    original.buildIndex(files, originalTst);
    for (const auto& file : files) {
        ASSERT_TRUE(IndexSerializer::save(original, file, shardFor(indexBin.path(), file)));
    }

    Indexer restored;
    TernarySearchTree restoredTst;
    IndexSerializer::load(restored, restoredTst, indexBin.path());

    EXPECT_TRUE(dse_test::indexesMatch(original.getIndex(), restored.getIndex()));
    EXPECT_EQ(restored.getIndex().at("shared").size(), files.size());
    EXPECT_EQ(restoredTst.countWords(), restored.getTotalIndexTerms());
}

// -------------------------------------------------------------------- delete

TEST_F(SerializerTest, DeleteFileRemovesTheShard) {
    const auto a = corpus.writeFile("a.txt", "alpha");
    Indexer indexer;
    TernarySearchTree tst;
    indexer.buildIndex(a, tst);
    const auto shard = shardFor(indexBin.path(), a);
    ASSERT_TRUE(IndexSerializer::save(indexer, a, shard));
    ASSERT_TRUE(std::filesystem::exists(shard));

    EXPECT_TRUE(IndexSerializer::deleteFile(a, indexBin.path()));
    EXPECT_FALSE(std::filesystem::exists(shard));
}

TEST_F(SerializerTest, DeletedShardDoesNotReappearOnLoad) {
    const auto a = corpus.writeFile("a.txt", "alpha");
    const auto b = corpus.writeFile("b.txt", "beta");
    Indexer original;
    TernarySearchTree originalTst;
    original.buildIndex(std::vector<std::filesystem::path>{a, b}, originalTst);
    ASSERT_TRUE(IndexSerializer::save(original, a, shardFor(indexBin.path(), a)));
    ASSERT_TRUE(IndexSerializer::save(original, b, shardFor(indexBin.path(), b)));
    ASSERT_TRUE(IndexSerializer::deleteFile(a, indexBin.path()));

    Indexer restored;
    TernarySearchTree restoredTst;
    IndexSerializer::load(restored, restoredTst, indexBin.path());

    EXPECT_FALSE(restored.filePresent(a));
    EXPECT_TRUE(restored.filePresent(b));
    EXPECT_EQ(restored.getIndex().count("alpha"), 0u);
    EXPECT_EQ(restored.getTotalIndexTerms(), 1);
}

TEST_F(SerializerTest, DeletingAShardThatWasNeverWrittenDoesNotThrow) {
    const auto ghost = corpus.path() / "never_saved.txt";
    EXPECT_NO_THROW(IndexSerializer::deleteFile(ghost, indexBin.path()));
}

// ---------------------------------------------------- contracts to nail down

// save() returns bool but Engine::serializerLoop ignores it, so a failed write
// is currently silent. Decide what save() does when asked for a file the
// indexer has never seen -- returning false is the useful answer -- then enable
// this and check the result at the call site.
TEST_F(SerializerTest, DISABLED_SavingAnUnindexedFileReturnsFalse) {
    const auto ghost = corpus.path() / "never_indexed.txt";
    Indexer indexer;
    EXPECT_FALSE(IndexSerializer::save(indexer, ghost, shardFor(indexBin.path(), ghost)));
}

// A truncated or corrupt shard is the realistic failure mode here: the
// background worker can be killed mid-write. load() should reject the bad shard
// and keep going rather than crash or half-populate the index.
TEST_F(SerializerTest, DISABLED_CorruptShardIsRejectedRatherThanCrashing) {
    {
        std::ofstream garbage(indexBin.path() / "garbage.txt.bin", std::ios::binary);
        garbage << "this is definitely not a serialized index";
    }
    Indexer restored;
    TernarySearchTree restoredTst;
    EXPECT_NO_THROW(IndexSerializer::load(restored, restoredTst, indexBin.path()));
    EXPECT_EQ(restored.getTotalIndexTerms(), 0);
}

TEST_F(SerializerTest, DISABLED_TruncatedShardDoesNotPartiallyPopulateTheIndex) {
    const auto a = corpus.writeFile("a.txt", "alpha beta gamma delta epsilon");
    Indexer original;
    TernarySearchTree originalTst;
    original.buildIndex(a, originalTst);
    const auto shard = shardFor(indexBin.path(), a);
    ASSERT_TRUE(IndexSerializer::save(original, a, shard));

    const auto full = std::filesystem::file_size(shard);
    std::filesystem::resize_file(shard, full / 2);

    Indexer restored;
    TernarySearchTree restoredTst;
    EXPECT_NO_THROW(IndexSerializer::load(restored, restoredTst, indexBin.path()));
    EXPECT_EQ(restored.getTotalIndexTerms(), 0);
    EXPECT_FALSE(restored.filePresent(a));
}

}  // namespace