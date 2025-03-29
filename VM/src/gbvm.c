#include "gbvm.h"
#include "sasm_memory.h"
#pragma GCC diagnostic ignored "-Wswitch-enum"

void dumpStack(WINDOW* win, const Vm* vm)
{
    wprintw(win, "  ");
    for (uint64_t i = 0; i < 256; i++) {
        wprintw(win, "%02X ", vm->mem.memory[i]);
        if (i % 32 == 31) {
            wprintw(win, "\n  ");
        }
    }
}

void dumpFlags(WINDOW* win, CPU* cpu)
{
    wprintw(win,
        "\n  Halt : %d",
        getFlag(HALT, cpu));
}

void dumpDetails(WINDOW* win, OpcodeDetails* details, Instruction* inst)
{
    wprintw(win,
        "\n  Instruction Number :\t%d"
        "\n  Instruction :\t\t%s      ",
        details->type, details->name);

    if (details->has_operand) {
        wprintw(win,
            "\n  Operand1 : \t\t%ld",
            inst->operand.as_u64);
    }

    if (details->has_operand2) {
        wprintw(win,
            "\n  Operand2 : \t\t%ld",
            inst->operand2.as_u64);
    }
}

void executeProgram(Vm* vm, int debug, int lim)
{
    CPU* cpu = &(vm->cpu);
    Memory* mem = &(vm->mem);
    Program* prog = &(vm->prog);
    VmCalls* calls = &(vm->vmCalls);
    Instruction* inst = &(prog->instructions[vm->cpu.registers.IP.as_u64]);

    if (lim == 0 || getFlag(HALT, cpu)) {
        return;
    }

    OpcodeDetails details = getOpcodeDetails(inst->type);
    Error error = 0;
    switch (debug) {
    case 2:
        refreshWindow(vm->disp.windows[DETAILS], (String) { .data = "DETAILS", .length = 7 });
        refreshWindow(vm->disp.windows[MEMORY], (String) { .data = "DETAILS", .length = 7 });
        getch();
        wclear(vm->disp.windows[DETAILS]);
        wclear(vm->disp.windows[MEMORY]);
        dumpStack(vm->disp.windows[MEMORY], vm);
        dumpDetails(vm->disp.windows[DETAILS], &details, inst);
        dumpFlags(vm->disp.windows[DETAILS], cpu);
        error = executeInst(prog, mem, cpu, calls, vm->disp.windows[OUTPUT]);
        break;
    case 1:
        refreshWindow(vm->disp.windows[DETAILS], (String) { .data = "DETAILS", .length = 7 });
        refreshWindow(vm->disp.windows[MEMORY], (String) { .data = "DETAILS", .length = 7 });
        getch();
        wclear(vm->disp.windows[DETAILS]);
        wclear(vm->disp.windows[MEMORY]);
        dumpStack(vm->disp.windows[MEMORY], vm);
        dumpDetails(vm->disp.windows[DETAILS], &details, inst);
        error = executeInst(prog, mem, cpu, calls, vm->disp.windows[OUTPUT]);
        break;
    default:
        error = executeInst(prog, mem, cpu, calls, vm->disp.windows[OUTPUT]);
        break;
    }

    if (error != ERR_OK)
        executionErrorWithExit(&error);

    executeProgram(vm, debug, lim - 1);
}

#define READ_OP(prog, mem, cpu, type, out)                                       \
    {                                                                            \
        if (cpu->registers.SP.as_u64 < 1) {                                      \
            return ERR_STACK_UNDERFLOW;                                          \
        }                                                                        \
        const MemoryAddr addr = mem->stack[cpu->registers.SP.as_u64 - 1].as_u64; \
        if (addr >= MEMORY_CAPACITY) {                                           \
            return ERR_ILLEGAL_MEMORY_ACCESS;                                    \
        }                                                                        \
        type tmp;                                                                \
        memcpy(&tmp, &mem->memory[addr], sizeof(type));                          \
        mem->stack[cpu->registers.SP.as_u64 - 1].as_##out = tmp;                 \
        cpu->registers.IP.as_u64 += 1;                                           \
    }

#define BINARY_OP(prog, mem, cpu, in, out, op)                                                                                                                    \
    {                                                                                                                                                             \
        if (cpu->registers.SP.as_u64 < 2) {                                                                                                                       \
            return ERR_STACK_UNDERFLOW;                                                                                                                           \
        }                                                                                                                                                         \
        mem->stack[cpu->registers.SP.as_u64 - 2].as_##out = mem->stack[cpu->registers.SP.as_u64 - 2].as_##in op mem->stack[cpu->registers.SP.as_u64 - 1].as_##in; \
        cpu->registers.SP.as_u64 -= 1;                                                                                                                            \
        cpu->registers.IP.as_u64 += 1;                                                                                                                            \
    }

