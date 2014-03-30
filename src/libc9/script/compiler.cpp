#include <memory>
#include <vector>
#include <sstream>

#include "c9/channel9.hpp"
#include "c9/instruction.hpp"
#include "c9/istream.hpp"
#include "c9/script.hpp"
#include "c9/message.hpp"
#include "script/compiler.hpp"
#include "script/parser.hpp"

#include "pegtl.hh"
#include "json/json.h"

namespace Channel9 {
    namespace script
    {
        std::string compiler_state::prefix(const std::string &input)
        {
            std::stringstream str;
            str << input << "_" << label_counter++ << ".";
            return str.str();
        }

        compiler_scope::~compiler_scope()
        {
            assert(state.scope_stack.back() == this);
            state.scope_stack.pop_back();
        }

        std::string indent(unsigned int indent_level)
        {
            return std::string(indent_level, ' ');
        }

        void node<type::tuple>::pretty_print(std::ostream &out, unsigned int indent_level)
        {
            out << "tuple:" << std::endl;
            indent_level++;
            for(auto &element : elements)
            {
                out << indent(indent_level) << "- ";
                element->compiler->pretty_print(out, indent_level);
            }
        }

        void node<type::tuple>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
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

        void node<type::float_number>::pretty_print(std::ostream &out, unsigned int indent_level)
        {
            out << str << std::endl;
        }

        void node<type::string>::pretty_print(std::ostream &out, unsigned int indent_level)
        {
            out << "\"" << str << "\"" << std::endl;
        }
        void node<type::string>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
        {
            if (leave_on_stack) {
                stream.add(PUSH, value(str));
            }
        }

        void node<type::int_number>::pretty_print(std::ostream &out, unsigned int indent_level)
        {
            out << str << std::endl;
        }
        void node<type::int_number>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
        {
            if (leave_on_stack) {
                stream.add(PUSH, value(std::atoll(str.c_str())));
            }
        }

        void node<type::message_id>::pretty_print(std::ostream &out, unsigned int indent_level)
        {
            out << "message_id: \"" << str << "\"" << std::endl;
        }
        void node<type::message_id>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
        {
            if (leave_on_stack) {
                stream.add(PUSH, value(int64_t(make_message_id(str))));
            }
        }

        void node<type::protocol_id>::pretty_print(std::ostream &out, unsigned int indent_level)
        {
            out << "protocol_id: \"" << str << "\"" << std::endl;
        }
        void node<type::protocol_id>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
        {
            if (leave_on_stack) {
                stream.add(PUSH, value(int64_t(make_protocol_id(str))));
            }
        }

        void node<type::singleton>::pretty_print(std::ostream &out, unsigned int indent_level)
        {
            out << to_json(val) << std::endl;
        }
        void node<type::singleton>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
        {
            if (leave_on_stack) {
                stream.add(PUSH, val);
            }
        }

        void node<type::variable>::pretty_print(std::ostream &out, unsigned int indent_level)
        {
            out << "variable: " << name << std::endl;
        }

        // this is for compiling an assignment that already has the
        // value placed on the stack. Ie. receiver args.
        void node<type::variable>::compile_with_assignment(compiler_state &state, IStream &stream, const std::string &scope_type, int64_t level, bool leave)
        {
            if (leave) {
                stream.add(DUP_TOP);
            }
            if (scope_type == "local")
            {
                stream.add(LOCAL_SET, value(name));
            }
            else if (scope_type == "frame")
            {
                stream.add(FRAME_SET, value(name));
            }
            else if (scope_type == "lexical")
            {
                stream.add(LEXICAL_SET, value(int64_t(level)), value(name));
            }
        }

        void node<type::variable>::compile_with_assignment(compiler_state &state, IStream &stream, bool leave)
        {
            // find out the correct type
            std::string type = "local";
            int64_t lexical_level = state.get_lexical_level(name);
            if (lexical_level != -1)
            {
                type = "lexical";
            }
            else if (state.has_frame_var(name)) {
                type = "frame";
            }
            compile_with_assignment(state, stream, type, lexical_level, leave);
        }

        void node<type::variable>::compile_get(compiler_state &state, IStream &stream)
        {
            if (name[0] == '$') {
                stream.add(CHANNEL_SPECIAL, value(std::string(name.begin()+1, name.end())));
            } else {
                int64_t lexical_level = state.get_lexical_level(name);
                if (lexical_level != -1)
                {
                    stream.add(LEXICAL_GET, value(lexical_level), value(name));
                }
                else if (state.has_frame_var(name))
                {
                    stream.add(FRAME_GET, value(name));
                }
                else
                {
                    stream.add(LOCAL_GET, value(name));
                }
            }
        }

        void node<type::variable>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
        {
            if (!leave_on_stack) {
                return; // don't bother fetching the variable
            }
            compile_get(state, stream);
        }

