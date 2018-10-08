#include "includes/Parser.hpp"
#include "includes/AST/AST_Integer.hpp"
#include "includes/AST/AST_Float.hpp"
#include "includes/AST/AST_Str.hpp"
#include "includes/AST/AST_BinOp.hpp"
#include "includes/AST/AST_UnaryOp.hpp"
#include "includes/AST/AST_NoOp.hpp"
#include "includes/AST/AST_Compound.hpp"
#include "includes/AST/AST_Var.hpp"
#include "includes/AST/AST_Assign.hpp"
#include "includes/AST/AST_VarDecl.hpp"
#include "includes/AST/AST_If.hpp"
#include "includes/AST/AST_Else.hpp"
#include "includes/AST/AST_PrintCall.hpp"
#include "includes/AST/AST_UserDefinedFunctionCall.hpp"
#include "includes/AST/AST_DoWhile.hpp"
#include <ctype.h>
#include <iostream>
#include <sstream>


extern std::string T_INTEGER;
extern std::string T_STRING;
extern std::string T_FLOAT;
extern std::string T_PLUS;
extern std::string T_MINUS;
extern std::string T_MULTIPLY;
extern std::string T_DIVIDE;
extern std::string T_LPAREN;
extern std::string T_RPAREN;
extern std::string T_DOT;
extern std::string T_BEGIN;
extern std::string T_END;
extern std::string T_ID;
extern std::string T_SEMI;
extern std::string T_ASSIGN;
extern std::string T_DECLARE;
extern std::string T_NEWLINE;
extern std::string T_LARGER_THAN;
extern std::string T_LESS_THAN;
extern std::string T_LARGER_OR_EQUALS;
extern std::string T_LESS_OR_EQUALS;
extern std::string T_NOT_EQUALS;
extern std::string T_EQUALS;
extern std::string T_IF;
extern std::string T_ELSE;
extern std::string T_ELSE_IF;
extern std::string T_THEN;
extern std::string T_EOF;
extern std::string T_FUNCTION_CALL;
extern std::string T_FUNCTION_DEFINITION;
extern std::string T_COMMA;
extern std::string T_COLON;
extern std::string T_DO;
extern std::string T_LOOP;
extern std::string T_WHILE;

extern Scope* global_scope;

Parser::Parser(Lexer* lexer) {
    this->lexer = lexer;
    this->current_token = this->lexer->get_next_token();
};

/**
 * Takes an expected token_type as argument,
 * if the current_token->type is equal to that, it will
 * fetch the next token and assign it to this->current_token.
 *
 * If the token_type passed in is not equal to the current one, it will
 * throw an error.
 *
 * @param std::string token_type - the expected current token type.
 */
void Parser::eat(std::string token_type) {
    if (this->current_token->type == token_type)
        this->current_token = this->lexer->get_next_token();
    else
        this->error("Expected token type: `" + token_type + "`, but got `" + this->current_token->type + "`");
};

/**
 * Raises an error
 *
 * @param std::string message
 */
void Parser::error(std::string message) {
    throw std::runtime_error("[error][Parser](line=" + std::to_string(this->lexer->line) + ",pos=" + std::to_string(this->lexer->pos) + "): " + message);
};

/**
 * @return AST*
 */
AST* Parser::factor(Scope* scope) {
    Token* token = this->current_token;

    if (token->type == T_PLUS) {
        this->eat(T_PLUS);
        AST_UnaryOp* node = new AST_UnaryOp(token, this->factor(scope));
        node->scope = scope;
        return node;

    } else if (token->type == T_MINUS) {
        this->eat(T_MINUS);
        AST_UnaryOp* node = new AST_UnaryOp(token, this->factor(scope));
        node->scope = scope;
        return node;
    
    } else if (token->type == T_NOT_EQUALS) {
        this->eat(T_NOT_EQUALS);
        AST_UnaryOp* node = new AST_UnaryOp(token, this->factor(scope));
        node->scope = scope;
        return node;

    } else if (token->type == T_INTEGER) {
        this->eat(T_INTEGER);
        AST_Integer* num = new AST_Integer(token);
        num->scope = scope;
        return num;

    } else if (token->type == T_STRING) {
        this->eat(T_STRING);
        AST_Str* node = new AST_Str(token);
        node->scope = scope;
        return node;
    
    } else if (token->type == T_FLOAT) {
        this->eat(T_FLOAT);
        AST_Float* num = new AST_Float(token);
        num->scope = scope;
        return num;

    } else if (token->type == T_LPAREN) {
        this->eat(T_LPAREN);
        AST* node = this->expr(scope);
        this->eat(T_RPAREN);
        node->scope = scope;
        return node;
    } else if (token->type == T_FUNCTION_CALL) {
        AST* node = this->function_call(scope);
        node->scope = scope;
        return node;
    } else {
        AST* node = this->variable(scope);
        node->scope = scope;
        return node;
    }

    return nullptr;
};

