#pragma once

#include <llvm/Pass.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/ADT/StringRef.h>
#include "llvm/ADT/BitVector.h"
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/IR/CFG.h>

using namespace llvm;

std::string getShortValueName(Value *v);

bool isPointerToPointer(Type *T);