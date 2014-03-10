#pragma once

#include "json/json.h"

namespace Channel9 {
    namespace script {
        void parse_file(const std::string &filename, IStream &stream);
        void parse_string(const std::string &str, IStream &stream);
    }
}