        void node<type::variable_declaration>::pretty_print(std::ostream &out, unsigned int indent_level)
        {
            out << "variable_declaration:" << std::endl;
            indent_level++;
            out << indent(indent_level) << "type: " << scope_type << std::endl
                << indent(indent_level) << "name: " << name << std::endl;
            if (initializer)
            {
                out << indent(indent_level) << "initializer: ";
                initializer->compiler->pretty_print(out, indent_level);
            }
        }

        void node<type::variable_declaration>::compile_with_assignment(compiler_state &state, IStream &stream, bool leave)
        {
            node<type::variable>::compile_with_assignment(state, stream, scope_type, 0, leave);
        }

        void node<type::variable_declaration>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
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

        void node<type::assignment_op>::pretty_print(std::ostream &out, unsigned int indent_level)
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

        void node<type::assignment_op>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
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

        void variable_scope::add_variable(node_ptr variable_decl_node)
        {
            auto &variable_decl = variable_decl_node->get<type::variable_declaration>();
            if (variable_decl.scope_type == "lexical")
            {
                lexical_vars.insert(variable_decl_node);
            } else if (variable_decl.scope_type == "frame") {
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

        void node<type::bytecode_block>::pretty_print(std::ostream &out, unsigned int indent_level)
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

        void node<type::bytecode_block>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
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

        void node<type::receiver_block>::pretty_print(std::ostream &out, unsigned int indent_level)
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

        void node<type::receiver_block>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
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

        void node<type::else_block>::pretty_print(std::ostream &out, unsigned int indent_level)
        {
            out << "else_block:" << std::endl;
            indent_level++;
            for (auto statement : statements)
            {
                out << indent(indent_level) << "- ";
                statement->compiler->pretty_print(out, indent_level);
            }
        }

        void node<type::else_block>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
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
        void node<type::if_block>::pretty_print(std::ostream &out, unsigned int indent_level)
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

        void node<type::if_block>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
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

        void node<type::switch_block>::pretty_print(std::ostream &out, unsigned int indent_level)
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
        void node<type::switch_block>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
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
        void node<type::case_block>::pretty_print(std::ostream &out, unsigned int indent_level)
        {
            out << "case_block:" << std::endl;
            indent_level+=2;
            for (auto &statement : statements)
            {
                out << indent(indent_level) << "- ";
                statement->compiler->pretty_print(out, indent_level);
            }
        }

        void node<type::case_block>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
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
        void node<type::while_block>::pretty_print(std::ostream &out, unsigned int indent_level)
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

        void node<type::while_block>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
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
        void node<type::equality_op>::pretty_print(std::ostream &out, unsigned int indent_level)
        {
            out << "equality_op:" << std::endl;
            indent_level++;
            out << indent(indent_level) << "op: " << op << std::endl;
            out << indent(indent_level) << "lhs: ";
            lhs->compiler->pretty_print(out, indent_level);
            out << indent(indent_level) << "rhs:";
            rhs->compiler->pretty_print(out, indent_level);
        }
        void node<type::equality_op>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
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

        void node<type::method_send>::pretty_print(std::ostream &out, unsigned int indent_level)
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
        void node<type::method_send>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
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

        void node<type::return_send>::pretty_print(std::ostream &out, unsigned int indent_level)
        {
            out << "return_send:" << std::endl;
            indent_level++;
            out << indent(indent_level) << "receiver: ";
            receiver->compiler->pretty_print(out, indent_level);
            out << indent(indent_level) << "expression: ";
            expression->compiler->pretty_print(out, indent_level);
        }
        void node<type::return_send>::compile(compiler_state &state, IStream &stream, bool /*leave_on_stack doesn't make sense, never returns*/)
        {
            receiver->compiler->compile(state, stream, true);
            expression->compiler->compile(state, stream, true);
            stream.add(CHANNEL_RET);
        }
        void node<type::message_send>::pretty_print(std::ostream &out, unsigned int indent_level)
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
        void node<type::message_send>::compile(compiler_state &state, IStream &stream, bool leave_on_stack)
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

        void node<type::script_file>::pretty_print(std::ostream &out, unsigned int indent_level)
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

        void node<type::script_file>::compile(compiler_state &state, IStream &stream, bool /*no meaning to leave_on_stack*/)
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
        bool compiler_state::has_frame_var(const std::string &name)
        {
            for (auto scope : reverse_in(scope_stack))
            {
                if (scope->vars.has_frame(name))
                {
                    return true;
                }
            }
            return false;
        }

        void compile_file(const std::string &filename, IStream &stream)
        {
            node_ptr root = parse_file(filename);

            //state.root->compiler->pretty_print(std::cout, 0);

            compiler_state compiler_state;
            root->compiler->compile(compiler_state, stream);

            /*Json::StyledWriter writer;
            std::cout << writer.write(to_json(stream)) << std::endl;*/
        }
    }
}
