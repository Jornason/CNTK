//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.md file in the project root for full license information.
//

#pragma once

#include "SyncAction.h"
#include <unordered_map>
#include <memory>
#include <string>
#include "CUDATimer.h"
#include <utility>
#include <set>

namespace Microsoft { namespace MSR { namespace CNTK {


// forward declarations
class ComputationNodeBase;
class FrameRange;
class SwapInAction;
class SwapOutAction;

class SynchronizationManager
{

private:
    std::unordered_map<int, std::vector<SyncAction*> > m_stepNumber2Actions;
    // singleton constructor
    SynchronizationManager(){};

    static SynchronizationManager* s_synchronizationManager;
    bool m_isExecuting;
    CUDATimer m_timer;
    int m_currentStepNumber;

    // needed to identify a step
    std::unordered_map<std::string, int> m_stepName2StepNumber; 
    // steps to buffers; all these buffers need to be handled during one synchronization call for a given timestep
    // needed in order to determine dependencies
    std::unordered_map<Matrix<float>*, std::vector<int> > m_buffer2StepNumbers; 
    std::unordered_map<int, float> m_stepNumber2ComputationTime; 

    //these are for managing full memory swapping during the dryrun
    std::unordered_map<Matrix<float>*, SwapInAction*> m_buffer2SwapIn;
    std::unordered_map<Matrix<float>*, SwapOutAction*> m_buffer2SwapOut;
    std::unordered_map<Matrix<float>*, bool> m_buffer2IsFreed;
    std::unordered_map<int, std::vector<Matrix<float>*> > m_stepNumber2Buffer; 

    std::unordered_map<Matrix<float>*, std::pair<float, float> > m_buffer2SwapTime;
    std::set<Matrix<float> *> m_bufferSet; // contains all buffers which have the potential to be swapped (that is they are non-sharable)
    std::unordered_map<int, std::pair<float,float> > m_stepNumber2CumulativeSwapInTime;


    void FreeBuffersForDryRun(ComputationNodeBase *node, bool isForward);
    void SwapInFreedBuffers(ComputationNodeBase *node, bool isForward);
    void RegisterBuffers(ComputationNodeBase *node, bool isForward);
    void GatherRuntimeStatistics(ComputationNodeBase *node, const size_t idx, const FrameRange& fr, bool isForward);
    void FindSwapOrder();
    void CleanUp();

    void MeasureSwapTime(ComputationNodeBase *node, int stepNumber);
    std::string GetStepName(ComputationNodeBase *node, bool isForward);

public:
    // we use a singleton here; we could also injected the manager during node creation, but
    // sometimes this also makes sure that there is only a single instance available
    // which is quite handy for such a critical resource as memory
    static SynchronizationManager* GetSynchronizationManager();

    ~SynchronizationManager(){};
    // this is called BEFORE a ForwardProp / BackpropTo method call
    void BeginSynchronizeState(ComputationNodeBase *node, const size_t idx, const FrameRange& fr, bool isForward);
    // this is called AFTER a ForwardProp / BackpropTo method call
    void EndSynchronizeState(ComputationNodeBase *node, const size_t idx, const FrameRange& fr, bool isForward);
    // indicates the current state of the manager: 
    // (1) gather stats and determine swap order, or 
    // (2) active usage of swapping (=Executing==true)
    bool IsExecuting(){ return m_isExecuting; }
    // the config sets this to false by default
    bool m_useMemorySwapping;
    // this cleans the SynchronizationManager up after a action completes
    void ClearActionsAndTheirMemory();
	
};


}}}
