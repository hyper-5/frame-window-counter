#pragma once
#include <functional>

namespace FileIO {
    void exportFWC();
    void importReplay(std::function<void()> onSuccessCallback);
}