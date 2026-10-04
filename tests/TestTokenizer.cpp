/*
 * TestTokenizer.cpp
 *
 * Tokenizer is the bottom of the stack: every byteOffset the API returns for
 * highlighting, and every tokenPos the positional intersect relies on,
 * originates here. These tests pin both.
 *
 * Tests prefixed DISABLED_ document bugs that exist today. Run them with:
 *     ./runTests --gtest_also_run_disabled_tests
 */

#include <gtest/gtest.h>

#include "Tokenizer.h"

#include <cstdint>
#include <string>
#include <vector>

namespace {

using Tokens = std::vector<std::pair<std::string, WordLocation>>;

std::vector<std::string> words(const Tokens& tokens) {
    std::vector<std::string> out;
    out.reserve(tokens.size());
    for (const auto& token : tokens) out.push_back(token.first);
    return out;
}

// The UTF-8 right single quotation mark (U+2019). Built by concatenation so no
// adjacent character can be swallowed into the \x escape.
const std::string kCurlyApostrophe = std::string("\xE2\x80\x99");

// ---------------------------------------------------------------- tokenize()

TEST(TokenizerTokenize, EmptyTextProducesNoTokens) {
    Tokenizer tokenizer;
    EXPECT_TRUE(tokenizer.tokenize("").empty());
}

TEST(TokenizerTokenize, PunctuationOnlyProducesNoTokens) {
    Tokenizer tokenizer;
    EXPECT_TRUE(tokenizer.tokenize("!!! ??? ... ,,, ;;;").empty());
}

TEST(TokenizerTokenize, LowercasesAndStripsPunctuation) {
    Tokenizer tokenizer;
    EXPECT_EQ(words(tokenizer.tokenize("Hello, WORLD!")),
              (std::vector<std::string>{"hello", "world"}));
}

TEST(TokenizerTokenize, AssignsSequentialTokenPositions) {
    Tokenizer tokenizer;
    const auto tokens = tokenizer.tokenize("the quick brown fox");
    ASSERT_EQ(tokens.size(), 4u);
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        EXPECT_EQ(tokens[i].second.tokenPos, static_cast<uint64_t>(i))
            << "wrong position for token '" << tokens[i].first << "'";
    }
}

TEST(TokenizerTokenize, ByteOffsetPointsAtFirstByteOfWord) {
    const std::string text = "the quick brown fox";
    Tokenizer tokenizer;
    const auto tokens = tokenizer.tokenize(text);
    ASSERT_EQ(tokens.size(), 4u);

    EXPECT_EQ(tokens[0].second.byteOffset, 0u);
    EXPECT_EQ(tokens[1].second.byteOffset, 4u);
    EXPECT_EQ(tokens[2].second.byteOffset, 10u);
    EXPECT_EQ(tokens[3].second.byteOffset, 16u);

    // The offset must slice the original text back to the token. This is the
    // property the /search highlighting actually depends on.
    for (const auto& token : tokens) {
        EXPECT_EQ(text.compare(token.second.byteOffset, token.first.size(), token.first), 0)
            << "offset " << token.second.byteOffset << " does not slice back to '"
            << token.first << "'";
    }
}

TEST(TokenizerTokenize, CollapsesRunsOfDelimiters) {
    Tokenizer tokenizer;
    const auto tokens = tokenizer.tokenize("alpha    beta\n\n\tgamma");
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].second.tokenPos, 0u);
    EXPECT_EQ(tokens[1].second.tokenPos, 1u);
    EXPECT_EQ(tokens[2].second.tokenPos, 2u);
    EXPECT_EQ(tokens[1].second.byteOffset, 9u);
    EXPECT_EQ(tokens[2].second.byteOffset, 16u);
}

TEST(TokenizerTokenize, KeepsInternalHyphensAndApostrophes) {
    Tokenizer tokenizer;
    EXPECT_EQ(words(tokenizer.tokenize("well-known")),
              (std::vector<std::string>{"well-known"}));
    EXPECT_EQ(words(tokenizer.tokenize("don't")),
              (std::vector<std::string>{"don't"}));
}

TEST(TokenizerTokenize, NormalizesCurlyApostropheToAscii) {
    Tokenizer tokenizer;
    const auto tokens = tokenizer.tokenize("don" + kCurlyApostrophe + "t");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].first, "don't");
    EXPECT_EQ(tokens[0].second.byteOffset, 0u);
}

TEST(TokenizerTokenize, CurlyApostropheDoesNotSkewLaterOffsets) {
    // "it’s here" -- the apostrophe is 3 bytes, so "here" starts at byte 7.
    // If the multi-byte skip were wrong, every later offset would drift.
    const std::string text = "it" + kCurlyApostrophe + "s here";
    Tokenizer tokenizer;
    const auto tokens = tokenizer.tokenize(text);
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].first, "it's");
    EXPECT_EQ(tokens[0].second.byteOffset, 0u);
    EXPECT_EQ(tokens[1].first, "here");
    EXPECT_EQ(tokens[1].second.byteOffset, 7u);
    EXPECT_EQ(tokens[1].second.tokenPos, 1u);
}

