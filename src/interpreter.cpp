#include "interpreter.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace oxygen {

static std::string trim(const std::string &s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

static std::string to_lower(const std::string &s) {
    std::string out = s;
    for (char &ch : out) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return out;
}

static bool starts_with(const std::string &s, const std::string &prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

static std::string strip_quotes(const std::string &s) {
    std::string t = trim(s);
    if (t.size() >= 2 &&
        ((t.front() == '"' && t.back() == '"') || (t.front() == '\'' && t.back() == '\''))) {
        return t.substr(1, t.size() - 2);
    }
    return t;
}

Value Value::from_number(double n) {
    Value v;
    v.kind = Kind::Number;
    v.number = n;
    return v;
}

Value Value::from_text(const std::string &t) {
    Value v;
    v.kind = Kind::Text;
    v.text = t;
    return v;
}

Value Value::from_boolean(bool b) {
    Value v;
    v.kind = Kind::Boolean;
    v.boolean = b;
    return v;
}

Value Value::from_list(const std::vector<Value> &items) {
    Value v;
    v.kind = Kind::List;
    v.list = items;
    return v;
}

std::string Value::to_string() const {
    switch (kind) {
        case Kind::None:
            return "none";
        case Kind::Number: {
            if (std::floor(number) == number) {
                std::ostringstream oss;
                oss << static_cast<long long>(number);
                return oss.str();
            }
            std::ostringstream oss;
            oss << number;
            return oss.str();
        }
        case Kind::Text:
            return text;
        case Kind::Boolean:
            return boolean ? "true" : "false";
        case Kind::List: {
            std::string out = "[";
            for (size_t i = 0; i < list.size(); ++i) {
                if (i > 0) out += ", ";
                out += list[i].to_string();
            }
            out += "]";
            return out;
        }
    }
    return "";
}

double Value::to_number() const {
    switch (kind) {
        case Kind::Number:
            return number;
        case Kind::Boolean:
            return boolean ? 1.0 : 0.0;
        case Kind::Text:
            try {
                return std::stod(text);
            } catch (...) {
                return 0.0;
            }
        case Kind::List:
            return static_cast<double>(list.size());
        default:
            return 0.0;
    }
}

bool Value::is_truthy() const {
    switch (kind) {
        case Kind::Boolean:
            return boolean;
        case Kind::Number:
            return number != 0.0;
        case Kind::Text:
            return !text.empty();
        case Kind::List:
            return !list.empty();
        default:
            return false;
    }
}

Interpreter::Interpreter() {
    oxygen_runtime_init();
}

Interpreter::~Interpreter() {
    for (auto &t : all_threads_) {
        if (t && t->joinable()) {
            t->join();
        }
    }
    for (auto &kv : mutexes_) {
        oxygen_mutex_destroy(kv.second);
    }
    for (auto &kv : channels_) {
        oxygen_channel_destroy(kv.second);
    }
    oxygen_runtime_shutdown();
}

void Interpreter::set_base_dir(const std::string &dir) {
    base_dir_ = dir;
}

void Interpreter::set_output_sink(std::function<void(const std::string &)> sink) {
    output_sink_ = std::move(sink);
}

bool Interpreter::load_catalog(const std::string &catalog_path) {
    std::ifstream in(catalog_path);
    if (!in.is_open()) {
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> cols;
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, '\t')) {
            cols.push_back(item);
        }
        if (cols.size() >= 4) {
            CommandCatalogEntry entry;
            entry.name = cols[0];
            entry.category = cols[1];
            entry.syntax = cols[2];
            entry.description = cols[3];
            if (cols.size() >= 5) {
                entry.thread_safe = (cols[4] == "thread-safe" || cols[4] == "true");
            }
            catalog_[to_lower(entry.name)] = entry;
        }
    }
    return true;
}

size_t Interpreter::catalog_size() const {
    return catalog_.size();
}

