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
#include "includes/AST/AST_UserDefinedFunctionCall.hpp"
#include "includes/AST/AST_DoWhile.hpp"
#include "includes/AST/AST_Empty.hpp"
#include "includes/AST/builtin_objects/AST_WScript.hpp"
#include <ctype.h>
#include <iostream>
#include <sstream>


extern Scope* global_scope;

Parser::Parser(Lexer* lexer) {
    this->lexer = lexer;
    this->current_token = this->lexer->get_next_token();
};

Parser::~Parser() {
    delete this->lexer;

    if (this->current_token != nullptr)
        delete this->current_token;
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
void Parser::eat(TokenType token_type) {
    if (this->current_token->type == token_type)
        this->current_token = this->lexer->get_next_token();
    else
        this->error("Unexpected token");
        //this->error("Expected token type: `" + token_type + "`, but got `" + this->current_token->type + "`");
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

    if (token->type == TokenType::Plus) {
        this->eat(TokenType::Plus);
        AST_UnaryOp* node = new AST_UnaryOp(token, this->factor(scope));
        node->scope = scope;
        return node;

    } else if (token->type == TokenType::Minus) {
        this->eat(TokenType::Minus);
        AST_UnaryOp* node = new AST_UnaryOp(token, this->factor(scope));
        node->scope = scope;
        return node;
    
    } else if (token->type == TokenType::Noequals) {
        this->eat(TokenType::Noequals);
        AST_UnaryOp* node = new AST_UnaryOp(token, this->factor(scope));
        node->scope = scope;
        return node;

    } else if (token->type == TokenType::Integer) {
        this->eat(TokenType::Integer);
        AST_Integer* num = new AST_Integer(token);
        num->scope = scope;
        return num;

    } else if (token->type == TokenType::String) {
        this->eat(TokenType::String);
        AST_Str* node = new AST_Str(token);
        node->scope = scope;
        return node;
    
    } else if (token->type == TokenType::Float) {
        this->eat(TokenType::Float);
        AST_Float* num = new AST_Float(token);
        num->scope = scope;
        return num;
    
    } else if (token->type == TokenType::Empty) {
        this->eat(TokenType::Empty);
        AST_Empty* emp = new AST_Empty(token);
        emp->scope = scope;
        return emp;

    } else if (token->type == TokenType::Object) {
        this->eat(TokenType::Object);
        AST_Object* obj;


        std::transform(
            token->value.begin(),
            token->value.end(),
            token->value.begin(),
            ::tolower
        );

        if (token->value == "wscript")
            obj = new AST_WScript(token);
        else
            obj = new AST_Object(token);

        obj->scope = scope;
        return obj;
    } else if (this->current_token->type == TokenType::Id || this->current_token->type == TokenType::Object || this->current_token->type == TokenType::Dot) {
        return this->id_action(scope);
    } else if (token->type == TokenType::Lparen) {
        this->eat(TokenType::Lparen);
        AST* node = this->expr(scope);
        this->eat(TokenType::Rparen);
        node->scope = scope;
        return node;
    } else if (token->type == TokenType::Function_call) {
        AST* node = this->function_call(scope);
        node->scope = scope;
        return node;
    } else {
        AST* node = this->variable(scope);
        node->scope = scope;
        return node;
    }

    return this->expr(scope);
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
        this->current_token->type == TokenType::Multiply ||
        this->current_token->type == TokenType::Divide
    ) {
        token = this->current_token;

        if (token->type == TokenType::Multiply) {
            this->eat(TokenType::Multiply);
        } else if (token->type == TokenType::Divide) {
            this->eat(TokenType::Divide);
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
 * @return AST*
 */
AST* Parser::expr(Scope* scope) {
    Token* token = nullptr;

    AST* node = this->term(scope);
    bool is_binop = false;
    
    while(
        this->current_token->type == TokenType::Plus ||
        this->current_token->type == TokenType::Minus ||
        this->current_token->type == TokenType::Noequals ||
        this->current_token->type == TokenType::Less_than ||
        this->current_token->type == TokenType::Larger_than ||
        this->current_token->type == TokenType::Less_or_equals ||
        this->current_token->type == TokenType::Larger_or_equals ||
        this->current_token->type == TokenType::Equals ||
        this->current_token->type == TokenType::Dot
    ) {
        token = this->current_token;

        if (token->type == TokenType::Plus) {
            this->eat(TokenType::Plus);
            is_binop = true;
        } else if (token->type == TokenType::Minus) {
            this->eat(TokenType::Minus);
            is_binop = true;
        } else if (token->type == TokenType::Noequals) {
            this->eat(TokenType::Noequals);
            is_binop = true;
        } else if (token->type == TokenType::Less_than) {
            this->eat(TokenType::Less_than);
            is_binop = true;
        } else if (token->type == TokenType::Larger_than) {
            this->eat(TokenType::Larger_than);
            is_binop = true;
        } else if (token->type == TokenType::Less_or_equals) {
            this->eat(TokenType::Less_or_equals);
            is_binop = true;
        } else if (token->type == TokenType::Larger_or_equals) {
            this->eat(TokenType::Larger_or_equals);
            is_binop = true;
        } else if (token->type == TokenType::Equals) {
            this->eat(TokenType::Equals);
            is_binop = true;
        } else if (token->type == TokenType::Dot) {
            this->eat(TokenType::Dot);
            is_binop = false;
        }

        if (is_binop) {
            node = new AST_BinOp(node, token, this->term(scope));
        } else {
            node = new AST_AttributeAccess(node, this->term(scope));
        }

        node->scope = scope;
    };

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

    //nodes.clear();

    return root;
};

/**
 * @return std::vector<AST*>
 */
std::vector<AST*> Parser::statement_list(Scope* scope) {
    std::vector<AST*> results;
    AST* node = this->statement(scope);

    results.push_back(node);

    while (this->current_token->type == TokenType::Colon) {
        this->eat(TokenType::Colon);
        results.push_back(this->statement(scope));
    }
    
    while (this->current_token->type == TokenType::Newline) {
        this->eat(TokenType::Newline);
        results.push_back(this->statement(scope));
    }

    return results;
};

/**
 * Parses a single statement
 *
 * @return AST*
 */
AST* Parser::statement(Scope* scope) {
    if (this->current_token->type == TokenType::Function_definition)
        return this->function_definition(scope);
    else if (this->current_token->type == TokenType::Function_call)
        return this->function_call(scope);
    else if (this->current_token->type == TokenType::Declare)
        return this->variable_declaration(scope);
    else if (this->current_token->type == TokenType::If)
        return this->if_statement(scope);
    else if (this->current_token->type == TokenType::Do)
        return this->do_while(scope);
    else if (this->current_token->type == TokenType::Id || this->current_token->type == TokenType::Object)
        return this->expr(scope);
    else
        return this->empty(scope);

    return nullptr;
};

AST* Parser::id_action(Scope* scope) {
    AST* ast;

    if (current_token->type == TokenType::Id)
        ast = this->variable(scope);
    else
        ast = this->object(scope);

    if (current_token->type == TokenType::Assign)
        return this->assignment_statement((AST_Var*)ast, scope);
    else if (current_token->type == TokenType::Dot)
        return this->attribute_access(ast, scope);

    return ast;
};

AST_Object* Parser::object(Scope* scope) {
    std::transform(
        current_token->value.begin(),
        current_token->value.end(),
        current_token->value.begin(),
        ::tolower
    );

    if (current_token->value == "wscript") { // TODO: make this more dynamic
        AST_WScript* obj = new AST_WScript(this->current_token);
        obj->scope = scope;

        this->eat(TokenType::Object);

        return obj;
    }

    return nullptr;
}

AST_AttributeAccess* Parser::attribute_access(AST* left, Scope* scope) {
    this->eat(TokenType::Dot);

    AST_AttributeAccess* attr = new AST_AttributeAccess(
        &*left,
        this->statement(scope)
    );
    attr->scope = scope;

    return attr;
}

/**
 * Parses a function call
 *
 * @return AST_FunctionCall*
 */
AST_FunctionCall* Parser::function_call(Scope* scope) {
    std::vector<AST*> args;
    std::string function_name = this->current_token->value;

    this->eat(TokenType::Function_call);
    this->eat(TokenType::Lparen);

    // if we encounter a RPAREN, we assume no arguments are specified
    // and we dont have to try and parse any arguments.
    // Because of this, functions are not required to have any arguments.
    if (this->current_token->type != TokenType::Rparen) {
        AST* node = this->expr(scope);
        args.push_back(node);

        while(this->current_token->type == TokenType::Comma) {
            this->eat(TokenType::Comma);
            args.push_back(this->expr(scope));
        }
    }
    
    this->eat(TokenType::Rparen);

    AST_UserDefinedFunctionCall* udfc = new AST_UserDefinedFunctionCall(
        args,
        function_name
    );

    udfc->scope = scope;

    return udfc;
};

/**
 * Parses a function definition
 *
 * @return AST_FunctionDefinition*
 */
AST_FunctionDefinition* Parser::function_definition(Scope* scope) {
    AST_FunctionDefinition* fd = nullptr;
    std::vector<Token*> args;
    AST_Compound* body = new AST_Compound();
    std::vector<AST*> nodes;
    std::string function_name;

    this->eat(TokenType::Function_definition);
    function_name = this->current_token->value;
    Scope* new_scope = new Scope(function_name);
    this->eat(TokenType::Id);
    this->eat(TokenType::Lparen);

    // if we encounter a RPAREN, we assume no arguments are specified
    // and we dont have to try and parse any arguments.
    // Because of this, functions are not required to have any arguments.
    if (this->current_token->type != TokenType::Rparen) {
        args.push_back(this->current_token);
        this->eat(TokenType::Id);

        while(this->current_token->type == TokenType::Comma) {
            this->eat(TokenType::Comma);
            args.push_back(this->current_token);
            this->eat(TokenType::Id);
        }
    }
    
    this->eat(TokenType::Rparen);
    nodes = this->statement_list(new_scope);
    this->eat(TokenType::End);
    this->eat(TokenType::Function_definition);

    for (std::vector<AST*>::iterator it = nodes.begin(); it != nodes.end(); ++it)
        body->children.push_back((*it));

    fd = new AST_FunctionDefinition(
        function_name,
        args,
        body
    );
    fd->parent_scope = scope;
    fd->scope = new_scope;
    body->scope = new_scope;

    return fd;
};

/**
 * Parses an assign statement
 *
 * @return AST*
 */
AST* Parser::assignment_statement(AST_Var* left, Scope* scope) {
    left->scope = scope;
    Token* token = this->current_token;
    this->eat(TokenType::Assign);
    AST* right = this->expr(scope);

    if (left->value == scope->name) {
        AST_Return* ret = new AST_Return(right);
        ret->scope = scope;
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

    this->eat(TokenType::If);
    AST* if_expr = this->expr(scope);
    this->eat(TokenType::Then);
    if_nodes = this->statement_list(scope);

    AST_Compound* if_body = new AST_Compound();
    if_body->scope = scope;

    for(std::vector<AST*>::iterator it = if_nodes.begin(); it != if_nodes.end(); ++it)
        if_body->children.push_back((*it));

    std::vector<AST_Else*> elses;

    while (this->current_token->type == TokenType::Else_if) {
        this->eat(TokenType::Else_if);
        AST* else_expr = this->expr(scope);
        this->eat(TokenType::Then);
        AST_Compound* else_body = new AST_Compound();
        else_body->scope = scope;
        std::vector<AST*> else_nodes = this->statement_list(scope);
        for(std::vector<AST*>::iterator it = else_nodes.begin(); it != else_nodes.end(); ++it)
            else_body->children.push_back((*it));

        AST_Else* aelse = new AST_Else(else_expr, else_body, empty_else_vector);
        aelse->scope = scope; 
        elses.push_back(aelse);
    }

    if (this->current_token->type == TokenType::Else) {
        this->eat(TokenType::Else);
        AST_Integer* else_expr = new AST_Integer(new Token(TokenType::Integer, "1"));
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

    this->eat(TokenType::End);
    this->eat(TokenType::If);
    
    AST_If* _if = new AST_If(if_expr, if_body, elses);
    _if->scope = scope;

    return _if;
};

AST_DoWhile* Parser::do_while(Scope* scope) {
    AST* expr = nullptr;
    AST_Compound* body = new AST_Compound();
    body->scope = scope;
    std::vector<AST*> nodes;

    this->eat(TokenType::Do);

    if (this->current_token->type == TokenType::While) {
        this->eat(TokenType::While);
        expr = this->expr(scope);
        nodes = this->statement_list(scope);
        this->eat(TokenType::Loop);
    } else {
        nodes = this->statement_list(scope);
        this->eat(TokenType::Loop);
        this->eat(TokenType::While);
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
 * TODO: implement "Dim As {type}"
 *
 * @return AST*
 */
AST* Parser::variable_declaration(Scope* scope) {
    std::vector<Token*> tokens;
    
    this->eat(TokenType::Declare);

    tokens.push_back(this->current_token);
    this->eat(TokenType::Id);

    while (this->current_token->type == TokenType::Comma) {
        this->eat(TokenType::Comma);
        tokens.push_back(this->current_token);
        this->eat(TokenType::Id);
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
    this->eat(TokenType::Id);

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
