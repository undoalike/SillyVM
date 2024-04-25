/* SillyVM */
/* main.c */
/* Author: undoalike */
/* Date: 2024-04-25 */

/* Each operation takes exactly PC_OFFSET [4] words (opcode, operand1, operand2, result) */
/* Everything's an int32. */
/* All opcodes are scrambled. */
/* All register accesses (load, store) are modulo NUM_REGISTERS [8]. */

#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#define NUM_REGISTERS (8)
#define PC_OFFSET (4)
typedef enum
{
    VMS_OK = 0,
    VMS_HALTED = 1,
    VMS_DIV_0_ERROR = 2,
} VMStatus;

typedef struct {
    int32_t  reg[NUM_REGISTERS];
    int32_t  pc;   // Program counter
    int32_t* rx;   // Pointer to rx register (register 0)
    int32_t* ry;   // Pointer to ry register (register 1)
} VirtualMachine;

typedef enum {
    OP_HALT,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_LDS, /* LOAD SCRAMBLED */
    NUM_OPCODES
} Operation;

const char* op_names[NUM_OPCODES] = {
    "HALT",
    "ADD",
    "SUB",
    "MUL",
    "DIV",
    "MOD",
    "LDS"
};

int32_t vm_execute_op(VirtualMachine* vm, Operation opcode, int32_t op1, int32_t op2, int32_t result) {
    // Decode the opcode
    int32_t scrambled_opcode = (int32_t)opcode % NUM_OPCODES;
    int32_t real_opcode = (scrambled_opcode + (*(vm->rx) * *(vm->ry))) % NUM_OPCODES;

    // Decode the decoded opcode lol
    switch (real_opcode) {
    case OP_ADD:
        vm->reg[result % NUM_REGISTERS] = vm->reg[op1 % NUM_REGISTERS] + vm->reg[op2 % NUM_REGISTERS];
        break;
    case OP_SUB:
        vm->reg[result % NUM_REGISTERS] = vm->reg[op1 % NUM_REGISTERS] - vm->reg[op2 % NUM_REGISTERS];
        break;
    case OP_MUL:
        vm->reg[result % NUM_REGISTERS] = vm->reg[op1 % NUM_REGISTERS] * vm->reg[op2 % NUM_REGISTERS];
        break;
    case OP_DIV:
        if (vm->reg[op2 % NUM_REGISTERS] != 0) {
            vm->reg[result % NUM_REGISTERS] = vm->reg[op1 % NUM_REGISTERS] / vm->reg[op2 % NUM_REGISTERS];
        }
        else {
            return (int32_t)VMS_DIV_0_ERROR; // Domain error (division by zero)
        }
        break;
    case OP_MOD:
        if (vm->reg[op2 % NUM_REGISTERS] != 0) {
            vm->reg[result % NUM_REGISTERS] = vm->reg[op1 % NUM_REGISTERS] % vm->reg[op2 % NUM_REGISTERS];
        }
        else {
            return (int32_t)VMS_DIV_0_ERROR; // Domain error (division by zero)
        }
        break;
    case OP_LDS:
        // Change ry pointer
        vm->ry = &(vm->reg[op2 % NUM_REGISTERS]);
        // Load value of op1 into reg[result]
        vm->reg[result % NUM_REGISTERS] = (int32_t)op1;
        break;
    case OP_HALT:
    default: // Unreachable default due to modulo, but the compiler wouldn't stop yelling
        return VMS_HALTED;
    }

    return VMS_OK; // EXIT_SUCCESS is too fancy for me
}

