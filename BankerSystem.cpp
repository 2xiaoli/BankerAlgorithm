#include "BankerSystem.h"

#include <iomanip>
#include <numeric>
#include <sstream>

bool BankerSystem::isInitialized() const {
    return initialized_;
}

void BankerSystem::reset() {
    initialized_ = false;
    available_.clear();
    max_.clear();
    allocation_.clear();
    need_.clear();
}

OperationResult BankerSystem::loadDemo() {
    const ResourceVector totalResources{10, 5, 7};
    const ResourceMatrix max{
        {7, 5, 3},
        {3, 2, 2},
        {9, 0, 2},
        {2, 2, 2},
        {4, 3, 3}
    };
    const ResourceMatrix allocation{
        {0, 1, 0},
        {2, 0, 0},
        {3, 0, 2},
        {2, 1, 1},
        {0, 0, 2}
    };

    auto result = initialize(totalResources, max, allocation);
    if (result.success) {
        result.message = "已载入经典样例数据。当前系统处于安全状态。";
    }
    return result;
}

OperationResult BankerSystem::initialize(const ResourceVector& totalResources,
                                         const ResourceMatrix& max,
                                         const ResourceMatrix& allocation) {
    const auto processCount = static_cast<int>(max.size());
    const auto resourceCount = static_cast<int>(totalResources.size());

    if (processCount < Limits::MinProcessCount || processCount > Limits::MaxProcessCount) {
        return {false, "进程数量必须在 1 到 20 之间。", {}};
    }
    if (resourceCount < Limits::MinResourceCount || resourceCount > Limits::MaxResourceCount) {
        return {false, "资源类型数量必须在 1 到 10 之间。", {}};
    }
    if (allocation.size() != max.size()) {
        return {false, "Allocation 矩阵行数必须与 Max 矩阵一致。", {}};
    }

    for (int j = 0; j < resourceCount; ++j) {
        if (totalResources[j] < 0 || totalResources[j] > Limits::MaxResourceValue) {
            return {false, "总资源向量中存在非法资源值。", {}};
        }
    }

    for (int i = 0; i < processCount; ++i) {
        if (static_cast<int>(max[i].size()) != resourceCount ||
            static_cast<int>(allocation[i].size()) != resourceCount) {
            return {false, "Max 和 Allocation 的每一行都必须包含固定数量的资源值。", {}};
        }
        for (int j = 0; j < resourceCount; ++j) {
            if (max[i][j] < 0 || allocation[i][j] < 0 ||
                max[i][j] > Limits::MaxResourceValue ||
                allocation[i][j] > Limits::MaxResourceValue) {
                return {false, "矩阵中存在非法资源值，资源数必须为非负整数。", {}};
            }
            if (allocation[i][j] > max[i][j]) {
                std::ostringstream message;
                message << "初始化失败：P" << i << " 的 Allocation[R" << j
                        << "] 不能大于 Max[R" << j << "]。";
                return {false, message.str(), {}};
            }
        }
    }

    ResourceVector allocatedSum(resourceCount, 0);
    for (const auto& row : allocation) {
        for (int j = 0; j < resourceCount; ++j) {
            allocatedSum[j] += row[j];
        }
    }
    for (int j = 0; j < resourceCount; ++j) {
        if (allocatedSum[j] > totalResources[j]) {
            std::ostringstream message;
            message << "初始化失败：R" << j << " 已分配资源总数大于系统总资源。";
            return {false, message.str(), {}};
        }
    }

    available_.assign(resourceCount, 0);
    for (int j = 0; j < resourceCount; ++j) {
        available_[j] = totalResources[j] - allocatedSum[j];
    }
    max_ = max;
    allocation_ = allocation;
    recomputeNeed();
    initialized_ = true;

    auto safety = checkSafety();
    if (safety.success) {
        safety.message = "初始化成功，当前系统安全。";
        return safety;
    }
    safety.success = true;
    safety.message = "初始化成功，但当前系统不安全，建议调整初始数据。";
    return safety;
}

OperationResult BankerSystem::checkSafety() const {
    if (!initialized_) {
        return {false, "系统尚未初始化，请先使用 demo 或 new 命令。", {}};
    }

    const int n = static_cast<int>(allocation_.size());
    const int m = static_cast<int>(available_.size());
    ResourceVector work = available_;
    std::vector<bool> finish(n, false);
    std::vector<int> safeSequence;

    bool progressed = true;
    while (progressed) {
        progressed = false;
        for (int i = 0; i < n; ++i) {
            if (finish[i] || !lessOrEqual(need_[i], work)) {
                continue;
            }

            for (int j = 0; j < m; ++j) {
                work[j] += allocation_[i][j];
            }
            finish[i] = true;
            safeSequence.push_back(i);
            progressed = true;
        }
    }

    for (int i = 0; i < n; ++i) {
        if (!finish[i]) {
            std::ostringstream message;
            message << "安全性检查失败：找不到完整安全序列，系统处于不安全状态。";
            return {false, message.str(), safeSequence};
        }
    }

    std::ostringstream message;
    message << "安全性检查通过，安全序列为 " << safeSequenceToString(safeSequence) << "。";
    return {true, message.str(), safeSequence};
}

