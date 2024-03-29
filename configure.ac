AC_PREREQ([2.61])
AC_INIT([domino], [0.1], [anirudh@csail.mit.edu])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_SRCDIR([ssa.cc])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_RANLIB

# Add picky CXXFLAGS
CPPFLAGS="-std=c++17 -pthread -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -D_GLIBCXX_USE_CXX11_ABI=1"
PICKY_CXXFLAGS="-std=c++17"
AC_SUBST([PICKY_CXXFLAGS])

# Checks for header files.
AC_LANG_PUSH(C++)

# clang-11 development libraries and headers
AC_ARG_VAR(LLVM_VERSION, LLVM-10 or LLVM-13)
AC_ARG_VAR(CLANG_DEV_LIBS, Root directory of Clang development libraries and headers)
AC_SUBST([CLANG_DEV_LIBS])

# Search for g++, because we shell out to g++ when generating pisa code
AC_ARG_VAR([GPLUSPLUS], [path to g++])
AC_PATH_PROGS([GPLUSPLUS], [g++], [no], [$PATH$PATH_SEPARATOR/sbin$PATH_SEPARATOR/usr/sbin$PATH_SEPARATOR/bin$PATH_SEPARATOR/usr/bin])
if test "$GPLUSPLUS" = "no"; then
  AC_MSG_ERROR([g++ is required])
fi
AC_DEFINE_UNQUOTED([GPLUSPLUS], ["$GPLUSPLUS"], [path to g++])

AC_CHECK_HEADERS([algorithm array cmath queue \
cstdio string sys/stat.h sys/types.h ctime tuple unistd.h unordered_map \
utility vector], [], [AC_MSG_ERROR([Missing header file])])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T
AC_LANG_POP(C++)

AC_CONFIG_FILES([Makefile
                 third_party/Makefile])

AC_OUTPUT
