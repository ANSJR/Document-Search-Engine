/*
 * TestIndexer.cpp
 *
 * Covers the inverted index itself: posting construction, the generation
 * counter that the serializer's stale-job check depends on, re-index and
 * removal semantics, and BM25 scoring.
 *
 * Every test writes to its own temp corpus, so the suite is safe to run in
 * parallel (--gtest_shuffle and ctest -j both work).
 */

#include <gtest/gtest.h>

#include "Indexer.h"
#include "TernarySearchTree.h"
#include "TestHelpers.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <thread>
#include <string>
#include <vector>

namespace {

using dse_test::TempDir;

void overwrite(const std::filesystem::path& file, const std::string& contents) {
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    out << contents;
}

class IndexerTest : public ::testing::Test {
protected:
    TempDir corpus;
    Indexer indexer;
    TernarySearchTree tst;
};

// ------------------------------------------------------------ basic indexing

TEST_F(IndexerTest, EmptyIndexHasNoTerms) {
    EXPECT_EQ(indexer.getTotalIndexTerms(), 0);
    EXPECT_TRUE(indexer.getIndex().empty());
    EXPECT_FALSE(indexer.filePresent(corpus.path() / "nothing.txt"));
}

TEST_F(IndexerTest, IndexingOneFileRegistersEveryDistinctTerm) {
    const auto doc = corpus.writeFile("a.txt", "alpha beta beta");
    indexer.buildIndex(doc, tst);

    EXPECT_EQ(indexer.getTotalIndexTerms(), 2);
    EXPECT_TRUE(indexer.filePresent(doc));

    const auto& index = indexer.getIndex();
    ASSERT_EQ(index.count("alpha"), 1u);
    ASSERT_EQ(index.count("beta"), 1u);
    ASSERT_EQ(index.at("beta").count(doc), 1u);
    EXPECT_EQ(index.at("beta").at(doc).size(), 2u);
}

TEST_F(IndexerTest, PostingsCarryTokenPositionsAndByteOffsetsInOrder) {
    // "alpha beta beta" -- beta at token 1/byte 6 and token 2/byte 11.
    // Ascending order is a contract: positionalIntersect walks these with two
    // pointers and silently returns nothing if they're shuffled.
    const auto doc = corpus.writeFile("a.txt", "alpha beta beta");
    indexer.buildIndex(doc, tst);

    const auto& postings = indexer.getIndex().at("beta").at(doc);
    ASSERT_EQ(postings.size(), 2u);
    EXPECT_EQ(postings[0].tokenPos, 1u);
    EXPECT_EQ(postings[0].byteOffset, 6u);
    EXPECT_EQ(postings[1].tokenPos, 2u);
    EXPECT_EQ(postings[1].byteOffset, 11u);
}

TEST_F(IndexerTest, TermAppearingInTwoFilesHasTwoPostingLists) {
    const auto a = corpus.writeFile("a.txt", "shared alpha");
    const auto b = corpus.writeFile("b.txt", "shared beta");
    indexer.buildIndex(std::vector<std::filesystem::path>{a, b}, tst);

    const auto& shared = indexer.getIndex().at("shared");
    EXPECT_EQ(shared.size(), 2u);
    EXPECT_EQ(shared.count(a), 1u);
    EXPECT_EQ(shared.count(b), 1u);
}

TEST_F(IndexerTest, TreeAndIndexAgreeOnTermCount) {
    // This is the invariant /index/Health prints. If it ever drifts, prefix
    // search starts returning terms that aren't in the index and computeScore
    // throws out_of_range on a live request.
    const auto a = corpus.writeFile("a.txt", "alpha beta gamma");
    const auto b = corpus.writeFile("b.txt", "beta delta");
    indexer.buildIndex(std::vector<std::filesystem::path>{a, b}, tst);

    EXPECT_EQ(indexer.getTotalIndexTerms(), 4);
    EXPECT_EQ(tst.countWords(), indexer.getTotalIndexTerms());
}

TEST_F(IndexerTest, EmptyFileIsIndexedWithNoTerms) {
    const auto doc = corpus.writeFile("empty.txt", "");
    indexer.buildIndex(doc, tst);
    EXPECT_TRUE(indexer.filePresent(doc));
    EXPECT_EQ(indexer.getTotalIndexTerms(), 0);
}

// ---------------------------------------------------------------- generation

TEST_F(IndexerTest, GenerationIsZeroForAFileNeverIndexed) {
    EXPECT_EQ(indexer.getFileGen(corpus.path() / "ghost.txt"), 0u);
}

TEST_F(IndexerTest, GenerationStartsAtOneAndIncrementsOnEveryReindex) {
    // serializerLoop() drops a queued job when job.generation no longer matches
    // getFileGen(). That check is only meaningful if generation strictly
    // increases, so pin it hard.
    const auto doc = corpus.writeFile("a.txt", "alpha");

    indexer.buildIndex(doc, tst);
    EXPECT_EQ(indexer.getFileGen(doc), 1u);

    indexer.buildIndex(doc, tst);
    EXPECT_EQ(indexer.getFileGen(doc), 2u);

    indexer.buildIndex(doc, tst);
    EXPECT_EQ(indexer.getFileGen(doc), 3u);
}

TEST_F(IndexerTest, BatchIndexingBumpsGenerationTheSameWay) {
    // The single-file and fork-join paths duplicate this logic in two places.
    // If they ever diverge, stale-job invalidation breaks on one path only.
    const auto doc = corpus.writeFile("a.txt", "alpha");
    const std::vector<std::filesystem::path> files{doc};

    indexer.buildIndex(files, tst);
    EXPECT_EQ(indexer.getFileGen(doc), 1u);

    indexer.buildIndex(files, tst);
    EXPECT_EQ(indexer.getFileGen(doc), 2u);
}

TEST_F(IndexerTest, GenerationIsPerFile) {
    const auto a = corpus.writeFile("a.txt", "alpha");
    const auto b = corpus.writeFile("b.txt", "beta");
    indexer.buildIndex(a, tst);
    indexer.buildIndex(a, tst);
    indexer.buildIndex(b, tst);

    EXPECT_EQ(indexer.getFileGen(a), 2u);
    EXPECT_EQ(indexer.getFileGen(b), 1u);
}

TEST_F(IndexerTest, GenerationResetsAfterExplicitRemoval) {
    const auto doc = corpus.writeFile("a.txt", "alpha");
    indexer.buildIndex(doc, tst);
    indexer.buildIndex(doc, tst);
    ASSERT_EQ(indexer.getFileGen(doc), 2u);

    indexer.removeFileFromIndex(doc, tst);
    EXPECT_EQ(indexer.getFileGen(doc), 0u);

    indexer.buildIndex(doc, tst);
    EXPECT_EQ(indexer.getFileGen(doc), 1u);
}

// ------------------------------------------------------------------ re-index

TEST_F(IndexerTest, ReindexReplacesPostingsRatherThanAppending) {
    const auto doc = corpus.writeFile("a.txt", "alpha alpha");
    indexer.buildIndex(doc, tst);
    ASSERT_EQ(indexer.getIndex().at("alpha").at(doc).size(), 2u);

    overwrite(doc, "alpha");
    indexer.buildIndex(doc, tst);

    EXPECT_EQ(indexer.getIndex().at("alpha").at(doc).size(), 1u);
}

TEST_F(IndexerTest, ReindexDropsTermsNoLongerInTheFile) {
    const auto doc = corpus.writeFile("a.txt", "alpha beta");
    indexer.buildIndex(doc, tst);
    ASSERT_EQ(indexer.getTotalIndexTerms(), 2);

    overwrite(doc, "alpha");
    indexer.buildIndex(doc, tst);

    EXPECT_EQ(indexer.getIndex().count("beta"), 0u);
    EXPECT_EQ(indexer.getTotalIndexTerms(), 1);
    EXPECT_EQ(tst.countWords(), 1);
}

TEST_F(IndexerTest, ReindexPicksUpNewTerms) {
    const auto doc = corpus.writeFile("a.txt", "alpha");
    indexer.buildIndex(doc, tst);

    overwrite(doc, "alpha omega");
    indexer.buildIndex(doc, tst);

    EXPECT_EQ(indexer.getIndex().count("omega"), 1u);
    EXPECT_EQ(indexer.getTotalIndexTerms(), 2);
    EXPECT_EQ(tst.countWords(), 2);
}

TEST_F(IndexerTest, ReindexDoesNotDoubleCountDocumentLength) {
    // totalTokensInIndex feeds avgDocLen in BM25. If the re-index path forgets
    // to subtract the old token count, every score in the corpus drifts.
    const auto a = corpus.writeFile("a.txt", "alpha alpha alpha");
    const auto b = corpus.writeFile("b.txt", "beta beta beta");
    indexer.buildIndex(std::vector<std::filesystem::path>{a, b}, tst);
    const double before = indexer.computeScore(a, "alpha");

    indexer.buildIndex(a, tst);  // identical content, re-indexed

    EXPECT_NEAR(indexer.computeScore(a, "alpha"), before, 1e-12);
}

// ------------------------------------------------------------------- removal

TEST_F(IndexerTest, RemovingAFileClearsItsTermsFromIndexAndTree) {
    const auto a = corpus.writeFile("a.txt", "alpha shared");
    const auto b = corpus.writeFile("b.txt", "beta shared");
    indexer.buildIndex(std::vector<std::filesystem::path>{a, b}, tst);
    ASSERT_EQ(indexer.getTotalIndexTerms(), 3);

    indexer.removeFileFromIndex(a, tst);

    EXPECT_FALSE(indexer.filePresent(a));
    EXPECT_EQ(indexer.getIndex().count("alpha"), 0u);   // unique to a -- gone
    ASSERT_EQ(indexer.getIndex().count("shared"), 1u);  // still lives in b
    EXPECT_EQ(indexer.getIndex().at("shared").count(a), 0u);
    EXPECT_EQ(indexer.getTotalIndexTerms(), 2);
    EXPECT_EQ(tst.countWords(), indexer.getTotalIndexTerms());
}

TEST_F(IndexerTest, RemovingAnUnknownFileIsANoop) {
    const auto a = corpus.writeFile("a.txt", "alpha");
    indexer.buildIndex(a, tst);

    indexer.removeFileFromIndex(corpus.path() / "never_indexed.txt", tst);

    EXPECT_EQ(indexer.getTotalIndexTerms(), 1);
    EXPECT_TRUE(indexer.filePresent(a));
}

TEST_F(IndexerTest, RemovingEveryFileEmptiesIndexAndTree) {
    const auto a = corpus.writeFile("a.txt", "alpha shared");
    const auto b = corpus.writeFile("b.txt", "beta shared");
    indexer.buildIndex(std::vector<std::filesystem::path>{a, b}, tst);

    indexer.removeFileFromIndex(a, tst);
    indexer.removeFileFromIndex(b, tst);

    EXPECT_EQ(indexer.getTotalIndexTerms(), 0);
    EXPECT_EQ(tst.countWords(), 0);
    EXPECT_TRUE(indexer.getIndex().empty());
}

// ------------------------------------------------------------------ readText

TEST_F(IndexerTest, ReadTextReturnsFileContentsVerbatim) {
    const std::string contents = "alpha\nbeta\tgamma\r\n";
    const auto doc = corpus.writeFile("a.txt", contents);
    EXPECT_EQ(indexer.readText(doc), contents);
}

TEST_F(IndexerTest, ReadTextThrowsOnMissingFile) {
    EXPECT_THROW(indexer.readText(corpus.path() / "nope.txt"), std::runtime_error);
}

// buildIndex() catches the read failure, logs to cerr, and then registers the
// file with zero tokens. A missing file is now indistinguishable from an empty
// one, and fileToTerms carries a document that can never match -- which still
// inflates N and drags avgDocLen down for every real document.
TEST_F(IndexerTest, DISABLED_IndexingAMissingFileDoesNotRegisterIt) {
    const auto ghost = corpus.path() / "nope.txt";
    indexer.buildIndex(ghost, tst);
    EXPECT_FALSE(indexer.filePresent(ghost));
}

// --------------------------------------------------------------- BM25 scores

class Bm25Test : public ::testing::Test {
protected:
    void SetUp() override {
        docA = corpus.writeFile("a.txt", "alpha beta beta");  // 3 tokens
        docB = corpus.writeFile("b.txt", "alpha gamma");      // 2 tokens
        indexer.buildIndex(std::vector<std::filesystem::path>{docA, docB}, tst);
    }

