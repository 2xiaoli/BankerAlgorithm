#pragma once

#include "Types.h"

#include <cstddef>
#include <string>

class BankerSystem {
public:
    bool isInitialized() const;
    void reset();

    OperationResult loadDemo();
    OperationResult initialize(const ResourceVector& totalResources,
                               const ResourceMatrix& max,
                               const ResourceMatrix& allocation);

    OperationResult checkSafety() const;
    OperationResult requestResources(int pid, const ResourceVector& request);
    OperationResult releaseResources(int pid, const ResourceVector& release);

    std::string showState() const;
    SystemState state() const;
    std::size_t processCount() const;
    std::size_t resourceCount() const;

private:
    bool initialized_ = false;
    ResourceVector available_;
    ResourceMatrix max_;
    ResourceMatrix allocation_;
    ResourceMatrix need_;

    void recomputeNeed();
    OperationResult validateVectorForCurrentSystem(const ResourceVector& values,
                                                   const std::string& name) const;

    static bool lessOrEqual(const ResourceVector& left, const ResourceVector& right);
    static std::string vectorToString(const ResourceVector& values);
    static std::string safeSequenceToString(const std::vector<int>& sequence);
};
