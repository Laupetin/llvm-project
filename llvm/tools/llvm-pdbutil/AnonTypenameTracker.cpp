//===- AnonTypenameTracker.cpp -------------------------------- *- C++ --*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AnonTypenameTracker.h"

#include "llvm/DebugInfo/PDB/PDBSymbolTypeEnum.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeUDT.h"
#include "llvm/DebugInfo/PDB/PDBSymbolTypeTypedef.h"
#include "llvm/DebugInfo/PDB/PDBSymbolData.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/MD5.h"

using namespace llvm;
using namespace llvm::pdb;
using namespace llvm::support;

std::string AnonTypenameTracker::m_not_anon_name;

AnonTypenameTracker::AnonTypenameTracker() = default;

const std::string &
AnonTypenameTracker::getAnonTypename(const PDBSymbol &Symbol) {

  const auto previousName =
      PreviouslyCreatedTypenames.find(Symbol.getSymIndexId());

  if (previousName != PreviouslyCreatedTypenames.end()) {
    return previousName->second;
  }

  const auto &result = createAnonTypename(Symbol);

  if (hasNestedTypes(Symbol)) {
    createNestedTypesTypename(Symbol, result);
  }

  return result;
}

bool AnonTypenameTracker::isAnonSymbolName(const std::string &name) {
  size_t start, end;
  return getLastAnonTypenameOccurrence(name, nullptr, nullptr);
}

bool AnonTypenameTracker::getLastAnonTypenameOccurrence(
    const std::string &name, size_t *start, size_t *end) {
  return getLastAnonTypenameOccurrence(name, start, end, name.size());
}


bool AnonTypenameTracker::getLastAnonTypenameOccurrence(const std::string &name,
  size_t *start,
  size_t *end,
  const size_t beginPos) {
  size_t searchStartPos = beginPos;

  while (searchStartPos > 0) {
    auto searchEndPos = name.rfind("::", searchStartPos);
    if (searchEndPos == std::string::npos)
      searchEndPos = 0;
    else
      searchEndPos += 2;

    if (searchStartPos != searchEndPos && name[searchStartPos - 1] == '>' &&
        name[searchEndPos] == '<') {

      if (start)
        *start = searchEndPos;
      if (end)
        *end = searchStartPos - 1;
      return true;
    }

    if (searchEndPos >= 3) {
      searchStartPos = searchEndPos - 3;
    } else {
      return false;
    }
  }

  return false;
}

void AnonTypenameTracker::createNestedTypesTypename(
    const PDBSymbol &Symbol, const std::string &ParentAnonName) {
  const auto allNestedEnums = Symbol.findAllChildren<PDBSymbolTypeEnum>();
  if (allNestedEnums && allNestedEnums->getChildCount() > 0) {
    while (const auto nestedEnum = allNestedEnums->getNext()) {
      getAnonTypename(*nestedEnum);
    }
  }

  const auto allNestedTypedefs = Symbol.findAllChildren<PDBSymbolTypeTypedef>();
  if (allNestedTypedefs && allNestedTypedefs->getChildCount() > 0) {
    while (const auto nestedTypedef = allNestedTypedefs->getNext()) {
      getAnonTypename(*nestedTypedef);
    }
  }

  const auto allNestedUDTs = Symbol.findAllChildren<PDBSymbolTypeUDT>();
  if (allNestedUDTs && allNestedUDTs->getChildCount() > 0) {
    while (const auto nestedUDT = allNestedUDTs->getNext()) {
      getAnonTypename(*nestedUDT);
    }
  }
}

const std::string &AnonTypenameTracker::createAnonTypename(
    const PDBSymbol &Symbol) {

  const auto symbolName = Symbol.getName();
  size_t anonStart, anonEnd;
  if (!getLastAnonTypenameOccurrence(symbolName, &anonStart, &anonEnd))
    return m_not_anon_name;

  // Cannot resolve if the anon part is not this symbol
  if (anonEnd < symbolName.size() - 1)
    return m_not_anon_name;

  // Cannot resolve more than one anon occurrence
  if (getLastAnonTypenameOccurrence(symbolName, nullptr, nullptr, anonStart))
    return m_not_anon_name;

  std::string anonName;
  if (anonStart != 0)
    anonName =
        symbolName.substr(0, anonStart) + createAnonTypenameFromData(Symbol);
  else
    anonName = createAnonTypenameFromData(Symbol);

  return PreviouslyCreatedTypenames
         .emplace(std::make_pair<uint32_t, std::string>(Symbol.getSymIndexId(),
           std::move(anonName)))
         .first->second;
}

const std::string &AnonTypenameTracker::createAnonTypenameFromNestedParent(
    const PDBSymbol &Symbol, const PDBSymbol &Parent,
    const std::string &ParentAnonName) {
  const std::string nonNestedName =
      Symbol.getName().substr(Parent.getName().size() + 2);

  std::string nestedName;
  size_t nonNestedAnonStart, nonNestedAnonEnd;
  if (getLastAnonTypenameOccurrence(nonNestedName, &nonNestedAnonStart,
                                    &nonNestedAnonEnd)) {
    nestedName = ParentAnonName + "::" + createAnonTypenameFromData(Symbol);
  } else {
    nestedName = ParentAnonName + "::" + nonNestedName;
  }

  return PreviouslyCreatedTypenames
         .emplace(std::make_pair<uint32_t, std::string>(Symbol.getSymIndexId(),
           std::move(nestedName)))
         .first->second;
}

std::string AnonTypenameTracker::createAnonTypenameFromData(
    const PDBSymbol &Symbol) const {
  MD5 md5;
  uint32_t SymbolId = endian::byte_swap(Symbol.getSymIndexId(), little);
  md5.update(ArrayRef<uint8_t>(reinterpret_cast<uint8_t *>(&SymbolId),
                               sizeof(SymbolId)));

  auto children = Symbol.findAllChildren<PDBSymbolData>();
  if (children && children->getChildCount() > 0) {
    while (auto childValue = children->getNext()) {
      uint32_t typeId = endian::byte_swap(childValue->getTypeId(), little);
      md5.update(ArrayRef<uint8_t>(reinterpret_cast<uint8_t *>(&typeId),
                                   sizeof(childValue)));
      md5.update(childValue->getName());
    }
  }

  MD5::MD5Result result{};
  md5.final(result);

  SmallString<32> resultStr;
  MD5::stringifyResult(result, resultStr);

  return "$" + std::string(resultStr.c_str());
}

bool AnonTypenameTracker::hasNestedTypes(const PDBSymbol &Symbol) {
  switch (Symbol.getSymTag()) {
  case PDB_SymType::UDT:
    return static_cast<const PDBSymbolTypeUDT &>(Symbol).hasNestedTypes();

  case PDB_SymType::Enum:
    return static_cast<const PDBSymbolTypeEnum &>(Symbol).hasNestedTypes();

  case PDB_SymType::Typedef:
    return static_cast<const PDBSymbolTypeTypedef &>(Symbol).hasNestedTypes();

  default:
    return false;
  }
}