/**
 * Handles multiplication and division
 * term : factor ((MUL | DIV) factor)*
 *
 * @return AST*
 */
AST* Parser::term(Scope* scope) {
    Token* token = nullptr;

    AST* node = this->factor(scope);
    
    while (
        this->current_token->type == T_MULTIPLY ||
        this->current_token->type == T_DIVIDE
    ) {
        token = this->current_token;

        if (token->type == T_MULTIPLY) {
            this->eat(T_MULTIPLY);
        } else if (token->type == T_DIVIDE) {
            this->eat(T_DIVIDE);
        }

        node = new AST_BinOp(node, token, this->factor(scope));
        node->scope = scope;
    }

    return node;
};

/**
 * Tells the interpreter to parse a mathematical expression,
 * arithmetic expression parsing.
 *
 * @return std::string
 */
AST* Parser::expr(Scope* scope) {
    Token* token = nullptr;

    AST* node = this->term(scope);
    
    while(
        this->current_token->type == T_PLUS ||
        this->current_token->type == T_MINUS ||
        this->current_token->type == T_NOT_EQUALS ||
        this->current_token->type == T_LESS_THAN ||
        this->current_token->type == T_LARGER_THAN ||
        this->current_token->type == T_LESS_OR_EQUALS ||
        this->current_token->type == T_LARGER_OR_EQUALS ||
        this->current_token->type == T_EQUALS
    ) {
        token = this->current_token;

        if (token->type == T_PLUS) {
            this->eat(T_PLUS);
        } else if (token->type == T_MINUS) {
            this->eat(T_MINUS);
        } else if (token->type == T_NOT_EQUALS) {
            this->eat(T_NOT_EQUALS);
        } else if (token->type == T_LESS_THAN) {
            this->eat(T_LESS_THAN);
        } else if (token->type == T_LARGER_THAN) {
            this->eat(T_LARGER_THAN);
        } else if (token->type == T_LESS_OR_EQUALS) {
            this->eat(T_LESS_OR_EQUALS);
        } else if (token->type == T_EQUALS) {
            this->eat(T_EQUALS);
        }

        node = new AST_BinOp(node, token, this->term(scope));
        node->scope = scope;
    };

    return node;
};

/**
 * @deprecated
 *
 * @return AST*
 */
AST* Parser::program(Scope* scope) {
    AST* node = this->compound_statement(scope);
    this->eat(T_DOT);

    return node;
};

/**
 * This literally tries to parse anything.
 *
 * @return AST*
 */
AST* Parser::any_statement(Scope* scope) {
    std::vector<AST*> nodes;

    nodes = this->statement_list(scope);

    AST_Compound* root = new AST_Compound();
    root->scope = scope;

    for(std::vector<AST*>::iterator it = nodes.begin(); it != nodes.end(); ++it)
        root->children.push_back((*it));

    return root;
};

/**
 * @return AST*
 */
AST* Parser::compound_statement(Scope* scope) {
    std::vector<AST*> nodes;

    this->eat(T_BEGIN);
    nodes = this->statement_list(scope);
    this->eat(T_END);

    AST_Compound* root = new AST_Compound();
    root->scope = scope;

    for(std::vector<AST*>::iterator it = nodes.begin(); it != nodes.end(); ++it)
        root->children.push_back((*it));

    return root;
};

/**
 * @return std::vector<AST*>
 */
std::vector<AST*> Parser::statement_list(Scope* scope) {
    std::vector<AST*> results;
    AST* node = this->statement(scope);

    results.push_back(node);

    while (this->current_token->type == T_COLON) {
        this->eat(T_COLON);
        results.push_back(this->statement(scope));
    }
    
    while (this->current_token->type == T_NEWLINE) {
        this->eat(T_NEWLINE);
        results.push_back(this->statement(scope));
    }

    if (this->current_token->type == T_ID)
        this->error("Something bad happened");

    return results;
};

