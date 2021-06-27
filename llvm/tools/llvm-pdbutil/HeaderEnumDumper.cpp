//===- PrettyEnumDumper.cpp -------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "HeaderEnumDumper.h"

#include "LinePrinter.h"
#include "PrettyBuiltinDumper.h"
#include "llvm-pdbutil.h"

#include "llvm/DebugInfo/PDB/PDBSymbolData.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeBuiltin.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeEnum.h"

#include <ios>

using namespace llvm;
using namespace llvm::pdb;

HeaderEnumDumper::HeaderEnumDumper(LinePrinter &P, AnonTypenameTracker &A) : PDBSymDumper(true), Printer(P), AnonTypenames(A) {}

void HeaderEnumDumper::start(const PDBSymbolTypeEnum &Symbol) {
  if (Symbol.getUnmodifiedTypeId() != 0) {
    if (Symbol.isConstType())
      WithColor(Printer, PDB_ColorItem::Keyword).get() << "const ";
    if (Symbol.isVolatileType())
      WithColor(Printer, PDB_ColorItem::Keyword).get() << "volatile ";
    if (Symbol.isUnalignedType())
      WithColor(Printer, PDB_ColorItem::Keyword).get() << "unaligned ";
    WithColor(Printer, PDB_ColorItem::Keyword).get() << "enum ";
    WithColor(Printer, PDB_ColorItem::Type).get() << Symbol.getName();
    return;
  }

  WithColor(Printer, PDB_ColorItem::Keyword).get() << "enum ";

  auto symbolName = Symbol.getName();
  if (symbolName.size() >= 2 && symbolName[0] == '<' &&
      symbolName[symbolName.size() - 1] == '>') {
    WithColor(Printer, PDB_ColorItem::Type).get() << AnonTypenames.getAnonTypename(Symbol);
  } else {
    WithColor(Printer, PDB_ColorItem::Type).get() << symbolName;
  }

  auto UnderlyingType = Symbol.getUnderlyingType();
  if (!UnderlyingType)
    return;
  if (UnderlyingType->getBuiltinType() != PDB_BuiltinType::Int ||
      UnderlyingType->getLength() != 4) {
    Printer << " : ";
    BuiltinDumper Dumper(Printer);
    Dumper.start(*UnderlyingType);
  }

  auto EnumValues = Symbol.findAllChildren<PDBSymbolData>();
  Printer.NewLine();
  Printer << "{";
  Printer.Indent();
  if (EnumValues && EnumValues->getChildCount() > 0) {
    bool firstValue = true;
    while (auto EnumValue = EnumValues->getNext()) {
      if (EnumValue->getDataKind() != PDB_DataKind::Constant)
        continue;

      if (firstValue)
        firstValue = false;
      else
        Printer << ",";

      Printer.NewLine();
      WithColor(Printer, PDB_ColorItem::Identifier).get()
          << EnumValue->getName();
      Printer << " = ";
      PrintEnumValue(WithColor(Printer, PDB_ColorItem::LiteralValue).get(),
                     EnumValue->getValue());
    }
  }
  Printer.Unindent();
  Printer.NewLine();
  Printer << "};";
}

void HeaderEnumDumper::PrintEnumValue(raw_ostream &stream, const Variant &val) {

  uint64_t hexVal;

  switch (val.Type) {
  case Int8:
    hexVal = static_cast<uint8_t>(val.Value.Int8);
    break;

  case Int16:
    hexVal = static_cast<uint16_t>(val.Value.Int16);
    break;

  case Int32:
    hexVal = static_cast<uint32_t>(val.Value.Int32);
    break;

  case Int64:
    hexVal = static_cast<uint64_t>(val.Value.Int64);
    break;

  case UInt8:
    hexVal = val.Value.UInt8;
    break;

  case UInt16:
    hexVal = val.Value.UInt16;
    break;

  case UInt32:
    hexVal = val.Value.UInt32;
    break;

  case UInt64:
    hexVal = val.Value.UInt64;
    break;

  default:
    return;
  }

  stream << format_hex(hexVal, 2, true);
}
