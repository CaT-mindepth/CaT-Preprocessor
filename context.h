#ifndef TYPE_INFO_H_
#define TYPE_INFO_H_

#include <map>
#include <string>
#include <vector>
#include <iostream>

#include "third_party/assert_exception.h"

enum DominoType {
    D_UNKNOWN, D_INT, D_BIT
};

enum DominoVarKind {
    D_VAR_UNKNOWN, D_PKT_FIELD, D_STATEFUL, D_TMP
};

enum DominoOptLevels {
    D_NO_OPT, D_OPT
};

static std::string domino_type_to_string(DominoType dt) {
    switch (dt) {
        case D_UNKNOWN: return "D_UNKNOWN";
        case D_INT: return "D_INT";
        case D_BIT: return "D_BIT";
    }
    return "??";
}

static std::string domino_var_kind_to_string(DominoVarKind dk) {
    switch (dk) {
        case D_VAR_UNKNOWN: return "D_VAR_UNKNWON";
        case D_PKT_FIELD: return "D_PKT_FIELD";
        case D_STATEFUL: return "D_STATEFUL";
        case D_TMP: return "D_TMP";
    }
    return "??";
}

static std::string domino_opt_level_to_string(DominoOptLevels opt) {
    switch (opt) {
        case D_OPT: return "D_OPT";
        case D_NO_OPT: return "D_NO_OPT";
    }
    return "??";
}

/**
 * Singleton class that stores type info, etc.
 */
class Context {
    
    public:
    static Context & GetContext() {
        static Context c;
        return c;
    }
    
    // Returns type information.
    DominoType GetType(const std::string & name) {
        if (this->type_info[name] == D_UNKNOWN) {
            std::cout << "Error: cannot find variable name " << name << " in context.\n";
            assert_exception(false);
        }
        return this->type_info[name];
    }

    // Returns variable kind.
    DominoVarKind GetVarKind(const std::string & name) {
        return this->var_kind[name];
    }

    // Set variable type.
    void SetType(const std::string & name, DominoType ty) {
        this->type_info[name] = ty;
    }

    // Set variable kind.
    void SetVarKind(const std::string & name, DominoVarKind kind) {
        this->var_kind[name] = kind;
    }

    const std::string & GetLastDerived(std::string & base) {
        return this->last_derived[base];
    }

    const std::string & GetBase(std::string & derivation) {
        return this->base_derived[derivation];
    }

    // Add derivation pair.
    // If a variable is first added, set it to NO_OPT.
    // If it is a temporary, set it to OPT.
    void Derive(const std::string & base, const std::string & derivation) {
        // Previous last_der
        this->opt_levels[this->last_derived[base]] = D_OPT;
        this->last_derived[base] = derivation;
        this->base_derived[derivation] = base;
        this->opt_levels[this->last_derived[base]] = D_NO_OPT;
    }

    DominoOptLevels GetOptLevel(const std::string & v) {
        return this->opt_levels[v];
    }

    void SetOptLevel(const std::string & v, DominoOptLevels o) {
        this->opt_levels[v] = o;
    }

    void Print() {
        for (const auto & p : this->type_info) {
            std::cout << "typeof " << p.first << " : " << domino_type_to_string(p.second) << "\n";
        }
        std::cout << "------------------\n";
        for (const auto & p : this->var_kind) {
            std::cout << "varkind " << p.first << " : " << domino_var_kind_to_string(p.second) << "\n";
        }
        std::cout << "------------------\n";
        for (const auto & p : this->opt_levels)
            std::cout << "opt_level " << p.first << " : " << domino_opt_level_to_string(p.second) << "\n";
    }

    // Mark constructors as singleton

  //  Context(Context const&)               = delete;
  //  void operator=(Context const&)        = delete;


    private:
  // Non-C++11 semantics
    Context() {}
    Context(Context const&);              // Don't Implement
    void operator=(Context const&);       // Don't implement

    std::map<std::string, DominoType> type_info;
    std::map<std::string, DominoVarKind> var_kind; 

    std::map<std::string, std::string> last_derived;
    std::map<std::string, std::string> base_derived;

    std::map<std::string, DominoOptLevels> opt_levels;
    
};

#endif