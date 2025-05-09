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

#include "vparquet_file_metadata.h"

#include <gen_cpp/parquet_types.h>

#include <sstream>
#include <vector>

#include "runtime/exec_env.h"
#include "runtime/memory/mem_tracker_limiter.h"
#include "schema_desc.h"

namespace doris::vectorized {
#include "common/compile_check_begin.h"

FileMetaData::FileMetaData(tparquet::FileMetaData& metadata, size_t mem_size)
        : _metadata(metadata), _mem_size(mem_size) {
    ExecEnv::GetInstance()->parquet_meta_tracker()->consume(mem_size);
}

FileMetaData::~FileMetaData() {
    ExecEnv::GetInstance()->parquet_meta_tracker()->release(_mem_size);
}

Status FileMetaData::init_schema() {
    if (_metadata.schema[0].num_children <= 0) {
        return Status::Corruption("Invalid parquet schema");
    }
    return _schema.parse_from_thrift(_metadata.schema);
}

const tparquet::FileMetaData& FileMetaData::to_thrift() {
    return _metadata;
}

std::string FileMetaData::debug_string() const {
    std::stringstream out;
    out << "Parquet Metadata(";
    out << "; version=" << _metadata.version;
    out << ")";
    return out.str();
}
#include "common/compile_check_end.h"

} // namespace doris::vectorized