void vm_display_debug_op(VirtualMachine* vm, Operation opcode, int32_t op1, int32_t op2, int32_t result) {
    // Decode the opcode
    int32_t scrambled_opcode = (int32_t)opcode % NUM_OPCODES;
    int32_t real_opcode = (scrambled_opcode + (*(vm->rx) * *(vm->ry))) % NUM_OPCODES;

    // Print the scrambled opcode if necessary
    if (real_opcode != scrambled_opcode) {
        printf("OP_%s -> ", op_names[scrambled_opcode % NUM_OPCODES]);
    }
    else {
        printf("OP_");
    }

    switch (real_opcode) {
    case OP_ADD:
        printf("ADD: R%d = R%d(%d) + R%d(%d)\n", result, op1 % NUM_REGISTERS, op1, op2 % NUM_REGISTERS, op2);
        break;
    case OP_SUB:
        printf("SUB: R%d = R%d(%d) - R%d(%d)\n", result, op1 % NUM_REGISTERS, op1, op2 % NUM_REGISTERS, op2);
        break;
    case OP_MUL:
        printf("MUL: R%d = R%d(%d) * R%d(%d)\n", result, op1 % NUM_REGISTERS, op1, op2 % NUM_REGISTERS, op2);
        break;
    case OP_DIV:
        printf("DIV: R%d = R%d(%d) / R%d(%d)\n", result, op1 % NUM_REGISTERS, op1, op2 % NUM_REGISTERS, op2);
        break;
    case OP_MOD:
        printf("MOD: R%d = R%d(%d) %% R%d(%d)\n", result, op1 % NUM_REGISTERS, op1, op2 % NUM_REGISTERS, op2);
        break;
    case OP_LDS:
        printf("LDS: &RY = R%d(%d), R%d = %d\n", op2 % NUM_REGISTERS, op2, result, op1);
        break;
    case OP_HALT:
    default: // Unreachable default due to modulo, but the compiler wouldn't stop yelling
        printf("HALT!\n\n");
    }
}

void vm_print_state(VirtualMachine* vm) {
    printf("VM [PC: 0x%08x &RX: 0x%02x &RY: 0x%02x] R0: 0x%08x R1: 0x%08x R2: 0x%08x R3: 0x%08x R4: 0x%08x R5: 0x%08x R6: 0x%08x R7: 0x%08x\n",
        vm->pc, (unsigned)(vm->rx - vm->reg), (unsigned)(vm->ry - vm->reg),
        vm->reg[0], vm->reg[1], vm->reg[2], vm->reg[3],
        vm->reg[4], vm->reg[5], vm->reg[6], vm->reg[7]);
}

void vm_init(VirtualMachine* vm) {
    for (int i = 0; i < NUM_REGISTERS; i++) {
        vm->reg[i] = 0;
    }
    vm->pc = 0;  // Initialize program counter
    vm->rx = &(vm->reg[0]); // rx points to register 0
    vm->ry = &(vm->reg[0]); // ry points to register 0
}

void vm_print_error_code(int32_t error_code) {
    switch (error_code) {
    case VMS_OK:
        printf("VM OK\n"); break;
    case VMS_HALTED:
        printf("VM HALTED\n"); break;
    case VMS_DIV_0_ERROR:
        printf("VM ERROR (DIV0)\n"); break;
    default:
        printf("VM ERROR CODE %d\n", error_code);
    }
}

int32_t vm_execute(VirtualMachine* vm, int32_t program[], int program_size) {
    while (vm->pc < program_size) {
        Operation opcode = program[vm->pc];
        int32_t op1 = program[vm->pc + 1];
        int32_t op2 = program[vm->pc + 2];
        int32_t result = program[vm->pc + 3];

        // Print vm state
        vm_print_state(vm);

        vm_display_debug_op(vm, opcode, op1, op2, result);
        int32_t error_code = vm_execute_op(vm, opcode, op1, op2, result);
        if (error_code) {
            vm_print_error_code(error_code);
            // Print vm state
            vm_print_state(vm);
            printf("\n");
            return error_code;
        }

        printf("\n");
        // Move to the next operation
        vm->pc += PC_OFFSET;
    }

    return 0; // Execution completed successfully
}

int main() {
    VirtualMachine vm;
    vm_init(&vm);

    int32_t program[] = {
        OP_LDS, 0x4a, 1, 1,             // Change ry to reg[1] (0x00) and load 0x4a into reg[1] (ry)
        OP_LDS, 0x40, 2, 2,             // Change ry to reg[0] (0x00) and load 0x40 into reg[2]
        OP_LDS, 0x29, 1, 0,             // Change ry to reg[1] (0x4a) and load 0x29 into reg[0] (rx)
        OP_ADD, 2, 1, 1,                // Add reg[2] (0x40) and reg[1] (0x4a) and store the result in reg[1] (ry)
                                        // Psych, lol! ADD->DIV (and the result changes reg[1] (ry) to 0)
        OP_ADD, 2, 1, 1,                // Add reg[2] (0x40) and reg[1] (0x4a) and store the result in reg[1] (ry)
                                        // This time it works!
        OP_LDS, 0, 0, 0,                // Reset rx, ry
        OP_HALT, 0x21, 0x37, 0x21372137 // We're done!
    };

    int32_t error_code = vm_execute(&vm, program, sizeof(program));
    if (error_code != 1) {
        return error_code;
    }

    return 0;
}
