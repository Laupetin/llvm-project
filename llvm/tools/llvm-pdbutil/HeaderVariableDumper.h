//===- HeaderVariableDumper.h - PDBSymDumper variable dumper ----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVMPDBDUMP_HEADERVARIABLEDUMPER_H
#define LLVM_TOOLS_LLVMPDBDUMP_HEADERVARIABLEDUMPER_H

#include "AnonTypenameTracker.h"

#include "llvm/DebugInfo/PDB/PDBSymDumper.h"

namespace llvm {

class StringRef;

namespace pdb {

class LinePrinter;

class HeaderVariableDumper : public PDBSymDumper {
public:
  HeaderVariableDumper(LinePrinter &P, AnonTypenameTracker &A);

  void start(const PDBSymbolData &Var, uint32_t Offset = 0);
  void start(const PDBSymbolTypeVTable &Var, uint32_t Offset = 0);
  void startVbptr(uint32_t Offset, uint32_t Size);

  void dump(const PDBSymbolTypeArray &Symbol) override;
  void dump(const PDBSymbolTypeBuiltin &Symbol) override;
  void dump(const PDBSymbolTypeEnum &Symbol) override;
  void dump(const PDBSymbolTypeFunctionSig &Symbol) override;
  void dump(const PDBSymbolTypePointer &Symbol) override;
  void dump(const PDBSymbolTypeTypedef &Symbol) override;
  void dump(const PDBSymbolTypeUDT &Symbol) override;
  void dump(const PDBSymbolTypeVTableShape &Symbol) override;

  void dumpRight(const PDBSymbolTypeArray &Symbol) override;
  void dumpRight(const PDBSymbolTypeFunctionSig &Symbol) override;
  void dumpRight(const PDBSymbolTypePointer &Symbol) override;

private:
  void dumpSymbolTypeAndName(const PDBSymbol &Type, StringRef Name);

  LinePrinter &Printer;
  AnonTypenameTracker &AnonTypenames;
};
} // namespace pdb
} // namespace llvm
#endif
