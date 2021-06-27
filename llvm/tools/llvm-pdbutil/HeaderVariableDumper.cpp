//===- HeaderVariableDumper.cpp ---------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "HeaderVariableDumper.h"

#include "LinePrinter.h"
#include "PrettyBuiltinDumper.h"
#include "PrettyFunctionDumper.h"
#include "llvm-pdbutil.h"

#include "llvm/DebugInfo/PDB/IPDBSession.h"
#include "llvm/DebugInfo/PDB/PDBSymbolData.h"
#include "llvm/DebugInfo/PDB/PDBSymbolFunc.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeArray.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeEnum.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeFunctionSig.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypePointer.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeTypedef.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeUDT.h"
#include "llvm/DebugInfo/PDB/PDBTypes.h"

#include "llvm/Support/Format.h"

using namespace llvm;
using namespace llvm::codeview;
using namespace llvm::pdb;

HeaderVariableDumper::HeaderVariableDumper(LinePrinter &P, AnonTypenameTracker &A)
    : PDBSymDumper(true), Printer(P), AnonTypenames(A) {}

void HeaderVariableDumper::start(const PDBSymbolData &Var, uint32_t Offset) {
  if (Var.isCompilerGenerated() && opts::pretty::ExcludeCompilerGenerated)
    return;
  if (Printer.IsSymbolExcluded(Var.getName()))
    return;

  auto VarType = Var.getType();

  uint64_t Length = VarType->getRawSymbol().getLength();

  switch (auto LocType = Var.getLocationType()) {
  case PDB_LocType::Static:
    Printer.NewLine();
    WithColor(Printer, PDB_ColorItem::Keyword).get() << "static ";
    dumpSymbolTypeAndName(*VarType, Var.getName());
    Printer << ";";

    if (opts::header::ExtraInfo) {
      WithColor(Printer, PDB_ColorItem::Comment).get()
          << " // Size: " << Length;
    }
    break;
  case PDB_LocType::Constant:
    if (isa<PDBSymbolTypeEnum>(*VarType))
      break;
    Printer.NewLine();
    WithColor(Printer, PDB_ColorItem::Keyword).get() << "static constexpr ";
    dumpSymbolTypeAndName(*VarType, Var.getName());
    Printer << " = ";
    WithColor(Printer, PDB_ColorItem::LiteralValue).get() << Var.getValue();
    Printer << ";";

    if (opts::header::ExtraInfo) {
      WithColor(Printer, PDB_ColorItem::Comment).get()
          << " // Size: " << Length;
    }
    break;
  case PDB_LocType::ThisRel:
    Printer.NewLine();
    dumpSymbolTypeAndName(*VarType, Var.getName());
    Printer << ";";

    if (opts::header::ExtraInfo) {
      WithColor(Printer, PDB_ColorItem::Comment).get()
          << " // Offset: +" << format_hex(Offset + Var.getOffset(), 4)
          << "; Size: " << Length;
    }
    break;
  case PDB_LocType::BitField:
    Printer.NewLine();
    dumpSymbolTypeAndName(*VarType, Var.getName());
    Printer << " : ";
    WithColor(Printer, PDB_ColorItem::LiteralValue).get() << Var.getLength();
    Printer << ";";

    if (opts::header::ExtraInfo) {
      WithColor(Printer, PDB_ColorItem::Comment).get()
          << " // Offset: +" << format_hex(Offset + Var.getOffset(), 4)
          << "; Size: " << Length;
    }
    break;
  default:
    Printer.NewLine();
    WithColor(Printer, PDB_ColorItem::Comment).get()
        << " // Unknown(" << LocType << "): " << Var.getName();
    break;
  }
}

void HeaderVariableDumper::startVbptr(uint32_t Offset, uint32_t Size) {
  Printer.NewLine();
  Printer << "vbptr ";

  WithColor(Printer, PDB_ColorItem::Offset).get()
      << "+" << format_hex(Offset, 4) << " [sizeof=" << Size << "] ";
}

void HeaderVariableDumper::start(const PDBSymbolTypeVTable &Var,
                                 uint32_t Offset) {
  Printer.NewLine();
  Printer << "vfptr ";
  auto VTableType = cast<PDBSymbolTypePointer>(Var.getType());
  uint32_t PointerSize = VTableType->getLength();

  WithColor(Printer, PDB_ColorItem::Offset).get()
      << "+" << format_hex(Offset + Var.getOffset(), 4)
      << " [sizeof=" << PointerSize << "] ";
}

