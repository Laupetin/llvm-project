//===- HeaderClassDefinitionDumper.cpp --------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "HeaderClassDefinitionDumper.h"

#include "LinePrinter.h"
#include "HeaderClassLayoutGraphicalDumper.h"
#include "HeaderFunctionDumper.h"
#include "llvm-pdbutil.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeBaseClass.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeUDT.h"
#include "llvm/DebugInfo/PDB/UDTLayout.h"

#include "llvm/Support/Format.h"

using namespace llvm;
using namespace llvm::pdb;

HeaderClassDefinitionDumper::HeaderClassDefinitionDumper(
    LinePrinter &P, AnonTypenameTracker &A)
  : PDBSymDumper(true), Printer(P), AnonTypenames(A) {
}

void HeaderClassDefinitionDumper::start(const PDBSymbolTypeUDT &Class) {
  assert(opts::pretty::ClassFormat !=
      opts::pretty::ClassDefinitionFormat::None);

  ClassLayout Layout(Class);
  start(Layout);
}

void HeaderClassDefinitionDumper::start(const ClassLayout &Layout) {
  if (shouldDumpVtbl(Layout.getClass())) {
    prettyPrintVtbl(Layout.getClass());
  }

  prettyPrintClassIntro(Layout);

  HeaderClassLayoutGraphicalDumper Dumper(Printer, AnonTypenames, 0);
  Dumper.start(Layout);

  prettyPrintClassOutro(Layout);
}

bool HeaderClassDefinitionDumper::shouldDumpVtbl(
    const PDBSymbolTypeUDT &Class) {

  if (opts::header::Methods)
    return false;

  if (Class.getVirtualTableShapeId() == 0)
    return false;

  const auto shape = unique_dyn_cast<PDBSymbolTypeVTableShape>(
      Class.getVirtualTableShape());
  if (!shape)
    return false;

  if (shape->getCount() <= 0)
    return false;

  const auto allVTables = Class.findAllChildren<PDBSymbolTypeVTable>();
  if (!allVTables || allVTables->getChildCount() <= 0)
    return false;

  const auto parentId = Class.getSymIndexId();
  while (auto vtable = allVTables->getNext()) {
    if (vtable->getClassParentId() == parentId) {
      return true;
    }
  }

  return false;
}

void HeaderClassDefinitionDumper::prettyPrintVtbl(
    const PDBSymbolTypeUDT &Class) {

  Printer << "struct ";
  const auto className = Class.getName();
  if (AnonTypenameTracker::isAnonSymbolName(className)) {
    WithColor(Printer, PDB_ColorItem::Type).get()
        << AnonTypenames.getAnonTypename(Class) << "Vtbl";
  } else {
    WithColor(Printer, PDB_ColorItem::Type).get() << className << "Vtbl";
  }
  Printer.NewLine();
  Printer << "{";
  Printer.Indent();

  HeaderFunctionDumper Dumper(Printer);
  const auto children = Class.findAllChildren<PDBSymbolFunc>();
  if (children && children->getChildCount() > 0) {
    while (auto child = children->getNext()) {
      if (child->isVirtual()) {
        Printer.NewLine();
        Dumper.start(*child, HeaderFunctionDumper::PointerType::Pointer);
      }
    }
  }

  Printer.Unindent();
  Printer.NewLine();
  Printer << "};";
  Printer.NewLine();
  Printer.NewLine();
}

void HeaderClassDefinitionDumper::prettyPrintClassIntro(
    const ClassLayout &Layout) {
  uint32_t Size = Layout.getSize();
  const PDBSymbolTypeUDT &Class = Layout.getClass();

  if (Layout.getClass().isConstType())
    WithColor(Printer, PDB_ColorItem::Keyword).get() << "const ";
  if (Layout.getClass().isVolatileType())
    WithColor(Printer, PDB_ColorItem::Keyword).get() << "volatile ";
  if (Layout.getClass().isUnalignedType())
    WithColor(Printer, PDB_ColorItem::Keyword).get() << "unaligned ";

  WithColor(Printer, PDB_ColorItem::Keyword).get() << Class.getUdtKind();
  Printer << " ";

  const auto className = Class.getName();
  if (AnonTypenameTracker::isAnonSymbolName(className)) {
    WithColor(Printer, PDB_ColorItem::Type).get() << AnonTypenames.
        getAnonTypename(Class);
  } else {
    WithColor(Printer, PDB_ColorItem::Type).get() << className;
  }

  if (opts::header::ExtraInfo) {
    WithColor(Printer, PDB_ColorItem::Comment).get() << " // Size: " << Size;
  }

  uint32_t BaseCount = Layout.bases().size();
  if (BaseCount > 0) {
    Printer.Indent();
    char NextSeparator = ':';
    for (auto BC : Layout.bases()) {
      const auto &Base = BC->getBase();
      if (Base.isIndirectVirtualBaseClass())
        continue;

      Printer.NewLine();
      Printer << NextSeparator << " ";
      WithColor(Printer, PDB_ColorItem::Keyword).get() << Base.getAccess();
      if (BC->isVirtualBase())
        WithColor(Printer, PDB_ColorItem::Keyword).get() << " virtual";

      WithColor(Printer, PDB_ColorItem::Type).get() << " " << Base.getName();
      NextSeparator = ',';
    }

    Printer.Unindent();
  }

  Printer.NewLine();
  Printer << "{";
  Printer.Indent();
}

void HeaderClassDefinitionDumper::prettyPrintClassOutro(
    const ClassLayout &Layout) {
  Printer.Unindent();
  Printer.NewLine();
  Printer << "};";

  if (opts::header::ExtraInfo) {
    if (Layout.deepPaddingSize() > 0) {
      Printer.NewLine();
      APFloat Pct(100.0 * (double)Layout.deepPaddingSize() /
                  (double)Layout.getSize());
      SmallString<8> PctStr;
      Pct.toString(PctStr, 4);
      WithColor(Printer, PDB_ColorItem::Comment).get()
          << "// Total padding " << Layout.deepPaddingSize() << " bytes ("
          << PctStr << "% of class size)";
      Printer.NewLine();
      APFloat Pct2(100.0 * (double)Layout.immediatePadding() /
                   (double)Layout.getSize());
      PctStr.clear();
      Pct2.toString(PctStr, 4);
      WithColor(Printer, PDB_ColorItem::Comment).get()
          << "// Immediate padding " << Layout.immediatePadding() << " bytes ("
          << PctStr << "% of class size)";
    }
  }
}
