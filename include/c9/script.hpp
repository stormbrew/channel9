#pragma once

#include "json/json.h"

namespace Channel9 {
    namespace script {
        namespace parser {
            void parse_file(const std::string &filename, IStream &stream);
            void parse_string(const std::string &str, IStream &stream);
        }
    }
}