    TempDir corpus;
    Indexer indexer;
    TernarySearchTree tst;
    std::filesystem::path docA;
    std::filesystem::path docB;
};

TEST_F(Bm25Test, MatchesHandComputedScore) {
    // N = 2, df("beta") = 1, tf = 2, docLen = 3, avgDocLen = 5/2 = 2.5
    // k1 = 1.5, b = 0.75
    //   idf = ln((2 - 1 + 0.5) / (1 + 0.5) + 1)         = ln(2)   = 0.6931472
    //   num = 2 * (1.5 + 1)                             = 5.0
    //   den = 2 + 1.5 * (1 - 0.75 + 0.75 * (3 / 2.5))   = 3.725
    // score = 0.6931472 * 5.0 / 3.725                   = 0.930399
    EXPECT_NEAR(indexer.computeScore(docA, "beta"), 0.930399, 1e-5);
}

TEST_F(Bm25Test, ShorterDocumentWinsForTheSameTermAndFrequency) {
    // "alpha" occurs once in each document; docB is shorter, so length
    // normalization must favour it.
    EXPECT_GT(indexer.computeScore(docB, "alpha"), indexer.computeScore(docA, "alpha"));
}

TEST_F(Bm25Test, RarerTermOutranksCommonTermInTheSameDocument) {
    // "beta" is in 1 of 2 docs; "alpha" is in both.
    EXPECT_GT(indexer.computeScore(docA, "beta"), indexer.computeScore(docA, "alpha"));
}

TEST_F(Bm25Test, IdfStaysPositiveWhenEveryDocumentContainsTheTerm) {
    // The +1 inside the log is what keeps this from going negative. Classic
    // BM25 without it would score "alpha" below zero here and sort matching
    // documents underneath non-matching ones.
    EXPECT_GT(indexer.computeScore(docA, "alpha"), 0.0);
    EXPECT_GT(indexer.computeScore(docB, "alpha"), 0.0);
}

TEST_F(Bm25Test, ThrowsForATermThatIsNotInTheIndex) {
    EXPECT_THROW(indexer.computeScore(docA, "notatermanywhere"), std::out_of_range);
}

TEST_F(Bm25Test, ThrowsWhenTheTermIsNotInThatDocument) {
    // Engine::search only ever passes (file, term) pairs it pulled out of the
    // index, so this never fires in production -- but the contract should be
    // written down rather than discovered during a refactor.
    EXPECT_THROW(indexer.computeScore(docB, "beta"), std::out_of_range);
}

TEST(Bm25Saturation, TermFrequencyContributionIsSublinear) {
    // Both documents are 10 tokens long and both contain "beta", so docLen,
    // avgDocLen and idf are all identical. Only tf differs: 1 vs 2. BM25's
    // saturation means the second occurrence must help, but less than the
    // first -- doubling tf must not double the score.
    TempDir corpus;
    Indexer indexer;
    TernarySearchTree tst;
    const auto few = corpus.writeFile(
        "few.txt", "beta filler filler filler filler filler filler filler filler filler");
    const auto many = corpus.writeFile(
        "many.txt", "beta beta filler filler filler filler filler filler filler filler");
    indexer.buildIndex(std::vector<std::filesystem::path>{few, many}, tst);

    const double onceScore = indexer.computeScore(few, "beta");
    const double twiceScore = indexer.computeScore(many, "beta");

    EXPECT_GT(twiceScore, onceScore);
    EXPECT_LT(twiceScore, 2.0 * onceScore);
}

// --------------------------------------------------------- fork-join merging

TEST(IndexerConcurrency, BatchIndexingIsDeterministic) {
    // buildIndex(vector) forks one std::async task per file in chunks of
    // hardware_concurrency() and merges in future order, not completion order.
    // Two runs over the same corpus must therefore produce byte-identical
    // indexes. Build this under -fsanitize=thread for the version that actually
    // proves the partial workers don't touch shared state.
    TempDir corpus;
    std::vector<std::filesystem::path> files;
    for (int i = 0; i < 16; ++i) {
        files.push_back(corpus.writeFile(
            "doc" + std::to_string(i) + ".txt",
            "alpha beta gamma doc" + std::to_string(i) + " shared shared term" +
                std::to_string(i % 3)));
    }

    Indexer first;
    TernarySearchTree firstTst;
    first.buildIndex(files, firstTst);

    Indexer second;
    TernarySearchTree secondTst;
    second.buildIndex(files, secondTst);

    EXPECT_TRUE(dse_test::indexesMatch(first.getIndex(), second.getIndex()));
    EXPECT_EQ(first.getTotalIndexTerms(), second.getTotalIndexTerms());
    EXPECT_EQ(firstTst.countWords(), secondTst.countWords());
    EXPECT_EQ(firstTst.countWords(), first.getTotalIndexTerms());
}

TEST(IndexerConcurrency, BatchSpansMultipleForkJoinChunks) {
    // More files than cores, so the chunk loop runs at least twice and the
    // merge has to survive being handed a second wave of futures.
    TempDir corpus;
    const std::size_t count = std::thread::hardware_concurrency() * 3 + 5;
    std::vector<std::filesystem::path> files;
    for (std::size_t i = 0; i < count; ++i) {
        files.push_back(corpus.writeFile("doc" + std::to_string(i) + ".txt",
                                         "common unique" + std::to_string(i)));
    }

    Indexer indexer;
    TernarySearchTree tst;
    indexer.buildIndex(files, tst);

    EXPECT_EQ(indexer.getTotalIndexTerms(), static_cast<int>(count) + 1);  // uniques + "common"
    EXPECT_EQ(indexer.getIndex().at("common").size(), count);
    EXPECT_EQ(tst.countWords(), indexer.getTotalIndexTerms());
}

}  // namespace