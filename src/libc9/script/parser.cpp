#include <memory>
#include <vector>
#include <sstream>

#include "c9/channel9.hpp"
#include "c9/instruction.hpp"
#include "c9/istream.hpp"
#include "c9/script.hpp"
#include "c9/message.hpp"
#include "script/compiler.hpp"

#include "pegtl.hh"
#include "json/json.h"

namespace Channel9 {
    namespace script {
        using namespace pegtl;

        // forward declarations
        struct expression;

        template <type tNodeType>
        struct start;
        template <type tNodeType>
        struct undo;
        template <type tNodeType>
        struct add_literal;
        struct add_variable;
        struct add_statement;
        struct add_singleton;

        struct start_variable_declaration;
        struct add_simple_variable_declaration;
        struct add_name;

        struct start_receiver_declaration;
        struct add_arg;
        struct add_message_arg;
        struct add_return_arg;

        struct add_return_send;
        struct add_method_send;
        struct add_func_method_send; // method with no name

        struct add_equality_op;
        struct add_assignment_op;

        struct add_message_send;
        struct add_receiver;
        struct add_cont;

        struct add_case;
        struct add_else;
        struct add_condition;

        struct add_instruction;
        template <size_t tArgNum>
        struct add_instruction_arg;

        // terminals (more or less)
        struct line_comment
            : ifmust< sor< one<'#'>, string<'/','/'> >, until< at<eol>, any > > {};

        struct block_comment
            : ifmust< string<'/','*'>, until< string<'*','/'> > > {};

        struct return_op
            : string<'<','-'> {};
        struct send_op
            : string<'-','>'> {};

        struct product_op
            : one<'*','/','%'> {};
        struct sum_op
            : seq< one<'+','-'>, not_at<one<'>'>> > {};
        struct bitshift_op
            : sor< rep<2, one<'<'>>, rep<2, one<'>'>> > {};
        struct relational_op
            : seq< one<'<','>'>, sor< not_at<one<'-'>>, one<'='> > > {};
        struct equality_op
            : seq< one<'=','!'>, one<'='> > {};
        struct bitwise_op
            : one<'&','|','^'> {};
        struct logical_op
            : sor< rep<2, one<'&'>>, rep<2, one<'|'>> > {};
        struct assignment_op
            : seq<
                opt< sor<bitshift_op, bitwise_op, product_op, sum_op> >,
                one<'='>
            > {};

        struct variable_type
            : sor<
                string<'l','e','x','i','c','a','l'>,
                string<'l','o','c','a','l'>,
                string<'f','r','a','m','e'>
            > {};

        struct bytecode
            : string<'b','y','t','e','c','o','d','e'> {};

        struct nil
            : string<'n','i','l'> {};
        struct false_
            : string<'f','a','l','s','e'> {};
        struct true_
            : string<'t','r','u','e'> {};
        struct undef
            : string<'u','n','d','e','f'> {};

        struct if_
            : string<'i','f'> {};
        struct else_
            : string<'e','l','s','e'> {};
        struct switch_
            : string<'s','w','i','t','c','h'> {};
        struct case_
            : string<'c','a','s','e'> {};
        struct while_
            : string<'w','h','i','l','e'> {};

        struct open_block
            : one<'{'> {};
        struct close_block
            : one<'}'> {};

        // utilities for managing whitespace

        // this makes a rule that is greedy about prefixing
        // whitespace. That is, it will eat comments and line
        // endings.
        template <typename ...T>
        struct gws
            : seq< plus< sor<blank, eol, line_comment, block_comment> >, T... > {};
        // The optional version of gws.
        template <typename ...T>
        struct ogws
            : seq< star< sor<blank, eol, line_comment, block_comment> >, T... > {};

        // this makes a less greedy rule that only eats comments.
        template <typename ...T>
        struct ws
            : seq< plus< sor<blank, line_comment, block_comment> >, T... > {};
        // The optional version of ws.
        template <typename ...T>
        struct ows
            : seq< star< sor<blank, line_comment, block_comment> >, T... > {};

        struct statement
            : seq< ogws<expression>, ows< sor<eol, eof, at<close_block> > > > {};

        // [lexical|global|frame] var
        // [lexical|global|frame] var = val
        struct declare_var
            : seq<
                ifmust< ifapply< variable_type, start_variable_declaration>,
                    ws< ifapply< identifier, add_name > >
                >,
                opt< ifmust< ows<one<'='>>, ifapply< ows<expression>, add_statement > > >
            > {};

