//===- HeaderClassLayoutGraphicalDumper.h -----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVMPDBDUMP_HEADERCLASSLAYOUTGRAPHICALDUMPER_H
#define LLVM_TOOLS_LLVMPDBDUMP_HEADERCLASSLAYOUTGRAPHICALDUMPER_H

#include "AnonTypenameTracker.h"

#include "llvm/ADT/BitVector.h"

#include "llvm/DebugInfo/PDB/PDBSymDumper.h"

namespace llvm {

namespace pdb {

class UDTLayoutBase;
class LayoutItemBase;
class LinePrinter;

class HeaderClassLayoutGraphicalDumper : public PDBSymDumper {
public:
  HeaderClassLayoutGraphicalDumper(LinePrinter &P, AnonTypenameTracker &A, uint32_t InitialOffset);

  void start(const UDTLayoutBase &Layout);

  // Layout based symbol types.
  void dump(const PDBSymbolTypeBaseClass &Symbol) override;
  void dump(const PDBSymbolData &Symbol) override;
  void dump(const PDBSymbolTypeVTable &Symbol) override;

  // Non layout-based symbol types.
  void dump(const PDBSymbolTypeEnum &Symbol) override;
  void dump(const PDBSymbolFunc &Symbol) override;
  void dump(const PDBSymbolTypeTypedef &Symbol) override;
  void dump(const PDBSymbolTypeUDT &Symbol) override;
  void dump(const PDBSymbolTypeBuiltin &Symbol) override;

private:
  void printPaddingRow(uint32_t Amount);

  LinePrinter &Printer;
  AnonTypenameTracker &AnonTypenames;

  LayoutItemBase *CurrentItem = nullptr;
  uint32_t ClassOffsetZero = 0;
  uint32_t CurrentAbsoluteOffset = 0;
};
} // namespace pdb
} // namespace llvm
#endif