void Interpreter::print_safe(const std::string &msg) {
    std::lock_guard<std::mutex> lock(io_mutex_);
    if (output_sink_) {
        output_sink_(msg);
    } else {
        std::cout << msg << std::endl;
    }
}

Value Interpreter::get_var(const std::string &name, Scope &local_scope) {
    auto it = local_scope.vars.find(name);
    if (it != local_scope.vars.end()) {
        return it->second;
    }
    std::lock_guard<std::mutex> lock(state_mutex_);
    auto git = globals_.find(name);
    if (git != globals_.end()) {
        return git->second;
    }
    return Value();
}

void Interpreter::set_var(const std::string &name, const Value &val, Scope &local_scope) {
    if (local_scope.vars.find(name) != local_scope.vars.end()) {
        local_scope.vars[name] = val;
        return;
    }
    std::lock_guard<std::mutex> lock(state_mutex_);
    globals_[name] = val;
}

Value Interpreter::evaluate_expr(const std::string &raw_expr, Scope &local_scope) {
    std::string expr = trim(raw_expr);
    if (expr.empty()) return Value();

    if ((expr.front() == '"' && expr.back() == '"') ||
        (expr.front() == '\'' && expr.back() == '\'')) {
        return Value::from_text(strip_quotes(expr));
    }

    std::string low = to_lower(expr);
    if (low == "true" || low == "yes") return Value::from_boolean(true);
    if (low == "false" || low == "no") return Value::from_boolean(false);

    if (expr.front() == '[' && expr.back() == ']') {
        std::vector<Value> items;
        std::string inner = expr.substr(1, expr.size() - 2);
        std::stringstream ss(inner);
        std::string piece;
        while (std::getline(ss, piece, ',')) {
            std::string p = trim(piece);
            if (!p.empty()) items.push_back(evaluate_expr(p, local_scope));
        }
        return Value::from_list(items);
    }

    if (starts_with(low, "value of ")) {
        std::string var_name = trim(expr.substr(9));
        return get_var(var_name, local_scope);
    }

    if (starts_with(low, "length of ")) {
        Value v = evaluate_expr(expr.substr(10), local_scope);
        if (v.kind == Value::Kind::List) return Value::from_number(static_cast<double>(v.list.size()));
        return Value::from_number(static_cast<double>(v.to_string().size()));
    }

    // Check simple arithmetic: "<lhs> plus <rhs>", "<lhs> minus <rhs>", "<lhs> times <rhs>", "<lhs> divided by <rhs>"
    const std::vector<std::pair<std::string, int>> ops = {
        {" plus ", 1},
        {" minus ", 2},
        {" times ", 3},
        {" divided by ", 4},
        {" + ", 1},
        {" - ", 2},
        {" * ", 3},
        {" / ", 4}
    };
    for (const auto &op : ops) {
        size_t pos = low.find(op.first);
        if (pos != std::string::npos) {
            Value lhs = evaluate_expr(expr.substr(0, pos), local_scope);
            Value rhs = evaluate_expr(expr.substr(pos + op.first.size()), local_scope);
            if (op.second == 1 && (lhs.kind == Value::Kind::Text || rhs.kind == Value::Kind::Text)) {
                return Value::from_text(lhs.to_string() + rhs.to_string());
            }
            double a = lhs.to_number();
            double b = rhs.to_number();
            switch (op.second) {
                case 1: return Value::from_number(a + b);
                case 2: return Value::from_number(a - b);
                case 3: return Value::from_number(a * b);
                case 4: return Value::from_number(b == 0.0 ? 0.0 : a / b);
            }
        }
    }

    // Check number literal
    char *endptr = nullptr;
    double num = std::strtod(expr.c_str(), &endptr);
    if (endptr != expr.c_str() && *trim(endptr).c_str() == '\0') {
        return Value::from_number(num);
    }

    // Variable lookup
    Value found = get_var(expr, local_scope);
    if (found.kind != Value::Kind::None) {
        return found;
    }

    return Value::from_text(expr);
}