/**
 * Parses a single statement
 *
 * @return AST*
 */
AST* Parser::statement(Scope* scope) {
    AST* node;

    if (this->current_token->type == T_FUNCTION_DEFINITION)
        return this->function_definition(scope);
    else if (this->current_token->type == T_FUNCTION_CALL)
        return this->function_call(scope);
    else if (this->current_token->type == T_ID)
        node = this->assignment_statement(scope);
    else if (this->current_token->type == T_DECLARE)
        node = this->variable_declaration(scope);
    else if (this->current_token->type == T_IF)
        node = this->if_statement(scope);
    else if (this->current_token->type == T_DO)
        return this->do_while(scope);
    else
        node = this->empty(scope);

    return node;
};

/**
 * Parses a function call
 *
 * @return AST_FunctionCall*
 */
AST_FunctionCall* Parser::function_call(Scope* scope) {
    std::vector<AST*> args;
    std::string function_name = this->current_token->value;

    this->eat(T_FUNCTION_CALL);
    this->eat(T_LPAREN);

    // if we encounter a RPAREN, we assume no arguments are specified
    // and we dont have to try and parse any arguments.
    // Because of this, functions are not required to have any arguments.
    if (this->current_token->type != T_RPAREN) {
        AST* node = this->expr(scope);
        args.push_back(node);

        while(this->current_token->type == T_COMMA) {
            this->eat(T_COMMA);
            args.push_back(this->expr(scope));
        }
    }
    
    this->eat(T_RPAREN);

    AST_FunctionCall* fc;
    AST_UserDefinedFunctionCall* udfc;

    if (function_name == "print") {
        fc = new AST_PrintCall(args);
        fc->scope = scope;
        return fc;
    } else {
        udfc = new AST_UserDefinedFunctionCall(args, function_name);
        udfc->scope = scope;
        return udfc;
    }
    
    return nullptr;
};

/**
 * Parses a function definition
 *
 * @return AST_FunctionDefinition*
 */
AST_FunctionDefinition* Parser::function_definition(Scope* scope) {
    AST_FunctionDefinition* fd = nullptr;
    AST_Return* return_node = nullptr;
    std::vector<Token*> args;
    AST_Compound* body = new AST_Compound();
    body->scope = scope;
    std::vector<AST*> nodes;
    std::string function_name;

    this->eat(T_FUNCTION_DEFINITION);
    function_name = this->current_token->value;
    Scope* new_scope = new Scope(function_name);
    new_scope->variables = scope->variables;
    new_scope->function_definitions = scope->function_definitions;
    this->eat(T_ID);
    this->eat(T_LPAREN);

    // if we encounter a RPAREN, we assume no arguments are specified
    // and we dont have to try and parse any arguments.
    // Because of this, functions are not required to have any arguments.
    if (this->current_token->type != T_RPAREN) {
        args.push_back(this->current_token);
        this->eat(T_ID);

        while(this->current_token->type == T_COMMA) {
            this->eat(T_COMMA);
            args.push_back(this->current_token);
            this->eat(T_ID);
        }
    }
    
    this->eat(T_RPAREN);
    nodes = this->statement_list(new_scope);
    scope->return_node = new_scope->return_node;
    this->eat(T_END);
    this->eat(T_FUNCTION_DEFINITION);

    for (std::vector<AST*>::iterator it = nodes.begin(); it != nodes.end(); ++it)
        body->children.push_back((*it));

    fd = new AST_FunctionDefinition(
        function_name,
        args,
        body
    );
    scope->define_function(fd);
    fd->scope = new_scope;

    return fd;
};

/**
 * Parses an assign statement
 *
 * @return AST*
 */
AST* Parser::assignment_statement(Scope* scope) {
    AST_Var* left = this->variable(scope);
    Token* token = this->current_token;
    this->eat(T_ASSIGN);
    AST* right = this->expr(scope);

    if (left->value == scope->name) {
        AST_Return* ret = new AST_Return(right);
        scope->return_node = ret;
        return ret;
    }

    AST_Assign* node = new AST_Assign(left, token, right);
    node->scope = scope;

    return node;
};

