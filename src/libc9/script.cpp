#include <memory>
#include <vector>
#include <sstream>

#include "c9/channel9.hpp"
#include "c9/instruction.hpp"
#include "c9/istream.hpp"
#include "c9/script.hpp"
#include "pegtl.hh"
#include "json/json.h"

namespace Channel9 { namespace script
{
    template <typename tIter>
    class in_range
    {
        tIter begin_, end_;
        public:
        in_range(tIter begin, tIter end) : begin_(begin), end_(end) {}
        tIter begin() { return begin_; }
        tIter end() { return end_; }
    };
    template <typename tIter>
    in_range<tIter> in(tIter begin, tIter end) { return in_range<tIter>(begin, end); }
    template <typename tObj>
    auto reverse_in(tObj &obj) -> in_range<decltype(obj.rbegin())> { return in(obj.rbegin(), obj.rend()); }

    namespace compiler
    {
        enum class type {
            // value types
            int_number,
            float_number,
            string,
            tuple,
            variable,

            // function calls
            message_send,
            return_send,
            method_send,

            // complex values
            bytecode_block,
            receiver_block,

            // control flow
            if_block,
            else_block,
            switch_block,
            case_block,
            while_block,

            // variable declaration
            variable_declaration,

            // the file itself
            script_file,
        };

        template <type tNodeType>
        struct node;
        struct expression_any;
        typedef std::shared_ptr<expression_any> node_ptr;
        struct variable_scope;
        struct compiler_scope;

        struct compiler_state
        {
            std::vector<compiler_scope*> scope_stack;
            unsigned int current_lexical_level;
            unsigned int label_counter;

            compiler_state() : label_counter(1), current_lexical_level(0) {}

            int64_t get_lexical_level(const std::string &name);
            std::string prefix(const std::string &input)
            {
                std::stringstream str;
                str << input << "_" << label_counter++;
                return str.str();
            }
        };

        struct compiler_scope
        {
            compiler_state &state;
            variable_scope &vars;
            unsigned int lexical_level;
            bool lexical_start;

            compiler_scope(compiler_state &state, variable_scope &scope);
            ~compiler_scope()
            {
                assert(state.scope_stack.back() == this);
                state.scope_stack.pop_back();
            }
        };

        struct variable_scope
        {
            std::set<node_ptr> lexical_vars;
            std::set<node_ptr> frame_vars;

            void add_variable(node_ptr variable_decl_node);
            bool has_lexical(const std::string &name);
            bool has_frame(const std::string &name);
        };

        struct compilable
        {
            virtual void pretty_print(std::ostream &out, unsigned int indent_level) = 0;
            virtual void compile(compiler_state &state, IStream &stream)
            {
                stream.add(PUSH);
            } // TODO make = 0 when everything has implemented it.

            std::string indent(unsigned int indent_level)
            {
                return std::string(indent_level, ' ');
            }
        };

        struct expression_any
        {
            type node_type;

            // this seems redundant, but it avoids messing with the layout of a virtual base pointer.
            compilable *compiler;

            unsigned char data[0];

            template <type tNodeType>
            node<tNodeType> &get()
            {
                if (tNodeType == node_type) {
                    return *reinterpret_cast<node<tNodeType>*>(&data);
                } else {
                    throw "Incorrect type fetch attempted.";
                }
            }
            template <type tNodeType, typename tLambda>
            void when(tLambda func)
            {
                if (tNodeType == node_type) {
                    func(*reinterpret_cast<node<tNodeType>*>(&data));
                }
            }

            template <type tNodeType>
            class when_
            {
                node<tNodeType> *node_;
            public:
                when_(expression_any *maybe_node)
                 : node_(nullptr)
                {
                    // only set it if it matches.
                    if (maybe_node->node_type == tNodeType)
                    {
                        node_ = &maybe_node->get<tNodeType>();
                    }
                }

