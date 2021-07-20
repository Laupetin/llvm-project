//===- PrettyTypeDumper.cpp - PDBSymDumper type dumper *------------ C++ *-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "HeaderTypeDumper.h"

#include "HeaderEnumDumper.h"
#include "LinePrinter.h"
#include "PrettyBuiltinDumper.h"
#include "HeaderClassDefinitionDumper.h"
#include "HeaderTypedefDumper.h"
#include "HeaderFunctionDumper.h"
#include "llvm-pdbutil.h"

#include "llvm/DebugInfo/PDB/IPDBSession.h"
#include "llvm/DebugInfo/PDB/PDBSymbolExe.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeArray.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeBuiltin.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeEnum.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeFunctionSig.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypePointer.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeTypedef.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeUDT.h"
#include "llvm/DebugInfo/PDB/UDTLayout.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;
using namespace llvm::pdb;

using LayoutPtr = std::unique_ptr<ClassLayout>;

HeaderTypeDumper::HeaderTypeDumper(LinePrinter &P)
    : PDBSymDumper(true), Printer(P) {}

template <typename T>
static bool isTypeExcluded(LinePrinter &Printer, const T &Symbol) {
  return false;
}

static bool isTypeExcluded(LinePrinter &Printer,
                           const PDBSymbolTypeEnum &Enum) {
  if (Printer.IsTypeExcluded(Enum.getName(), Enum.getLength()))
    return true;
  // Dump member enums when dumping their class definition.
  if (nullptr != Enum.getClassParent())
    return true;
  return false;
}

static bool isTypeExcluded(LinePrinter &Printer,
                           const PDBSymbolTypeTypedef &Typedef) {
  return Printer.IsTypeExcluded(Typedef.getName(), Typedef.getLength());
}

template <typename SymbolT>
static void dumpSymbolCategory(LinePrinter &Printer, const PDBSymbolExe &Exe,
                               HeaderTypeDumper &TD) {
  if (auto Children = Exe.findAllChildren<SymbolT>()) {
    while (auto Child = Children->getNext()) {
      if (isTypeExcluded(Printer, *Child))
        continue;

      Printer.NewLine();
      Child->dump(TD);
      Printer.NewLine();
    }
  }
}

void HeaderTypeDumper::start(const PDBSymbolExe &Exe) {

  std::vector<LayoutPtr> ClassList;
  if (opts::header::Classes) {
    if (auto Classes = Exe.findAllChildren<PDBSymbolTypeUDT>()) {

      while (auto Class = Classes->getNext()) {
        if (Printer.IsTypeExcluded(Class->getName(), Class->getLength()))
          continue;

        // No point duplicating a full class layout.  Just print the modified
        // declaration and continue.
        if (Class->getUnmodifiedTypeId() != 0) {
          continue;
        }

        Printer.NewLine();
        dumpClassForwardDeclaration(*Class);
        ClassList.emplace_back(std::make_unique<ClassLayout>(std::move(Class)));
      }
      Printer.NewLine();
    }
  }

  if (opts::header::Enums)
    dumpSymbolCategory<PDBSymbolTypeEnum>(Printer, Exe, *this);

  if (opts::header::Typedefs)
    dumpSymbolCategory<PDBSymbolTypeTypedef>(Printer, Exe, *this);

  if (opts::header::Classes) {
    for (auto &Class : ClassList)
      dumpClassLayout(*Class);
  }
}

void HeaderTypeDumper::dump(const PDBSymbolTypeEnum &Symbol) {
  assert(opts::header::Enums);

  HeaderEnumDumper Dumper(Printer, AnonTypenames);
  Dumper.start(Symbol);
}

void HeaderTypeDumper::dump(const PDBSymbolTypeTypedef &Symbol) {
  assert(opts::header::Typedefs);

  HeaderTypedefDumper Dumper(Printer);
  Dumper.start(Symbol);
}

void HeaderTypeDumper::dumpClassLayout(const ClassLayout &Class) {
  assert(opts::header::Classes);

  Printer.NewLine();
  HeaderClassDefinitionDumper Dumper(Printer, AnonTypenames);
  Dumper.start(Class);
  Printer.NewLine();
}

void HeaderTypeDumper::dumpClassForwardDeclaration(const PDBSymbolTypeUDT &Symbol) {
  WithColor(Printer, PDB_ColorItem::Keyword).get() << Symbol.getUdtKind() << " ";
  WithColor(Printer, PDB_ColorItem::Type).get() << Symbol.getName();
  WithColor(Printer, PDB_ColorItem::None).get() << ";";
  Printer << " // id: " << Symbol.getSymIndexId()  <<" len:" << Symbol.getLength();
}
