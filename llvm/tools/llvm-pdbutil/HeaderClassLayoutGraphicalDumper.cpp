//===- HeaderClassLayoutGraphicalDumper.h -----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "HeaderClassLayoutGraphicalDumper.h"

#include "HeaderEnumDumper.h"
#include "HeaderVariableDumper.h"
#include "LinePrinter.h"
#include "PrettyClassDefinitionDumper.h"
#include "PrettyEnumDumper.h"
#include "PrettyFunctionDumper.h"
#include "PrettyTypedefDumper.h"
#include "llvm-pdbutil.h"

#include "llvm/DebugInfo/PDB/PDBSymbolData.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeBaseClass.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeUDT.h"
#include "llvm/DebugInfo/PDB/UDTLayout.h"
#include "llvm/Support/Format.h"

using namespace llvm;
using namespace llvm::pdb;

HeaderClassLayoutGraphicalDumper::HeaderClassLayoutGraphicalDumper(
    LinePrinter &P, AnonTypenameTracker &A,uint32_t InitialOffset)
    : PDBSymDumper(true), Printer(P), AnonTypenames(A),
      ClassOffsetZero(InitialOffset), CurrentAbsoluteOffset(InitialOffset) {}

void HeaderClassLayoutGraphicalDumper::start(const UDTLayoutBase &Layout) {

  for (auto &Other : Layout.other_items())
    Other->dump(*this);
  for (auto &Func : Layout.funcs())
    Func->dump(*this);

  const BitVector &UseMap = Layout.usedBytes();
  int NextPaddingByte = UseMap.find_first_unset();

  for (auto &Item : Layout.layout_items()) {
    // Calculate the absolute offset of the first byte of the next field.
    uint32_t RelativeOffset = Item->getOffsetInParent();
    CurrentAbsoluteOffset = ClassOffsetZero + RelativeOffset;

    // This might be an empty base, in which case it could extend outside the
    // bounds of the parent class.
    if (RelativeOffset < UseMap.size() && (Item->getSize() > 0)) {
      // If there is any remaining padding in this class, and the offset of the
      // new item is after the padding, then we must have just jumped over some
      // padding.  Print a padding row and then look for where the next block
      // of padding begins.
      if ((NextPaddingByte >= 0) &&
          (RelativeOffset > uint32_t(NextPaddingByte))) {
        printPaddingRow(RelativeOffset - NextPaddingByte);
        NextPaddingByte = UseMap.find_next_unset(RelativeOffset);
      }
    }

    CurrentItem = Item;
    if (Item->isVBPtr()) {
      VTableLayoutItem &Layout = static_cast<VTableLayoutItem &>(*CurrentItem);

      HeaderVariableDumper VarDumper(Printer, AnonTypenames);
      VarDumper.startVbptr(CurrentAbsoluteOffset, Layout.getSize());
    } else {
      if (auto Sym = Item->getSymbol())
        Sym->dump(*this);
    }

    if (Item->getLayoutSize() > 0) {
      uint32_t Prev = RelativeOffset + Item->getLayoutSize() - 1;
      if (Prev < UseMap.size())
        NextPaddingByte = UseMap.find_next_unset(Prev);
    }
  }

  if (opts::header::ExtraInfo) {
    auto TailPadding = Layout.tailPadding();
    if (TailPadding > 0) {
      if (TailPadding != 1 || Layout.getSize() != 1) {
        Printer.NewLine();
        WithColor(Printer, PDB_ColorItem::Comment).get()
            << "// <padding> (" << TailPadding << " bytes)";
      }
    }
  }
}

void HeaderClassLayoutGraphicalDumper::printPaddingRow(uint32_t Amount) {
  if (Amount == 0)
    return;

  if (opts::header::ExtraInfo) {
    Printer.NewLine();
    WithColor(Printer, PDB_ColorItem::Comment).get()
        << "// <padding> (" << Amount << " bytes)";
  }
}

void HeaderClassLayoutGraphicalDumper::dump(
    const PDBSymbolTypeBaseClass &Symbol) {
  assert(CurrentItem != nullptr);

  Printer.NewLine();
  BaseClassLayout &Layout = static_cast<BaseClassLayout &>(*CurrentItem);

  std::string Label = "base";
  if (Layout.isVirtualBase()) {
    Label.insert(Label.begin(), 'v');
    if (Layout.getBase().isIndirectVirtualBaseClass())
      Label.insert(Label.begin(), 'i');
  }
  Printer << Label << " ";

  uint32_t Size = Layout.isEmptyBase() ? 1 : Layout.getLayoutSize();

  WithColor(Printer, PDB_ColorItem::Offset).get()
      << "+" << format_hex(CurrentAbsoluteOffset, 4) << " [sizeof=" << Size
      << "] ";

  WithColor(Printer, PDB_ColorItem::Identifier).get() << Layout.getName();
}

void HeaderClassLayoutGraphicalDumper::dump(const PDBSymbolData &Symbol) {
  HeaderVariableDumper VarDumper(Printer, AnonTypenames);
  VarDumper.start(Symbol, ClassOffsetZero);
}

void HeaderClassLayoutGraphicalDumper::dump(const PDBSymbolTypeVTable &Symbol) {
  assert(CurrentItem != nullptr);

  HeaderVariableDumper VarDumper(Printer, AnonTypenames);
  VarDumper.start(Symbol, ClassOffsetZero);
}

void HeaderClassLayoutGraphicalDumper::dump(const PDBSymbolTypeEnum &Symbol) {
  Printer.NewLine();
  HeaderEnumDumper Dumper(Printer, AnonTypenames);
  Dumper.start(Symbol);
}

void HeaderClassLayoutGraphicalDumper::dump(
    const PDBSymbolTypeTypedef &Symbol) {
  Printer.NewLine();
  TypedefDumper Dumper(Printer);
  Dumper.start(Symbol);
}

void HeaderClassLayoutGraphicalDumper::dump(
    const PDBSymbolTypeBuiltin &Symbol) {}

void HeaderClassLayoutGraphicalDumper::dump(const PDBSymbolTypeUDT &Symbol) {}

void HeaderClassLayoutGraphicalDumper::dump(const PDBSymbolFunc &Symbol) {
  if (Printer.IsSymbolExcluded(Symbol.getName()))
    return;
  if (Symbol.isCompilerGenerated() && opts::pretty::ExcludeCompilerGenerated)
    return;
  if (Symbol.getLength() == 0 && !Symbol.isPureVirtual() &&
      !Symbol.isIntroVirtualFunction())
    return;
  
  Printer.NewLine();
  FunctionDumper Dumper(Printer);
  Dumper.start(Symbol, FunctionDumper::PointerType::None);
}
