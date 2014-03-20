#include "c9/channel9.hpp"
#include "c9/script.hpp"
#include "pegtl.hh"
#include "json/json.h"

namespace Channel9 {
    namespace script {
        using namespace pegtl;

        struct comment
            : sor<
                ifmust< one<'#'>, until< eol, any > >,
                ifmust< one<'/'>, one<'/'>, until< eol, any > >,
                ifmust< one<'/'>, one<'*'>, until< seq< one<'*'>, one<'/'> >, any > >
            > {};

        struct ws
            : seq< sor< comment, blank > > {};

        struct symbol
            : seq<
                sor< alpha, one<'_'> >,
                star< sor< alnum, one<'_'> > >
              > {};

        struct method_name
            : ifmust< symbol, opt< seq< one<':'>, symbol > > > {};

        struct local_var
            : symbol {};

        struct var_type
            : sor<
                string<'f','r','a','m','e'>,
                string<'l','e','x','i','c','a','l'>,
                string<'l','o','c','a','l'>
            > {};

        struct expression;

        struct declare_var
            : seq<
                var_type, ws, symbol,
                opt< seq< opt<ws>, one<'='>, opt<ws>, expression > >
            > {};

        struct arg_declare_var
            : sor<
                seq<var_type, ws, symbol>,
                symbol
            > {};

        struct special_var
            : seq< one<'$'>, symbol > {};

        struct variable
            : sor<declare_var, local_var, special_var> {};

        struct nil_const
            : string<'n','i','l'> {};

        struct undef_const
            : string<'u','n','d','e','f'> {};

        struct true_const
            : string<'t','r','u','e'> {};

        struct false_const
            : string<'f','a','l','s','e'> {};

        struct integer_const
            : ifmust< digit, star<digit> > {};

        template <typename tStringPrefix>
        struct surrounded_string
            : ifmust< tStringPrefix, until< tStringPrefix, any > > {};

        struct string_const
            : sor<
                enclose<one<'"'>, any>,
                enclose<one<'\''>, any>,
                ifmust< one<':'>, symbol >
            > {};

        struct message_id_const
            : ifmust< one<'@'>, string_const > {};

        struct protocol_id_const
            : ifmust< string<'@', '@'>, string_const > {};

        struct list_const
            : ifmust< one<'['>, opt<ws>, opt< list<expression, one<','> > >, one<']'> > {};

        struct argdef_list
            : ifmust< one<'('>, opt<ws>,
                opt<
                    list<
                    arg_declare_var,
                    sor<
                        ifmust< one<'@'>, arg_declare_var, opt<ws> >,
                        arg_declare_var
                    >
                >,
                one<')'>
            > {};

        struct const_expr
            : sor<
                nil_const, undef_const, true_const, false_const,
                integer_const, protocol_id_const, message_id_const,
                string_const, list_const
            > {};

        struct statement_block;

        struct func
            : ifmust< argdef_list, opt<ws>,
                string<'-','>'>, opt<ws>,
                opt< seq< local_var, opt<ws> > >,
                statement_block
            > {};

        struct call_expression;

        struct prefix_op_expression
            : seq< one<'!','+','-','~'>, opt<ws>, call_expression > {};

        struct value_expression
            : sor<
                prefix_op_expression, const_expr, func, variable,
                ifmust< one<'('>, opt<ws>, expression, opt<ws>, one<')'> >
            > {};

        struct argslist
            : ifmust< one<'('>, opt<ws>, comma_list<expression>, one<')'> > {};

        struct statement;

        struct statement_sequence
            : opt< list<statement, eol> > {}

        struct statement_block
            : ifmust< one<'{'>, opt<ws>, opt< seq< eol, opt<ws> > >,
                statement_sequence,
                one<'}'>
            > {};

        struct member_invoke
            : ifmust< one<'.'>, opt<ws>, method_name, opt<ws>, opt<argslist> > {};

        struct array_access
            : ifmust< one<'['>, opt<ws>, expression, opt<ws>, one<']'> > {};

