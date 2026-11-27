#pragma once
#include <functional>
#include <string>

namespace FileIO {
    void exportFWC();
    void importReplay(std::function<void()> onSuccessCallback);

    void exportToFile(const std::filesystem::path& path, bool isJson);
    void importFromFile(const std::filesystem::path& path, std::function<void()> onSuccessCallback);
    std::string getDefaultExportName();
}