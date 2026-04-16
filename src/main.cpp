#include "ApiServer.h"

int main() {

    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator("data")) {
        if (entry.is_regular_file() && (entry.path().extension() == ".txt" || entry.path().extension() == ".md")) {
            files.push_back(entry.path().string());
        }
    }
    ApiServer server;
    server.run(2323);

    return 0;
}