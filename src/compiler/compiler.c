/*
int bc_gen(lgx_bc_t *bc, lgx_ast_node_t *node) {
    unsigned i;
    lgx_val_t k, v;
    lgx_str_t s;
    lgx_ast_node_token_t *token;
    switch(node->type) {
        // Statement
        case BLOCK_STATEMENT:
            //bc_scope_new(bc);

            for(i = 0; i < node->children; i++) {
                bc_gen(bc, node->child[i]);
            }

            //bc_scope_delete(bc);
            break;
        case IF_STATEMENT:
            bc_gen(bc, node->child[0]);

            // 写入条件跳转

            bc_gen(bc, node->child[1]);

            // 更新条件跳转
            break;
        case IF_ELSE_STATEMENT:
            bc_gen(bc, node->child[0]);

            // 写入条件跳转

            bc_gen(bc, node->child[1]);

            // 写入无条件跳转
            // 更新条件跳转

            bc_gen(bc, node->child[2]);

            // 更新无条件跳转
            break;
        case FOR_STATEMENT:

            break;
        case WHILE_STATEMENT:
            bc_gen(bc, node->child[0]);

            // 写入条件跳转

            bc_gen(bc, node->child[1]);

            // 写入无条件跳转

            // 更新条件跳转
            break;
        case DO_WHILE_STATEMENT:

            bc_gen(bc, node->child[0]);
            bc_gen(bc, node->child[1]);
            break;
        case CONTINUE_STATEMENT:

            break;
        case BREAK_STATEMENT: // break 只应该出现在块级作用域中
            //bc_scope_delete(bc);

            // 写入跳转指令
            // 保存指令位置以便未来更新跳转地址
            break;
        case SWITCH_CASE_STATEMENT:

            break;
        case RETURN_STATEMENT:
            // 计算返回值
            if (node->child[0]) {
                bc_gen(bc, node->child[0]);
            }

            // 释放参数与局部变量

            // 更新返回值寄存器

            // 写入返回指令

            break;
        case ASSIGNMENT_STATEMENT:

            break;
        // Declaration
        case FUNCTION_DECLARATION:
            // 跳过函数体
            bc_gen(bc, node->child[0]);
            break;
        case VARIABLE_DECLARATION:
            
            break;
        // Expression
        case CONDITIONAL_EXPRESSION:

            break;
        case BINARY_EXPRESSION:
            bc_gen(bc, node->child[0]);
            bc_gen(bc, node->child[1]);

            switch (node->u.op) {
                case '+':

                    break;
                case '-':

                    break;
                case '*':

                    break;
                case '/':

                    break;
                default:
                    // error
                    break;
            }

            break;
        case UNARY_EXPRESSION:

            bc_gen(bc, node->child[0]);

            switch (node->u.op) {
                case '!':

                    break;
                case '~':

                    break;
                case '-':

                    break;
                default:
                    // error
                    break;
            }

            break;
        // Other
        case IDENTIFIER_TOKEN:

            break;
        case NUMBER_TOKEN:
            //bc->bc[bc->bc_offset++] = OP_PUSH;
            token = (lgx_ast_node_token_t *)node;

            k.type = T_LONG;
            k.v.l = atoi(token->tk_start);
            
            i = lgx_hash_get(&bc->constants, &k);
            break;
        case STRING_TOKEN:

            break;
        default:
            printf("%s %d\n", "ERROR!", node->type);
    }
    
    return 0;
}

int lgx_bc_gen(lgx_bc_t *bc) {
    return bc_gen(bc, bc->ast->root);
}*/