        // Like the above declare_var, but also allows the type to be removed.
        template <typename tAction>
        struct declare_arg_var
            : ifapply<
                sor<
                    declare_var,
                    ifmust<
                        ifapply< identifier, add_simple_variable_declaration >,
                        opt< ifmust< ows<one<'='>>, ifapply< ows<expression>, add_statement > > >
                    >
                >,
                tAction
            > {};

        struct code_block
            : ifmust<
                open_block,
                until<ogws<close_block>, ogws< ifapply< statement, add_statement > >>
            > {};

        // if (expr) {
        //   expr
        // }
        // if (expr) {
        //   expr
        // } else {
        //   expr
        // }
        // if (expr) {
        //   expr
        // } else if (expr) {
        //   expr
        // } else {
        //   expr
        // }
        struct else_expr;
        struct if_expr
            : ifmust<
                ifapply< if_, start<type::if_block> >,
                ogws<one<'('>>, ifapply< ogws<expression>, add_condition >, ogws<one<')'>>,
                ogws<code_block>,
                opt< ogws<ifapply< else_expr, add_else >> >
            > {};
        struct else_expr
            : ifmust<
                ifapply< else_, start<type::else_block> >,
                ogws<
                    sor<
                        code_block,
                        ifapply< if_expr, add_statement >
                    >
                >
            > {};

        // while (expr) {
        //   expr
        // }
        struct while_expr
            : ifmust<
                ifapply< while_, start<type::while_block> >,
                ogws<one<'('>>, ifapply< ogws<expression>, add_condition >, ogws<one<')'>>,
                ogws<code_block>
            > {};

        // switch (expr)
        // case (constval) {
        //   expr
        // } case (constval) {
        //   expr
        // } else {
        //   expr
        // }
        struct case_expr;
        struct const_val;
        struct switch_expr
            : ifmust<
                ifapply<switch_, start<type::switch_block>>,
                ogws<one<'('>>, ifapply<ogws<expression>, add_condition>, ogws<one<')'>>,
                plus< ifapply<ogws<case_expr>, add_case> >,
                opt<ogws<ifapply<else_expr, add_else>>>
            > {};
        struct case_expr
            : ifmust<
                case_,
                ifapply<seq<ogws<one<'('>>, ogws<const_val>, ogws<one<')'>>>, start<type::case_block> >,
                ogws<code_block>
            > {};

        // expr(expr, expr)
        struct func_apply
            : ifmust<
                ifapply< one<'('>, add_func_method_send >,
                    opt< list< ifapply< ogws<expression>, add_arg >, ogws<one<','>> > >,
                ogws<one<')'>>
            > {};

        // expr.expr(expr, expr)
        struct method_apply
            : ifmust<
                one<'.'>,
                ifapply< ogws<opt<identifier, one<':'>>, identifier>, add_method_send >,
                opt<
                    ogws<one<'('>>,
                        opt< list< ifapply< ogws<expression>, add_arg >, ogws<one<','>> > >,
                    ogws<one<')'>>
                >
            > {};

        // () -> {
        //   expr
        //   expr
        // }
        // (arg1, arg2) -> {
        //   expr
        //   expr
        // }
        // note: this doesn't use ifmust even though it would be good if it did
        // because it needs to be able to backtrack to expr_val. A language change
        // should be considered to clean this up.
        struct receiver_val
            : seq<
                ifapply< one<'('>, start<type::receiver_block> >,
                    sor<
                        seq<
                            list< seq< ogws<declare_arg_var<add_arg>> >, ogws<one<','>> >,
                            opt< ogws<one<','>>, ogws<ifmust<one<'@'>, declare_arg_var<add_message_arg>>> >
                        >,
                        opt< ogws<ifmust<one<'@'>, declare_arg_var<add_message_arg>>> >
                    >,
                ogws<one<')'>>,
                opt< ifmust< ogws<string<'-','>'>>, ogws<declare_arg_var<add_return_arg>> > >,
                ogws<code_block>
            > {};

        // (expr)
        // note: the really ugly ifapply undo below is because we will have tried it being
        // a receiver first, and that will have left cruft on the stack. We need to
        // remove that cruft.
        // there are probably better solutions to this particular problem, but the
        // best solution is to, once this is fully functional, rip it out and make the
        // grammar clearer and not allow this alternative formulation to occur.
        struct expr_val
            : ifmust< ifapply< one<'('>, undo<type::receiver_block> >, ogws<expression>, ogws<one<')'>> > {};

        // environment global: $var
        // other variables: var
        struct variable_val
            : ifapply< seq< opt<one<'$'>>, identifier >, add_variable > {};

