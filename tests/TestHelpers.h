/*
 * TestHelpers.h
 *
 * Shared scaffolding for the DSE test suite:
 *   - TempDir: an RAII scratch directory, so no test touches the real corpus
 *     or the real indexBin.
 *   - indexesMatch(): a deep comparison of two inverted indexes, used by the
 *     serializer round-trip and determinism tests.
 */

#ifndef DSE_TEST_HELPERS_H
#define DSE_TEST_HELPERS_H

#include <gtest/gtest.h>

#include "WordLocation.h"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace dse_test {

using Index = std::unordered_map<
    std::string,
    std::unordered_map<std::filesystem::path, std::vector<WordLocation>>>;

/*
 * A unique directory under the system temp dir, removed on destruction.
 * Paths handed back are weakly_canonical, matching what ApiServer does before
 * it calls into Engine, so test paths hash the same way production ones do.
 */
class TempDir {
public:
    TempDir() {
        static std::atomic<unsigned long long> counter{0};
        std::random_device rd;
        const unsigned long long unique =
            (static_cast<unsigned long long>(rd()) << 20) ^ counter.fetch_add(1);
        dir_ = std::filesystem::temp_directory_path() / ("dse_test_" + std::to_string(unique));
        std::filesystem::create_directories(dir_);
        dir_ = std::filesystem::weakly_canonical(dir_);
    }

    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(dir_, ec);
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    const std::filesystem::path& path() const { return dir_; }

    // Creates (or truncates) a file and returns its canonical path.
    std::filesystem::path writeFile(const std::string& name, const std::string& contents) const {
        const auto p = dir_ / name;
        std::ofstream out(p, std::ios::binary | std::ios::trunc);
        out << contents;
        out.close();
        return std::filesystem::weakly_canonical(p);
    }

private:
    std::filesystem::path dir_;
};

inline bool sameLocations(const std::vector<WordLocation>& lhs,
                          const std::vector<WordLocation>& rhs) {
    if (lhs.size() != rhs.size()) return false;
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (lhs[i].tokenPos != rhs[i].tokenPos) return false;
        if (lhs[i].byteOffset != rhs[i].byteOffset) return false;
    }
    return true;
}

/*
 * Deep-compares two indexes, including posting order. Posting order matters:
 * Searcher::positionalIntersect assumes ascending tokenPos, so a round trip
 * that shuffles postings would silently break phrase queries.
 */
inline ::testing::AssertionResult indexesMatch(const Index& lhs, const Index& rhs) {
    if (lhs.size() != rhs.size()) {
        return ::testing::AssertionFailure()
               << "term count differs: " << lhs.size() << " vs " << rhs.size();
    }
    for (const auto& [term, lhsFiles] : lhs) {
        const auto rhsIt = rhs.find(term);
        if (rhsIt == rhs.end()) {
            return ::testing::AssertionFailure() << "term missing on the right: " << term;
        }
        const auto& rhsFiles = rhsIt->second;
        if (lhsFiles.size() != rhsFiles.size()) {
            return ::testing::AssertionFailure()
                   << "term '" << term << "' appears in " << lhsFiles.size()
                   << " files on the left and " << rhsFiles.size() << " on the right";
        }
        for (const auto& [file, lhsLocations] : lhsFiles) {
            const auto rhsFileIt = rhsFiles.find(file);
            if (rhsFileIt == rhsFiles.end()) {
                return ::testing::AssertionFailure()
                       << "term '" << term << "' missing file on the right: " << file.string();
            }
            if (!sameLocations(lhsLocations, rhsFileIt->second)) {
                return ::testing::AssertionFailure()
                       << "postings differ for term '" << term << "' in " << file.string();
            }
        }
    }
    return ::testing::AssertionSuccess();
}

}  // namespace dse_test

#endif