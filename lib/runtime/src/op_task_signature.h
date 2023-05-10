#ifndef _FLEXFLOW_RUNTIME_SRC_OP_TASK_SIGNATURE_H
#define _FLEXFLOW_RUNTIME_SRC_OP_TASK_SIGNATURE_H

#include "utils/visitable.h"
#include "task_signature.h"

namespace FlexFlow {

enum class TensorRole {
  INPUT,
  WEIGHT,
  OUTPUT,
};

enum class OpTaskType {
  INIT,
  FWD,
  BWD
};

struct OpTensorSlotSpec : public use_visitable_cmp<OpTensorSlotSpec> {
public:
  OpTensorSlotSpec() = delete;
  OpTensorSlotSpec(slot_id, SlotType, TensorRole);

public:
  slot_id name;
  SlotType slot_type;
  TensorRole tensor_role;
};

struct OpTaskSignature {
  OpTaskSignature() = delete;
  explicit OpTaskSignature(OpTaskType);

  OpTaskType get_task_type() const;

  void add_input_slot(slot_id, SlotType slot_type = SlotType::TENSOR);
  void add_output_slot(slot_id, SlotType slot_type = SlotType::TENSOR);
  void add_weight_slot(slot_id, SlotType slot_type = SlotType::TENSOR);

  /* void add_input_slot(slot_id, Legion::PrivilegeMode); */
  /* void add_input_slot(slot_id, SlotType, Legion::PrivilegeMode); */

  bool operator==(OpTaskSignature const &) const;
  bool operator!=(OpTaskSignature const &) const;

  template <typename T>
  void add_arg_slot(slot_id name) {
    static_assert(is_serializable<T>, "Type must be serializable");
  }

private:
  std::unordered_map<slot_id, std::type_index> task_arg_types;
  std::unordered_map<slot_id, TensorRole> slots;
};

template <task_id_t> OpTaskSignature get_signature();

template <typename F>
void register_task(task_id_t, std::string const &name, OpTaskSignature const &, F const &func);

template <typename F>
void register_task(task_id_t, std::string const &name, OpTaskSignature const &, F const &func, F const &cpu_func);

}

#endif
