#include "algebraic_simplifier.h"
#include "array_replacer.h"
#include "array_validator.h"
#include "bool_to_int.h"
#include "branch_var_creator.h"
#include "const_prop.h"
#include "context.h"
#include "cse.h"
#include "csi.h"
#include "dce.h"
#include "dde.h"
#include "desugar_compound_assignment.h"
#include "elim_ternary.h"
#include "expr_flattener_handler.h"
#include "expr_prop.h"
#include "gen_used_fields.h"
#include "if_conversion_handler.h"
#include "initial_pass.h"
#include "int_type_checker.h"
#include "paren_remover.h"
#include "reduce_var_types_helper.h"
#include "redundancy_remover.h"
#include "rename_pkt_fields.h"
#include "ssa.h"
#include "stateful_flanks.h"
#include "validator.h"

#include <csignal>

#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <set>
#include <string>
#include <utility>

#include "third_party/assert_exception.h"

#include "compiler_pass.h"
#include "pkt_func_transform.h"
#include "util.h"

// For debugging purposes
#include <cassert>

#define DEBUG 0

// For the _1, and _2 in std::bind
// (Partial Function Application)
using std::placeholders::_1;
using std::placeholders::_2;

// Convenience typedefs
typedef std::string PassName;
typedef std::function<std::unique_ptr<CompilerPass>(void)> PassFunctor;
typedef std::map<PassName, PassFunctor> PassFactory;
typedef std::vector<PassFunctor> PassFunctorVector;

// Both SinglePass and Transformer take a parameter pack as template
// arguments to allow additional arguments to the function carrying out
// the transformation (e.g., atom templates and pipeline width and depth in
// sketch_backend) Unfortunately, you can't provide a default value for such
// parameter packs As
// http://en.cppreference.com/w/cpp/language/template_parameters#Default_template_arguments
// says "Defaults can be specified for any kind of template parameter (type,
// non-type, or template), but not to parameter packs." This is my clunky
// workaround.
typedef Transformer<> DefaultTransformer;
typedef SinglePass<> DefaultSinglePass;

// Map to store factory for all passes
static PassFactory all_passes;

void populate_passes() {
  // We need to explicitly call populate_passes instead of using an initializer
  // list to populate PassMap all_passes because initializer lists don't play
  // well with move-only types like unique_ptrs
  // (http://stackoverflow.com/questions/9618268/initializing-container-of-unique-ptrs-from-initializer-list-fails-with-gcc-4-7)
  all_passes["initial_pass"] = []() {
    return std::make_unique<DefaultSinglePass>(initial_pass_transform);
  };
  all_passes["create_branch_var"] = []() {
    return std::make_unique<DefaultSinglePass>(branch_var_creator_transform);
  };
  all_passes["array_replacer"] = []() {
    return std::make_unique<DefaultSinglePass>(array_replacer_transform);
  };
  all_passes["paren_remover"] = []() {
    return std::make_unique<DefaultSinglePass>(paren_remover_transform);
  };
  all_passes["cse"] = []() {
    return std::make_unique<
        FixedPointPass<CompoundPass, std::vector<DefaultTransformer>>>(
        std::vector<DefaultTransformer>(
            {std::bind(&AlgebraicSimplifier::ast_visit_transform,
                       AlgebraicSimplifier(), _1),
             csi_transform, cse_transform /*, dce_transform*/}));
  };
  all_passes["redundancy_remover"] = []() {
    return std::make_unique<
        FixedPointPass<DefaultSinglePass, DefaultTransformer>>(
        redundancy_remover_transform);
  };
  all_passes["array_validator"] = []() {
    return std::make_unique<DefaultSinglePass>(
        std::bind(&ArrayValidator::ast_visit_transform, ArrayValidator(), _1));
  };
  all_passes["validator"] = []() {
    return std::make_unique<DefaultSinglePass>(
        std::bind(&Validator::ast_visit_transform, Validator(), _1));
  };
  all_passes["int_type_checker"] = []() {
    return std::make_unique<DefaultSinglePass>(
        std::bind(&IntTypeChecker::ast_visit_transform, IntTypeChecker(), _1));
  };
  all_passes["desugar_comp_asgn"] = []() {
    return std::make_unique<DefaultSinglePass>(
        std::bind(&DesugarCompAssignment::ast_visit_transform,
                  DesugarCompAssignment(), _1));
  };
  all_passes["if_converter"] = []() {
    return std::make_unique<DefaultSinglePass>(
        std::bind(&IfConversionHandler::transform, IfConversionHandler(), _1));
  };
  all_passes["algebra_simplify"] = []() {
    return std::make_unique<
        FixedPointPass<DefaultSinglePass, DefaultTransformer>>(std::bind(
        &AlgebraicSimplifier::ast_visit_transform, AlgebraicSimplifier(), _1));
  };
  all_passes["bool_to_int"] = []() {
    return std::make_unique<DefaultSinglePass>(
        std::bind(&BoolToInt::ast_visit_transform, BoolToInt(), _1));
  };
  all_passes["expr_flattener"] = []() {
    return std::make_unique<
        FixedPointPass<DefaultSinglePass, DefaultTransformer>>(std::bind(
        &ExprFlattenerHandler::transform, ExprFlattenerHandler(), _1));
  };
  all_passes["expr_propagater"] = []() {
    return std::make_unique<DefaultSinglePass>(expr_prop_transform);
  };
  all_passes["stateful_flanks"] = []() {
    return std::make_unique<DefaultSinglePass>(stateful_flank_transform);
  };
  all_passes["ssa"] = []() {
    return std::make_unique<DefaultSinglePass>(ssa_transform);
  };
  all_passes["echo"] = []() {
    return std::make_unique<DefaultSinglePass>(clang_decl_printer);
  };
  all_passes["gen_used_fields"] = []() {
    return std::make_unique<DefaultSinglePass>(gen_used_field_transform);
  };
  all_passes["const_prop"] = []() {
    return std::make_unique<
        FixedPointPass<CompoundPass, std::vector<DefaultTransformer>>>(
        std::vector<DefaultTransformer>(
            {std::bind(&AlgebraicSimplifier::ast_visit_transform,
                       AlgebraicSimplifier(), _1),
             const_prop_transform}));
  };
  all_passes["dce"] = []() {
    return std::make_unique<
        FixedPointPass<DefaultSinglePass, DefaultTransformer>>(dce_transform);
  };
  all_passes["dde"] = []() {
    return std::make_unique<DefaultSinglePass>(dde_transform);
  };
  all_passes["rename_pkt_fields"] = []() {
    return std::make_unique<CompoundPass>(std::vector<DefaultTransformer>(
        {reduce_var_types_helper_transform, rename_pkt_fields_transform}));
  };
}

