#include "c9/channel9.hpp"
#include "c9/script.hpp"
#include "pegtl.hh"
#include "json/json.h"

namespace Channel9 {
    namespace script {
        using namespace pegtl;

        struct expression;

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
            : one<'+','-'> {};
        struct bitshift_op
            : sor< rep<2, one<'<'>>, rep<2, one<'>'>> > {};
        struct relational_op
            : seq< one<'<','>'>, opt< one<'='> > > {};
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
        struct undefined
            : string<'u','n','d','e','f','i','n','e','d'> {};

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
            : ifmust<variable_type, ws<identifier>,
                opt< ifmust< ows<one<'='>>, ows<expression> > >
            > {};

        struct code_block
            : ifmust<
                open_block,
                until<ogws<close_block>, ogws<statement>>
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
                if_,
                ogws<one<'('>>, ogws<expression>, ogws<one<')'>>,
                ogws<code_block>,
                star<ogws<else_expr>>
            > {};
        struct else_expr
            : ifmust<
                else_,
                ogws<
                    sor<
                        code_block,
                        if_expr
                    >
                >
            > {};

        // while (expr) {
        //   expr
        // }
        struct while_expr
            : ifmust<
                while_,
                ogws<one<'('>>, ogws<expression>, ogws<one<')'>>,
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
                switch_,
                ogws<one<'('>>, ogws<expression>, ogws<one<')'>>,
                plus<ogws<case_expr>>,
                opt<ogws<else_expr>>
            > {};
        struct case_expr
            : ifmust<
                case_,
                ogws<one<'('>>, ogws<const_val>, ogws<one<')'>>,
                ogws<code_block>
            > {};

        // expr(expr, expr)
        struct func_apply
            : ifmust<
                one<'('>,
                    opt< list< ogws<expression>, ogws<one<','>> > >,
                ogws<one<')'>>
            > {};

        // expr.expr(expr, expr)
        struct method_apply
            : ifmust<
                one<'.'>,
                ogws<opt<identifier, one<':'>>, identifier>,
                opt<
                    ogws<one<'('>>,
                        opt< list< ogws<expression>, ogws<one<','>> > >,
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
        struct definition_arg
            : sor<
                ifmust<variable_type, gws<identifier>>,
                identifier
            > {};
        struct receiver_val
            : seq<
                one<'('>,
                    sor<
                        seq<
                            list< seq< ogws<definition_arg> >, ogws<one<','>> >,
                            opt< ogws<one<','>>, ogws<ifmust<one<'@'>, identifier>> >
                        >,
                        opt< ogws<ifmust<one<'@'>, identifier>> >
                    >,
                ogws<one<')'>>,
                opt< ifmust< ogws<string<'-','>'>>, ogws<identifier> > >,
                ogws<code_block>
            > {};

        // (expr)
        struct expr_val
            : ifmust< one<'('>, ogws<expression>, ogws<one<')'>> > {};

        // environment global: $var
        // other variables: var
        struct variable_val
            : seq< opt<one<'$'>>, identifier > {};

        // 42
        struct numeric_val
            : plus<digit> {};

        // "blah"
        // 'blah'
        struct string_val
            : sor<
                ifmust< one<'"'>, until<one<'"'>, any> >,
                ifmust< one<'\''>, until<one<'\''>, any> >,
                ifmust< one<':'>, identifier >
            > {};

        // @"blah" -- message id
        // @@"blah" -- protocol id
        struct message_id_val
            : ifmust<rep2<1,2, one<'@'>>, string_val> {};

        struct singleton_val
            : sor<nil, true_, false_, undefined> {};

        struct const_val
            : sor<
                singleton_val,
                string_val,
                numeric_val,
                message_id_val
            > {};

        // [expr, expr, expr]
        struct tuple_val
            : ifmust<
                one<'['>,
                opt< list< ogws<expression>, ogws<one<','>> > >,
                ogws<one<']'>>
            > {};

        // bytecode(expr, expr) {
        //   opcode arg1 arg2 arg3
        //   opcode arg1 arg2 arg3
        // }
        struct bytecode_statement
            : ifmust<
                identifier,
                opt<ws<const_val>>,
                opt<ws<const_val>>,
                opt<ws<const_val>>,
                ows< sor<eol, eof, at<close_block> > >
            > {};

        struct bytecode_val
            : ifmust<
                bytecode,
                ogws<one<'('>>,
                    opt< list< ogws<expression>, ogws<one<','>> > >,
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
        template <typename tNext, typename tOp>
        struct basic_op_expr
            : seq<tNext, star< ifmust< ows<tOp>, ogws<tNext> > > > {};

        struct return_op_expr    : basic_op_expr< value, return_op > {};
        struct send_op_expr // send has the extra continuation definition.
            : seq<return_op_expr, star< ifmust<ows<send_op>, ogws<return_op_expr>,
                opt< ows<one<':'>>, ogws<definition_arg>> > >
            > {};
        struct product_op_expr   : basic_op_expr< send_op_expr, seq< product_op, not_at<one<'='>> > > {};
        struct sum_op_expr       : basic_op_expr< product_op_expr, seq< sum_op, not_at<one<'='>> > > {};
        struct bitshift_op_expr  : basic_op_expr< sum_op_expr, seq< bitshift_op, not_at<one<'='>> > > {};
        struct relational_op_expr: basic_op_expr< bitshift_op_expr, relational_op > {};
        struct equality_op_expr  : basic_op_expr< relational_op_expr, equality_op > {};
        struct bitwise_op_expr   : basic_op_expr< equality_op_expr, seq< bitwise_op, not_at<one<'='>> > > {};
        struct logical_op_expr   : basic_op_expr< bitwise_op_expr, logical_op > {};
        struct assignment_op_expr: basic_op_expr< logical_op_expr, assignment_op > {};
        struct op_expr
            : assignment_op_expr {};

        struct expression
            : sor<
                declare_var,
                if_expr,
                while_expr,
                switch_expr,
                op_expr
            > {};

        struct grammar
            : until< ogws<eof>, ogws<statement> > {};

        void parse_file(const std::string &filename, IStream &stream)
        {
            pegtl::trace_parse_file<grammar>(true, filename);
        }
    }
}
