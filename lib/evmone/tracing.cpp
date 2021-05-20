// evmone: Fast Ethereum Virtual Machine implementation
// Copyright 2021 The evmone Authors.
// SPDX-License-Identifier: Apache-2.0

#include "tracing.hpp"
#include "../../evmc/tools/utils/utils.hpp"
#include "execution_state.hpp"
#include "instruction_traits.hpp"
#include <evmc/hex.hpp>
#include <stack>

namespace evmone
{
namespace
{
std::string get_name(const char* const* names, uint8_t opcode)
{
    const auto name = names[opcode];
    return (name != nullptr) ? name : "0x" + evmc::hex(opcode);
}

/// @see create_histogram_tracer()
class HistogramTracer : public Tracer
{
    struct Context
    {
        const int32_t depth;
        const uint8_t* const code;
        const char* const* const opcode_names;
        uint32_t counts[256]{};

        Context(int32_t _depth, const uint8_t* _code, const char* const* _opcode_names) noexcept
          : depth{_depth}, code{_code}, opcode_names{_opcode_names}
        {}
    };

    std::stack<Context> m_contexts;
    std::ostream& m_out;

    void on_execution_start(
        evmc_revision rev, const evmc_message& msg, bytes_view code) noexcept override
    {
        m_contexts.emplace(msg.depth, code.data(), evmc_get_instruction_names_table(rev));
    }

    void on_instruction_start(uint32_t pc, const ExecutionState& /*state*/) noexcept override
    {
        auto& ctx = m_contexts.top();
        ++ctx.counts[ctx.code[pc]];
    }

    void on_execution_end(const evmc_result& /*result*/) noexcept override
    {
        const auto& ctx = m_contexts.top();
        const auto names = ctx.opcode_names;

        m_out << "--- # HISTOGRAM depth=" << ctx.depth << "\nopcode,count\n";
        for (size_t i = 0; i < std::size(ctx.counts); ++i)
        {
            if (ctx.counts[i] != 0)
                m_out << get_name(names, static_cast<uint8_t>(i)) << ',' << ctx.counts[i] << '\n';
        }

        m_contexts.pop();
    }

public:
    explicit HistogramTracer(std::ostream& out) noexcept : m_out{out} {}
};

/// Base class for Tracer implementations.
class BaseTracer : public Tracer
{
protected:
    std::ostream& m_out;              ///< Output stream.
    const uint8_t* m_code = nullptr;  ///< Reference to the code being executed.

    void on_execution_start(
        evmc_revision /*rev*/, const evmc_message& /*msg*/, bytes_view code) noexcept override
    {
        m_code = code.data();
    }

    void on_execution_end(const evmc_result& /*result*/) noexcept override { m_code = nullptr; }

public:
    explicit BaseTracer(std::ostream& out) noexcept : m_out{out} {}
};


class InstructionTracer : public BaseTracer
{
    const char* const* m_opcode_names = nullptr;

    void output_stack(const evm_stack& stack, uint8_t opcode)
    {
        const int req = instr::traits[opcode].stack_height_required;
        const int size = stack.size();

        const auto n = std::min(size, std::max(req, 1));

        m_out << R"("stack":[)";

        for (int i = 0; i < n; ++i)
        {
            if (i != 0)
                m_out << ',';
            m_out << R"("0x)" << to_string(stack[i], 16) << '"';
        }

        if (n < size)
            m_out << R"(,"...")";

        m_out << R"(])";

        m_out << R"(,"stackSize":)" << size;
    }

    void output_memory(const evm_memory& memory)
    {
        m_out << R"("memory":")" << evmc::hex({memory.data(), memory.size()})
              << R"(","memorySize":)" << memory.size();
    }

    void on_execution_start(
        evmc_revision rev, const evmc_message& msg, bytes_view code) noexcept override
    {
        using namespace evmc;

        BaseTracer::on_execution_start(rev, msg, code);
        m_opcode_names = evmc_get_instruction_names_table(rev);
        m_out << std::dec;  // Set number formatting to dec, JSON does not support other forms.

        m_out << "{";
        m_out << R"("depth":)" << msg.depth;
        m_out << R"(,"rev":")" << rev << '"';
        m_out << "}\n";
    }

    void on_instruction_start(uint32_t pc, const ExecutionState& state) noexcept override
    {
        const auto opcode = m_code[pc];
        m_out << "{";
        m_out << R"("pc":)" << pc;
        m_out << R"(,"op":)" << int{opcode};
        m_out << R"(,"opName":")" << m_opcode_names[opcode] << '"';
        m_out << R"(,"gas":)" << state.gas_left;
        m_out << ',';
        output_stack(state.stack, opcode);
        m_out << ',';
        output_memory(state.memory);
        m_out << "}\n";
    }

    void on_execution_end(const evmc_result& result) noexcept override
    {
        using namespace evmc;

        m_out << "{";
        m_out << R"("error":)";
        if (result.status_code == EVMC_SUCCESS)
            m_out << "null";
        else
            m_out << '"' << result.status_code << '"';
        m_out << R"(,"gas":)" << result.gas_left;
        m_out << "}\n";

        BaseTracer::on_execution_end(result);
    }

public:
    using BaseTracer::BaseTracer;
};
}  // namespace

std::unique_ptr<Tracer> create_histogram_tracer(std::ostream& out)
{
    return std::make_unique<HistogramTracer>(out);
}

std::unique_ptr<Tracer> create_instruction_tracer(std::ostream& out)
{
    return std::make_unique<InstructionTracer>(out);
}
}  // namespace evmone
