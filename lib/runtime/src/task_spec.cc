#include "task_spec.h"
#include "operator.h"
#include "utils/containers.h"
#include "variant.h"

using namespace Legion;

namespace FlexFlow {

Legion::PrivilegeMode get_default_fwd_privs(TensorRole tensor_role, IsGrad is_grad) {
  assert (is_grad == IsGrad::NO); 
  
  if (tensor_role == TensorRole::INPUT || tensor_role == TensorRole::PARAM) {
    return READ_ONLY;
  } else {
    return WRITE_ONLY;
  }
}

Legion::PrivilegeMode get_default_bwd_privs(TensorRole tensor_role, IsGrad is_grad) {
  if (tensor_role == TensorRole::INPUT && is_grad == IsGrad::YES) {
    return READ_WRITE;
  } else if (tensor_role == TensorRole::INPUT && is_grad == IsGrad::NO) {
    return READ_ONLY;
  } else if (tensor_role == TensorRole::PARAM && is_grad == IsGrad::YES) {

  }
}

Legion::PrivilegeMode TensorSlotSpec::get_privileges(OpTaskType task_type) const {
  if (this->mode.has_value()) {
    return this->mode.value();
  }

  switch (task_type) {
    case FWD:
  };
}

TensorSlotSpec get_backward_slot(TensorSlotSpec const &forward_slot) {
  assert (forward_slot.is_grad == IsGrad::NO);
  return {
    forward_slot.name,
    forward_slot.slot_type,
    forward_slot.tensor_role,
    IsGrad::NO
  };
}

TensorSlotSpec get_backward_grad_slot(TensorSlotSpec const &forward_slot) {
  return {
    forward_slot.name,
    forward_slot.slot_type,
    forward_slot.tensor_role,
    IsGrad::YES
  };
}

OpTaskSignature infer_bwd_signature(OpTaskSignature const &fwd) {
  OpTaskSignature bwd(OpTaskType::BWD);

  for (SlotSpec const &slot : fwd.get_slots()) {
    if (is_tensor_slot(slot)) {
      TensorSlotSpec tensor_slot = get_tensor_slot(slot);
      assert (tensor_slot.is_grad == IsGrad::NO);

      TensorSlotSpec grad_slot = tensor_slot;
      grad_slot.is_grad = IsGrad::YES;

      bwd.add_slot(tensor_slot);
      bwd.add_slot(grad_slot);
    } else if (is_arg_slot(slot)) {
      ArgSlotSpec arg_slot = get_arg_slot(slot);

      bwd.add_slot(arg_slot);
    }
  }
}

OpTaskBinding infer_bwd_binding(OpTaskBinding const &fwd) {
  OpTaskBinding bwd;

  for (auto const &kv : fwd.get_bindings()) {
    bwd.bind(kv.first, kv.second);
    bwd.bind_grad(kv.first, kv.second.grad());
  }
}

TensorSpec input_tensor(Op const *op, int idx) {
  return {
    TensorRole::INPUT,
    idx, 
    op->trainableInputs.at(idx),
    IsGrad::NO,
  };
}

TensorSpec output_tensor(Op const *op, int idx) {
  return {
    TensorRole::OUTPUT,
    idx,
    IsTrainable::YES,
    IsGrad::NO
  };
}

TensorSpec param_tensor(Op const *op, int idx) {
  return {
    TensorRole::PARAM,
    idx,
    IsTrainable::YES,
    IsGrad::NO
  };
}

using TensorSpecCollection = std::unordered_map<slot_id, variant<TensorSpec, std::vector<TensorSpec>>>;

static TensorSpecCollection
collect_tensor_specs(OpTaskSignature const &signature, OpTaskBinding const &binding) {
  std::unordered_map<slot_id, std::vector<TensorSpec>> intermediate;
  for (auto const &kv : binding.get_bindings()) {
    slot_id slot = kv.first;
    TensorSpec tensor_spec = kv.second;
    SlotSpec slot_spec = signature.get_slot(slot); 

    if (is_tensor_slot(slot_spec)) {
      intermediate[slot].push_back(tensor_spec);
    }
  }

  // handle co- and contra-variance
  auto privs_satisfies_slot = [&](TensorSlotSpec const &slot, Legion::PrivilegeMode m2) -> bool {
    Legion::PrivilegeMode slot_privs = slot.get_privileges(signature.get_task_type()); 
    if (slot.tensor_role == TensorRole::INPUT && slot.is_grad == IsGrad::NO ||
        slot.tensor_role == TensorRole::PARAM && slot.is_grad == IsGrad::NO || 
        slot.tensor_role == TensorRole::OUTPUT && slot.is_grad == IsGrad::YES) {
      return (slot_privs | m2) == slot_privs;
    } else {
      return (m2 | slot_privs) == m2;
    }
  };

  TensorSpecCollection result;
  for (slot_id slot : keys(binding.get_tensor_bindings())) {
    SlotSpec slot_spec = signature.get_slot(slot); 
    std::vector<TensorSpec> tensors = intermediate.at(slot);

    assert (is_tensor_slot(slot_spec));

    TensorSlotSpec tensor_slot = get_tensor_slot(slot_spec);
    for (TensorSpec const &tensor : tensors) {
      assert (privs_satisfies_slot(tensor_slot, tensor.get_privileges()));
    }

    if (tensor_slot.slot_type == SlotType::TENSOR) {
      result[slot] = get_only(tensors);
    } else {
      assert (tensor_slot.slot_type == SlotType::VARIADIC);
      result[slot] = tensors;
    }
  }
}

static std::unordered_map<slot_id, TensorArgumentFormat>
allocate_region_idxs(OpTaskSignature const &signature, OpTaskBinding const &binding) {
  auto collected = collect_tensor_specs(signature, binding);
  std::unordered_map<slot_id, TensorArgumentFormat> result;

  int region_idx = 0;
  for (auto const &kv : collected) {
    slot_id slot = kv.first;
    if (holds_alternative<TensorSpec>(kv.second)) {
      auto tensor_spec = get<TensorSpec>(kv.second);

      result[slot] = std::pair<int, TensorSpec>{
        region_idx,
        tensor_spec
      };
      region_idx++;
    } else {
      auto tensor_specs = get<std::vector<TensorSpec>>(kv.second);
      std::vector<std::pair<int, TensorSpec>> region_idx_mappings;

      for (TensorSpec const &tensor_spec : tensor_specs) {
        region_idx_mappings.push_back({
          region_idx,
          tensor_spec
        });
        region_idx++;
      }

      result[slot] = region_idx_mappings;
    }
  }

  return result;
}

static std::unordered_map<slot_id, ArgSpec>
allocate_argument_offsets(OpTaskSignature const &signature, OpTaskBinding const &binding) {
  static_assert(is_trivially_serializable<OpTaskArgumentFormat>, "OpTaskArgumentFormat must be trivially serializable for this to work");

  for (auto const &kv : binding.get_arg_bindings()) {
    slot_id slot = kv.first;
    ArgSpec const &arg = kv.second;

    SlotSpec slot_spec = signature.get_slot(slot);
    assert (is_arg_slot(slot_spec));
    ArgSlotSpec arg_slot = get_arg_slot(slot_spec);

    assert (arg_slot == arg.type);
  }

  return binding.get_arg_bindings();
}

OpTaskArgumentFormat compile_task_invocation(OpTaskSignature const &signature, OpTaskBinding &binding) {
  OpTaskArgumentFormat result;

  result.region_idxs = allocate_region_idxs(signature, binding);
  result.argument_offsets = allocate_argument_offsets(signature, binding);
  *((OpTaskArgumentFormat *)binding.task_format_location) = result;

  return result;
}

OpTaskArgumentAccessor::OpTaskArgumentAccessor(Legion::Task const *task,
                                               std::vector<Legion::PhysicalRegion> const &regions,
                                               Legion::Context ctx,
                                               Legion::Runtime *runtime)
  : task(task), regions(regions), ctx(ctx), runtime(runtime), args_fmt(*(OpTaskArgumentFormat const *)task->args)
{ }

/* TensorSpec::TensorSpec(TensorRole tensor_role, int idx, IsGrad is_grad, optional<Legion::PrivilegeMode> mode) */
/*   : role(tensor_role), idx(idx), is_grad(is_grad), mode(mode) */
/* { } */

/* OpTaskSpec::OpTaskSpec(TaskID task_id, OpTaskType task_type) */ 
/*   : task_id(task_id), task_type(task_type) */
/* { } */

/* void OpTaskSpec::add_input_slot(int slot) { */
/*   this->slots.insert({slot, TensorRole::INPUT}); */
/* } */

/* void OpTaskSpec::add_param_slot(int slot) { */
/*   this->slots.insert({slot, TensorRole::PARAM}); */
/* } */

/* void OpTaskSpec::add_output_slot(int slot) { */
/*   this->slots.insert({slot, TensorRole::OUTPUT}); */
/* } */

/* static bool spec_satisfies_slot_role(TensorRole slot_role, TensorSpec const &tensor_spec) { */
/*   if (slot_role == TensorRole::INPUT || slot_role == TensorRole::PARAM) { */
/*     return (tensor_spec.mode == READ_ONLY || tensor_spec.mode == READ_WRITE); */
/*   } else if (slot_role == TensorRole::OUTPUT) { */
/*     return (tensor_spec.mode == WRITE_ONLY || tensor_spec.mode == READ_WRITE); */
/*   } */
/* } */

/* void OpTaskSpec::bind(int slot, TensorSpec const &tensor_spec) { */
/*   assert (contains_key(this->slots, slot)); */
/*   assert (spec_satisfies_slot_role(this->slots.at(slot), tensor_spec)); */

/*   if (!contains_l(this->region_idxs, tensor_spec)) { */
/*     region_idxs.equate(tensor_spec, this->new_region_idx()); */  
/*   } */

/*   this->bindings.insert({slot, tensor_spec}); */
/* } */

/* void OpTaskSpec::bind(std::vector<std::pair<int, TensorSpec>> const &bindings) { */
/*   for (auto const &binding : bindings) { */
/*     this->bind(binding.first, binding.second); */
/*   } */
/* } */

/* tl::optional<TensorSpec const &> OpTaskSpec::in_slot(int slot) const { */
/*   return this->bindings.at(slot); */
/* } */

/* int OpTaskSpec::get_region_idx(TensorSpec const &tensor_spec) const { */
/*   return this->region_idxs.at_l(tensor_spec); */
/* }; */

/* optional<int> OpTaskSpec::get_region_idx(int slot) const { */
/*   auto tensor_spec = this->in_slot(slot); */
/*   if (!tensor_spec.has_value()) { */
/*     return nullopt; */
/*   } else { */
/*     return this->get_region_idx(tensor_spec.value()); */
/*   } */
/* } */

/* int OpTaskSpec::new_region_idx() { */
/*   int result = this->region_idx_counter; */
/*   this->region_idx_counter++; */
/*   return result; */
/* } */

/* TaskAccessor(Task const *task, std::vector<Legion::PhysicalRegion> const &regions, Legion::Context const &ctx, Legion::Runtime *runtime, TaskSpec const &spec) */
/*   : task(task), regions(regions), ctx(ctx), runtime(runtime), spec(spec) */
/* { } */

/* TaskAccessor(Task const *task, std::vector<Legion::PhysicalRegion> const &regions, Legion::Context const &ctx, Legion::Runtime *runtime, OpTaskType task_type) */
/*   : TaskAccessor(task, regions, ctx, runtime, ((Op const *)task->args)->get_tasks_spec(task_type)) */
/* { } */

}