        // 42
        struct numeric_val
            : ifapply< plus<digit>, add_literal<type::int_number> > {};

        // "blah"
        // 'blah'
        struct string_val
            : sor<
                enclose< one<'"'>, any, one<'"'>, add_literal<type::string> >,
                enclose< one<'\''>, any, one<'\''>, add_literal<type::string> >,
                ifmust< one<':'>, ifapply< identifier, add_literal<type::string> > >
            > {};

        // @"blah" -- message id
        struct message_id_val
            : ifmust<seq<one<'@'>,not_at<one<'@'>>>,
                sor<
                    enclose< one<'"'>, any, one<'"'>, add_literal<type::message_id> >,
                    enclose< one<'\''>, any, one<'\''>, add_literal<type::message_id> >
                >
            > {};

        // @@"blah" -- protocol id
        struct protocol_id_val
            : ifmust<string<'@','@'>,
                sor<
                    enclose< one<'"'>, any, one<'"'>, add_literal<type::protocol_id> >,
                    enclose< one<'\''>, any, one<'\''>, add_literal<type::protocol_id> >
                >
            > {};

        // nil, true, false, or undef.
        struct singleton_val
            : ifapply< sor<nil, true_, false_, undef>, add_singleton > {};

        struct const_val
            : sor<
                singleton_val,
                string_val,
                numeric_val,
                protocol_id_val,
                message_id_val
            > {};

        // [expr, expr, expr]
        struct tuple_val
            : ifmust<
                ifapply< one<'['>, start<type::tuple> >,
                opt< list< ifapply< ogws<expression>, add_arg >, ogws<one<','>> > >,
                ogws<one<']'>>
            > {};

        // bytecode(expr, expr) {
        //   opcode arg1 arg2 arg3
        //   opcode arg1 arg2 arg3
        // }
        struct bytecode_statement
            : ifmust<
                ifapply<identifier, add_instruction>,
                opt<ifapply<ws<const_val>, add_instruction_arg<0> > >,
                opt<ifapply<ws<const_val>, add_instruction_arg<1> > >,
                opt<ifapply<ws<const_val>, add_instruction_arg<2> > >,
                ows< sor<eol, eof, at<close_block> > >
            > {};

        struct bytecode_val
            : ifmust<
                ifapply< bytecode, start<type::bytecode_block> >,
                ogws<one<'('>>,
                    opt< list< ifapply< ogws<expression>, add_arg >, ogws<one<','>> > >,
                ogws<one<')'>>,
                ogws<open_block>,
                plus<ogws<bytecode_statement>>,
                ogws<close_block>
            > {};

        struct value
            : seq<
                sor<
                    singleton_val,
                    receiver_val,
                    expr_val,
                    bytecode_val,
                    variable_val,
                    numeric_val,
                    string_val,
                    tuple_val,
                    protocol_id_val,
                    message_id_val
                >,
                star<
                    ows<
                        sor<
                            func_apply,
                            method_apply
                        >
                    >
                >
            > {};

        // binary operators.
        // The tree structure of this makes operator precedence
        // work correctly.
        template <typename tNext, typename tOp, typename tAction = add_method_send>
        struct basic_op_expr
            : seq<tNext, star< ifmust< ows< ifapply< tOp, tAction > >, ifapply< ogws<tNext>, add_arg > > > > {};

        struct product_op_expr   : basic_op_expr< value, seq< product_op, not_at<one<'='>> > > {};
        struct sum_op_expr       : basic_op_expr< product_op_expr, seq< sum_op, not_at<one<'='>> > > {};
        struct bitshift_op_expr  : basic_op_expr< sum_op_expr, seq< bitshift_op, not_at<one<'='>> > > {};
        struct relational_op_expr: basic_op_expr< bitshift_op_expr, relational_op > {};
        struct equality_op_expr  : basic_op_expr< relational_op_expr, equality_op, add_equality_op > {};
        struct bitwise_op_expr   : basic_op_expr< equality_op_expr, seq< bitwise_op, not_at<one<'='>> > > {};
        struct logical_op_expr   : basic_op_expr< bitwise_op_expr, logical_op > {};
        struct assignment_op_expr: basic_op_expr< logical_op_expr, assignment_op, add_assignment_op > {};
        struct send_op_expr // send has the extra continuation definition.
            : seq<
                assignment_op_expr,
                star<
                    ifmust<
                        ifapply< ows<send_op>, add_message_send >,
                        ifapply< ogws<assignment_op_expr>, add_receiver >,
                        opt< ows<one<':'>>, ogws<declare_arg_var<add_cont>>>
                    >
                >
            > {};
        struct return_op_expr    : basic_op_expr< send_op_expr, return_op, add_return_send > {};
        struct op_expr
            : return_op_expr {};

