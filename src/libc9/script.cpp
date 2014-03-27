#include <memory>
#include <vector>
#include <sstream>

#include "c9/channel9.hpp"
#include "c9/instruction.hpp"
#include "c9/istream.hpp"
#include "c9/script.hpp"
#include "c9/message.hpp"
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
            std::string prefix(const std::string &input)
            {
                std::stringstream str;
                str << input << "_" << label_counter++ << ".";
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
            virtual void compile(compiler_state &state, IStream &stream, bool leave_on_stack = true) = 0;

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
        struct node<type::tuple> : public compilable
        {
            std::vector<node_ptr> elements;

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "tuple:" << std::endl;
                indent_level++;
                for(auto &element : elements)
                {
                    out << indent(indent_level) << "- ";
                    element->compiler->pretty_print(out, indent_level);
                }
            }

            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                for (auto &element : reverse_in(elements))
                {
                    element->compiler->compile(state, stream, leave_on_stack);
                }
                if (leave_on_stack)
                {
                    stream.add(TUPLE_NEW, value(int64_t(elements.size())));
                }
            }
        };

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
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                if (leave_on_stack) {
                    stream.add(PUSH, value(str));
                }
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
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                if (leave_on_stack) {
                    stream.add(PUSH, value(std::atoll(str.c_str())));
                }
            }
        };

        template <>
        struct node<type::message_id> : public compilable
        {
            std::string str;

            node(const std::string &str) : str(str) {}

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "message_id: \"" << str << "\"" << std::endl;
            }
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                if (leave_on_stack) {
                    stream.add(PUSH, value(int64_t(make_message_id(str))));
                }
            }
        };

        template <>
        struct node<type::protocol_id> : public compilable
        {
            std::string str;

            node(const std::string &str) : str(str) {}

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "protocol_id: \"" << str << "\"" << std::endl;
            }
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                if (leave_on_stack) {
                    stream.add(PUSH, value(int64_t(make_protocol_id(str))));
                }
            }
        };


        template <>
        struct node<type::singleton> : public compilable
        {
            Value val;

            node(const Value &val) : val(val) {}

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << to_json(val) << std::endl;
            }
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                if (leave_on_stack) {
                    stream.add(PUSH, val);
                }
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

            // this is for compiling an assignment that already has the
            // value placed on the stack. Ie. receiver args.
            void compile_with_assignment(compiler_state &state, IStream &stream, const std::string &type, int64_t level, bool leave)
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

            void compile_with_assignment(compiler_state &state, IStream &stream, bool leave)
            {
                // find out the correct type
                std::string type = "local";
                int64_t lexical_level = state.get_lexical_level(name);
                if (lexical_level != -1)
                {
                    type = "lexical";
                }
                else if (state.scope_stack.back()->vars.has_frame(name)) {
                    type = "frame";
                }
                compile_with_assignment(state, stream, type, lexical_level, leave);
            }

            void compile_get(compiler_state &state, IStream &stream)
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

            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                if (!leave_on_stack) {
                    return; // don't bother fetching the variable
                }
                compile_get(state, stream);
            }
        };

        template <>
        struct node<type::variable_declaration> : public node<type::variable>
        {
            std::string type;
            node_ptr initializer;

            node(const std::string &type, const std::string &name)
                : node<compiler::type::variable>(name), type(type)
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

            void compile_with_assignment(compiler_state &state, IStream &stream, bool leave)
            {
                node<compiler::type::variable>::compile_with_assignment(state, stream, type, 0, leave);
            }

            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                if (initializer)
                {
                    initializer->compiler->compile(state, stream, true);
                    compile_with_assignment(state, stream, leave_on_stack);
                }
                else if (leave_on_stack) {
                    stream.add(PUSH, Undef);
                }
                // if we're not leaving it on the stack and there's no initializer,
                // we just don't need to do anything. It will have already been
                // added to the scope list anyways.
            }
        };

        template <>
        struct node<type::assignment_op> : public compilable
        {
            node_ptr lhs;
            node_ptr rhs;
            std::string compound_op;

            node(node_ptr lhs, std::string compound_op) : lhs(lhs), compound_op(compound_op) {}

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "assignment_op:" << std::endl;
                indent_level++;
                if (compound_op.size() > 0) {
                    out << indent(indent_level) << "compound_op: " << compound_op << std::endl;
                }
                out << indent(indent_level) << "lhs: ";
                lhs->compiler->pretty_print(out, indent_level);
                out << indent(indent_level) << "rhs: ";
                rhs->compiler->pretty_print(out, indent_level);
            }

            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                auto &variable = lhs->get<type::variable>();
                if (compound_op.size() > 0) {
                    variable.compile_get(state, stream);
                    rhs->compiler->compile(state, stream, true);
                    stream.add(MESSAGE_NEW, value(compound_op), value(int64_t(0)), value(int64_t(1)));
                    stream.add(CHANNEL_CALL);
                    stream.add(POP);
                }
                else {
                    rhs->compiler->compile(state, stream, true);
                }
                variable.compile_with_assignment(state, stream, leave_on_stack);
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
        struct node<type::bytecode_block> : public compilable
        {
            std::vector<node_ptr> arguments;
            std::vector<Instruction> instructions;

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "bytecode:" << std::endl;
                indent_level++;
                if (arguments.size() > 0)
                {
                    out << indent(indent_level) << "arguments:" << std::endl;
                    indent_level++;
                    for (auto &argument : arguments)
                    {
                        out << indent(indent_level) << "- ";
                        argument->compiler->pretty_print(out, indent_level);
                    }
                    indent_level--;
                }
                out << indent(indent_level) << "instructions:" << std::endl;
                indent_level++;
                for (auto &instruction : instructions)
                {
                    out << indent(indent_level) << "- " << to_json(instruction) << std::endl;
                }
            }

            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                for (auto &argument : arguments)
                {
                    argument->compiler->compile(state, stream, true);
                }
                for (auto &instruction : instructions)
                {
                    stream.add(instruction);
                }
                if (!leave_on_stack)
                {
                    stream.add(POP);
                }
            }
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

            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                if (!leave_on_stack) {
                    // this may seem extreme, but if we're not leaving this on the stack
                    // it can just go away. There should be no side-effects of defining
                    // a channel.
                    return;
                }
                compiler_scope cscope(state, scope);
                auto prefix = state.prefix("func");

                auto done_label = prefix + "done",
                     body_label = prefix + "body";

                stream.add(JMP, value(done_label));
                stream.set_label(body_label);

                if (cscope.vars.lexical_vars.size() > 0)
                {
                    stream.add(LEXICAL_LINKED_SCOPE);
                }

                auto &return_arg_decl = return_arg->get<type::variable_declaration>();
                return_arg_decl.compile_with_assignment(state, stream, false);

                if (arguments.size() > 0)
                {
                    stream.add(MESSAGE_UNPACK,
                            value(int64_t(arguments.size())),
                            value(int64_t(0)),
                            value(int64_t(0)));

                    for (auto argument : arguments)
                    {
                        argument->get<type::variable_declaration>().compile_with_assignment(state, stream, false);
                    }
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
                    statement->compiler->compile(state, stream, !(--not_last));
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

            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                size_t not_last = statements.size();
                for (auto statement : statements)
                {
                    statement->compiler->compile(state, stream, --not_last == 0 && leave_on_stack);
                }
                assert(not_last == 0);

                if (leave_on_stack && statements.size() == 0) {
                    stream.add(PUSH);
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

            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                auto prefix = state.prefix("if");
                auto done_label = prefix + "done",
                     else_label = prefix + "else";

                condition->compiler->compile(state, stream, true);
                stream.add(JMP_IF_NOT, value(else_label));

                size_t not_last = statements.size();
                for (auto statement : statements)
                {
                    statement->compiler->compile(state, stream, !(--not_last) && leave_on_stack);
                }
                assert(not_last == 0);

                if (leave_on_stack && statements.size() == 0) {
                    // empty block needs to return something.
                    stream.add(PUSH);
                }
                stream.add(JMP, value(done_label));

                stream.set_label(else_label);
                if (else_block) {
                    else_block->compiler->compile(state, stream, leave_on_stack);
                } else if (leave_on_stack) {
                    stream.add(PUSH);
                }
                stream.set_label(done_label);
            }
        };

        Value const_node_value(node_ptr node)
        {
            if (auto string_node = node->when<type::string>())
            {
                return value(string_node->str);
            }
            else if (auto int_num_node = node->when<type::int_number>())
            {
                return value(std::atoll(int_num_node->str.c_str()));
            }
            else if (auto singleton = node->when<type::singleton>())
            {
                return singleton->val;
            }
            else if (auto message_id = node->when<type::message_id>())
            {
                return value(int64_t(make_message_id(message_id->str)));
            }
            else if (auto protocol_id = node->when<type::protocol_id>())
            {
                return value(int64_t(make_protocol_id(protocol_id->str)));
            }
            else
            {
                throw "Not a constant node.";
            }
        }

        template <>
        struct node<type::switch_block> : public compilable
        {
            node_ptr condition;
            typedef std::pair<node_ptr, node_ptr> case_pair;
            std::vector<case_pair> cases;
            node_ptr else_block;

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "switch_block:" << std::endl;
                indent_level++;
                out << indent(indent_level) << "condition: ";
                condition->compiler->pretty_print(out, indent_level);
                out << indent(indent_level) << "cases:" << std::endl;
                indent_level++;
                for (auto &case_ : cases)
                {
                    out << indent(indent_level) << "- condition: ";
                    indent_level+=2;
                    case_.first->compiler->pretty_print(out, indent_level);
                    out << indent(indent_level);
                    case_.second->compiler->pretty_print(out, indent_level);
                    indent_level-=2;
                }
                indent_level--;
                if (else_block)
                {
                    out << indent(indent_level);
                    indent_level++;
                    else_block->compiler->pretty_print(out, indent_level);
                }
            }
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                auto prefix = state.prefix("switch");
                auto else_label = prefix + "else",
                     done_label = prefix + "done";

                // TODO: Make this a hash table jumper.
                condition->compiler->compile(state, stream, true);
                uint64_t idx = 0;
                std::vector<std::string> labels;
                for (auto &case_ : cases)
                {
                    std::stringstream case_label;
                    case_label << prefix << idx++;
                    labels.push_back(case_label.str());

                    stream.add(DUP_TOP);
                    stream.add(IS, const_node_value(case_.first));
                    stream.add(JMP_IF, value(case_label.str()));
                }
                stream.add(POP); // get rid of the test value, don't need it anymore.
                stream.add(JMP, value(else_label));

                idx = 0;
                for (auto &case_ : cases)
                {
                    stream.set_label(labels[idx++]);
                    stream.add(POP); // also get rid of test value
                    case_.second->compiler->compile(state, stream, leave_on_stack);
                    stream.add(JMP, value(done_label));
                }
                stream.set_label(else_label);
                if (else_block)
                {
                    else_block->compiler->compile(state, stream, leave_on_stack);
                }
                else if (leave_on_stack)
                {
                    stream.add(PUSH);
                }
                stream.set_label(done_label);
            }
        };
        template <>
        struct node<type::case_block> : public compilable
        {
            std::vector<node_ptr> statements;

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "case_block:" << std::endl;
                indent_level+=2;
                for (auto &statement : statements)
                {
                    out << indent(indent_level) << "- ";
                    statement->compiler->pretty_print(out, indent_level);
                }
            }

            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                size_t not_last = statements.size();
                for (auto statement : statements)
                {
                    statement->compiler->compile(state, stream, !(--not_last) && leave_on_stack);
                }
                assert(not_last == 0);

                if (leave_on_stack && statements.size() == 0) {
                    // empty block needs to return something.
                    stream.add(PUSH);
                }

            }
        };
        template <>
        struct node<type::while_block> : public compilable
        {
            node_ptr condition;
            std::vector<node_ptr> statements;

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "while_block:" << std::endl;
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
            }

            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                auto prefix = state.prefix("while");
                auto done_label = prefix + "done",
                     start_label = prefix + "start";

                if (leave_on_stack)
                {
                    // this makes it so that if we're leaving the result
                    // of the last iteration on the stack, the first
                    // iteration has something to pop off.
                    stream.add(PUSH);
                }
                stream.set_label(start_label);
                condition->compiler->compile(state, stream, true);
                stream.add(JMP_IF_NOT, value(done_label));
                if (leave_on_stack)
                {
                    // pop off the result of the last iteration, only
                    // the final one leaves something behind.
                    stream.add(POP);
                }

                size_t not_last = statements.size();
                for (auto statement : statements)
                {
                    statement->compiler->compile(state, stream, !(--not_last) && leave_on_stack);
                }
                assert(not_last == 0);

                if (leave_on_stack && statements.size() == 0) {
                    // empty block needs to return something.
                    stream.add(PUSH);
                }

                stream.add(JMP, value(start_label));
                stream.set_label(done_label);
            }
        };
        template <>
        struct node<type::equality_op> : public compilable
        {
            node_ptr lhs;
            std::string op;
            node_ptr rhs;

            node(node_ptr lhs, const std::string &op) : lhs(lhs), op(op) {};

            void pretty_print(std::ostream &out, unsigned int indent_level)
            {
                out << "equality_op:" << std::endl;
                indent_level++;
                out << indent(indent_level) << "op: " << op << std::endl;
                out << indent(indent_level) << "lhs: ";
                lhs->compiler->pretty_print(out, indent_level);
                out << indent(indent_level) << "rhs:";
                rhs->compiler->pretty_print(out, indent_level);
            }
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                lhs->compiler->compile(state, stream, true);
                rhs->compiler->compile(state, stream, true);
                if (op == "==")
                {
                    stream.add(IS_EQ);
                }
                else if (op == "!=")
                {
                    stream.add(IS_NOT_EQ);
                }
                else
                {
                    throw "Invalid operator.";
                }
                if (!leave_on_stack) {
                    stream.add(POP);
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
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                receiver->compiler->compile(state, stream, true);
                for (auto argument : arguments)
                {
                    argument->compiler->compile(state, stream, true);
                }
                stream.add(MESSAGE_NEW, value(name), value(int64_t(0)), value(int64_t(arguments.size())));
                stream.add(CHANNEL_CALL);
                stream.add(POP);
                if (!leave_on_stack) {
                    stream.add(POP);
                }
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
            void compile(compiler_state &state, IStream &stream, bool /*leave_on_stack doesn't make sense, never returns*/)
            {
                receiver->compiler->compile(state, stream, true);
                expression->compiler->compile(state, stream, true);
                stream.add(CHANNEL_RET);
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
            void compile(compiler_state &state, IStream &stream, bool leave_on_stack)
            {
                receiver->compiler->compile(state, stream, true);
                expression->compiler->compile(state, stream, true);
                stream.add(CHANNEL_CALL);
                if (continuation)
                {
                    continuation->get<type::variable_declaration>().compile_with_assignment(state, stream, false);
                } else {
                    stream.add(POP);
                }
                if (!leave_on_stack) {
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

            void compile(compiler_state &state, IStream &stream, bool /*no meaning to leave_on_stack*/)
            {
                compiler_scope cscope(state, scope);
                stream.set_source_pos(SourcePos(filename, 1, 0, ""));
                stream.add(SWAP);
                stream.add(POP);

                size_t not_last = statements.size();
                for (auto statement : statements)
                {
                    statement->compiler->compile(state, stream, !(--not_last));
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
            int64_t last_level = -1;
            for (auto scope : reverse_in(scope_stack))
            {
                if (last_level == -1)
                {
                    last_level = scope->lexical_level;
                }
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
                state.push(compiler::make<type::singleton>(val));
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
                state.push(compiler::make<type::equality_op>(lhs, str));
            }
        };

        struct add_assignment_op : action_base<add_assignment_op>
        {
            static void apply(const std::string &str, parser_state &state)
            {
                node_ptr lhs = state.pop();
                state.push(compiler::make<type::assignment_op>(lhs, std::string(str.begin(), str.end()-1)));
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

                Value val = compiler::const_node_value(val_node);

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

            //state.root->compiler->pretty_print(std::cout, 0);

            compiler::compiler_state compiler_state;
            state.root->compiler->compile(compiler_state, stream);

            /*Json::StyledWriter writer;
            std::cout << writer.write(to_json(stream)) << std::endl;*/
        }
    }
}}
