require 'parslet'

module Channel9
  module Script
    class Parser < Parslet::Parser
      rule(:ws) { match('[ \t]').repeat(1) }
      rule(:ws?) { ws.maybe }
      rule(:br) { str("\n") }

      rule(:line_comment_text) { (br.absnt? >> any).repeat }
      rule(:multiline_comment_text) { (str('*/').absnt? >> any).repeat }

      rule(:comment) { 
        str('#') >> line_comment_text.as(:text) |
        str('//') >> line_comment_text.as(:text) |
        str('/*') >> multiline_comment_text.as(:text) >> str('*/')
      }

      # Empty whitespace is any whitespace that has no content of any sort (
      # so unlike iws, it doesn't include comments).
      # Basically, it's whitespace before something really starts.
      rule(:ews) { (ws | br).repeat(1) }
      rule(:ews?) { ews.maybe }

      # Line whitespace is any kind of whitespace on the same line (including comments).
      # Use for stuff that shouldn't go to the next line, but still expects something
      # more to complete the expression (eg. a >> lws >> '+' >> iws >> b)
      rule(:lws) { (ws | comment.as(:inline_doc)).repeat(1) }
      rule(:lws?) { lws.maybe }

      # Inner whitespace is space between elements of the same line.
      # It can include spaces, line breaks, and comments.
      rule(:iws) { (ws | br | comment.as(:inline_doc)).repeat(1) }
      rule(:iws?) { iws.maybe }

      # End of line can be any amount of whitespace, including comments,
      # and ending in either a \n or semicolon
      rule(:eol) { (ws | comment.as(:inline_doc)).repeat >> (br | str(';')) }

      rule(:symbol) {
        match('[a-zA-Z_]') >> match('[a-zA-Z0-9_]').repeat
      }

      rule(:local_var) {
        symbol.as(:local_var)
      }
      rule(:special_var) {
        str('$') >> symbol.as(:special_var)
      }

      rule(:variable) { # Note: order IS important here.
        (local_var | special_var)
      }

      rule(:nil_const) {
        str("nil").as(:nil)
      }
      rule(:undef_const) {
        str("undef").as(:undef)
      }
      rule(:true_const) {
        str("true").as(:true)
      }
      rule(:false_const) {
        str("false").as(:false)
      }

      rule(:integer_const) {
        match('[0-9]').repeat(1).as(:integer)
      }

      rule(:string_const) {
        ((str('"') >> (str('"').absnt? >> any).repeat >> str('"')) |
         (str("'") >> (str("'").absnt? >> any).repeat >> str("'")) |
         (str(":") >> (ws.absnt? >> any).repeat)
        ).as(:string)
      }

      rule(:list_const) {
        (str("[") >> iws? >> (expression >> iws? >> str(",").maybe).repeat.as(:entries) >> iws? >> str("]")).as(:list)
      }

      rule(:argdef) {
        local_var >> (iws? >> str(',') >> iws? >> local_var).repeat
      }

      rule(:argdef_list) {
        str('(') >> iws? >> argdef.maybe.as(:argdef) >> iws? >> str(')')
      }

      rule(:func_const) {
        (
          argdef_list.as(:args) >> iws? >> 
          (str("->") >> iws? >> local_var.as(:output_var) >> iws?).maybe >>
          statement_block.as(:func)
        )
      }

      rule(:const) {
        nil_const | undef_const | true_const | false_const | integer_const | string_const | list_const | func_const
      }

      rule(:prefix_op_expression) {
        ((str('!') | str('+') | str('-') | str('~')).as(:op) >> iws? >> call_expression).as(:prefix_op)
      }

      rule(:value_expression) {
        prefix_op_expression | const | variable |
        (str('(') >> iws? >> expression >> iws? >> str(')'))
      }

      rule(:args) {
        (iws? >> expression >> (iws? >> str(',') >> iws? >> expression).repeat).as(:args)
      }

      rule(:arglist) {
        str('(') >> iws? >> args.maybe >> iws? >> str(')')
      }

      rule(:line_statement) {
        statement >> eol
      }
      rule(:tail_statement) {
        statement >> iws?
      }
      rule(:statement_sequence) {
        ((ews? >> line_statement).repeat >> ews? >> tail_statement |
         (ews? >> line_statement).repeat(1) |
         ews? >> tail_statement |
         ews?).as(:sequence)
      }

      rule(:statement_block) {
        (ews? >> str("{") >> statement_sequence >> iws? >> str("}"))
      }

      rule(:member_invoke) {
        (lws? >> str('.') >> iws? >> symbol.as(:name) >> lws? >> arglist.maybe).as(:member_invoke)
      }
      rule(:array_access) {
        (lws? >> str('[') >> iws? >> expression >> iws? >> str(']')).as(:index_invoke)
      }
      rule(:value_invoke) {
        (lws? >> arglist).as(:value_invoke)
      }

      rule(:call_expression) {
        value_expression.as(:on) >> (member_invoke | array_access | value_invoke).repeat(1).as(:call) |
        value_expression
      }

      rule(:product_op_expression) {
        call_expression.as(:left) >> ((lws? >> (str('*') | str('/') | str('%')).as(:op) >> iws? >> call_expression.as(:right)).repeat(1)).as(:product) |
        call_expression
      }

      rule(:sum_op_expression) {
        product_op_expression.as(:left) >> ((lws? >> (str('+') | str('-')).as(:op) >> iws? >> product_op_expression.as(:right)).repeat(1)).as(:sum) |
        product_op_expression
      }

      rule(:bitshift_op_expression) {
        sum_op_expression.as(:left) >> ((lws? >> (str('>>') | str('<<')).as(:op) >> iws? >> sum_op_expression.as(:right)).repeat(1)).as(:bitshift) |
        sum_op_expression
      }

      rule(:relational_op_expression) {
        bitshift_op_expression.as(:left) >> ((lws? >> (str('>') | str('<') | str('<=') | str('>=')).as(:op) >> iws? >> bitshift_op_expression.as(:right)).repeat(1)).as(:relational) |
        bitshift_op_expression
      }

      rule(:equality_op_expression) {
        relational_op_expression.as(:left) >> ((lws? >> (str('==') | str('!=')).as(:op) >> iws? >> relational_op_expression.as(:right)).repeat(1)).as(:equality) |
        relational_op_expression
      }

      rule(:bitwise_op_expression) {
        equality_op_expression.as(:left) >> ((lws? >> (str('&') | str('^') | str('|')).as(:op) >> iws? >> equality_op_expression.as(:right)).repeat(1)).as(:bitwise) |
        equality_op_expression
      }

      rule(:logical_op_expression) {
        bitwise_op_expression.as(:left) >> ((lws? >> (str('&&') | str('||')).as(:op) >> iws? >> bitwise_op_expression.as(:right)).repeat(1)).as(:logical) |
        bitwise_op_expression
      }

      rule(:assignment_expression) {
        asgn_ops = (str('=') | str('+=') | str('-=') | str('*=') | str('/=') | str('%=') | str('&=') | str('^=') | str('|=') | str('<<=') | str('>>=')).as(:op)
        ((local_var.as(:assign_to) >> lws? >> asgn_ops >> iws?).repeat(1).as(:left)) >> logical_op_expression.as(:assign) |
        logical_op_expression
      }

      rule(:send_expression) {
        (variable >> (lws? >> str("<-") >> iws? >> assignment_expression).repeat(1)).as(:send) |
        assignment_expression
      }

      rule(:else_expression) {
        (str("else") >> lws? >> if_expression) |
        (str("else") >> lws? >> statement_block)
      }
      rule(:if_expression) {
        (str("if") >> lws? >> str("(") >> iws? >> expression.as(:if) >> iws? >> str(")")) >>
         iws? >> statement_block.as(:block) >>
         iws? >> else_expression.as(:else).maybe

#        (str("if") >> lws? >> str("(") >> iws? >> expression.as(:condition) >> iws? >> str(")") >> iws >> statement_block.as(:block)).as(:if) 
      }

      rule(:conditional_expression) {
        if_expression |
        send_expression
      }

      rule(:conditional_sexpression) {
        (if_expression >> iws? >>
         else_if_expression.repeat >> iws? >>
         else_expression.maybe >> iws?
        ).as(:conditional) |
        send_expression 
      }

      rule(:expression) {
        conditional_expression
      }

      rule(:statement) {
        ((ews | comment).repeat.as(:doc) >> ws? >> expression.as(:expr)).as(:statement)
      }

      rule(:script) {
        statement_sequence >> iws?
      }

      root(:script)
    end
  end
end