bool Interpreter::evaluate_condition(const std::string &raw_cond, Scope &local_scope) {
    std::string cond = trim(raw_cond);
    std::string low = to_lower(cond);

    const std::vector<std::pair<std::string, int>> comps = {
        {" is greater than or equal to ", 5},
        {" is less than or equal to ", 6},
        {" is not equal to ", 2},
        {" is greater than ", 3},
        {" is less than ", 4},
        {" is equal to ", 1},
        {" is at least ", 5},
        {" is at most ", 6},
        {" contains ", 7},
        {" equals ", 1},
        {" != ", 2},
        {" == ", 1},
        {" >= ", 5},
        {" <= ", 6},
        {" > ", 3},
        {" < ", 4}
    };

    for (const auto &c : comps) {
        size_t pos = low.find(c.first);
        if (pos != std::string::npos) {
            Value lhs = evaluate_expr(cond.substr(0, pos), local_scope);
            Value rhs = evaluate_expr(cond.substr(pos + c.first.size()), local_scope);
            switch (c.second) {
                case 1:
                    if (lhs.kind == Value::Kind::Number && rhs.kind == Value::Kind::Number) {
                        return lhs.number == rhs.number;
                    }
                    return lhs.to_string() == rhs.to_string();
                case 2:
                    if (lhs.kind == Value::Kind::Number && rhs.kind == Value::Kind::Number) {
                        return lhs.number != rhs.number;
                    }
                    return lhs.to_string() != rhs.to_string();
                case 3:
                    return lhs.to_number() > rhs.to_number();
                case 4:
                    return lhs.to_number() < rhs.to_number();
                case 5:
                    return lhs.to_number() >= rhs.to_number();
                case 6:
                    return lhs.to_number() <= rhs.to_number();
                case 7:
                    return lhs.to_string().find(rhs.to_string()) != std::string::npos;
            }
        }
    }

    return evaluate_expr(cond, local_scope).is_truthy();
}

int Interpreter::execute_script(const std::string &source) {
    std::vector<std::string> lines;
    std::istringstream iss(source);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    Scope root_scope;
    int rc = execute_block(lines, root_scope);
    std::vector<std::shared_ptr<std::thread>> pending;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        pending = all_threads_;
        all_threads_.clear();
    }
    for (auto &t : pending) {
        if (t && t->joinable()) {
            t->join();
        }
    }
    return rc;
}

int Interpreter::execute_command(const std::string &command_line) {
    return execute_script(command_line);
}

