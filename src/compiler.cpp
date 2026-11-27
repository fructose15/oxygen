#include "interpreter.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace oxygen {

static std::string escape_cpp_raw_string(const std::string &src) {
    return "R\"OXYGEN_SCRIPT(" + src + ")OXYGEN_SCRIPT\"";
}

int compile_script_to_binary(const std::string &script_path,
                             const std::string &output_path,
                             const std::string &oxygen_root) {
    std::ifstream in(script_path);
    if (!in.is_open()) {
        std::cerr << "Error: cannot open Oxygen script '" << script_path << "'" << std::endl;
        return 1;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string script_content = ss.str();

    std::string tmp_cpp = output_path + ".oxygen_compiled.cpp";
    std::ofstream out(tmp_cpp);
    if (!out.is_open()) {
        std::cerr << "Error: cannot create temporary build file '" << tmp_cpp << "'" << std::endl;
        return 1;
    }

    out << "#include \"oxygen.h\"\n"
        << "#include \"interpreter.hpp\"\n\n"
        << "static const char *EMBEDDED_OXYGEN_SCRIPT = " << escape_cpp_raw_string(script_content) << ";\n\n"
        << "int main(void) {\n"
        << "    oxygen::Interpreter interp;\n"
        << "    return interp.execute_script(EMBEDDED_OXYGEN_SCRIPT);\n"
        << "}\n";
    out.close();

    std::string cxx = "g++";
#ifdef OXYGEN_CXX_COMPILER
    cxx = OXYGEN_CXX_COMPILER;
#endif
    std::ostringstream cmd;
    cmd << cxx << " -O2 -std=c++17 -I\"" << oxygen_root << "/src\" "
        << "\"" << tmp_cpp << "\" "
        << "\"" << oxygen_root << "/src/runtime.c\" "
        << "\"" << oxygen_root << "/src/interpreter.cpp\" "
        << "-lpthread -o \"" << output_path << "\"";

    int rc = std::system(cmd.str().c_str());
    std::remove(tmp_cpp.c_str());

    if (rc != 0) {
        std::cerr << "Error: compilation failed for '" << script_path << "'" << std::endl;
        return rc;
    }
    std::cout << "Compiled '" << script_path << "' -> '" << output_path << "' (with <oxygen.h>)" << std::endl;
    return 0;
}

} // namespace oxygen
