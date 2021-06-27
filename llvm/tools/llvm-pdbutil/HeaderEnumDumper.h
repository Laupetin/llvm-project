//===- PrettyEnumDumper.h ---------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVMPDBDUMP_HEADERENUMDUMPER_H
#define LLVM_TOOLS_LLVMPDBDUMP_HEADERENUMDUMPER_H

#include "AnonTypenameTracker.h"

#include "llvm/DebugInfo/PDB/PDBSymDumper.h"

namespace llvm {
namespace pdb {

class LinePrinter;

class HeaderEnumDumper : public PDBSymDumper {
public:
  HeaderEnumDumper(LinePrinter &P, AnonTypenameTracker& A);

  void start(const PDBSymbolTypeEnum &Symbol);

private:
  static void PrintEnumValue(raw_ostream& stream, const Variant& val);

  LinePrinter &Printer;
  AnonTypenameTracker &AnonTypenames;
};
} // namespace pdb
} // namespace llvm
#endif
