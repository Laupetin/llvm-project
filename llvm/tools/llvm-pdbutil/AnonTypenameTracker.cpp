//===- AnonTypenameTracker.cpp -------------------------------- *- C++ --*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "AnonTypenameTracker.h"

#include "llvm/DebugInfo/PDB/PDBSymbolData.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/MD5.h"

using namespace llvm;
using namespace llvm::pdb;
using namespace llvm::support;

AnonTypenameTracker::AnonTypenameTracker() = default;

const std::string &AnonTypenameTracker::getAnonTypename(
    const PDBSymbol &Symbol) {

  const auto previousName = PreviouslyCreatedTypenames.find(Symbol.getSymIndexId());

  if (previousName != PreviouslyCreatedTypenames.end()) {
    return previousName->second;
  }

  return createAnonTypename(Symbol);
}

bool AnonTypenameTracker::isAnonSymbolName(const std::string &name) {

  return name.size() >= 2 && name[0] == '<' && name[name.size() - 1] == '>';
}

const std::string & AnonTypenameTracker::createAnonTypename(
    const PDBSymbol &Symbol) {
  MD5 md5;
  uint32_t SymbolId = endian::byte_swap(Symbol.getSymIndexId(), little);
  md5.update(ArrayRef<uint8_t>(reinterpret_cast<uint8_t *>(&SymbolId), sizeof(SymbolId)));

  auto Children = Symbol.findAllChildren<PDBSymbolData>();
  if (Children && Children->getChildCount() > 0) {
    while (auto ChildValue = Children->getNext()) {
      uint32_t TypeId = endian::byte_swap(ChildValue->getTypeId(), little);
      md5.update(ArrayRef<uint8_t>(reinterpret_cast<uint8_t*>(&TypeId), sizeof(ChildValue)));
      md5.update(ChildValue->getName());
    }
  }

  MD5::MD5Result result{};
  md5.final(result);

  SmallString<32> resultStr;
  MD5::stringifyResult(result, resultStr);
  
  return PreviouslyCreatedTypenames.emplace(
      std::make_pair<uint32_t, std::string>(
          Symbol.getSymIndexId(), "$" + std::string(resultStr.c_str()))).first->second;
}
