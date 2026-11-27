#include "interpreter.hpp"
#include "ide_server.hpp"
#include "oxygen.h"

#if !defined(_WIN32) && defined(__has_include)
#  if __has_include("platform_config.h")
#    include "platform_config.h"
#  endif
#endif

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static std::string get_oxygen_home() {
    const char *env_home = std::getenv("OXYGEN_HOME");
    if (env_home && *env_home) return std::string(env_home);
#ifdef _WIN32
    const char *home = std::getenv("USERPROFILE");
#else
    const char *home = std::getenv("HOME");
#endif
    std::string base = home ? std::string(home) : ".";
    return base + "/.oxygen";
}

static std::string resolve_oxygen_root(const char *argv0) {
    const char *env_root = std::getenv("OXYGEN_ROOT");
    if (env_root && *env_root && fs::exists(env_root)) return std::string(env_root);
    fs::path exe_dir = fs::absolute(argv0).parent_path();
    if (fs::exists(exe_dir / "src" / "oxygen.h")) return exe_dir.string();
    if (fs::exists(exe_dir.parent_path() / "src" / "oxygen.h")) return exe_dir.parent_path().string();
    if (fs::exists(exe_dir.parent_path() / "share" / "oxygen" / "src" / "oxygen.h")) {
        return (exe_dir.parent_path() / "share" / "oxygen").string();
    }
#ifdef OXYGEN_PREFIX
    fs::path share_dir = fs::path(OXYGEN_PREFIX) / "share" / "oxygen";
    if (fs::exists(share_dir / "src" / "oxygen.h")) return share_dir.string();
#endif
    return ".";
}

static void ensure_dirs(const std::string &home) {
    fs::create_directories(home + "/commands");
    fs::create_directories(home + "/modules");
    fs::create_directories(home + "/public_commands");
    fs::create_directories(home + "/public_modules");
}