/**
 * TODO: add documentation
 */
AST* Parser::if_statement(Scope* scope) {
    std::vector<AST*> if_nodes;
    std::vector<AST_Else*> empty_else_vector;

    this->eat(T_IF);
    AST* if_expr = this->expr(scope);
    this->eat(T_THEN);
    if_nodes = this->statement_list(scope);

    AST_Compound* if_body = new AST_Compound();
    if_body->scope = scope;

    for(std::vector<AST*>::iterator it = if_nodes.begin(); it != if_nodes.end(); ++it)
        if_body->children.push_back((*it));

    std::vector<AST_Else*> elses;

    while (this->current_token->type == T_ELSE_IF) {
        this->eat(T_ELSE_IF);
        AST* else_expr = this->expr(scope);
        this->eat(T_THEN);
        AST_Compound* else_body = new AST_Compound();
        else_body->scope = scope;
        std::vector<AST*> else_nodes = this->statement_list(scope);
        for(std::vector<AST*>::iterator it = else_nodes.begin(); it != else_nodes.end(); ++it)
            else_body->children.push_back((*it));

        AST_Else* aelse = new AST_Else(else_expr, else_body, empty_else_vector);
        aelse->scope = scope; 
        elses.push_back(aelse);
    }

    if (this->current_token->type == T_ELSE) {
        this->eat(T_ELSE);
        AST_Integer* else_expr = new AST_Integer(new Token(T_INTEGER, "1"));
        else_expr->scope = scope;
        AST_Compound* else_body = new AST_Compound();
        else_body->scope = scope;
        std::vector<AST*> else_nodes = this->statement_list(scope);
        for(std::vector<AST*>::iterator it = else_nodes.begin(); it != else_nodes.end(); ++it)
            else_body->children.push_back((*it));

        AST_Else* aelse = new AST_Else(else_expr, else_body, empty_else_vector);
        aelse->scope = scope;
        elses.push_back(aelse);
    }

    this->eat(T_END);
    this->eat(T_IF);
    
    AST_If* _if = new AST_If(if_expr, if_body, elses);
    _if->scope = scope;

    return _if;
};

AST_DoWhile* Parser::do_while(Scope* scope) {
    AST* expr = nullptr;
    AST_Compound* body = new AST_Compound();
    body->scope = scope;
    std::vector<AST*> nodes;

    this->eat(T_DO);

    if (this->current_token->type == T_WHILE) {
        this->eat(T_WHILE);
        expr = this->expr(scope);
        nodes = this->statement_list(scope);
        this->eat(T_LOOP);
    } else {
        nodes = this->statement_list(scope);
        this->eat(T_LOOP);
        this->eat(T_WHILE);
        expr = this->expr(scope);
    }

    for(std::vector<AST*>::iterator it = nodes.begin(); it != nodes.end(); ++it)
        body->children.push_back((*it));

    AST_DoWhile* dw = new AST_DoWhile(expr, body);
    dw->scope = scope;

    return dw;
};

/**
 * Parses the declaration of a variable
 *
 * @return AST*
 */
AST* Parser::variable_declaration(Scope* scope) {
    std::vector<Token*> tokens;
    
    this->eat(T_DECLARE);

    tokens.push_back(this->current_token);
    this->eat(T_ID);

    while (this->current_token->type == T_COMMA) {
        this->eat(T_COMMA);
        tokens.push_back(this->current_token);
        this->eat(T_ID);
    }

    AST_VarDecl* vd = new AST_VarDecl(tokens);
    vd->scope = scope;

    return vd;
};

/**
 * Parses a variable
 *
 * @return AST_Var*
 */
AST_Var* Parser::variable(Scope* scope) {
    AST_Var* node = new AST_Var(this->current_token);
    node->scope = scope;
    this->eat(T_ID);

    return node;
};

/**
 * Parses a none operation
 *
 * @return AST*
 */
AST* Parser::empty(Scope* scope) {
    AST_NoOp* node = new AST_NoOp();
    node->scope = scope;

    return node;
};

/**
 * Parses everything, the main endpoint of the Parser.
 *
 * @return AST*
 */
AST* Parser::parse() {
    return this->any_statement(global_scope);
};
