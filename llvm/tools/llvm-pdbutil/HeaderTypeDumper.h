//===- PrettyTypeDumper.h - PDBSymDumper implementation for types *- C++ *-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVMPDBDUMP_HEADERTYPEDUMPER_H
#define LLVM_TOOLS_LLVMPDBDUMP_HEADERTYPEDUMPER_H

#include "AnonTypenameTracker.h"
#include "llvm/DebugInfo/PDB/PDBSymDumper.h"

namespace llvm {
namespace pdb {
class LinePrinter;
class ClassLayout;

class HeaderTypeDumper : public PDBSymDumper {
public:
  HeaderTypeDumper(LinePrinter &P);

  void start(const PDBSymbolExe &Exe);

  void dump(const PDBSymbolTypeEnum &Symbol) override;
  void dump(const PDBSymbolTypeTypedef &Symbol) override;

  void dumpClassForwardDeclaration(const PDBSymbolTypeUDT &Symbol);
  void dumpClassLayout(const ClassLayout &Class);

private:
  AnonTypenameTracker AnonTypenames;
  LinePrinter &Printer;
};
} // namespace pdb
} // namespace llvm
#endif
