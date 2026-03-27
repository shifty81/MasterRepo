#pragma once
/**
 * @file BytecodeVM.h
 * @brief Tiny stack VM: opcodes push/pop/arith/cmp/jmp/call/ret, register file, execute.
 *
 * Features:
 *   - Opcode enum: NOP, PUSH_I, PUSH_F, POP, ADD, SUB, MUL, DIV, MOD,
 *                  NEG, AND, OR, NOT, EQ, NEQ, LT, LE, GT, GE,
 *                  JMP, JZ, JNZ, CALL, RET, LOAD_R, STORE_R, HALT
 *   - VMValue: tagged union (int32 or float32)
 *   - Program: vector of Instructions (opcode + optional operand)
 *   - 8 general-purpose registers
 *   - Call stack (max 256 frames)
 *   - Execute(program, maxInstructions) → VMResult {ok, haltCode, cyclesRun}
 *   - ExecuteFrom(pc, maxInstructions) for resumable execution
 *   - Stack accessors: Top(), Peek(n), StackSize()
 *   - Reset() — clear stack, registers, pc
 *   - Disassemble(program) → human-readable string
 *
 * Typical usage:
 * @code
 *   BytecodeVM vm;
 *   vm.Init();
 *   Program prog = {
 *     {PUSH_I, 10}, {PUSH_I, 32}, {ADD}, {HALT}
 *   };
 *   auto r = vm.Execute(prog, 100);
 *   int result = vm.Top().i; // 42
 * @endcode
 */

#include <cstdint>
#include <string>
#include <vector>

namespace Engine {

enum class Opcode : uint8_t {
    NOP, PUSH_I, PUSH_F, POP,
    ADD, SUB, MUL, DIV, MOD, NEG,
    AND, OR, NOT,
    EQ, NEQ, LT, LE, GT, GE,
    JMP, JZ, JNZ, CALL, RET,
    LOAD_R, STORE_R,
    HALT
};

struct VMValue {
    union { int32_t i; float f; };
    bool isFloat{false};
    VMValue()              : i(0), isFloat(false) {}
    explicit VMValue(int32_t v) : i(v), isFloat(false) {}
    explicit VMValue(float  v) : f(v), isFloat(true)  {}
};

struct Instruction {
    Opcode   op;
    VMValue  operand;
    Instruction(Opcode o)            : op(o) {}
    Instruction(Opcode o, int32_t v) : op(o), operand(v) {}
    Instruction(Opcode o, float   v) : op(o), operand(v) {}
};

using Program = std::vector<Instruction>;

struct VMResult {
    bool     ok       {false};
    int32_t  haltCode {0};
    uint64_t cyclesRun{0};
    std::string error;
};

class BytecodeVM {
public:
    BytecodeVM();
    ~BytecodeVM();

    void Init    ();
    void Shutdown();

    VMResult Execute    (const Program& prog, uint64_t maxInstructions=100000);
    VMResult ExecuteFrom(const Program& prog, uint32_t& pc, uint64_t maxInstructions);

    void Reset();

    // Stack
    VMValue  Top       () const;
    VMValue  Peek      (uint32_t offset) const;
    uint32_t StackSize () const;

    // Registers
    void    SetReg(uint8_t reg, VMValue val);
    VMValue GetReg(uint8_t reg) const;

    // Disassembler
    static std::string Disassemble(const Program& prog);

    uint32_t PC() const;

private:
    struct Impl;
    Impl* m_impl{nullptr};
};

} // namespace Engine