        struct expression
            : sor<
                declare_var,
                if_expr,
                while_expr,
                switch_expr,
                op_expr
            > {};

        struct grammar
            : until< ogws<eof>, ifapply< ogws<statement>, add_statement > > {};

        // parser state management and actions
        struct parser_state
        {
            node_ptr root;
            std::vector<node_ptr> stack;

            parser_state(const std::string &filename)
                : root(make<type::script_file>(filename))
            {
                stack.push_back(root);
            }

            void push(node_ptr node)
            {
                stack.push_back(node);
            }
            node_ptr pop()
            {
                node_ptr top = stack.back();
                stack.pop_back();
                return top;
            }
        };

        template <type tType>
        struct start : action_base<start<tType>>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                state.push(make<tType>());
            }
        };

        template <type tType>
        struct undo : action_base<undo<tType>>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr last = state.stack.back();
                if (last->node_type == tType) {
                    state.stack.pop_back();
                } else {
                    throw "Unexpected problem undoing a node. This should not happen.";
                }
            }
        };

        struct start_variable_declaration : action_base<start_variable_declaration>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr dec = make<type::variable_declaration>(str, "");
                state.push(dec);
                // add it to the appropriate frame.
                for (node_ptr level : reverse_in(state.stack))
                {
                    if (auto receiver = level->when<type::receiver_block>())
                    {
                        receiver->scope.add_variable(dec);
                        break;
                    }
                    if (auto script = level->when<type::script_file>())
                    {
                        script->scope.add_variable(dec);
                        break;
                    }
                }
            }
        };
        struct add_simple_variable_declaration : action_base<add_simple_variable_declaration>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                state.push(make<type::variable_declaration>("local", str));
                // these don't need to be added because we don't add locals to the
                // table anyways.
            }
        };
        struct add_name : action_base<add_name>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                state.stack.back()->get<type::variable_declaration>().name = str;
            }
        };

        template <type tLiteralType>
        struct add_literal : action_base<add_literal<tLiteralType>>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                state.push(make<tLiteralType>(str));
            }
        };
        struct add_singleton : action_base<add_singleton>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                Value val;
                if (str == "nil")
                {
                    val = Nil;
                }
                else if (str == "false")
                {
                    val = False;
                }
                else if (str == "true")
                {
                    val = True;
                }
                else if (str == "undef")
                {
                    val = Undef;
                }
                state.push(make<type::singleton>(val));
            }
        };

        struct add_variable : action_base<add_variable>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                state.push(make<type::variable>(str));
            }
        };

        struct add_statement : action_base<add_statement>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr expr = state.pop();
                node_ptr into = state.stack.back();

                if (auto script = into->when<type::script_file>()) {
                    script->statements.push_back(expr);
                }
                else if (auto variable_decl = into->when<type::variable_declaration>()) {
                    variable_decl->initializer = expr;
                }
                else if (auto receiver = into->when<type::receiver_block>()) {
                    receiver->statements.push_back(expr);
                }
                else if (auto if_block = into->when<type::if_block>()) {
                    if_block->statements.push_back(expr);
                }
                else if (auto else_block = into->when<type::else_block>()) {
                    else_block->statements.push_back(expr);
                }
                else if (auto while_block = into->when<type::while_block>()) {
                    while_block->statements.push_back(expr);
                }
                else if (auto case_block = into->when<type::case_block>()) {
                    case_block->statements.push_back(expr);
                }
                else {
                    throw "add_statement called on unknown block type.";
                }
            }
        };

        struct add_condition : action_base<add_condition>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr expr = state.pop();
                node_ptr into = state.stack.back();

                if (auto if_block = into->when<type::if_block>()) {
                    if_block->condition = expr;
                }
                else if (auto while_block = into->when<type::while_block>()) {
                    while_block->condition = expr;
                }
                else if (auto switch_block = into->when<type::switch_block>()) {
                    switch_block->condition = expr;
                }
                else {
                    throw "add_condition called on unknown block type.";
                }
            }
        };

        struct add_case : action_base<add_case>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr case_block = state.pop();
                node_ptr condition = state.pop();
                node_ptr into = state.stack.back();

                into->get<type::switch_block>().cases.push_back(std::make_pair(condition, case_block));
            }
        };


        struct add_else : action_base<add_else>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr else_block = state.pop();
                node_ptr into = state.stack.back();

                if (auto if_block = into->when<type::if_block>()) {
                    if_block->else_block = else_block;
                }
                else if (auto switch_block = into->when<type::switch_block>()) {
                    switch_block->else_block = else_block;
                }
                else {
                    throw "add_else called on unknown block type.";
                }
            }
        };

        struct add_equality_op : action_base<add_equality_op>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr lhs = state.pop();
                state.push(make<type::equality_op>(lhs, str));
            }
        };

        struct add_assignment_op : action_base<add_assignment_op>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr lhs = state.pop();
                state.push(make<type::assignment_op>(lhs, std::string(str.begin(), str.end()-1)));
            }
        };

        struct add_message_send : action_base<add_message_send>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr receiver = state.pop();
                state.push(make<type::message_send>(receiver));
            }
        };
        struct add_receiver : action_base<add_receiver>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr receiver = state.pop();
                node_ptr message_send = state.stack.back();
                message_send->get<type::message_send>().receiver = receiver;
            }
        };
        struct add_cont : action_base<add_cont>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr continuation = state.pop();
                node_ptr message_send = state.stack.back();
                message_send->get<type::message_send>().continuation = continuation;
            }
        };
        struct add_return_send : action_base<add_return_send>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr receiver = state.pop();
                state.push(make<type::return_send>(receiver));
            }
        };
        struct add_method_send : action_base<add_method_send>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr receiver = state.pop();
                state.push(make<type::method_send>(receiver, str));
            }
        };
        struct add_func_method_send : action_base<add_func_method_send>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr receiver = state.pop();
                state.push(make<type::method_send>(receiver, "call"));
            }
        };
        struct add_arg : action_base<add_arg>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr arg = state.pop();
                node_ptr into = state.stack.back();

                if (auto method_send = into->when<type::method_send>()) {
                    method_send->arguments.push_back(arg);
                }
                else if (auto receiver = into->when<type::receiver_block>()) {
                    receiver->arguments.push_back(arg);
                }
                else if (auto return_send = into->when<type::return_send>()) {
                    return_send->expression = arg;
                }
                else if (auto equality_op = into->when<type::equality_op>()) {
                    equality_op->rhs = arg;
                }
                else if (auto assignment_op = into->when<type::assignment_op>()) {
                    assignment_op->rhs = arg;
                }
                else if (auto bytecode_block = into->when<type::bytecode_block>()) {
                    bytecode_block->arguments.push_back(arg);
                }
                else if (auto tuple = into->when<type::tuple>()) {
                    tuple->elements.push_back(arg);
                }
                else {
                    throw "add_arg called on invalid parent type.";
                }
            }
        };
        struct add_message_arg : action_base<add_arg>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr arg = state.pop();
                node_ptr into = state.stack.back();

                if (auto receiver = into->when<type::receiver_block>()) {
                    receiver->message_arg = arg;
                }
            }
        };

        struct add_instruction : action_base<add_instruction>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                auto &block = state.stack.back()->get<type::bytecode_block>();
                block.instructions.push_back(Instruction {inum(str)});
            }
        };
        template <size_t tArgNum>
        struct add_instruction_arg : action_base<add_instruction_arg<tArgNum>>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                auto val_node = state.pop();
                auto &block = state.stack.back()->get<type::bytecode_block>();

                Value val = const_node_value(val_node);

                block.instructions.back().args[tArgNum] = val;
            }
        };

        struct add_return_arg : action_base<add_arg>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr arg = state.pop();
                node_ptr into = state.stack.back();

                if (auto receiver = into->when<type::receiver_block>()) {
                    receiver->return_arg = arg;
                }
            }
        };

        node_ptr parse_file(const std::string &filename)
        {
            parser_state state(filename);
            pegtl::trace_parse_file<grammar>(false, filename, state);

            // sanity checks
            if (state.stack.size() != 1)
            {
                std::cerr << "Error: State stack was not 1. Following items on stack:" << std::endl;
                size_t n = 0;
                for (auto item : state.stack)
                {
                    std::cerr << "Item " << n++ << ":" << std::endl;
                    item->compiler->pretty_print(std::cerr, 4);
                }
                throw "Parse stack error.";
            }
            if (state.stack.front()->node_type != type::script_file)
            {
                std::cerr << "Error: Expected script file to be at the top of the stack." << std::endl
                    << "Found instead:" << std::endl;
                state.stack.front()->compiler->pretty_print(std::cerr, 4);
                throw "Parse stack error.";
            }
            return state.stack.front();
        }
    }
}
