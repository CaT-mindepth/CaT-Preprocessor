
# CaT Preprocessor

This is a preprocessor capable of optimizing P4 action blocks given as Domino program input, modified from the Domino compiler (https://github.com/packet-transactions/domino-compiler). In particular, we made several enhancements to the Domino optimizations, and stripped it of all its backend capabilities to simply output a preprocessed intermediate representation to be fed into the CaT code generator.

## Installation guide

First, make sure `automake`, `clang`, `clang++`, and `lld` are installed on your target system. They can be installed either from source or via your favorite package manager.


Next, go download LLVM 10.0.0 for your own architecture from the LLVM website: https://releases.llvm.org/download.html#10.0.0. For instance, 
 - This is the download link for Ubuntu 18.04+, x86-64: `https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz`
 - This is the download link for macOS: `https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/clang+llvm-10.0.0-x86_64-apple-darwin.tar.xz`
 - Alternatively, you can compile LLVM from scratch, but make sure to include the clang and clang-libtooling subprojects.


Finally you can simply perform a typical autotools make process, with `CLANG_DEV_LIBS` set to the LLVM directory:
```
./autogen.sh
CXX="clang++" CC="clang" CLANG_DEV_LIBS="<path to your llvm installation>" LLVM_VERSION="<LLVM-10 or LLVM-13>" ./configure 

make # -j4
```

## Using the preprocessor

You should get a binary named `domino` in the current folder after you successfully compiled everything. The usage is:
```
./domino <input domino .c file> > preprocessed.in
```

Afterwards, you can feed the content inside `preprocessed.in` into the CaT codegen.






