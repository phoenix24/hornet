#include "hornet/java.hh"

#include "hornet/vm.hh"

#include <classfile_constants.h>
#include <cassert>
#include <stack>

namespace hornet {

static inline uint16_t read_opc_u2(char *p)
{
    return p[1] << 8 | p[2];
}

void interp(method* method)
{
    std::valarray<object*> locals(method->max_locals);

    std::stack<object*> ostack;

    uint16_t pc = 0;

next_insn:
    assert(pc < method->code_length);

    uint8_t opc = method->code[pc];

    switch (opc) {
    case JVM_OPC_aload_0:
    case JVM_OPC_aload_1:
    case JVM_OPC_aload_2:
    case JVM_OPC_aload_3: {
        uint16_t idx = opc - JVM_OPC_aload_0;
        ostack.push(locals[idx]);
        break;
    }
    case JVM_OPC_pop: {
        ostack.pop();
        break;
    }
    case JVM_OPC_dup: {
        auto value = ostack.top();
        ostack.push(value);
        break;
    }
    case JVM_OPC_goto: {
        int16_t offset = read_opc_u2(method->code + pc);
        pc += offset;
        goto next_insn;
    }
    case JVM_OPC_return: {
        ostack.empty();
        return;
    }
    case JVM_OPC_invokespecial: {
        break;
    }
    case JVM_OPC_new: {
        auto obj = gc_new_object(nullptr);
        ostack.push(obj);
        break;
    }
    default:
        fprintf(stderr, "unsupported bytecode: %u\n", opc);
        assert(0);
    }

    pc += opcode_length[opc];

    goto next_insn;
}

}