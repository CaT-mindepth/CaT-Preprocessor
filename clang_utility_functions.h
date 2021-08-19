#ifndef CLANG_UTILITY_FUNCTIONS_H_
#define CLANG_UTILITY_FUNCTIONS_H_

#include <set>
#include <string>
#include <map>
#include <type_traits> // C++17 feature, required by stmt_type_census

#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/OperationKinds.h"
#include "clang/AST/Expr.h"
/// Enum class to represent Variable type
/// PACKET is for the names of all packet fields (in identifier_census)
/// and packet fields prefixed with "pkt." (in gen_var_list)
/// STATE_SCALAR is for all scalar state variables
/// STATE_ARRAY is for all array state variables
/// FUNCTION_PARAMETER is for the function name and function parameters
enum class VariableType {PACKET, STATE_SCALAR, STATE_ARRAY, FUNCTION_PARAMETER};

/// Map from VariableType to bool,
/// denoting whether a variable should be selected or not
typedef std::map<VariableType, bool> VariableTypeSelector;

/// General purpose printer for clang expressions
std::string clang_expr_printer(const clang::Expr * expr);

/// General puprose printer for clang stmts
/// Everything executable subclasses from clang::Stmt, including clang::Expr
std::string clang_stmt_printer(const clang::Stmt * stmt);

/// Print name of a value declaration (http://clang.llvm.org/doxygen/classclang_1_1ValueDecl.html#details)
/// We use it to print:
/// 1. Field names within a packet structure.
/// 2. Packet parameter names passed to packet functions.
/// 3. State variable names.
std::string clang_value_decl_printer(const clang::ValueDecl * value_decl);

/// Print all kinds of clang declarations
/// This is best used when we want to pass through certain statements unchanged.
/// It prints the entire declaration (along with the definition if it accompanies the declaration).
std::string clang_decl_printer(const clang::Decl * decl);

/// Is this a packet function: does it have struct Packet as an argument
bool is_packet_func(const clang::FunctionDecl * func_decl);

/// Return the current set of identifiers
/// so that we can generate unique names afterwards
std::set<std::string> identifier_census(const clang::TranslationUnitDecl * decl,
                                        const VariableTypeSelector & var_selector =
                                        {{VariableType::PACKET, true}, {VariableType::FUNCTION_PARAMETER, true}, {VariableType::STATE_SCALAR, true}, {VariableType::STATE_ARRAY, true}});

/// Decides if a AST node `stmt` contains a descendant of type `T` that
/// subclasses Stmt, by recursively visiting all descendatns of the given
/// AST node.
/// Requires T to be a subclass of Stmt.
template <typename T,
          typename = std::enable_if_t<std::is_base_of<clang::Stmt, T>::value>>
bool stmt_type_census(const clang::Stmt *stmt);

/// Gets all constant values inside a statement. Complements `gen_var_list`
/// which works for only variables.
std::set<const std::string> get_constants_in(const clang::Stmt * expr);

/// Decide if binary operatoion `bin_op` contains
/// only operators in the list.
bool binop_contains_only(const clang::BinaryOperator * bin_op, const std::set<clang::BinaryOperatorKind> & operators);

/// Decide if binary operation `bin_op` contains
/// any sub-operator in list `operators`.
/// contains_alternate_stmt: Returns false if bin_op contains a statement that isn't a clang::BinaryOperator.
bool binop_contains(const clang::BinaryOperator * bin_op, const std::set<clang::BinaryOperatorKind> & operators, bool contains_alternate_stmt = true);

/// Determine all variables (either packet or state) used within a clang::Stmt,
std::set<std::string> gen_var_list(const clang::Stmt * stmt,
                                   const VariableTypeSelector & var_selector =
                                   {{VariableType::PACKET, true},
                                    {VariableType::STATE_SCALAR, true},
                                    {VariableType::STATE_ARRAY, true}}
                                   );

/// Generate scalar function declarations,
/// including function definitions if provided
std::string generate_scalar_func_def(const clang::FunctionDecl * func_decl);

/// List out all packet fields in a translation unit,
/// by first calling identifier_census and then serializing the result
std::string gen_pkt_fields(const clang::TranslationUnitDecl * tu_decl);

/// Replace a particular variable
/// using a replacement map
std::string replace_var_helper(const clang::Expr * expr, const std::map<std::string, std::string> & repl_map);

/// Replace a specific string with a new string within expr
/// Using var_selector to determine which variables to replace
std::string replace_vars(const clang::Expr * expr,
                         const std::map<std::string, std::string> & repl_map,
                         const VariableTypeSelector & var_selector);

/// Check if program is in SSA form
bool is_in_ssa(const clang::CompoundStmt * compound_stmt);

#endif  // CLANG_UTILITY_FUNCTIONS_H_