                explicit operator bool() const
                {
                    return node_;
                }
                node<tNodeType> *operator->()
                {
                    return node_;
                }
                const node<tNodeType> *operator->() const
                {
                    return node_;
                }
                node<tNodeType> &operator*()
                {
                    return *node_;
                }
                const node<tNodeType> &operator*() const
                {
                    return *node_;
                }
            };
            template <type tNodeType>
            when_<tNodeType> when()
            {
                return when_<tNodeType>(this);
            }
        };
        template <type tNodeType, typename... tArgs>
        std::shared_ptr<expression_any> make(tArgs... args)
        {
            std::shared_ptr<expression_any> ptr(
                reinterpret_cast<expression_any*>(new char[sizeof(expression_any) + sizeof(node<tNodeType>)]),
                [](expression_any *obj) {
                    reinterpret_cast<node<tNodeType>*>(obj->data)->~node<tNodeType>();
                    delete[] reinterpret_cast<char*>(obj);
                }
            );
            node<tNodeType> *inner_ptr = reinterpret_cast<node<tNodeType>*>(ptr->data);
            new(inner_ptr) node<tNodeType>(args...);
            ptr->node_type = tNodeType;
            ptr->compiler = inner_ptr;
            return ptr;
        }

        template <>
        struct node<type::float_number> : public compilable
        {
            std::string str;

            node(const std::string &str) : str(str) {}

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << str << std::endl;
            }
        };

        template <>
        struct node<type::string> : public compilable
        {
            std::string str;

            node(const std::string &str) : str(str) {}

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "\"" << str << "\"" << std::endl;
            }
            void compile(compiler_state &state, IStream &stream)
            {
                stream.add(PUSH, value(str));
            }
        };

        template <>
        struct node<type::int_number> : public compilable
        {
            std::string str;