int Interpreter::execute_block(const std::vector<std::string> &lines, Scope &local_scope) {
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string raw = trim(lines[i]);
        if (raw.empty() || starts_with(raw, "note:") || starts_with(raw, "#") || starts_with(raw, "//")) {
            continue;
        }

        std::string low = to_lower(raw);

        // Task definition: define task <name> [with parameter <param>]
        if (starts_with(low, "define task ")) {
            std::string rest = trim(raw.substr(12));
            std::string task_name = rest;
            std::string param_name;
            size_t wp = to_lower(rest).find(" with parameter ");
            if (wp != std::string::npos) {
                task_name = trim(rest.substr(0, wp));
                param_name = trim(rest.substr(wp + 16));
            }
            TaskDefinition def;
            def.name = task_name;
            def.parameter_name = param_name;
            int depth = 1;
            ++i;
            while (i < lines.size()) {
                std::string bl = trim(lines[i]);
                std::string blow = to_lower(bl);
                if (starts_with(blow, "define task ")) depth++;
                if (blow == "end task") {
                    depth--;
                    if (depth == 0) break;
                }
                def.body_lines.push_back(bl);
                ++i;
            }
            std::lock_guard<std::mutex> lock(state_mutex_);
            tasks_[task_name] = def;
            continue;
        }

        // Conditional block: if <cond> then
        if (starts_with(low, "if ") && low.size() > 8 && low.substr(low.size() - 5) == " then") {
            std::string cond_str = raw.substr(3, raw.size() - 8);
            std::vector<std::string> then_lines;
            std::vector<std::string> else_lines;
            bool in_else = false;
            int depth = 1;
            ++i;
            while (i < lines.size()) {
                std::string bl = trim(lines[i]);
                std::string blow = to_lower(bl);
                if (starts_with(blow, "if ") && blow.size() > 8 && blow.substr(blow.size() - 5) == " then") {
                    depth++;
                } else if (blow == "end if") {
                    depth--;
                    if (depth == 0) break;
                } else if (depth == 1 && (blow == "otherwise" || blow == "else")) {
                    in_else = true;
                    ++i;
                    continue;
                }
                if (in_else) else_lines.push_back(bl);
                else then_lines.push_back(bl);
                ++i;
            }
            if (evaluate_condition(cond_str, local_scope)) {
                execute_block(then_lines, local_scope);
            } else if (!else_lines.empty()) {
                execute_block(else_lines, local_scope);
            }
            continue;
        }

        // Repeat loop: repeat <n> times
        if (starts_with(low, "repeat ") && low.size() > 13 && low.substr(low.size() - 6) == " times") {
            std::string count_expr = raw.substr(7, raw.size() - 13);
            int count = static_cast<int>(evaluate_expr(count_expr, local_scope).to_number());
            std::vector<std::string> body;
            int depth = 1;
            ++i;
            while (i < lines.size()) {
                std::string bl = trim(lines[i]);
                std::string blow = to_lower(bl);
                if (starts_with(blow, "repeat ") && blow.size() > 13 && blow.substr(blow.size() - 6) == " times") {
                    depth++;
                } else if (blow == "end repeat") {
                    depth--;
                    if (depth == 0) break;
                }
                body.push_back(bl);
                ++i;
            }
            for (int iter = 0; iter < count; ++iter) {
                local_scope.vars["iteration"] = Value::from_number(iter + 1);
                execute_block(body, local_scope);
            }
            continue;
        }

        // While loop: while <cond> do
        if (starts_with(low, "while ") && low.size() > 9 && low.substr(low.size() - 3) == " do") {
            std::string cond_str = raw.substr(6, raw.size() - 9);
            std::vector<std::string> body;
            int depth = 1;
            ++i;
            while (i < lines.size()) {
                std::string bl = trim(lines[i]);
                std::string blow = to_lower(bl);
                if (starts_with(blow, "while ") && blow.size() > 9 && blow.substr(blow.size() - 3) == " do") {
                    depth++;
                } else if (blow == "end while") {
                    depth--;
                    if (depth == 0) break;
                }
                body.push_back(bl);
                ++i;
            }
            int safety = 0;
            while (evaluate_condition(cond_str, local_scope) && safety < 100000) {
                execute_block(body, local_scope);
                safety++;
            }
            continue;
        }

        // For each loop: for each <item> in <list> do
        if (starts_with(low, "for each ") && low.size() > 12 && low.substr(low.size() - 3) == " do") {
            std::string middle = raw.substr(9, raw.size() - 12);
            size_t in_pos = to_lower(middle).find(" in ");
            std::string item_var = "item";
            std::string list_expr = "[]";
            if (in_pos != std::string::npos) {
                item_var = trim(middle.substr(0, in_pos));
                list_expr = trim(middle.substr(in_pos + 4));
            }
            std::vector<std::string> body;
            int depth = 1;
            ++i;
            while (i < lines.size()) {
                std::string bl = trim(lines[i]);
                std::string blow = to_lower(bl);
                if (starts_with(blow, "for each ") && blow.size() > 12 && blow.substr(blow.size() - 3) == " do") {
                    depth++;
                } else if (blow == "end for") {
                    depth--;
                    if (depth == 0) break;
                }
                body.push_back(bl);
                ++i;
            }
            Value list_val = evaluate_expr(list_expr, local_scope);
            for (const Value &elem : list_val.list) {
                local_scope.vars[item_var] = elem;
                execute_block(body, local_scope);
            }
            continue;
        }

        int rc = execute_single(raw, local_scope);
        if (rc != 0) return rc;
    }
    return 0;
}