        struct value_invoke
            : argslist {};

        struct call_expression
            : ifmust< value_expression,
                star< sor< member_invoke, array_access, value_invoke > >
            > {};

        struct bytecode_instruction
            : ifmust< symbol, star< opt<ws>, const_expr> > {};

        struct bytecode_block
            : ifmust< one<'{'>, opt<ws>, opt< seq< eol, opt<ws> > >,
                star< seq< bytecode_instruction, opt<ws>, eol > >,
                opt< seq< bytecode_instruction, opt<ws> > >,
                one<'}'>
            > {};

        struct bytecode_expression
            : ifmust< string<'b','y','t','e','c','o','d','e'>, opt<ws>,
                argslist, opt<ws>,
                bytecode_block
            > {};

        template <typename tOp, typename tExpr>
        struct binop_expression
            : seq< tExpr, opt<ws>, star< seq< tOp, opt<ws>, tExpr, opt<ws> > > > {};

        struct product_op_expression
            : binop_expression< one<'*','/','%'>, bytecode_expression > {};

        struct sum_op_expression
            : binop_expression< one<'+','-'>, product_op_expression > {};

        struct bitshift_op_expression
            : binop_expression< sor< string<'>','>'>, string<'<','<'> >, sum_op_expression > {};

        struct relational_op_expression
            : binop_expression< seq< one<'<','>'>, one<'='> >, bitshift_op_expression > {};

        struct equality_op_expression
            : binop_expression< seq< one<'=','!'>, one<'='> >, relational_op_expression > {};

        struct bitwise_op_expression
            : binop_expression< one<'&','^','|'>, equality_op_expression > {};

        struct logical_op_expression
            : binop_expression< sor< string<'&','&'>, string<'|','|'> >, bitwise_op_expression > {};

        struct assignment_expression
            : seq<
                opt<
                    ifmust<
                        seq<
                            local_var, opt<ws>,
                            opt<
                                sor<
                                    one<'+','-','*','/','%','&','^'>,
                                    seq< rep2< 1,2, one<'|'> > >,
                                    sor< string<'<','<'>, string<'>','>'> >
                                >
                            >,
                            one<'='>
                        >,
                        opt<ws>
                    >
                >,
                logical_op_expression
            > {};

        struct send_expression
            : binop_expression<string<'-','>'>, assignment_expression> {};

        struct return_expression
            : seq<
                opt< seq< value_expression, opt<ws>, string<'<','-'>, opt<ws> > >,
                send_expression
            > {};

        struct if_expression;

        struct else_expression
            : ifmust<
                string<'e','l','s','e'>,
                opt<ws>,
                sor< if_expression, statement_block >
            > {};

        struct if_expression
            : ifmust<
                string<'i','f'>, opt<ws>, one<'('>, opt<ws>, expression, opt<ws>, one<')'>, opt<ws>,
                statement_block, opt<ws>,
                opt<else_expression>
            > {};

        struct while_expression
            : ifmust<
                string<'w','h','i','l','e'>, opt<ws>, one<'('>, opt<ws>, expression, opt<ws>, one<')'>, opt<ws>,
                statement_block, opt<ws>
            > {};

        struct cases
            : seq<
                plus<
                    ifmust< string<'c','a','s','e'>, opt<ws>, one<'('>, opt<ws>, const_expr, opt<ws>, one<')'>, opt<ws> >,
                    statement_block, opt<ws>
                >,
                opt<else_expression>
            > {};

        struct switch_expression
            : ifmust<
                string<'s','w','i','t','c','h'>, opt<ws>, one<'('>, opt<ws>, expression, opt<ws>, one<')'>, opt<ws>,
                cases
            > {};

        struct expression
            : sor<if_expression, while_expression, switch_expression, return_expression> {};

        struct statement
            : seq< expression, eol > {};

        struct grammar
            : seq< statement_sequence, eof > {};

        void parse_file(const std::string &filename, IStream &stream)
        {
            pegtl::trace_parse_file<grammar>(true, filename);
        }
    }
}
