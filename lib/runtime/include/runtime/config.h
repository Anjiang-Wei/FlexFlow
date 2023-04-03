/* Copyright 2023 CMU, Facebook, LANL, MIT, NVIDIA, and Stanford (alphabetical)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _FLEXFLOW_CONFIG_H_
#define _FLEXFLOW_CONFIG_H_
#include "op-attrs/ffconst.h"
#include "legion.h"
#include <cstring>
#include "utils/optional.h"

namespace FlexFlow {

// ========================================================
// Define Runtime Constants
// ========================================================
// Pre-assigned const flags
#define MAP_TO_FB_MEMORY 0xABCD0000
#define MAP_TO_ZC_MEMORY 0xABCE0000

#ifdef FF_USE_NCCL
constexpr ParameterSyncType CHOSEN_SYNC_TYPE = ParameterSyncType::NCCL;
#else
constexpr ParameterSyncType CHOSEN_SYNC_TYPE = ParameterSyncType::PS;
#endif

struct FFInitInfo {
  size_t workSpaceSize;
  bool allowTensorOpMathConversion;
};

struct LegionConfig {
  LegionConfig();

  Legion::Context lg_ctx;
  Legion::Runtime *lg_hlr;
  Legion::FieldSpace field_space;
};

class FFConfig {
public:
  enum PreservedIDs {
    InvalidID = 0,
    DataParallelism_GPU = 1,
    // DataParallelism_GPU_2D = 2,
    // DataParallelism_GPU_3D = 3,
    // DataParallelism_GPU_4D = 4,
    // DataParallelism_GPU_5D = 5,
    DataParallelism_CPU = 11,
    // DataParallelism_CPU_2D = 12,
    // DataParallelism_CPU_3D = 13,
    // DataParallelism_CPU_4D = 14,
    // DataParallelism_CPU_5D = 15,
  };

  FFConfig();
  static Legion::MappingTagID get_hash_id(std::string const &pcname);
public:
  int epochs = 1;
  int batchSize = 64;
  int numNodes = 1;
  int cpusPerNode = 0;
  int workersPerNode = 0;
  float learningRate = 0.01f;
  float weightDecay = 0.0001f;
  size_t workSpaceSize = (size_t)1 * 1024 * 1024 * 1024; // 2GB
  bool profiling = false; 
  LegionConfig legion_config;
  bool perform_fusion = false;
  size_t simulator_work_space_size = (size_t)2 * 1024 * 1024 * 1024; // 2GB
  size_t search_budget = -1;
  float search_alpha = 1.2f;
  bool search_overlap_backward_update = false;
  CompMode computationMode = COMP_MODE_TRAINING;
  // Control parallelizable dimensions
  bool only_data_parallel = false;
  bool enable_parameter_parallel = false;
  bool enable_inplace_optimizations = false;
  // Control Tensor Op Math Conversion
  bool allow_tensor_op_math_conversion = false;
  optional<std::string> dataset_path = nullopt;
  optional<std::string> export_strategy_computation_graph_file = nullopt;
  bool include_costs_dot_graph = false;
  optional<std::string> substitution_json_path = nullopt;
  int machine_model_version = 0;
  optional<std::string> machine_model_file = nullopt;
  int simulator_segment_size = 16777216; // 16 MB
  int simulator_max_num_segments = 1;
  optional<int> search_num_nodes = nullopt;
  optional<int> search_num_workers = nullopt;
  int base_optimize_threshold = 10;
  bool enable_control_replication = true;
  // The default python data loader type is 2 to enable control replication
  int python_data_loader_type = 2;
};

class FFIterationConfig {
public:
  FFIterationConfig();
  void reset();
  int seq_length;
};

enum FieldIDs {
  FID_DATA,
};

}

#endif 