TEST(TokenizerTokenize, FlushesTrailingTokenAtEndOfText) {
    Tokenizer tokenizer;
    const auto tokens = tokenizer.tokenize("alpha beta");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[1].first, "beta");
    EXPECT_EQ(tokens[1].second.tokenPos, 1u);
}

TEST(TokenizerTokenize, TrailingDelimiterDoesNotEmitAPhantomToken) {
    Tokenizer tokenizer;
    EXPECT_EQ(tokenizer.tokenize("alpha beta   ").size(), 2u);
    EXPECT_EQ(tokenizer.tokenize("alpha beta.\n").size(), 2u);
}

TEST(TokenizerTokenize, DigitsAreTokens) {
    Tokenizer tokenizer;
    EXPECT_EQ(words(tokenizer.tokenize("c++17 released in 2017")),
              (std::vector<std::string>{"c", "17", "released", "in", "2017"}));
}

// A bare '-' or '\'' currently becomes its own token, because the token-char
// test does not require at least one alphanumeric. That pollutes the index and
// the TST with terms nobody will ever search for. Decide whether you want it;
// if not, require an alnum before pushing and enable this.
TEST(TokenizerTokenize, DISABLED_StandaloneHyphenIsNotAToken) {
    Tokenizer tokenizer;
    EXPECT_EQ(words(tokenizer.tokenize("alpha - beta")),
              (std::vector<std::string>{"alpha", "beta"}));
}

// ---------------------------------------------------------- simpleTokenize()

TEST(TokenizerSimpleTokenize, AgreesWithTokenizeOnWordSequence) {
    Tokenizer tokenizer;
    const std::string text = "The Quick, brown fox-trot don't stop";
    EXPECT_EQ(tokenizer.simpleTokenize(text), words(tokenizer.tokenize(text)));
}

TEST(TokenizerSimpleTokenize, EmptyTextProducesNoTokens) {
    Tokenizer tokenizer;
    EXPECT_TRUE(tokenizer.simpleTokenize("").empty());
}

// ---------------------------------------------------------- getTotalTokens()

TEST(TokenizerGetTotalTokens, AgreesWithTokenizeOnSimpleText) {
    Tokenizer tokenizer;
    const std::string text = "hello world";
    EXPECT_EQ(tokenizer.getTotalTokens(text), tokenizer.tokenize(text).size());
}

// getTotalTokens() seeds its counter at 1 to account for a final token that may
// never be flushed, so it over-counts whenever there isn't one. It is also dead
// code right now -- Indexer uses tokenize().size(). Either fix the seed or
// delete the method; leaving a wrong token count reachable is a ranking bug
// waiting to happen, since docLen feeds BM25 directly.
TEST(TokenizerGetTotalTokens, DISABLED_EmptyTextHasZeroTokens) {
    Tokenizer tokenizer;
    EXPECT_EQ(tokenizer.getTotalTokens(""), 0u);
}

TEST(TokenizerGetTotalTokens, DISABLED_TrailingDelimiterDoesNotAddPhantomToken) {
    Tokenizer tokenizer;
    EXPECT_EQ(tokenizer.getTotalTokens("hello world "), 2u);
}

TEST(TokenizerGetTotalTokens, DISABLED_PunctuationOnlyTextHasZeroTokens) {
    Tokenizer tokenizer;
    EXPECT_EQ(tokenizer.getTotalTokens("!!! ???"), 0u);
}

// -------------------------------------------------------- isolateLastToken()

TEST(TokenizerIsolateLastToken, ReturnsTextAfterFinalSpace) {
    Tokenizer tokenizer;
    EXPECT_EQ(tokenizer.isolateLastToken("machine learning algo"), "algo");
}

TEST(TokenizerIsolateLastToken, SingleWordIsReturnedWhole) {
    Tokenizer tokenizer;
    EXPECT_EQ(tokenizer.isolateLastToken("single"), "single");
}

TEST(TokenizerIsolateLastToken, EmptyQueryYieldsEmptyToken) {
    Tokenizer tokenizer;
    EXPECT_EQ(tokenizer.isolateLastToken(""), "");
}

TEST(TokenizerIsolateLastToken, TrailingSpaceMeansNoPartialToken) {
    Tokenizer tokenizer;
    EXPECT_EQ(tokenizer.isolateLastToken("machine "), "");
}

// isolateLastToken() only splits on ' ', while tokenize() splits on every
// non-alphanumeric. A query arriving with a tab or punctuation therefore yields
// a "partial token" that can never match anything in the TST. Reuse the
// tokenizer's own delimiter rule here and enable this.
TEST(TokenizerIsolateLastToken, DISABLED_SplitsOnTheSameDelimitersAsTokenize) {
    Tokenizer tokenizer;
    EXPECT_EQ(tokenizer.isolateLastToken("machine\tlear"), "lear");
    EXPECT_EQ(tokenizer.isolateLastToken("machine,lear"), "lear");
}

}  // namespace