void HeaderVariableDumper::dump(const PDBSymbolTypeArray &Symbol) {
  auto ElementType = Symbol.getElementType();
  assert(ElementType);
  if (!ElementType)
    return;
  ElementType->dump(*this);
}

void HeaderVariableDumper::dumpRight(const PDBSymbolTypeArray &Symbol) {
  auto ElementType = Symbol.getElementType();
  assert(ElementType);
  if (!ElementType)
    return;
  Printer << '[' << Symbol.getCount() << ']';
  ElementType->dumpRight(*this);
}

void HeaderVariableDumper::dump(const PDBSymbolTypeBuiltin &Symbol) {
  BuiltinDumper Dumper(Printer);
  Dumper.start(Symbol);
}

void HeaderVariableDumper::dump(const PDBSymbolTypeEnum &Symbol) {
  WithColor(Printer, PDB_ColorItem::Type).get() << Symbol.getName();
}

void HeaderVariableDumper::dump(const PDBSymbolTypeFunctionSig &Symbol) {
  auto ReturnType = Symbol.getReturnType();
  ReturnType->dump(*this);
  Printer << " ";

  uint32_t ClassParentId = Symbol.getClassParentId();
  auto ClassParent =
      Symbol.getSession().getConcreteSymbolById<PDBSymbolTypeUDT>(
          ClassParentId);

  if (ClassParent) {
    WithColor(Printer, PDB_ColorItem::Identifier).get()
        << ClassParent->getName();
    Printer << "::";
  }
}

void HeaderVariableDumper::dumpRight(const PDBSymbolTypeFunctionSig &Symbol) {
  Printer << "(";
  if (auto Arguments = Symbol.getArguments()) {
    uint32_t Index = 0;
    while (auto Arg = Arguments->getNext()) {
      Arg->dump(*this);
      if (++Index < Arguments->getChildCount())
        Printer << ", ";
    }
  }
  Printer << ")";

  if (Symbol.isConstType())
    WithColor(Printer, PDB_ColorItem::Keyword).get() << " const";
  if (Symbol.isVolatileType())
    WithColor(Printer, PDB_ColorItem::Keyword).get() << " volatile";

  if (Symbol.getRawSymbol().isRestrictedType())
    WithColor(Printer, PDB_ColorItem::Keyword).get() << " __restrict";
}

void HeaderVariableDumper::dump(const PDBSymbolTypePointer &Symbol) {
  auto PointeeType = Symbol.getPointeeType();
  if (!PointeeType)
    return;
  PointeeType->dump(*this);
  if (auto FuncSig = unique_dyn_cast<PDBSymbolTypeFunctionSig>(PointeeType)) {
    // A hack to get the calling convention in the right spot.
    Printer << " (";
    PDB_CallingConv CC = FuncSig->getCallingConvention();
    WithColor(Printer, PDB_ColorItem::Keyword).get() << CC << " ";
  } else if (isa<PDBSymbolTypeArray>(PointeeType)) {
    Printer << " (";
  }
  Printer << (Symbol.isReference() ? "&" : "*");
  if (Symbol.isConstType())
    WithColor(Printer, PDB_ColorItem::Keyword).get() << " const ";
  if (Symbol.isVolatileType())
    WithColor(Printer, PDB_ColorItem::Keyword).get() << " volatile ";

  if (Symbol.getRawSymbol().isRestrictedType())
    WithColor(Printer, PDB_ColorItem::Keyword).get() << " __restrict ";
}

void HeaderVariableDumper::dumpRight(const PDBSymbolTypePointer &Symbol) {
  auto PointeeType = Symbol.getPointeeType();
  assert(PointeeType);
  if (!PointeeType)
    return;
  if (isa<PDBSymbolTypeFunctionSig>(PointeeType) ||
      isa<PDBSymbolTypeArray>(PointeeType)) {
    Printer << ")";
  }
  PointeeType->dumpRight(*this);
}

void HeaderVariableDumper::dump(const PDBSymbolTypeTypedef &Symbol) {
  WithColor(Printer, PDB_ColorItem::Keyword).get() << "typedef ";
  WithColor(Printer, PDB_ColorItem::Type).get() << Symbol.getName();
}

void HeaderVariableDumper::dump(const PDBSymbolTypeUDT &Symbol) {
  WithColor(Printer, PDB_ColorItem::Type).get() << Symbol.getName();
}

void HeaderVariableDumper::dumpSymbolTypeAndName(const PDBSymbol &Type,
                                           StringRef Name) {
  Type.dump(*this);
  WithColor(Printer, PDB_ColorItem::Identifier).get() << " " << Name;
  Type.dumpRight(*this);
}