int Interpreter::execute_single(const std::string &raw, Scope &local_scope) {
    std::string low = to_lower(raw);

    if (starts_with(low, "use module ")) {
        std::string mod = trim(raw.substr(11));
        std::lock_guard<std::mutex> lock(state_mutex_);
        loaded_modules_.push_back(mod);
        return 0;
    }

    if (starts_with(low, "print text ")) {
        Value v = evaluate_expr(raw.substr(11), local_scope);
        print_safe(v.to_string());
        return 0;
    }

    if (starts_with(low, "print value of ")) {
        std::string var_name = trim(raw.substr(15));
        print_safe(get_var(var_name, local_scope).to_string());
        return 0;
    }

    if (starts_with(low, "print ")) {
        Value v = evaluate_expr(raw.substr(6), local_scope);
        print_safe(v.to_string());
        return 0;
    }

    if (starts_with(low, "set number ") || starts_with(low, "set text ") ||
        starts_with(low, "set boolean ") || starts_with(low, "set list ") ||
        starts_with(low, "set ")) {
        std::string rest = raw;
        if (starts_with(low, "set number ")) rest = raw.substr(11);
        else if (starts_with(low, "set text ")) rest = raw.substr(9);
        else if (starts_with(low, "set boolean ")) rest = raw.substr(12);
        else if (starts_with(low, "set list ")) rest = raw.substr(9);
        else rest = raw.substr(4);

        size_t to_pos = to_lower(rest).find(" to ");
        if (to_pos != std::string::npos) {
            std::string var_name = trim(rest.substr(0, to_pos));
            Value val = evaluate_expr(rest.substr(to_pos + 4), local_scope);
            set_var(var_name, val, local_scope);
        }
        return 0;
    }

    if (starts_with(low, "add ")) {
        std::string rest = raw.substr(4);
        size_t to_pos = to_lower(rest).find(" to ");
        if (to_pos != std::string::npos) {
            double delta = evaluate_expr(rest.substr(0, to_pos), local_scope).to_number();
            std::string var_name = trim(rest.substr(to_pos + 4));
            Value cur = get_var(var_name, local_scope);
            set_var(var_name, Value::from_number(cur.to_number() + delta), local_scope);
        }
        return 0;
    }

    if (starts_with(low, "subtract ")) {
        std::string rest = raw.substr(9);
        size_t from_pos = to_lower(rest).find(" from ");
        if (from_pos != std::string::npos) {
            double delta = evaluate_expr(rest.substr(0, from_pos), local_scope).to_number();
            std::string var_name = trim(rest.substr(from_pos + 6));
            Value cur = get_var(var_name, local_scope);
            set_var(var_name, Value::from_number(cur.to_number() - delta), local_scope);
        }
        return 0;
    }

    if (starts_with(low, "multiply ")) {
        std::string rest = raw.substr(9);
        size_t by_pos = to_lower(rest).find(" by ");
        if (by_pos != std::string::npos) {
            std::string var_name = trim(rest.substr(0, by_pos));
            double factor = evaluate_expr(rest.substr(by_pos + 4), local_scope).to_number();
            Value cur = get_var(var_name, local_scope);
            set_var(var_name, Value::from_number(cur.to_number() * factor), local_scope);
        }
        return 0;
    }

    if (starts_with(low, "divide ")) {
        std::string rest = raw.substr(7);
        size_t by_pos = to_lower(rest).find(" by ");
        if (by_pos != std::string::npos) {
            std::string var_name = trim(rest.substr(0, by_pos));
            double divisor = evaluate_expr(rest.substr(by_pos + 4), local_scope).to_number();
            Value cur = get_var(var_name, local_scope);
            set_var(var_name, Value::from_number(divisor == 0.0 ? 0.0 : cur.to_number() / divisor), local_scope);
        }
        return 0;
    }

    if (starts_with(low, "increment atomic counter ")) {
        std::string name = trim(raw.substr(25));
        oxygen_atomic_counter_t *c = nullptr;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            c = atomics_[name];
        }
        if (c) oxygen_atomic_increment(c);
        return 0;
    }

    if (starts_with(low, "increment ")) {
        std::string var_name = trim(raw.substr(10));
        Value cur = get_var(var_name, local_scope);
        set_var(var_name, Value::from_number(cur.to_number() + 1.0), local_scope);
        return 0;
    }

    if (starts_with(low, "decrement ")) {
        std::string var_name = trim(raw.substr(10));
        Value cur = get_var(var_name, local_scope);
        set_var(var_name, Value::from_number(cur.to_number() - 1.0), local_scope);
        return 0;
    }

    if (starts_with(low, "sleep for ")) {
        std::string rest = raw.substr(10);
        size_t ms_pos = to_lower(rest).find(" millisecond");
        std::string num_str = (ms_pos != std::string::npos) ? rest.substr(0, ms_pos) : rest;
        uint32_t ms = static_cast<uint32_t>(evaluate_expr(num_str, local_scope).to_number());
        oxygen_sleep_ms(ms);
        return 0;
    }

    if (starts_with(low, "create mutex ")) {
        std::string name = trim(raw.substr(13));
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (mutexes_.find(name) == mutexes_.end()) {
            mutexes_[name] = oxygen_mutex_create(name.c_str());
        }
        return 0;
    }

    if (starts_with(low, "lock mutex ")) {
        std::string name = trim(raw.substr(11));
        oxygen_mutex_handle_t *m = nullptr;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            m = mutexes_[name];
        }
        if (m) oxygen_mutex_lock(m);
        return 0;
    }

    if (starts_with(low, "unlock mutex ")) {
        std::string name = trim(raw.substr(13));
        oxygen_mutex_handle_t *m = nullptr;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            m = mutexes_[name];
        }
        if (m) oxygen_mutex_unlock(m);
        return 0;
    }

    if (starts_with(low, "create channel ")) {
        std::string name = trim(raw.substr(15));
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (channels_.find(name) == channels_.end()) {
            channels_[name] = oxygen_channel_create(name.c_str(), 128);
        }
        return 0;
    }

    if (starts_with(low, "send ") && low.find(" to channel ") != std::string::npos) {
        size_t pos = low.find(" to channel ");
        std::string msg_expr = trim(raw.substr(5, pos - 5));
        if (starts_with(to_lower(msg_expr), "text ")) msg_expr = trim(msg_expr.substr(5));
        std::string ch_name = trim(raw.substr(pos + 12));
        Value val = evaluate_expr(msg_expr, local_scope);
        oxygen_channel_handle_t *ch = nullptr;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            ch = channels_[ch_name];
        }
        if (ch) oxygen_channel_send(ch, val.to_string().c_str());
        return 0;
    }

    if (starts_with(low, "receive message from channel ")) {
        std::string rest = raw.substr(29);
        size_t into_pos = to_lower(rest).find(" into ");
        if (into_pos != std::string::npos) {
            std::string ch_name = trim(rest.substr(0, into_pos));
            std::string var_name = trim(rest.substr(into_pos + 6));
            oxygen_channel_handle_t *ch = nullptr;
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                ch = channels_[ch_name];
            }
            if (ch) {
                char buf[1024];
                oxygen_channel_receive(ch, buf, sizeof(buf));
                set_var(var_name, Value::from_text(buf), local_scope);
            }
        }
        return 0;
    }

    if (starts_with(low, "create atomic counter ")) {
        std::string rest = raw.substr(22);
        size_t wv = to_lower(rest).find(" with value ");
        std::string name = rest;
        int64_t init_val = 0;
        if (wv != std::string::npos) {
            name = trim(rest.substr(0, wv));
            init_val = static_cast<int64_t>(evaluate_expr(rest.substr(wv + 12), local_scope).to_number());
        }
        std::lock_guard<std::mutex> lock(state_mutex_);
        atomics_[name] = oxygen_atomic_create(name.c_str(), init_val);
        return 0;
    }

    if (starts_with(low, "read atomic counter ") && low.find(" into ") != std::string::npos) {
        size_t into_pos = low.find(" into ");
        std::string name = trim(raw.substr(20, into_pos - 20));
        std::string var_name = trim(raw.substr(into_pos + 6));
        oxygen_atomic_counter_t *c = nullptr;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            c = atomics_[name];
        }
        if (c) {
            set_var(var_name, Value::from_number(static_cast<double>(oxygen_atomic_read(c))), local_scope);
        }
        return 0;
    }

    // Run task in background: run task <name> [with <arg>] in background [as <handle>]
    if (starts_with(low, "run task ")) {
        std::string rest = raw.substr(9);
        std::string low_rest = to_lower(rest);
        bool background = false;
        std::string handle_name;
        size_t bg_pos = low_rest.find(" in background");
        if (bg_pos != std::string::npos) {
            background = true;
            std::string after_bg = trim(rest.substr(bg_pos + 14));
            if (starts_with(to_lower(after_bg), "as ")) {
                handle_name = trim(after_bg.substr(3));
            }
            rest = trim(rest.substr(0, bg_pos));
        }

        std::string task_name = rest;
        Value arg_val;
        size_t with_pos = to_lower(rest).find(" with ");
        if (with_pos != std::string::npos) {
            task_name = trim(rest.substr(0, with_pos));
            arg_val = evaluate_expr(rest.substr(with_pos + 6), local_scope);
        }

        TaskDefinition def;
        bool found = false;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            auto it = tasks_.find(task_name);
            if (it != tasks_.end()) {
                def = it->second;
                found = true;
            }
        }
        if (!found) {
            std::cerr << "Oxygen error: undefined task '" << task_name << "'" << std::endl;
            return 1;
        }

        if (background) {
            auto t = std::make_shared<std::thread>([this, def, arg_val]() {
                Scope task_scope;
                if (!def.parameter_name.empty()) {
                    task_scope.vars[def.parameter_name] = arg_val;
                }
                this->execute_block(def.body_lines, task_scope);
            });
            std::lock_guard<std::mutex> lock(state_mutex_);
            all_threads_.push_back(t);
            if (!handle_name.empty()) {
                thread_handles_[handle_name] = t;
            }
        } else {
            Scope task_scope;
            if (!def.parameter_name.empty()) {
                task_scope.vars[def.parameter_name] = arg_val;
            }
            return execute_block(def.body_lines, task_scope);
        }
        return 0;
    }

    if (low == "wait for all background tasks to finish" || low == "wait for all tasks") {
        std::vector<std::shared_ptr<std::thread>> pending;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            pending = all_threads_;
            all_threads_.clear();
            thread_handles_.clear();
        }
        for (auto &t : pending) {
            if (t && t->joinable()) t->join();
        }
        return 0;
    }

    if (starts_with(low, "wait for task ")) {
        std::string handle_name = trim(raw.substr(14));
        std::shared_ptr<std::thread> t;
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            auto it = thread_handles_.find(handle_name);
            if (it != thread_handles_.end()) {
                t = it->second;
                thread_handles_.erase(it);
            }
        }
        if (t && t->joinable()) t->join();
        return 0;
    }

    // Dynamic lookup against the 1 MB Standard Command Catalog
    for (const auto &kv : catalog_) {
        if (starts_with(low, kv.first)) {
            return 0;
        }
    }

    return 0;
}

} // namespace oxygen
