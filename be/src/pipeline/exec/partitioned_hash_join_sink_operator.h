// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#pragma once

#include <stdint.h>

#include <atomic>

#include "common/be_mock_util.h"
#include "common/status.h"
#include "operator.h"
#include "pipeline/dependency.h"
#include "pipeline/exec/hashjoin_build_sink.h"
#include "pipeline/exec/hashjoin_probe_operator.h"
#include "pipeline/exec/join_build_sink_operator.h"
#include "pipeline/exec/spill_utils.h"
#include "vec/core/block.h"
#include "vec/runtime/partitioner.h"

namespace doris {
#include "common/compile_check_begin.h"
class RuntimeState;

namespace pipeline {

class PartitionedHashJoinSinkOperatorX;

class PartitionedHashJoinSinkLocalState
        : public PipelineXSpillSinkLocalState<PartitionedHashJoinSharedState> {
public:
    using Parent = PartitionedHashJoinSinkOperatorX;
    ENABLE_FACTORY_CREATOR(PartitionedHashJoinSinkLocalState);
    ~PartitionedHashJoinSinkLocalState() override = default;
    Status init(RuntimeState* state, LocalSinkStateInfo& info) override;
    Status open(RuntimeState* state) override;
    Status close(RuntimeState* state, Status exec_status) override;
    Status revoke_memory(RuntimeState* state, const std::shared_ptr<SpillContext>& spill_context);
    size_t revocable_mem_size(RuntimeState* state) const;
    Status terminate(RuntimeState* state) override;
    [[nodiscard]] size_t get_reserve_mem_size(RuntimeState* state, bool eos);
    void update_memory_usage();
    MOCK_FUNCTION void update_profile_from_inner();

    Dependency* finishdependency() override;

protected:
    PartitionedHashJoinSinkLocalState(DataSinkOperatorXBase* parent, RuntimeState* state)
            : PipelineXSpillSinkLocalState<PartitionedHashJoinSharedState>(parent, state) {}

    Status _spill_to_disk(uint32_t partition_index,
                          const vectorized::SpillStreamSPtr& spilling_stream);

    Status _partition_block(RuntimeState* state, vectorized::Block* in_block, size_t begin,
                            size_t end);

    Status _revoke_unpartitioned_block(RuntimeState* state,
                                       const std::shared_ptr<SpillContext>& spill_context);

    Status _finish_spilling();

    Status _setup_internal_operator(RuntimeState* state);

    friend class PartitionedHashJoinSinkOperatorX;

    bool _child_eos {false};

    std::unique_ptr<vectorized::PartitionerBase> _partitioner;

    std::unique_ptr<RuntimeProfile> _internal_runtime_profile;
    std::shared_ptr<Dependency> _finish_dependency;

    RuntimeProfile::Counter* _partition_timer = nullptr;
    RuntimeProfile::Counter* _partition_shuffle_timer = nullptr;
    RuntimeProfile::Counter* _spill_build_timer = nullptr;
    RuntimeProfile::Counter* _in_mem_rows_counter = nullptr;
    RuntimeProfile::Counter* _memory_usage_reserved = nullptr;
};

class PartitionedHashJoinSinkOperatorX
        : public JoinBuildSinkOperatorX<PartitionedHashJoinSinkLocalState> {
public:
    PartitionedHashJoinSinkOperatorX(ObjectPool* pool, int operator_id, int dest_id,
                                     const TPlanNode& tnode, const DescriptorTbl& descs,
                                     uint32_t partition_count);

    Status init(const TDataSink& tsink) override {
        return Status::InternalError("{} should not init with TDataSink",
                                     PartitionedHashJoinSinkOperatorX::_name);
    }

    Status init(const TPlanNode& tnode, RuntimeState* state) override;

    Status prepare(RuntimeState* state) override;

    Status sink(RuntimeState* state, vectorized::Block* in_block, bool eos) override;

    bool should_dry_run(RuntimeState* state) override { return false; }

    size_t revocable_mem_size(RuntimeState* state) const override;

    Status revoke_memory(RuntimeState* state,
                         const std::shared_ptr<SpillContext>& spill_context) override;

    size_t get_reserve_mem_size(RuntimeState* state, bool eos) override;

    DataDistribution required_data_distribution() const override {
        if (_join_op == TJoinOp::NULL_AWARE_LEFT_ANTI_JOIN) {
            return {ExchangeType::NOOP};
        }

        return _join_distribution == TJoinDistributionType::BUCKET_SHUFFLE ||
                               _join_distribution == TJoinDistributionType::COLOCATE
                       ? DataDistribution(ExchangeType::BUCKET_HASH_SHUFFLE,
                                          _distribution_partition_exprs)
                       : DataDistribution(ExchangeType::HASH_SHUFFLE,
                                          _distribution_partition_exprs);
    }

    bool is_shuffled_operator() const override {
        return _join_distribution == TJoinDistributionType::PARTITIONED;
    }

    void set_inner_operators(const std::shared_ptr<HashJoinBuildSinkOperatorX>& sink_operator,
                             const std::shared_ptr<HashJoinProbeOperatorX>& probe_operator) {
        _inner_sink_operator = sink_operator;
        _inner_probe_operator = probe_operator;
    }

    bool require_data_distribution() const override {
        return _inner_probe_operator->require_data_distribution();
    }

private:
    friend class PartitionedHashJoinSinkLocalState;
#ifdef BE_TEST
    friend class PartitionedHashJoinSinkOperatorTest;
#endif

    const TJoinDistributionType::type _join_distribution;

    std::vector<TExpr> _build_exprs;

    std::shared_ptr<HashJoinBuildSinkOperatorX> _inner_sink_operator;
    std::shared_ptr<HashJoinProbeOperatorX> _inner_probe_operator;

    const std::vector<TExpr> _distribution_partition_exprs;
    const TPlanNode _tnode;
    const DescriptorTbl _descriptor_tbl;
    const uint32_t _partition_count;
    std::unique_ptr<vectorized::PartitionerBase> _partitioner;
};

} // namespace pipeline
#include "common/compile_check_end.h"
} // namespace doris