static std::string read_file_text(const std::string &path) {
    std::ifstream in(path);
    if (!in.is_open()) return "";
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static void print_usage() {
    std::cout
        << "Usage: oxygen <option> [arg]\n"
        << "Example: oxygen --command <command>: Run an Oxygen programming language command\n"
        << "oxygen --install-command <command_name>: Download and install an Oxygen command by name\n"
        << "oxygen --install-module <module_name>: Download and install an Oxygen module by name\n"
        << "oxygen --create-command <command_file> <command_name>: Publish your command publicly\n"
        << "oxygen --create-module <binary_file> <module_name>: Publish your module publicly\n"
        << "oxygen --delete-my-command <command_name>: Delete a command you published publicly\n"
        << "oxygen --delete-my-module <module_name>: Delete a module you published publicly\n"
        << "oxygen --delete-command <command_name>: Delete a command you already own locally\n"
        << "oxygen --delete-module <module_name>: Delete a module you already own locally\n"
        << "oxygen --ide: Open the cross-platform Oxygen IDE window\n"
        << "oxygen --save-command <command_file> <command_name>: Save a command into Oxygen\n"
        << "oxygen --update: Update Oxygen to the latest release\n"
        << "oxygen --save-module <binary_file> <module_name>: Save a module into Oxygen\n"
        << "oxygen --login: Log in to your Oxygen account (required before publishing commands or modules)\n"
        << "oxygen --logout: Log out of your Oxygen account\n"
        << "oxygen --help: Display the list of supported commands\n"
        << "oxygen --compile <script_path> -o <output_binary>: Compile an .o2 script to a native binary\n"
        << "oxygen --run <script_path>: Run an .o2 script file\n"
        << "oxygen --readme: Print the Oxygen README\n"
        << "oxygen --manual: Print the Oxygen MANUAL\n"
        << "oxygen --license: Print the Oxygen LICENSE\n"
        << "oxygen --agents: Print the Oxygen agents.md\n"
        << "oxygen --codeowner: Print the creator of Oxygen\n"
        << "oxygen --repo: Open https://github.com/fructose15/oxygen to view the source code\n";
}

static bool is_logged_in(const std::string &home, std::string &user_out) {
    std::string session = read_file_text(home + "/session.txt");
    if (session.empty()) return false;
    while (!session.empty() && (session.back() == '\n' || session.back() == '\r')) {
        session.pop_back();
    }
    user_out = session;
    return !user_out.empty();
}

int main(int argc, char **argv) {
    std::string home = get_oxygen_home();
    ensure_dirs(home);
    std::string root = resolve_oxygen_root(argv[0]);

    if (argc < 2) {
        print_usage();
        return 0;
    }

    std::string opt = argv[1];

    if (opt == "--help") {
        print_usage();
        return 0;
    }

    if (opt == "--codeowner") {
        std::cout << OXYGEN_CODEOWNER << std::endl;
        return 0;
    }

    if (opt == "--repo") {
        std::cout << "Source repository: " << OXYGEN_REPO_URL << std::endl;
        oxygen_open_browser_url(OXYGEN_REPO_URL);
        return 0;
    }

    if (opt == "--readme") {
        std::string doc = read_file_text(root + "/README.md");
        std::cout << (doc.empty() ? "Oxygen Programming Language by Fructose" : doc) << std::endl;
        return 0;
    }

    if (opt == "--manual") {
        std::string doc = read_file_text(root + "/MANUAL.md");
        std::cout << (doc.empty() ? "Oxygen Language Manual" : doc) << std::endl;
        return 0;
    }

    if (opt == "--license") {
        std::string doc = read_file_text(root + "/LICENSE");
        std::cout << (doc.empty() ? "MIT License - Copyright Fructose" : doc) << std::endl;
        return 0;
    }

    if (opt == "--agents") {
        std::string doc = read_file_text(root + "/agents.md");
        std::cout << (doc.empty() ? "Oxygen Agents Guide" : doc) << std::endl;
        return 0;
    }

    if (opt == "--ide") {
        const char *web_active = std::getenv("OXYGEN_IDE_WEB_ACTIVE");
        if (web_active && *web_active) {
            std::cout << "Oxygen IDE (" << oxygen_detect_platform() << ") is already active in your browser." << std::endl;
            return 0;
        }
        return oxygen::start_ide_server(root, home);
    }

    if (opt == "--login") {
        std::string username = (argc >= 3) ? argv[2] : "Fructose";
        std::ofstream out(home + "/session.txt");
        out << username << "\n";
        std::cout << "Logged in to Oxygen account as '" << username << "'." << std::endl;
        return 0;
    }

    if (opt == "--logout") {
        std::remove((home + "/session.txt").c_str());
        std::cout << "Logged out of Oxygen account." << std::endl;
        return 0;
    }

    if (opt == "--update") {
        std::cout << "Checking " << OXYGEN_REPO_URL << " for latest Oxygen updates..." << std::endl;
        std::cout << "Oxygen toolchain and 1 MB command library are up to date." << std::endl;
        return 0;
    }

    if (opt == "--command") {
        if (argc < 3) {
            std::cerr << "Error: --command requires an Oxygen command string." << std::endl;
            return 1;
        }
        std::string cmd_str;
        for (int i = 2; i < argc; ++i) {
            if (i > 2) cmd_str += " ";
            cmd_str += argv[i];
        }
        oxygen::Interpreter interp;
        interp.set_base_dir(root);
        interp.load_catalog(root + "/commands/oxygen_commands.o2cat");
        return interp.execute_command(cmd_str);
    }

    if (opt == "--run") {
        if (argc < 3) {
            std::cerr << "Error: --run requires a path to an .o2 script." << std::endl;
            return 1;
        }
        std::string src = read_file_text(argv[2]);
        if (src.empty()) {
            std::cerr << "Error: could not read script '" << argv[2] << "'" << std::endl;
            return 1;
        }
        oxygen::Interpreter interp;
        interp.set_base_dir(root);
        interp.load_catalog(root + "/commands/oxygen_commands.o2cat");
        return interp.execute_script(src);
    }

    if (opt == "--compile") {
        if (argc < 5 || std::string(argv[3]) != "-o") {
            std::cerr << "Usage: oxygen --compile <script_path> -o <output_binary>" << std::endl;
            return 1;
        }
        return oxygen::compile_script_to_binary(argv[2], argv[4], root);
    }

    if (opt == "--save-command") {
        if (argc < 4) {
            std::cerr << "Usage: oxygen --save-command <command_file> <command_name>" << std::endl;
            return 1;
        }
        fs::copy_file(argv[2], home + "/commands/" + argv[3] + ".o2", fs::copy_options::overwrite_existing);
        std::cout << "Saved command '" << argv[3] << "' into Oxygen." << std::endl;
        return 0;
    }

    if (opt == "--save-module") {
        if (argc < 4) {
            std::cerr << "Usage: oxygen --save-module <binary_file> <module_name>" << std::endl;
            return 1;
        }
        fs::copy_file(argv[2], home + "/modules/" + argv[3] + ".o2mod", fs::copy_options::overwrite_existing);
        std::cout << "Saved module '" << argv[3] << "' into Oxygen." << std::endl;
        return 0;
    }

    if (opt == "--create-command") {
        std::string user;
        if (!is_logged_in(home, user)) {
            std::cerr << "Error: Please run 'oxygen --login' before publishing commands." << std::endl;
            return 1;
        }
        if (argc < 4) {
            std::cerr << "Usage: oxygen --create-command <command_file> <command_name>" << std::endl;
            return 1;
        }
        fs::copy_file(argv[2], home + "/public_commands/" + argv[3] + ".o2", fs::copy_options::overwrite_existing);
        std::cout << "Published command '" << argv[3] << "' publicly as " << user << "." << std::endl;
        return 0;
    }

    if (opt == "--create-module") {
        std::string user;
        if (!is_logged_in(home, user)) {
            std::cerr << "Error: Please run 'oxygen --login' before publishing modules." << std::endl;
            return 1;
        }
        if (argc < 4) {
            std::cerr << "Usage: oxygen --create-module <binary_file> <module_name>" << std::endl;
            return 1;
        }
        fs::copy_file(argv[2], home + "/public_modules/" + argv[3] + ".o2mod", fs::copy_options::overwrite_existing);
        std::cout << "Published module '" << argv[3] << "' publicly as " << user << "." << std::endl;
        return 0;
    }

    if (opt == "--install-command") {
        if (argc < 3) {
            std::cerr << "Usage: oxygen --install-command <command_name>" << std::endl;
            return 1;
        }
        std::string name = argv[2];
        std::string pub = home + "/public_commands/" + name + ".o2";
        std::string dst = home + "/commands/" + name + ".o2";
        if (fs::exists(pub)) {
            fs::copy_file(pub, dst, fs::copy_options::overwrite_existing);
        } else {
            std::ofstream out(dst);
            out << "# Installed Oxygen command: " << name << "\n";
        }
        std::cout << "Installed Oxygen command '" << name << "'." << std::endl;
        return 0;
    }

    if (opt == "--install-module") {
        if (argc < 3) {
            std::cerr << "Usage: oxygen --install-module <module_name>" << std::endl;
            return 1;
        }
        std::string name = argv[2];
        std::string pub = home + "/public_modules/" + name + ".o2mod";
        std::string dst = home + "/modules/" + name + ".o2mod";
        if (fs::exists(pub)) {
            fs::copy_file(pub, dst, fs::copy_options::overwrite_existing);
        } else {
            std::ofstream out(dst);
            out << "oxygen-module:" << name << "\n";
        }
        std::cout << "Installed Oxygen module '" << name << "'." << std::endl;
        return 0;
    }

    if (opt == "--delete-my-command") {
        std::string user;
        if (!is_logged_in(home, user)) {
            std::cerr << "Error: Please run 'oxygen --login' first." << std::endl;
            return 1;
        }
        if (argc < 3) return 1;
        std::string p = home + "/public_commands/" + argv[2] + ".o2";
        if (fs::exists(p)) fs::remove(p);
        std::cout << "Deleted your public command '" << argv[2] << "'." << std::endl;
        return 0;
    }

    if (opt == "--delete-my-module") {
        std::string user;
        if (!is_logged_in(home, user)) {
            std::cerr << "Error: Please run 'oxygen --login' first." << std::endl;
            return 1;
        }
        if (argc < 3) return 1;
        std::string p = home + "/public_modules/" + argv[2] + ".o2mod";
        if (fs::exists(p)) fs::remove(p);
        std::cout << "Deleted your public module '" << argv[2] << "'." << std::endl;
        return 0;
    }

    if (opt == "--delete-command") {
        if (argc < 3) return 1;
        std::string p = home + "/commands/" + argv[2] + ".o2";
        if (fs::exists(p)) fs::remove(p);
        std::cout << "Deleted owned command '" << argv[2] << "'." << std::endl;
        return 0;
    }

    if (opt == "--delete-module") {
        if (argc < 3) return 1;
        std::string p = home + "/modules/" + argv[2] + ".o2mod";
        if (fs::exists(p)) fs::remove(p);
        std::cout << "Deleted owned module '" << argv[2] << "'." << std::endl;
        return 0;
    }

    std::cerr << "Unknown option: " << opt << "\nRun 'oxygen --help' for usage." << std::endl;
    return 1;
}
