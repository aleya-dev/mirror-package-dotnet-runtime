// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#pragma once
#pragma warning(disable : 4503) // 'identifier' : decorated name length exceeded, name was truncated

#include "compiler.h"
#include "ssarenamestate.h"

typedef int LclVarNum;

// Pair of a local var name eg: V01 and Ssa number; eg: V01_01
typedef std::pair<LclVarNum, int> SsaVarName;

class SsaBuilder
{
private:
    inline void EndPhase(Phases phase)
    {
        m_pCompiler->EndPhase(phase);
    }

public:
    // Constructor
    SsaBuilder(Compiler* pCompiler);

    // Requires stmt nodes to be already sequenced in evaluation order. Analyzes the graph
    // for introduction of phi-nodes as GT_PHI tree nodes at the beginning of each block.
    // Each GT_LCL_VAR is given its ssa number through its GetSsaNum() field in the node.
    // Each GT_PHI node will be under a STORE_LCL_VAR node as the store's value operand.
    // The inputs to the PHI are represented as a linked list of GT_PHI_ARG nodes. Each
    // use or def is denoted by the corresponding local nodes. All defs of a particular
    // variable are stored in the "per SSA data" on the local descriptor.
    void Build();

private:
    // Ensures that the basic block graph has a root for the dominator graph, by ensuring
    // that there is a first block that is not in a try region (adding an empty block for that purpose
    // if necessary).  Eventually should move to Compiler.
    void SetupBBRoot();

    // Requires "postOrder" to be an array of size "count". Requires "count" to at least
    // be the size of the flow graph. Sorts the current compiler's flow-graph and places
    // the blocks in post order (i.e., a node's children first) in the array. Returns the
    // number of nodes visited while sorting the graph. In other words, valid entries in
    // the output array.
    int TopologicalSort(BasicBlock** postOrder, int count);

    // Requires "postOrder" to hold the blocks of the flowgraph in topologically sorted
    // order. Requires count to be the valid entries in the "postOrder" array. Computes
    // each block's immediate dominator and records it in the BasicBlock in bbIDom.
    void ComputeImmediateDom(BasicBlock** postOrder, int count);

    // Compute flow graph dominance frontiers.
    void ComputeDominanceFrontiers(BasicBlock** postOrder, int count, BlkToBlkVectorMap* mapDF);

    // Compute the iterated dominance frontier for the specified block.
    void ComputeIteratedDominanceFrontier(BasicBlock* b, const BlkToBlkVectorMap* mapDF, BlkVector* bIDF);

    // Insert a new GT_PHI statement.
    void InsertPhi(BasicBlock* block, unsigned lclNum);

    // Add a new GT_PHI_ARG node to an existing GT_PHI node
    void AddPhiArg(
        BasicBlock* block, Statement* stmt, GenTreePhi* phi, unsigned lclNum, unsigned ssaNum, BasicBlock* pred);

    // Requires "postOrder" to hold the blocks of the flowgraph in topologically sorted order. Requires
    // count to be the valid entries in the "postOrder" array. Inserts GT_PHI nodes at the beginning
    // of basic blocks that require them.
    void InsertPhiFunctions(BasicBlock** postOrder, int count);

    // Rename all definitions and uses within the compiled method.
    void RenameVariables();
    // Rename all definitions and uses within a block.
    void BlockRenameVariables(BasicBlock* block);
    // Rename a local or memory definition generated by a local store/GT_CALL node.
    void RenameDef(GenTree* defNode, BasicBlock* block);
    unsigned RenamePushDef(GenTree* defNode, BasicBlock* block, unsigned lclNum, bool isFullDef);
    // Rename a use of a local variable.
    void RenameLclUse(GenTreeLclVarCommon* lclNode, BasicBlock* block);

    // Assumes that "block" contains a definition for local var "lclNum", with SSA number "ssaNum".
    // IF "block" is within one or more try blocks,
    // and the local variable is live at the start of the corresponding handlers,
    // add this SSA number "ssaNum" to the argument list of the phi for the variable in the start
    // block of those handlers.
    void AddDefToHandlerPhis(BasicBlock* block, unsigned lclNum, unsigned ssaNum);

    // Same as above, for memory.
    void AddMemoryDefToHandlerPhis(MemoryKind memoryKind, BasicBlock* block, unsigned ssaNum);

    // Add GT_PHI_ARG nodes to the GT_PHI nodes within block's successors.
    void AddPhiArgsToSuccessors(BasicBlock* block);

private:
    Compiler*     m_pCompiler;
    CompAllocator m_allocator;

    // Bit vector used by TopologicalSort and ComputeImmediateDom to track already visited blocks.
    BitVecTraits m_visitedTraits;
    BitVec       m_visited;

    SsaRenameState m_renameStack;
};