OperationResult BankerSystem::requestResources(int pid, const ResourceVector& request) {
    if (!initialized_) {
        return {false, "系统尚未初始化，请先使用 demo 或 new 命令。", {}};
    }
    if (pid < 0 || pid >= static_cast<int>(processCount())) {
        return {false, "进程编号越界，请使用 0 到 n-1 之间的编号。", {}};
    }

    auto validation = validateVectorForCurrentSystem(request, "Request");
    if (!validation.success) {
        return validation;
    }
    if (!lessOrEqual(request, need_[pid])) {
        return {false, "请求失败：Request 不能大于该进程的 Need。", {}};
    }
    if (!lessOrEqual(request, available_)) {
        return {false, "请求失败：Request 大于 Available，当前可用资源不足。", {}};
    }

    for (std::size_t j = 0; j < request.size(); ++j) {
        available_[j] -= request[j];
        allocation_[pid][j] += request[j];
        need_[pid][j] -= request[j];
    }

    auto safety = checkSafety();
    if (!safety.success) {
        for (std::size_t j = 0; j < request.size(); ++j) {
            available_[j] += request[j];
            allocation_[pid][j] -= request[j];
            need_[pid][j] += request[j];
        }
        safety.message = "请求会使系统进入不安全状态，已拒绝并回滚。";
        return safety;
    }

    safety.message = "资源请求成功，系统仍处于安全状态。";
    return safety;
}

OperationResult BankerSystem::releaseResources(int pid, const ResourceVector& release) {
    if (!initialized_) {
        return {false, "系统尚未初始化，请先使用 demo 或 new 命令。", {}};
    }
    if (pid < 0 || pid >= static_cast<int>(processCount())) {
        return {false, "进程编号越界，请使用 0 到 n-1 之间的编号。", {}};
    }

    auto validation = validateVectorForCurrentSystem(release, "Release");
    if (!validation.success) {
        return validation;
    }
    if (!lessOrEqual(release, allocation_[pid])) {
        return {false, "释放失败：Release 不能大于该进程已分配的 Allocation。", {}};
    }

    for (std::size_t j = 0; j < release.size(); ++j) {
        allocation_[pid][j] -= release[j];
        available_[j] += release[j];
        need_[pid][j] += release[j];
    }

    auto safety = checkSafety();
    if (safety.success) {
        safety.message = "资源释放成功，当前系统处于安全状态。";
    } else {
        safety.message = "资源释放成功，但当前系统不安全，请继续释放资源或调整状态。";
        safety.success = true;
    }
    return safety;
}

std::string BankerSystem::showState() const {
    if (!initialized_) {
        return "系统尚未初始化，请先使用 demo 或 new 命令。\n";
    }

    std::ostringstream out;
    const int n = static_cast<int>(processCount());
    const int m = static_cast<int>(resourceCount());

    out << "\n当前系统状态\n";
    out << "进程数量: " << n << "，资源类型数量: " << m << "\n";
    out << "Available: " << vectorToString(available_) << "\n\n";

    out << std::left << std::setw(8) << "进程";
    for (int j = 0; j < m; ++j) {
        out << std::setw(8) << ("Max.R" + std::to_string(j));
    }
    for (int j = 0; j < m; ++j) {
        out << std::setw(8) << ("All.R" + std::to_string(j));
    }
    for (int j = 0; j < m; ++j) {
        out << std::setw(8) << ("Need.R" + std::to_string(j));
    }
    out << "\n";

    for (int i = 0; i < n; ++i) {
        out << std::left << std::setw(8) << ("P" + std::to_string(i));
        for (int value : max_[i]) {
            out << std::setw(8) << value;
        }
        for (int value : allocation_[i]) {
            out << std::setw(8) << value;
        }
        for (int value : need_[i]) {
            out << std::setw(8) << value;
        }
        out << "\n";
    }
    out << "\n";
    return out.str();
}

SystemState BankerSystem::state() const {
    return {initialized_, available_, max_, allocation_, need_};
}

std::size_t BankerSystem::processCount() const {
    return allocation_.size();
}

std::size_t BankerSystem::resourceCount() const {
    return available_.size();
}

void BankerSystem::recomputeNeed() {
    need_ = max_;
    for (std::size_t i = 0; i < max_.size(); ++i) {
        for (std::size_t j = 0; j < max_[i].size(); ++j) {
            need_[i][j] = max_[i][j] - allocation_[i][j];
        }
    }
}

OperationResult BankerSystem::validateVectorForCurrentSystem(const ResourceVector& values,
                                                             const std::string& name) const {
    if (values.size() != resourceCount()) {
        std::ostringstream message;
        message << name << " 向量长度错误，应输入 " << resourceCount() << " 个资源值。";
        return {false, message.str(), {}};
    }
    for (int value : values) {
        if (value < 0 || value > Limits::MaxResourceValue) {
            return {false, name + " 向量中存在非法资源值。", {}};
        }
    }
    return {true, "", {}};
}

bool BankerSystem::lessOrEqual(const ResourceVector& left, const ResourceVector& right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (left[i] > right[i]) {
            return false;
        }
    }
    return true;
}

std::string BankerSystem::vectorToString(const ResourceVector& values) {
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << values[i];
    }
    out << "]";
    return out.str();
}

std::string BankerSystem::safeSequenceToString(const std::vector<int>& sequence) {
    std::ostringstream out;
    out << "<";
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        if (i > 0) {
            out << " -> ";
        }
        out << "P" << sequence[i];
    }
    out << ">";
    return out.str();
}
