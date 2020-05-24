#pragma once


namespace nano {

struct error_manager_t;
struct error_t;

struct lexer_t;
struct token_t;
struct token_stream_t;

struct parser_t;
struct function_t;
struct identifier_t;

struct ast_t;
struct ast_block_t;
struct ast_node_t;
struct ast_program_t;
struct ast_exp_ident_t;
struct ast_exp_array_t;
struct ast_exp_call_t;
struct ast_exp_bin_op_t;
struct ast_exp_unary_op_t;
struct ast_stmt_if_t;
struct ast_stmt_while_t;
struct ast_stmt_for_t;
struct ast_stmt_return_t;
struct ast_stmt_assign_var_t;
struct ast_stmt_assign_array_t;
struct ast_decl_func_t;
struct ast_decl_var_t;

struct source_t;
struct source_manager_t;

struct program_t;
struct program_builder_t;
struct line_t;

struct codegen_t;

struct disassembler_t;

struct thread_t;


typedef void(*nano_syscall_t)(struct thread_t &thread, int32_t num_args);

} // namespace nano