            node(const std::string &str) : str(str) {}

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << str << std::endl;
            }
            void compile(compiler_state &state, IStream &stream)
            {
                stream.add(PUSH, value(std::atoll(str.c_str())));
            }
        };

        template <>
        struct node<type::variable> : public compilable
        {
            std::string name;

            node(const std::string &name) : name(name) {}

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "variable: " << name << std::endl;
            }
            void compile(compiler_state &state, IStream &stream)
            {
                if (name[0] == '$') {
                    stream.add(CHANNEL_SPECIAL, value(std::string(name.begin()+1, name.end())));
                } else {
                    int64_t lexical_level = state.get_lexical_level(name);
                    if (lexical_level != -1)
                    {
                        stream.add(LEXICAL_GET, value(lexical_level), value(name));
                    }
                    else if (state.scope_stack.back()->vars.has_frame(name))
                    {
                        stream.add(FRAME_GET, value(name));
                    }
                    else
                    {
                        stream.add(LOCAL_GET, value(name));
                    }
                }
            }
        };

        template <>
        struct node<type::variable_declaration> : public compilable
        {
            std::string type;
            std::string name;
            node_ptr initializer;

            node(const std::string &type, const std::string &name)
                : type(type), name(name)
            {}

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "variable_declaration:" << std::endl;
                indent_level++;
                out << indent(indent_level) << "type: " << type << std::endl
                    << indent(indent_level) << "name: " << name << std::endl;
                if (initializer)
                {
                    out << indent(indent_level) << "initializer: ";
                    initializer->compiler->pretty_print(out, indent_level);
                }
            }

            // this is for compiling an assignment that already has the
            // value placed on the stack. Ie. receiver args.
            void compile_with_assignment(compiler_state &state, IStream &stream, bool leave = true)
            {
                if (leave) {
                    stream.add(DUP_TOP);
                }
                if (type == "local")
                {
                    stream.add(LOCAL_SET, value(name));
                }
                else if (type == "frame")
                {
                    stream.add(FRAME_SET, value(name));
                }
                else if (type == "lexical")
                {
                    stream.add(LEXICAL_SET, value(int64_t(0)), value(name));
                }
            }

            // this is pretty much just for when generating the automatic
            // return at the end of a receiver.
            void compile_get(compiler_state &state, IStream &stream)
            {
                if (type == "local")
                {
                    stream.add(LOCAL_GET, value(name));
                }
                else if (type == "frame")
                {
                    stream.add(FRAME_GET, value(name));
                }
                else if (type == "lexical")
                {
                    // Note: if used anywhere else (it shouldn't be),
                    // this would have to be smarter.
                    stream.add(LEXICAL_GET, value(int64_t(0)), value(name));
                }
            }

            void compile(compiler_state &state, IStream &stream)
            {
                if (initializer)
                {
                    initializer->compiler->compile(state, stream);
                } else {
                    stream.add(PUSH, Undef);
                }
                compile_with_assignment(state, stream);
            }
        };

        void variable_scope::add_variable(node_ptr variable_decl_node)
        {
            auto &variable_decl = variable_decl_node->get<type::variable_declaration>();
            if (variable_decl.type == "lexical")
            {
                lexical_vars.insert(variable_decl_node);
            } else if (variable_decl.type == "frame") {
                frame_vars.insert(variable_decl_node);
            }
        }

        bool variable_scope::has_lexical(const std::string &name)
        {
            for (auto var : lexical_vars)
            {
                if (var->get<type::variable_declaration>().name == name)
                {
                    return true;
                }
            }
            return false;
        }

        bool variable_scope::has_frame(const std::string &name)
        {
            for (auto var : frame_vars)
            {
                if (var->get<type::variable_declaration>().name == name)
                {
                    return true;
                }
            }
            return false;
        }

        template <>
        struct node<type::receiver_block> : public compilable
        {
            std::vector<node_ptr> arguments;
            node_ptr message_arg;
            node_ptr return_arg;

            std::vector<node_ptr> statements;

            variable_scope scope;

            node()
            {
                // create a default return argument.
                return_arg = make<type::variable_declaration>("local", "return");
            }

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "receiver_block:" << std::endl;
                indent_level++;
                out << indent(indent_level) << "arguments:" << std::endl;
                indent_level++;
                for (auto argument : arguments)
                {
                    out << indent(indent_level) << "- ";
                    argument->compiler->pretty_print(out, indent_level);
                }
                indent_level--;
                if (message_arg)
                {
                    out << indent(indent_level) << "message_arg: ";
                    message_arg->compiler->pretty_print(out, indent_level);
                }
                if (return_arg)
                {
                    out << indent(indent_level) << "return_arg: ";
                    return_arg->compiler->pretty_print(out, indent_level);
                }
                out << indent(indent_level) << "body:" << std::endl;
                indent_level++;
                for (auto statement : statements)
                {
                    out << indent(indent_level) << "- ";
                    statement->compiler->pretty_print(out, indent_level);
                }
            }

            void compile(compiler_state &state, IStream &stream)
            {
                compiler_scope cscope(state, scope);
                auto prefix = state.prefix("func");

                auto done_label = prefix + ".done",
                     body_label = prefix + ".body";

                stream.add(JMP, value(done_label));
                stream.set_label(body_label);

                if (cscope.vars.lexical_vars.size() > 0)
                {
                    stream.add(LEXICAL_LINKED_SCOPE);
                }

                auto &return_arg_decl = return_arg->get<type::variable_declaration>();
                return_arg_decl.compile_with_assignment(state, stream, false);

                stream.add(MESSAGE_UNPACK,
                        value(int64_t(arguments.size())),
                        value(int64_t(0)),
                        value(int64_t(0)));

                for (auto argument : arguments)
                {
                    argument->get<type::variable_declaration>().compile_with_assignment(state, stream, false);
                }

                if (message_arg)
                {
                    message_arg->get<type::variable_declaration>().compile_with_assignment(state, stream, false);
                } else {
                    stream.add(POP); // get rid of the message
                }

                size_t not_last = statements.size();
                for (auto statement : statements)
                {
                    statement->compiler->compile(state, stream);
                    if (--not_last) {
                        stream.add(POP);
                    }
                }
                assert(not_last == 0);

                if (statements.size() == 0) {
                    // empty block needs to return something.
                    stream.add(PUSH);
                }
                return_arg_decl.compile_get(state, stream);
                stream.add(SWAP);
                stream.add(CHANNEL_RET);

                stream.set_label(done_label);
                stream.add(CHANNEL_NEW, value(body_label));
            }
        };

        template <>
        struct node<type::else_block> : public compilable
        {
            std::vector<node_ptr> statements;

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "else_block:" << std::endl;
                indent_level++;
                for (auto statement : statements)
                {
                    out << indent(indent_level) << "- ";
                    statement->compiler->pretty_print(out, indent_level);
                }
            }
        };
        template <>
        struct node<type::if_block> : public compilable
        {
            node_ptr condition;
            std::vector<node_ptr> statements;
            node_ptr else_block;

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "if_block:" << std::endl;
                indent_level++;
                out << indent(indent_level) << "condition: ";
                condition->compiler->pretty_print(out, indent_level);

                out << indent(indent_level) << "body:" << std::endl;
                indent_level++;
                for (auto statement : statements)
                {
                    out << indent(indent_level) << "- ";
                    statement->compiler->pretty_print(out, indent_level);
                }
                indent_level--;
                if (else_block)
                {
                    out << indent(indent_level);
                    else_block->compiler->pretty_print(out, indent_level);
                }
            }
        };
        template <>
        struct node<type::method_send> : public compilable
        {
            node_ptr receiver;
            std::string name;
            std::vector<node_ptr> arguments;

            node(node_ptr receiver, const std::string &name) : receiver(receiver), name(name) {};

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "method_send:" << std::endl;
                indent_level++;
                out << indent(indent_level) << "name: " << name << std::endl;
                out << indent(indent_level) << "receiver: ";
                receiver->compiler->pretty_print(out, indent_level);
                if (arguments.size() > 0)
                {
                    out << indent(indent_level) << "arguments:" << std::endl;
                    indent_level++;
                    for (auto argument : arguments)
                    {
                        out << indent(indent_level) << "- ";
                        argument->compiler->pretty_print(out, indent_level);
                    }
                }
            }
            void compile(compiler_state &state, IStream &stream)
            {
                receiver->compiler->compile(state, stream);
                for (auto argument : arguments)
                {
                    argument->compiler->compile(state, stream);
                }
                stream.add(MESSAGE_NEW, value(name), value(int64_t(0)), value(int64_t(arguments.size())));
                stream.add(CHANNEL_CALL);
                stream.add(POP);
            }
        };

        template <>
        struct node<type::return_send> : public compilable
        {
            node_ptr receiver;
            node_ptr expression;

            node(node_ptr receiver) : receiver(receiver) {};

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "return_send:" << std::endl;
                indent_level++;
                out << indent(indent_level) << "receiver: ";
                receiver->compiler->pretty_print(out, indent_level);
                out << indent(indent_level) << "expression: ";
                expression->compiler->pretty_print(out, indent_level);
            }
        };
        template <>
        struct node<type::message_send> : public compilable
        {
            node_ptr receiver;
            node_ptr expression;
            node_ptr continuation;

            node(node_ptr expression) : expression(expression) {};

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "message_send:" << std::endl;
                indent_level++;
                out << indent(indent_level) << "receiver: ";
                receiver->compiler->pretty_print(out, indent_level);
                out << indent(indent_level) << "expression: ";
                expression->compiler->pretty_print(out, indent_level);
                if (continuation)
                {
                    out << indent(indent_level) << "continuation: ";
                    continuation->compiler->pretty_print(out, indent_level);
                }
            }
            void compile(compiler_state &state, IStream &stream)
            {
                receiver->compiler->compile(state, stream);
                expression->compiler->compile(state, stream);
                stream.add(CHANNEL_CALL);
                if (continuation)
                {
                    // TODO: Implement this properly
                    stream.add(POP);
                } else {
                    stream.add(POP);
                }
            }
        };

        template <>
        struct node<type::script_file> : public compilable
        {
            std::string filename;
            std::vector<node_ptr> statements;

            variable_scope scope;

            node(const std::string &filename) : filename(filename) {}

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                indent_level++;
                out << "script_file:" << std::endl
                    << indent(indent_level) << "filename: \"" << filename << "\"" << std::endl
                    << indent(indent_level) << "body:" << std::endl;

                indent_level++;
                for (auto statement : statements)
                {
                    out << indent(indent_level) << "- ";
                    statement->compiler->pretty_print(out, indent_level);
                }
            }

            void compile(compiler_state &state, IStream &stream)
            {
                compiler_scope cscope(state, scope);
                stream.set_source_pos(SourcePos(filename, 1, 0, ""));
                stream.add(SWAP);
                stream.add(POP);

                size_t not_last = statements.size();
                for (auto statement : statements)
                {
                    statement->compiler->compile(state, stream);
                    if (--not_last) {
                        stream.add(POP);
                    }
                }
                assert(not_last == 0);

                stream.add(CHANNEL_RET);
            }
        };

        compiler_scope::compiler_scope(compiler_state &state, variable_scope &vars)
            : state(state), vars(vars), lexical_level(0), lexical_start(false)
        {
            if (state.scope_stack.size() > 0)
            {
                compiler_scope *prev = state.scope_stack.back();
                lexical_level = prev->lexical_level;

                if (vars.lexical_vars.size() > 0)
                {
                    lexical_level++;
                    lexical_start = true;
                }
            }
            state.scope_stack.push_back(this);
        }

        int64_t compiler_state::get_lexical_level(const std::string &name)
        {
            int64_t last_level = current_lexical_level;
            for (auto scope : reverse_in(scope_stack))
            {
                if (scope->vars.has_lexical(name))
                {
                    assert(last_level >= scope->lexical_level);
                    return last_level - scope->lexical_level;
                }
            }
            return -1;
        }
    }

    namespace parser
    {
        using namespace pegtl;
        using compiler::type;
        using compiler::node_ptr;

        // forward declarations
        struct expression;

        template <compiler::type tNodeType>
        struct start;
        template <compiler::type tNodeType>
        struct undo;
        template <compiler::type tNodeType>
        struct add_literal;
        struct add_variable;
        struct add_statement;

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

        struct add_message_send;
        struct add_receiver;
        struct add_cont;

        struct add_else;
        struct add_condition;

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
                ogws<ifapply< else_expr, add_else >>
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
            : ifapply< plus<digit>, add_literal<compiler::type::int_number> > {};

        // "blah"
        // 'blah'
        struct string_val
            : sor<
                enclose< one<'"'>, any, one<'"'>, add_literal<type::string> >,
                enclose< one<'\''>, any, one<'\''>, add_literal<type::string> >,
                ifmust< one<':'>, ifapply< identifier, add_literal<type::string> > >
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
        template <typename tNext, typename tOp, typename tAction = add_method_send>
        struct basic_op_expr
            : seq<tNext, star< ifmust< ows< ifapply< tOp, tAction > >, ifapply< ogws<tNext>, add_arg > > > > {};

        struct send_op_expr // send has the extra continuation definition.
            : seq<
                value,
                star<
                    ifmust<
                        ifapply< ows<send_op>, add_message_send >,
                        ifapply< ogws<value>, add_receiver >,
                        opt< ows<one<':'>>, ogws<declare_arg_var<add_cont>>>
                    >
                >
            > {};
        struct return_op_expr    : basic_op_expr< send_op_expr, return_op, add_return_send > {};
        struct product_op_expr   : basic_op_expr< return_op_expr, seq< product_op, not_at<one<'='>> > > {};
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
            : until< ogws<eof>, ifapply< ogws<statement>, add_statement > > {};

        // parser state management and actions
        struct parser_state
        {
            node_ptr root;
            std::vector<node_ptr> stack;

            parser_state(const std::string &filename)
                : root(compiler::make<compiler::type::script_file>(filename))
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

        template <compiler::type tType>
        struct start : action_base<start<tType>>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                state.push(compiler::make<tType>());
            }
        };

        template <compiler::type tType>
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
                node_ptr dec = compiler::make<type::variable_declaration>(str, "");
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
                state.push(compiler::make<type::variable_declaration>("local", str));
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

        template <compiler::type tLiteralType>
        struct add_literal : action_base<add_literal<tLiteralType>>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                state.push(compiler::make<tLiteralType>(str));
            }
        };

        struct add_variable : action_base<add_variable>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                state.push(compiler::make<type::variable>(str));
            }
        };

        struct add_statement : action_base<add_statement>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                using namespace compiler;

                node_ptr expr = state.pop();
                node_ptr into = state.stack.back();

                if (auto script = into->when<type::script_file>()) {
                    script->statements.push_back(expr);
                }
                if (auto variable_decl = into->when<type::variable_declaration>()) {
                    variable_decl->initializer = expr;
                }
                if (auto receiver = into->when<type::receiver_block>()) {
                    receiver->statements.push_back(expr);
                }
                if (auto if_block = into->when<type::if_block>()) {
                    if_block->statements.push_back(expr);
                }
                if (auto else_block = into->when<type::else_block>()) {
                    else_block->statements.push_back(expr);
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
            }
        };

        struct add_message_send : action_base<add_message_send>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr receiver = state.pop();
                state.push(compiler::make<type::message_send>(receiver));
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
                state.push(compiler::make<type::return_send>(receiver));
            }
        };
        struct add_method_send : action_base<add_method_send>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr receiver = state.pop();
                state.push(compiler::make<type::method_send>(receiver, str));
            }
        };
        struct add_func_method_send : action_base<add_func_method_send>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr receiver = state.pop();
                state.push(compiler::make<type::method_send>(receiver, "call"));
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
                if (auto receiver = into->when<type::receiver_block>()) {
                    receiver->arguments.push_back(arg);
                }
                if (auto return_send = into->when<type::return_send>()) {
                    return_send->expression = arg;
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


        void parse_file(const std::string &filename, IStream &stream)
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

            state.root->compiler->pretty_print(std::cout, 0);

            compiler::compiler_state compiler_state;
            state.root->compiler->compile(compiler_state, stream);

            Json::StyledWriter writer;
            std::cout << writer.write(to_json(stream)) << std::endl;
        }
    }
}}
