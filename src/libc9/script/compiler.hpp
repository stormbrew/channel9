#pragma once

#include <vector>

#include "c9/channel9.hpp"
#include "c9/script.hpp"

namespace Channel9 {
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

    namespace script
    {
        enum class type {
            // value types
            int_number,
            float_number,
            string,
            tuple,
            variable,
            message_id,
            protocol_id,

            // singletons (nil, undef, true, false)
            singleton,

            // function calls
            message_send,
            return_send,
            method_send,

            // raw operators
            equality_op,
            assignment_op,

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
            unsigned int label_counter;

            compiler_state() : label_counter(1) {}

            int64_t get_lexical_level(const std::string &name);
            bool has_frame_var(const std::string &name);
            std::string prefix(const std::string &input);
        };

        struct compiler_scope
        {
            compiler_state &state;
            variable_scope &vars;
            unsigned int lexical_level;
            bool lexical_start;

            compiler_scope(compiler_state &state, variable_scope &scope);
            ~compiler_scope();
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
            virtual void compile(compiler_state &state, IStream &stream, bool leave_on_stack = true) = 0;

            virtual ~compilable() {}
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

        Value const_node_value(node_ptr node);

        template <>
        struct node<type::tuple> : public compilable
        {
            std::vector<node_ptr> elements;

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };

        template <>
        struct node<type::float_number> : public compilable
        {
            std::string str;

            node(const std::string &str) : str(str) {}

            void pretty_print(std::ostream &out, unsigned int indent_level);
        };

        template <>
        struct node<type::string> : public compilable
        {
            std::string str;

            node(const std::string &str) : str(str) {}

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };

        template <>
        struct node<type::int_number> : public compilable
        {
            std::string str;

            node(const std::string &str) : str(str) {}

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };

        template <>
        struct node<type::message_id> : public compilable
        {
            std::string str;

            node(const std::string &str) : str(str) {}

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };

        template <>
        struct node<type::protocol_id> : public compilable
        {
            std::string str;

            node(const std::string &str) : str(str) {}

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };


        template <>
        struct node<type::singleton> : public compilable
        {
            Value val;

            node(const Value &val) : val(val) {}

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };

        template <>
        struct node<type::variable> : public compilable
        {
            std::string name;

            node(const std::string &name) : name(name) {}

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);

            // this is for compiling an assignment that already has the
            // value placed on the stack. Ie. receiver args.
            void compile_with_assignment(compiler_state &state, IStream &stream, const std::string &type, int64_t level, bool leave);
            void compile_with_assignment(compiler_state &state, IStream &stream, bool leave);
            void compile_get(compiler_state &state, IStream &stream);
        };

        template <>
        struct node<type::variable_declaration> : public node<type::variable>
        {
            std::string scope_type;
            node_ptr initializer;

            node(const std::string &scope_type, const std::string &name)
                : node<type::variable>(name), scope_type(scope_type)
            {}

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile_with_assignment(compiler_state &state, IStream &stream, bool leave);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };

        template <>
        struct node<type::assignment_op> : public compilable
        {
            node_ptr lhs;
            node_ptr rhs;
            std::string compound_op;

            node(node_ptr lhs, std::string compound_op) : lhs(lhs), compound_op(compound_op) {}

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };

        template <>
        struct node<type::bytecode_block> : public compilable
        {
            std::vector<node_ptr> arguments;
            std::vector<Instruction> instructions;

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };

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

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };

        template <>
        struct node<type::else_block> : public compilable
        {
            std::vector<node_ptr> statements;

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };
        template <>
        struct node<type::if_block> : public compilable
        {
            node_ptr condition;
            std::vector<node_ptr> statements;
            node_ptr else_block;

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };

        template <>
        struct node<type::switch_block> : public compilable
        {
            node_ptr condition;
            typedef std::pair<node_ptr, node_ptr> case_pair;
            std::vector<case_pair> cases;
            node_ptr else_block;

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };
        template <>
        struct node<type::case_block> : public compilable
        {
            std::vector<node_ptr> statements;

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };
        template <>
        struct node<type::while_block> : public compilable
        {
            node_ptr condition;
            std::vector<node_ptr> statements;

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };
        template <>
        struct node<type::equality_op> : public compilable
        {
            node_ptr lhs;
            std::string op;
            node_ptr rhs;

            node(node_ptr lhs, const std::string &op) : lhs(lhs), op(op) {};

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };

        template <>
        struct node<type::method_send> : public compilable
        {
            node_ptr receiver;
            std::string name;
            std::vector<node_ptr> arguments;

            node(node_ptr receiver, const std::string &name) : receiver(receiver), name(name) {};

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };

        template <>
        struct node<type::return_send> : public compilable
        {
            node_ptr receiver;
            node_ptr expression;

            node(node_ptr receiver) : receiver(receiver) {};

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool /*leave_on_stack doesn't make sense, never returns*/);
        };
        template <>
        struct node<type::message_send> : public compilable
        {
            node_ptr receiver;
            node_ptr expression;
            node_ptr continuation;

            node(node_ptr expression) : expression(expression) {};

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack);
        };

        template <>
        struct node<type::script_file> : public compilable
        {
            std::string filename;
            std::vector<node_ptr> statements;

            variable_scope scope;

            node(const std::string &filename) : filename(filename) {}

            void pretty_print(std::ostream &out, unsigned int indent_level);
            void compile(compiler_state &state, IStream &stream, bool /*no meaning to leave_on_stack*/);
        };
    }
}
