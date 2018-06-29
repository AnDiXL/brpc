// Copyright (c) 2014 Baidu, Inc.G
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Authors: Lei He (helei@qiyi.com)

#ifndef BRPC_POLICY_GRANDIENT_CONCURRENCY_LIMITER_H
#define BRPC_POLICY_GRANDIENT_CONCURRENCY_LIMITER_H

#include "brpc/concurrency_limiter.h"
#include "bvar/bvar.h"

namespace brpc {
namespace policy {

class GradientConcurrencyLimiter : public ConcurrencyLimiter {
public:
    GradientConcurrencyLimiter();
    ~GradientConcurrencyLimiter() {}
    bool OnRequested() override;
    void OnResponded(int error_code, int64_t latency_us) override;
    int MaxConcurrency() const override;
    
    // For compatibility with the MaxConcurrencyOf() interface. When using 
    // an automatic concurrency adjustment strategy, you should not manually 
    // adjust the maximum concurrency. In spite of this, calling this 
    // interface is safe and it can normally return the current maximum 
    // concurrency. But your changes to the maximum concurrency will not take
    // effect.
    int& MaxConcurrency() override;

    int Expose(const butil::StringPiece& prefix) override;
    GradientConcurrencyLimiter* New() const override;
    void Destroy() override;
    void Describe(std::ostream&, const DescribeOptions& options) override;

private:
    struct SampleWindow {
        SampleWindow() 
            : start_time_us(0)
            , succ_count(0)
            , failed_count(0)
            , total_failed_us(0)
            , total_succ_us(0) {}
        int64_t start_time_us;
        int32_t succ_count;
        int32_t failed_count;
        int64_t total_failed_us;
        int64_t total_succ_us;
    };

    struct WindowSnap {
        WindowSnap(int64_t latency_us, int32_t concurrency)
            : avg_latency_us(latency_us)
            , actuall_concurrency(concurrency) {}
        int64_t avg_latency_us;
        int32_t actuall_concurrency;
    };

    void AddSample(int error_code, int64_t latency_us, int64_t sampling_time_us);

    //NOT thread-safe, should be called in AddSample()
    void UpdateConcurrency();
    void ResetSampleWindow(int64_t sampling_time_us);
    
    SampleWindow _sw;
    std::vector<WindowSnap> _ws_queue;
    uint32_t _ws_index;
    int32_t _unused_max_concurrency;
    butil::Mutex _sw_mutex;
    bvar::PassiveStatus<int32_t> _max_concurrency_bvar;
    butil::atomic<int64_t> BAIDU_CACHELINE_ALIGNMENT _last_sampling_time_us;
    butil::atomic<int32_t> BAIDU_CACHELINE_ALIGNMENT _max_concurrency;
    butil::atomic<int32_t> BAIDU_CACHELINE_ALIGNMENT _current_concurrency;
};

}  // namespace policy
}  // namespace brpc


#endif // BRPC_POLICY_GRANDIENT_CONCURRENCY_LIMITER_H
