#pragma once

#include "script/compiler.hpp"

#include <vector>
#include <string>

namespace Channel9
{
    namespace script
    {
        node_ptr parse_file(const std::string &filename);
    }
}
