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

  // Do the work of marking referenced types.
  const std::string &getAnonTypename(const PDBSymbol &Symbol);

  static bool isAnonSymbolName(const std::string &name);

private:
  const std::string &createAnonTypename(const PDBSymbol &Symbol);

  std::unordered_map<uint32_t, std::string> PreviouslyCreatedTypenames;
};

} // namespace pdb
} // namespace llvm

#endif // LLVM_TOOLS_LLVMPDBDUMP_ANONTYPENAMETRACKER_H
