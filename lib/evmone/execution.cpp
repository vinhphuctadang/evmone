// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2019-2020 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include "execution.hpp"
#include "analysis.hpp"
#include <papi.h>
#include <iostream>
#include <memory>

namespace evmone
{
long_long start_cycles;
long_long end_cycles;

evmc_result execute(evmc_vm* /*unused*/, const evmc_host_interface* host, evmc_host_context* ctx,
    evmc_revision rev, const evmc_message* msg, const uint8_t* code, size_t code_size) noexcept
{
    auto analysis = analyze(rev, code, code_size);

    auto state = std::make_unique<execution_state>(*msg, rev, *host, ctx, code, code_size);
    state->analysis = &analysis;

    start_cycles = PAPI_get_real_cyc();

    const auto* instr = &state->analysis->instrs[0];
    while (instr != nullptr)
    {
        end_cycles = PAPI_get_real_cyc();
        instr = instr->fn(instr, *state);
    }

    const auto gas_left =
        (state->status == EVMC_SUCCESS || state->status == EVMC_REVERT) ? state->gas_left : 0;

    std::cerr << "Total cycles: " << (end_cycles - start_cycles) << "\n";
    return evmc::make_result(
        state->status, gas_left, &state->memory[state->output_offset], state->output_size);
}
}  // namespace evmone
