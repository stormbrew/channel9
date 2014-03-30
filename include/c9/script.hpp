#pragma once

#include "json/json.h"

namespace Channel9 {
    namespace script {
        void compile_file(const std::string &filename, IStream &stream);
    }
}
