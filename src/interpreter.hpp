#ifndef OXYGEN_INTERPRETER_HPP
#define OXYGEN_INTERPRETER_HPP

#include "oxygen.h"

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <functional>

namespace oxygen {

struct Value {
    enum class Kind {
        None,
        Number,
        Text,
        Boolean,
        List
    };

    Kind kind = Kind::None;
    double number = 0.0;
    std::string text;
    bool boolean = false;
    std::vector<Value> list;

    static Value from_number(double n);
    static Value from_text(const std::string &t);
    static Value from_boolean(bool b);
    static Value from_list(const std::vector<Value> &items);
    std::string to_string() const;
    double to_number() const;
    bool is_truthy() const;
};

struct TaskDefinition {
    std::string name;
    std::string parameter_name;
    std::vector<std::string> body_lines;
};

struct CommandCatalogEntry {
    std::string name;
    std::string category;
    std::string syntax;
    std::string description;
    bool thread_safe = true;
};

class Interpreter {
public:
    Interpreter();
    ~Interpreter();

    void set_base_dir(const std::string &dir);
    void set_output_sink(std::function<void(const std::string &)> sink);
    bool load_catalog(const std::string &catalog_path);
    size_t catalog_size() const;

    int execute_script(const std::string &source);
    int execute_command(const std::string &command_line);

private:
    struct Scope {
        std::unordered_map<std::string, Value> vars;
    };

    int execute_block(const std::vector<std::string> &lines, Scope &local_scope);
    int execute_single(const std::string &line, Scope &local_scope);
    Value evaluate_expr(const std::string &expr, Scope &local_scope);
    bool evaluate_condition(const std::string &cond, Scope &local_scope);
    Value get_var(const std::string &name, Scope &local_scope);
    void set_var(const std::string &name, const Value &val, Scope &local_scope);
    void print_safe(const std::string &msg);

    std::string base_dir_;
    std::function<void(const std::string &)> output_sink_;
    std::unordered_map<std::string, Value> globals_;
    std::unordered_map<std::string, TaskDefinition> tasks_;
    std::unordered_map<std::string, oxygen_mutex_handle_t *> mutexes_;
    std::unordered_map<std::string, oxygen_channel_handle_t *> channels_;
    std::unordered_map<std::string, oxygen_atomic_counter_t *> atomics_;
    std::unordered_map<std::string, std::shared_ptr<std::thread>> thread_handles_;
    std::vector<std::shared_ptr<std::thread>> all_threads_;
    std::vector<std::string> loaded_modules_;
    std::unordered_map<std::string, CommandCatalogEntry> catalog_;

    mutable std::mutex state_mutex_;
    mutable std::mutex io_mutex_;
};

int compile_script_to_binary(const std::string &script_path,
                             const std::string &output_path,
                             const std::string &oxygen_root);

} // namespace oxygen

#endif /* OXYGEN_INTERPRETER_HPP */
