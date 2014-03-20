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
                string<'g','l','o','b','a','l'>
            > {};

        struct bytecode
            : string<'b','y','t','e','c','o','d','e'> {};

        struct if_
            : string<'i','f'> {};
        struct else_
            : string<'e','l','s','e'> {};
        struct switch_
            : string<'s','w','i','t','c','h'> {};
        struct case_
            : string<'c','a','s','e'> {};

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
            : seq< ogws<expression>, ows< sor<eol, eof> > > {};

        // [lexical|global|local] var
        // [lexical|global|local] var = val
        struct declare_var
            : ifmust<variable_type, ogws<identifier>,
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

        // expr(expr, expr)
        struct func_apply
            : ifmust<
                one<'('>,
                    opt< list< ogws<expression>, ows<one<','>> > >,
                ogws<one<')'>>
            > {};

        // () -> {
        //   expr
        //   expr
        // }
        // (arg1, arg2) -> {
        //   expr
        //   expr
        // }
        struct receiver_val
            : ifmust<
                one<'('>,
                    opt< list< ogws<identifier>, ows<one<','>> > >,
                ogws<one<')'>>,
                ogws<string<'-','>'>>,
                ogws<identifier>,
                ogws<code_block>
            > {};

        struct variable_val
            : seq< opt<one<'$'>>, identifier > {};

        struct numeric_val
            : plus<digit> {};

        struct value
            : seq<
                sor<
                    receiver_val,
                    variable_val,
                    numeric_val
                >,
                star<
                    ows<
                        sor<
                            func_apply
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
        struct send_op_expr      : basic_op_expr< return_op_expr, send_op > {};
        struct product_op_expr   : basic_op_expr< send_op_expr, product_op > {};
        struct sum_op_expr       : basic_op_expr< product_op_expr, sum_op > {};
        struct bitshift_op_expr  : basic_op_expr< sum_op_expr, bitshift_op > {};
        struct relational_op_expr: basic_op_expr< bitshift_op_expr, relational_op > {};
        struct equality_op_expr  : basic_op_expr< relational_op_expr, equality_op > {};
        struct bitwise_op_expr   : basic_op_expr< equality_op_expr, bitwise_op > {};
        struct logical_op_expr   : basic_op_expr< bitwise_op_expr, logical_op > {};
        struct op_expr
            : logical_op_expr {};

        struct expression
            : sor<
                declare_var,
                if_expr,
                op_expr
            > {};

        struct grammar
            : star< ogws<expression> > {};

        void parse_file(const std::string &filename, IStream &stream)
        {
            pegtl::trace_parse_file<grammar>(true, filename);
        }
    }
}
