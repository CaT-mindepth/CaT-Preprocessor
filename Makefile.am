SUBDIRS = third_party . 

AM_CXXFLAGS = $(PICKY_CXXFLAGS) -I$(CLANG_DEV_LIBS)/include/
AM_CPPFLAGS = -I$(CLANG_DEV_LIBS)/include/    -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS
AM_LDFLAGS = -fuse-ld=lld -L $(CLANG_DEV_LIBS)/lib  

# There are some strange circular deps within clang requiring us to list libraries twice
# https://github.com/eliben/llvm-clang-samples/blob/master/Makefile#L71 #-lLLVM-12
CLANG_LLVM_LIBS = -l$(LLVM_VERSION) -lclangAST -lclangAnalysis -lclangBasic -lclangDriver -lclangEdit -lclangFrontend -lclangFrontendTool -lclangLex -lclangParse -lclangSema -lclangSerialization  -lclangAST -lclangAnalysis -lclangBasic -lclangDriver -lclangEdit -lclangFrontend -lclangFrontendTool -lclangLex -lclangParse -lclangSema -lclangSerialization -lpthread -ltermcap -ldl -l$(LLVM_VERSION)

common_source = \
    clang_utility_functions.cc  clang_utility_functions.h unique_identifiers.h unique_identifiers.cc \
    pkt_func_transform.h pkt_func_transform.cc graph.h expr_flattener_handler.cc expr_flattener_handler.h \
    expr_prop.cc stateful_flanks.cc ssa.cc if_conversion_handler.cc if_conversion_handler.h ssa.h \
    stateful_flanks.h expr_prop.h util.cc util.h assert_exception.h \
    desugar_compound_assignment.h desugar_compound_assignment.cc \
    int_type_checker.h algebraic_simplifier.cc algebraic_simplifier.h \
    array_validator.h redundancy_remover.cc redundancy_remover.h \
    csi.cc csi.h cse.cc cse.h const_prop.cc const_prop.h dce.cc dce.h dde.cc dde.h \
    rename_pkt_fields.cc rename_pkt_fields.h  \
    gen_used_fields.cc gen_used_fields.h validator.h bool_to_int.h ast_visitor.cc ast_visitor.h \
    elim_ternary.cc elim_ternary.h initial_pass.cc initial_pass.h branch_var_creator.cc branch_var_creator.h \
    array_replacer.cc array_replacer.h paren_remover.cc paren_remover.h \
    elim_identical_lhs_rhs.cc elim_identical_lhs_rhs.h \
    flow_based_ite_simplifier.cc flow_based_ite_simplifier.h

#noinst_LIBRARIES = libdomino.a
bin_PROGRAMS = domino

#libdomino_a_SOURCES = $(common_source)
#-lLLVM-12
domino_CXXFLAGS = $(PICKY_CXXFLAGS) -fno-rtti  
domino_SOURCES = $(common_source) domino.cc
#domino_LDADD = $(CLANG_LLVM_LIBS) libdomino.a $(srcdir)/third_party/libmahimahi.a 
domino_LDADD = $(CLANG_LLVM_LIBS) $(srcdir)/third_party/libmahimahi.a

