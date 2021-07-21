//===- AnonTypenameTracker.h ---------------------------------- *- C++ --*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TOOLS_LLVMPDBDUMP_ANONTYPENAMETRACKER_H
#define LLVM_TOOLS_LLVMPDBDUMP_ANONTYPENAMETRACKER_H

#include "llvm/DebugInfo/PDB/PDBSymbol.h"

#include <unordered_map>

namespace llvm {
namespace pdb {

class AnonTypenameTracker {
public:
  AnonTypenameTracker();
  
  const std::string &getAnonTypename(const PDBSymbol &Symbol);
  static bool isAnonSymbolName(const std::string &name);

private:
  static bool getLastAnonTypenameOccurrence(const std::string &name,
                                            size_t *start, size_t *end);
  static bool getLastAnonTypenameOccurrence(const std::string &name,
                                            size_t *start, size_t *end,
                                            size_t beginPos);
  void createNestedTypesTypename(const PDBSymbol &Symbol, const std::string &ParentAnonName);
  const std::string &createAnonTypename(const PDBSymbol &Symbol);
  const std::string &createAnonTypenameFromNestedParent(const PDBSymbol &Symbol, const PDBSymbol &Parent, const std::string &ParentAnonName);
  std::string createAnonTypenameFromData(const PDBSymbol &Symbol) const;

  static bool hasNestedTypes(const PDBSymbol &Symbol);

  std::unordered_map<uint32_t, std::string> PreviouslyCreatedTypenames;
  static std::string m_not_anon_name;
};

} // namespace pdb
} // namespace llvm

#endif // LLVM_TOOLS_LLVMPDBDUMP_ANONTYPENAMETRACKER_H
