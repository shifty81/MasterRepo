#include "Engine/Scripting/VM/BytecodeVM/BytecodeVM.h"
#include <sstream>
#include <stdexcept>
#include <vector>

namespace Engine {

struct CallFrame { uint32_t returnPc; };

struct BytecodeVM::Impl {
    std::vector<VMValue>    stack;
    VMValue                 regs[8];
    std::vector<CallFrame>  callStack;
    uint32_t                pc{0};

    void Push(VMValue v){ stack.push_back(v); }
    VMValue Pop(){ if(stack.empty()) throw std::runtime_error("stack underflow"); VMValue v=stack.back(); stack.pop_back(); return v; }
    VMValue Top() const { return stack.empty()?VMValue():stack.back(); }
    VMValue Peek(uint32_t n) const { return (n<stack.size())?stack[stack.size()-1-n]:VMValue(); }

    static float ToF(VMValue v){ return v.isFloat?v.f:(float)v.i; }
    static int32_t ToI(VMValue v){ return v.isFloat?(int32_t)v.f:v.i; }
};

BytecodeVM::BytecodeVM()  : m_impl(new Impl){}
BytecodeVM::~BytecodeVM() { Shutdown(); delete m_impl; }
void BytecodeVM::Init()     {}
void BytecodeVM::Shutdown() { m_impl->stack.clear(); m_impl->callStack.clear(); m_impl->pc=0; }

void BytecodeVM::Reset(){
    m_impl->stack.clear(); m_impl->callStack.clear(); m_impl->pc=0;
    for(auto& r:m_impl->regs) r=VMValue();
}

VMResult BytecodeVM::Execute(const Program& prog, uint64_t maxInstr){
    m_impl->pc=0; return ExecuteFrom(prog, m_impl->pc, maxInstr);
}

VMResult BytecodeVM::ExecuteFrom(const Program& prog, uint32_t& pc, uint64_t maxInstr){
    VMResult res; res.ok=false;
    uint64_t cycles=0;
    while(pc<prog.size()&&cycles<maxInstr){
        const Instruction& inst=prog[pc]; pc++; cycles++;
        switch(inst.op){
        case Opcode::NOP: break;
        case Opcode::PUSH_I: m_impl->Push(VMValue(inst.operand.i)); break;
        case Opcode::PUSH_F: m_impl->Push(VMValue(inst.operand.f)); break;
        case Opcode::POP: m_impl->Pop(); break;
        case Opcode::ADD:{ auto b=m_impl->Pop(),a=m_impl->Pop();
            if(a.isFloat||b.isFloat) m_impl->Push(VMValue(Impl::ToF(a)+Impl::ToF(b)));
            else m_impl->Push(VMValue(a.i+b.i)); break; }
        case Opcode::SUB:{ auto b=m_impl->Pop(),a=m_impl->Pop();
            if(a.isFloat||b.isFloat) m_impl->Push(VMValue(Impl::ToF(a)-Impl::ToF(b)));
            else m_impl->Push(VMValue(a.i-b.i)); break; }
        case Opcode::MUL:{ auto b=m_impl->Pop(),a=m_impl->Pop();
            if(a.isFloat||b.isFloat) m_impl->Push(VMValue(Impl::ToF(a)*Impl::ToF(b)));
            else m_impl->Push(VMValue(a.i*b.i)); break; }
        case Opcode::DIV:{ auto b=m_impl->Pop(),a=m_impl->Pop();
            if(a.isFloat||b.isFloat){ float bv=Impl::ToF(b); m_impl->Push(VMValue(bv!=0.f?Impl::ToF(a)/bv:0.f)); }
            else{ m_impl->Push(VMValue(b.i!=0?a.i/b.i:0)); } break; }
        case Opcode::MOD:{ auto b=m_impl->Pop(),a=m_impl->Pop(); m_impl->Push(VMValue(b.i!=0?a.i%b.i:0)); break; }
        case Opcode::NEG:{ auto a=m_impl->Pop(); if(a.isFloat) m_impl->Push(VMValue(-a.f)); else m_impl->Push(VMValue(-a.i)); break; }
        case Opcode::EQ:{ auto b=m_impl->Pop(),a=m_impl->Pop(); m_impl->Push(VMValue(Impl::ToF(a)==Impl::ToF(b)?1:0)); break; }
        case Opcode::NEQ:{ auto b=m_impl->Pop(),a=m_impl->Pop(); m_impl->Push(VMValue(Impl::ToF(a)!=Impl::ToF(b)?1:0)); break; }
        case Opcode::LT:{ auto b=m_impl->Pop(),a=m_impl->Pop(); m_impl->Push(VMValue(Impl::ToF(a)<Impl::ToF(b)?1:0)); break; }
        case Opcode::LE:{ auto b=m_impl->Pop(),a=m_impl->Pop(); m_impl->Push(VMValue(Impl::ToF(a)<=Impl::ToF(b)?1:0)); break; }
        case Opcode::GT:{ auto b=m_impl->Pop(),a=m_impl->Pop(); m_impl->Push(VMValue(Impl::ToF(a)>Impl::ToF(b)?1:0)); break; }
        case Opcode::GE:{ auto b=m_impl->Pop(),a=m_impl->Pop(); m_impl->Push(VMValue(Impl::ToF(a)>=Impl::ToF(b)?1:0)); break; }
        case Opcode::AND:{ auto b=m_impl->Pop(),a=m_impl->Pop(); m_impl->Push(VMValue(a.i&&b.i?1:0)); break; }
        case Opcode::OR:{ auto b=m_impl->Pop(),a=m_impl->Pop(); m_impl->Push(VMValue(a.i||b.i?1:0)); break; }
        case Opcode::NOT:{ auto a=m_impl->Pop(); m_impl->Push(VMValue(a.i?0:1)); break; }
        case Opcode::JMP: pc=(uint32_t)inst.operand.i; break;
        case Opcode::JZ:{ auto a=m_impl->Pop(); if(!a.i) pc=(uint32_t)inst.operand.i; break; }
        case Opcode::JNZ:{ auto a=m_impl->Pop(); if(a.i) pc=(uint32_t)inst.operand.i; break; }
        case Opcode::CALL: m_impl->callStack.push_back({pc}); pc=(uint32_t)inst.operand.i; break;
        case Opcode::RET:
            if(m_impl->callStack.empty()){ res.ok=true; res.haltCode=0; res.cyclesRun=cycles; return res; }
            pc=m_impl->callStack.back().returnPc; m_impl->callStack.pop_back(); break;
        case Opcode::LOAD_R: m_impl->Push(m_impl->regs[inst.operand.i&7]); break;
        case Opcode::STORE_R: m_impl->regs[inst.operand.i&7]=m_impl->Pop(); break;
        case Opcode::HALT:
            res.ok=true; res.haltCode=inst.operand.i; res.cyclesRun=cycles; return res;
        default: res.error="unknown opcode"; return res;
        }
    }
    res.ok=(pc>=prog.size()); res.cyclesRun=cycles; return res;
}

VMValue  BytecodeVM::Top()             const { return m_impl->Top(); }
VMValue  BytecodeVM::Peek(uint32_t n)  const { return m_impl->Peek(n); }
uint32_t BytecodeVM::StackSize()       const { return (uint32_t)m_impl->stack.size(); }
void     BytecodeVM::SetReg(uint8_t r, VMValue v){ m_impl->regs[r&7]=v; }
VMValue  BytecodeVM::GetReg(uint8_t r) const  { return m_impl->regs[r&7]; }
uint32_t BytecodeVM::PC()              const  { return m_impl->pc; }

std::string BytecodeVM::Disassemble(const Program& prog){
    std::ostringstream ss;
    static const char* names[]={
        "NOP","PUSH_I","PUSH_F","POP","ADD","SUB","MUL","DIV","MOD","NEG",
        "AND","OR","NOT","EQ","NEQ","LT","LE","GT","GE",
        "JMP","JZ","JNZ","CALL","RET","LOAD_R","STORE_R","HALT"};
    for(uint32_t i=0;i<(uint32_t)prog.size();i++){
        auto& inst=prog[i];
        uint8_t op=(uint8_t)inst.op;
        ss<<i<<": "<<(op<27?names[op]:"??");
        if(inst.op==Opcode::PUSH_I||inst.op==Opcode::JMP||inst.op==Opcode::JZ||
           inst.op==Opcode::JNZ||inst.op==Opcode::CALL||inst.op==Opcode::LOAD_R||
           inst.op==Opcode::STORE_R||inst.op==Opcode::HALT) ss<<" "<<inst.operand.i;
        else if(inst.op==Opcode::PUSH_F) ss<<" "<<inst.operand.f;
        ss<<"\n";
    }
    return ss.str();
}

} // namespace Engine