#define CAST_OP(prog, mem, cpu, src, dst, cast)                                                                     \
    {                                                                                                               \
        if (cpu->registers.SP.as_u64 < 1) {                                                                         \
            return ERR_STACK_UNDERFLOW;                                                                             \
        }                                                                                                           \
        mem->stack[cpu->registers.SP.as_u64 - 1].as_##dst = cast mem->stack[cpu->registers.SP.as_u64 - 1].as_##src; \
        cpu->registers.IP.as_u64 += 1;                                                                              \
    }
Error executeInst(const Program* prog, Memory* mem, CPU* cpu, const VmCalls* vmCalls, WINDOW* win)
{
    if (cpu->registers.IP.as_u64 >= prog->instruction_count) {
        printf("error %ld %ld", cpu->registers.IP.as_u64, prog->instruction_count);
        return ERR_ILLEGAL_INST_ACCESS;
    }
    Instruction inst = prog->instructions[cpu->registers.IP.as_u64];

    QuadWord *operand1 = NULL, *operand2 = NULL;
    evaluateAddressingMode(mem, cpu, inst.opr1Mode, &inst.operand, operand1);
    evaluateAddressingMode(mem, cpu, inst.opr2Mode, &inst.operand2, operand2);

    // printf("\nenter : %d %s", inst.type, OpcodeDetailsMap[inst.type].name);
    switch (inst.type) {
    case INST_CPY:
        *operand1 = *operand2;
        cpu->registers.IP.as_u64 += 1;
        break;
    case INST_DONOP:
        cpu->registers.IP.as_u64 += 1;
        break;

    case INST_PUSH:
        if (cpu->registers.SP.as_u64 >= STACK_CAPACITY) {
            return ERR_STACK_OVERFLOW;
        }
        mem->stack[cpu->registers.SP.as_u64++] = inst.operand;
        cpu->registers.IP.as_u64 += 1;
        break;

    case INST_DROP:
        if (cpu->registers.SP.as_u64 < 1) {
            return ERR_STACK_UNDERFLOW;
        }
        cpu->registers.SP.as_u64 -= 1;
        cpu->registers.IP.as_u64 += 1;
        break;

    case INST_ADDI:
        BINARY_OP(prog, mem, cpu, u64, u64, +);
        break;

    case INST_SUBI:
        BINARY_OP(prog, mem, cpu, u64, u64, -);
        break;

    case INST_MULI:
        BINARY_OP(prog, mem, cpu, i64, i64, *);
        break;

    case INST_MULU:
        BINARY_OP(prog, mem, cpu, u64, u64, *);
        break;

    case INST_DIVI:
        {
            if (mem->stack[cpu->registers.SP.as_u64 - 1].as_i64 == 0) {
                return ERR_DIV_BY_ZERO;
            }
            BINARY_OP(prog, mem, cpu, i64, i64, /);
        }
        break;

    case INST_DIVU:
        {
            if (mem->stack[cpu->registers.SP.as_u64 - 1].as_u64 == 0) {
                return ERR_DIV_BY_ZERO;
            }
            BINARY_OP(prog, mem, cpu, u64, u64, /);
        }
        break;

    case INST_MODI:
        {
            if (mem->stack[cpu->registers.SP.as_u64 - 1].as_i64 == 0) {
                return ERR_DIV_BY_ZERO;
            }
            BINARY_OP(prog, mem, cpu, i64, i64, %);
        }
        break;

    case INST_MODU:
        {
            if (mem->stack[cpu->registers.SP.as_u64 - 1].as_u64 == 0) {
                return ERR_DIV_BY_ZERO;
            }
            BINARY_OP(prog, mem, cpu, u64, u64, %);
        }
        break;

    case INST_ADDF:
        BINARY_OP(prog, mem, cpu, f64, f64, +);
        break;

    case INST_SUBF:
        BINARY_OP(prog, mem, cpu, f64, f64, -);
        break;

    case INST_MULF:
        BINARY_OP(prog, mem, cpu, f64, f64, *);
        break;

    case INST_DIVF:
        BINARY_OP(prog, mem, cpu, f64, f64, /);
        break;

    case INST_JMPU:
        cpu->registers.IP.as_u64 = inst.operand.as_u64;
        break;

    case INST_RETRN:
        if (cpu->registers.SP.as_u64 < 1) {
            return ERR_STACK_UNDERFLOW;
        }

        cpu->registers.IP.as_u64 = mem->stack[cpu->registers.SP.as_u64 - 1].as_u64;
        cpu->registers.SP.as_u64 -= 1;
        break;

    case INST_CALLN:
        if (cpu->registers.SP.as_u64 >= STACK_CAPACITY) {
            return ERR_STACK_OVERFLOW;
        }

        mem->stack[cpu->registers.SP.as_u64++].as_u64 = cpu->registers.IP.as_u64 + 1;
        cpu->registers.IP.as_u64 = inst.operand.as_u64;
        break;

    case INST_CALLF:
        if (inst.operand.as_u64 > vmCalls->internalVmCallsDefined) {
            return ERR_ILLEGAL_OPERAND;
        }

        if (!vmCalls->VmCallI[inst.operand.as_u64]) {
            return ERR_NULL_CALL;
        }

        const Error err = vmCalls->VmCallI[inst.operand.as_u64](cpu, mem, win);
        if (err != ERR_OK) {
            return err;
        }
        cpu->registers.IP.as_u64 += 1;
        break;

    case INST_SHUTS:
        setFlag(HALT, cpu, 1);
        break;

    case INST_EQF:
        BINARY_OP(prog, mem, cpu, f64, u64, ==);
        break;

    case INST_GEF:
        BINARY_OP(prog, mem, cpu, f64, u64, >=);
        break;

    case INST_GTF:
        BINARY_OP(prog, mem, cpu, f64, u64, >);
        break;

    case INST_LEF:
        BINARY_OP(prog, mem, cpu, f64, u64, <=);
        break;

    case INST_LTF:
        BINARY_OP(prog, mem, cpu, f64, u64, <);
        break;

    case INST_NEF:
        BINARY_OP(prog, mem, cpu, f64, u64, !=);
        break;

    case INST_EQI:
        BINARY_OP(prog, mem, cpu, i64, u64, ==);
        break;

    case INST_GEI:
        BINARY_OP(prog, mem, cpu, i64, u64, >=);
        break;

    case INST_GTI:
        BINARY_OP(prog, mem, cpu, i64, u64, >);
        break;

    case INST_LEI:
        BINARY_OP(prog, mem, cpu, i64, u64, <=);
        break;

    case INST_LTI:
        BINARY_OP(prog, mem, cpu, i64, u64, <);
        break;

    case INST_NEI:
        BINARY_OP(prog, mem, cpu, i64, u64, !=);
        break;

    case INST_EQU:
        BINARY_OP(prog, mem, cpu, u64, u64, ==);
        break;

    case INST_GEU:
        BINARY_OP(prog, mem, cpu, u64, u64, >=);
        break;

    case INST_GTU:
        BINARY_OP(prog, mem, cpu, u64, u64, >);
        break;

    case INST_LEU:
        BINARY_OP(prog, mem, cpu, u64, u64, <=);
        break;

    case INST_LTU:
        BINARY_OP(prog, mem, cpu, u64, u64, <);
        break;

    case INST_NEU:
        BINARY_OP(prog, mem, cpu, u64, u64, !=);
        break;

    case INST_JMPC:
        if (cpu->registers.SP.as_u64 < 1) {
            return ERR_STACK_UNDERFLOW;
        }

        if (mem->stack[cpu->registers.SP.as_u64 - 1].as_u64) {
            cpu->registers.IP.as_u64 = inst.operand.as_u64;
        } else {
            cpu->registers.IP.as_u64 += 1;
        }

        cpu->registers.SP.as_u64 -= 1;
        break;

    case INST_DUP:
        if (cpu->registers.SP.as_u64 >= STACK_CAPACITY) {
            return ERR_STACK_OVERFLOW;
        }

        if (cpu->registers.SP.as_u64 - inst.operand.as_u64 <= 0) {
            return ERR_STACK_UNDERFLOW;
        }

        mem->stack[cpu->registers.SP.as_u64] = mem->stack[cpu->registers.SP.as_u64 - 1 - inst.operand.as_u64];
        cpu->registers.SP.as_u64 += 1;
        cpu->registers.IP.as_u64 += 1;
        break;

    case INST_SWAP:
        if (inst.operand.as_u64 >= cpu->registers.SP.as_u64) {
            return ERR_STACK_UNDERFLOW;
        }

        const uint64_t a = cpu->registers.SP.as_u64 - 1;
        const uint64_t b = cpu->registers.SP.as_u64 - 1 - inst.operand.as_u64;

        QuadWord t = mem->stack[a];
        mem->stack[a] = mem->stack[b];
        mem->stack[b] = t;
        cpu->registers.IP.as_u64 += 1;
        break;

    case INST_NOT:
        if (cpu->registers.SP.as_u64 < 1) {
            return ERR_STACK_UNDERFLOW;
        }

        mem->stack[cpu->registers.SP.as_u64 - 1].as_u64 = !mem->stack[cpu->registers.SP.as_u64 - 1].as_u64;
        cpu->registers.IP.as_u64 += 1;
        break;

    case INST_ANDB:
        BINARY_OP(prog, mem, cpu, u64, u64, &);
        break;

    case INST_ORB:
        BINARY_OP(prog, mem, cpu, u64, u64, |);
        break;

    case INST_XOR:
        BINARY_OP(prog, mem, cpu, u64, u64, ^);
        break;

    case INST_SHR:
        BINARY_OP(prog, mem, cpu, u64, u64, >>);
        break;

    case INST_SHL:
        BINARY_OP(prog, mem, cpu, u64, u64, <<);
        break;

    case INST_NOTB:
        if (cpu->registers.SP.as_u64 < 1) {
            return ERR_STACK_UNDERFLOW;
        }

        mem->stack[cpu->registers.SP.as_u64 - 1].as_u64 = ~mem->stack[cpu->registers.SP.as_u64 - 1].as_u64;
        cpu->registers.IP.as_u64 += 1;
        break;

    case INST_READ1U:
        READ_OP(prog, mem, cpu, uint8_t, u64);
        break;

    case INST_READ2U:
        READ_OP(prog, mem, cpu, uint16_t, u64);
        break;

    case INST_READ4U:
        READ_OP(prog, mem, cpu, uint32_t, u64);
        break;

    case INST_READ8U:
        READ_OP(prog, mem, cpu, uint64_t, u64);
        break;

    case INST_READ1I:
        READ_OP(prog, mem, cpu, int8_t, i64);
        break;

    case INST_READ2I:
        READ_OP(prog, mem, cpu, int16_t, i64);
        break;

    case INST_READ4I:
        READ_OP(prog, mem, cpu, int32_t, i64);
        break;

    case INST_READ8I:
        READ_OP(prog, mem, cpu, int64_t, i64);
        break;

    case INST_WRITE1:
        {
            if (cpu->registers.SP.as_u64 < 2) {
                return ERR_STACK_UNDERFLOW;
            }
            const MemoryAddr addr = mem->stack[cpu->registers.SP.as_u64 - 2].as_u64;
            if (addr >= MEMORY_CAPACITY) {
                return ERR_ILLEGAL_MEMORY_ACCESS;
            }
            mem->memory[addr] = (MemoryAddr)mem->stack[cpu->registers.SP.as_u64 - 1].as_u64;
            cpu->registers.SP.as_u64 -= 2;
            cpu->registers.IP.as_u64 += 1;
        }
        break;

    case INST_WRITE2:
        {
            if (cpu->registers.SP.as_u64 < 2) {
                return ERR_STACK_UNDERFLOW;
            }
            const MemoryAddr addr = mem->stack[cpu->registers.SP.as_u64 - 2].as_u64;
            if (addr >= MEMORY_CAPACITY - 1) {
                return ERR_ILLEGAL_MEMORY_ACCESS;
            }
            Word value = (Word)mem->stack[cpu->registers.SP.as_u64 - 1].as_u64;
            memcpy(&mem->memory[addr], &value, sizeof(value));
            cpu->registers.SP.as_u64 -= 2;
            cpu->registers.IP.as_u64 += 1;
        }
        break;

    case INST_WRITE4:
        {
            if (cpu->registers.SP.as_u64 < 2) {
                return ERR_STACK_UNDERFLOW;
            }
            const MemoryAddr addr = mem->stack[cpu->registers.SP.as_u64 - 2].as_u64;
            if (addr >= MEMORY_CAPACITY - 3) {
                return ERR_ILLEGAL_MEMORY_ACCESS;
            }
            DoubleWord value = (DoubleWord)mem->stack[cpu->registers.SP.as_u64 - 1].as_u64;
            memcpy(&mem->memory[addr], &value, sizeof(value));
            cpu->registers.SP.as_u64 -= 2;
            cpu->registers.IP.as_u64 += 1;
        }
        break;

    case INST_WRITE8:
        {
            if (cpu->registers.SP.as_u64 < 2) {
                return ERR_STACK_UNDERFLOW;
            }
            const MemoryAddr addr = mem->stack[cpu->registers.SP.as_u64 - 2].as_u64;
            if (addr >= MEMORY_CAPACITY - 7) {
                return ERR_ILLEGAL_MEMORY_ACCESS;
            }
            QuadWord value = mem->stack[cpu->registers.SP.as_u64 - 1];
            memcpy(&mem->memory[addr], &value, sizeof(value));
            cpu->registers.SP.as_u64 -= 2;
            cpu->registers.IP.as_u64 += 1;
        }
        break;

    case INST_I2F:
        CAST_OP(prog, mem, cpu, i64, f64, (double));
        break;

    case INST_U2F:
        CAST_OP(prog, mem, cpu, u64, f64, (double));
        break;

    case INST_F2I:
        CAST_OP(prog, mem, cpu, f64, i64, (int64_t));
        break;

    case INST_F2U:
        CAST_OP(prog, mem, cpu, f64, u64, (uint64_t)(int64_t));
        break;

    case NUMBER_OF_INSTS:
    default:
        return ERR_ILLEGAL_INST;
    }

    return ERR_OK;
}