PassFunctor get_pass_functor(const std::string &pass_name,
                             const PassFactory &pass_factory) {
  if (pass_factory.find(pass_name) != pass_factory.end()) {
    // This is a bit misleading because we don't have deep const correctness
    return pass_factory.at(pass_name);
  } else {
    throw std::logic_error("Unknown pass " + pass_name);
  }
}

std::string all_passes_as_string(const PassFactory &pass_factory) {
  std::string ret;
  for (const auto &pass_pair : pass_factory)
    ret += pass_pair.first + "\n";
  return ret;
}

void print_usage() {
  std::cerr << "You are using the domino preprocessor for the CaT project."
            << std::endl;
  std::cerr << "Usage: domino_preprocessor <source_file> " << std::endl;
  std::cerr << "List of passes: " << std::endl;
  std::cerr << all_passes_as_string(all_passes);
}

int main(int argc, const char **argv) {
  try {
    // Populate all passes
    populate_passes();

    // Default pass list
    // expr_flattener
    // const auto default_pass_list =
    // "int_type_checker,desugar_comp_asgn,if_converter,algebra_simplify,array_validator,stateful_flanks,ssa,expr_propagater,expr_flattener,const_prop,cse,dde,rename_pkt_fields";
    // // dce pass list w/o anything after SSA
    // const auto default_pass_list =
    // "int_type_checker,desugar_comp_asgn,if_converter,algebra_simplify,array_validator,stateful_flanks,ssa,expr_propagater,expr_flattener";
    // const auto default_pass_list =
    // "initial_pass,int_type_checker,desugar_comp_asgn,if_converter,array_validator,stateful_flanks,ssa,elim_ternary";
    // const auto default_pass_list =
    // "initial_pass,int_type_checker,desugar_comp_asgn,if_converter,stateful_flanks,algebra_simplify,ssa,rename_pkt_fields";

    const auto default_pass_list =
        "desugar_comp_asgn,array_replacer,array_validator,initial_pass,int_"
        "type_checker,if_converter,stateful_flanks,algebra_simplify,ssa,expr_"
        "propagater,algebra_simplify,paren_remover,create_branch_var,algebra_"
        "simplify,dce,dde,rename_pkt_fields"; //,stateful_flanks,algebra_simplify,create_branch_var,ssa,paren_remover,expr_propagater,expr_flattener,dde,rename_pkt_fields";//,expr_flattener,rename_pkt_fields";//,stateful_flanks,algebra_simplify,ssa";

    // new pipeline:
    // initial_pass -> int_type_checker -> desugar_comp_asgn -> if_converter ->
    // stateful_flanks -> ssa
    //       -> expr_propagator -> expr_flattener -> const-prop -> cse -> dde
    //       -> rename_pkt_fields

    // Usage: domino <source_file>
    if (argc == 2) {
      // Get cmdline args
      const auto string_to_parse = file_to_str(std::string(argv[1]));
      const auto pass_list = split(default_pass_list, ",");

      // add all preprocessing passes
      PassFunctorVector passes_to_run;
      for (const auto &pass_name : pass_list)
        passes_to_run.emplace_back(get_pass_functor(pass_name, all_passes));

      // Some sanity checks...
      assert(pass_list.size() == passes_to_run.size());

      /// Process them one after the other
      size_t i = 0;
      const auto &result = std::accumulate(
          passes_to_run.begin(), passes_to_run.end(), string_to_parse,
          [&i, &pass_list](const auto &current_output,
                           const auto &pass_functor __attribute__((unused))) {
            if (DEBUG) {
              std::cout << "processing pass " << i << ": " << pass_list[i]
                        << std::endl;
            }
            i++;
            const auto p = (*pass_functor())(current_output);
            if (DEBUG) {
              std::cout << "program: " << p << std::endl;
              std::cout << "...pass " << i << " done.\n";
            }
            return p;
          });

      if (DEBUG) {
        std::cout << "result: " << result << std::endl;

        std::cout << " ------------- context: ---------\n";
        Context &ctx = Context::GetContext();
        ctx.Print();
      }
      return EXIT_SUCCESS;
    } else {
      print_usage();
      return EXIT_FAILURE;
    }
  } catch (const std::exception &e) {
    std::cerr << "domino_preprocessor: Caught exception in main " << std::endl
              << e.what() << std::endl;
    return EXIT_FAILURE;
  }
}
