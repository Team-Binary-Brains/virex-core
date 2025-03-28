/**
 * @file O_codegenerator.h
 * @brief Header file for the intermediate code generator module.
 *
 * This file contains the declarations of functions related to intermediate code generation from the syntax tree.
 *
 * @author Omkar Jagtap
 * @date January 2025
 */

#pragma once
#include "univ_defs.h"
#include "O_parse_tree.h"

#pragma GCC diagnostic ignored "-Wunused-function"

// Enumeration of TAC operations.
typedef enum {
    TAC_ASSIGN,
    TAC_ADD,
    TAC_SUB,
    TAC_MUL,
    TAC_DIV,
    TAC_MOD,
    TAC_EQ,
    TAC_NE,
    TAC_LT,
    TAC_LE,
    TAC_GT,
    TAC_GE,
    TAC_LABEL,
    TAC_GOTO,
    TAC_IF,
    TAC_PRINT,
    TAC_SCAN,
    TAC_EXIT
} TacOp;

// Structure representing a single TAC instruction.
typedef struct TACInstruction {
    TacOp op;         // Operation code
    char* result;     // Destination (or label name)
    char* arg1;       // First operand or condition
    char* arg2;       // Second operand (if needed)
    struct TACInstruction* next;
} TACInstruction;

// Interface for generating and printing intermediate code.
TACInstruction** generateIntermediateCode(ParseTreeNode* root);

static char* generateExpression(ParseTreeNode* node);

// Generate code for statements and control structures.
static void generateCode(ParseTreeNode* node);

// Utility: Create a new TAC instruction.
static TACInstruction* newTAC(TacOp op, char* result, char* arg1, char* arg2);

// Utility: Create a new label name.
static char* newLabel(void);

// Utility: Create a new temporary variable name.
static char* newTemp(void);

static void emit(TACInstruction* inst);

void freeIntermediateCode(void);