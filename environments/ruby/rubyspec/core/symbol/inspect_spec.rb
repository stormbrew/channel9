require File.expand_path('../../../spec_helper', __FILE__)

describe "Symbol#inspect" do
  symbols = {
    :fred      => ":fred",
    :fred?     => ":fred?",
    :fred!     => ":fred!",
    :$ruby     => ":$ruby",
    :@ruby     => ":@ruby",
    :@@ruby    => ":@@ruby",
    :"$ruby!"  => ":\"$ruby!\"",
    :"$ruby?"  => ":\"$ruby?\"",
    :"@ruby!"  => ":\"@ruby!\"",
    :"@ruby?"  => ":\"@ruby?\"",
    :"@@ruby!" => ":\"@@ruby!\"",
    :"@@ruby?" => ":\"@@ruby?\"",

    :$-w       => ":$-w",
    :"$-ww"    => ":\"$-ww\"",
    :"$+"      => ":$+",
    :"$~"      => ":$~",
    :"$:"      => ":$:",
    :"$?"      => ":$?",
    :"$<"      => ":$<",
    :"$_"      => ":$_",
    :"$/"      => ":$/",
    :"$'"      => ":$'",
    :"$\""     => ":$\"",
    :"$$"      => ":$$",
    :"$."      => ":$.",
    :"$,"      => ":$,",
    :"$`"      => ":$`",
    :"$!"      => ":$!",
    :"$;"      => ":$;",
    :"$\\"     => ":$\\",
    :"$="      => ":$=",
    :"$*"      => ":$*",
    :"$>"      => ":$>",
    :"$&"      => ":$&",
    :"$@"      => ":$@",
    :"$1234"   => ":$1234",

    :-@        => ":-@",
    :+@        => ":+@",
    :%         => ":%",
    :&         => ":&",
    :*         => ":*",
    :**        => ":**",
    :"/"       => ":/",     # lhs quoted for emacs happiness
    :<         => ":<",
    :<=        => ":<=",
    :<=>       => ":<=>",
    :==        => ":==",
    :===       => ":===",
    :=~        => ":=~",
    :>         => ":>",
    :>=        => ":>=",
    :>>        => ":>>",
    :[]        => ":[]",
    :[]=       => ":[]=",
    :"\<\<"    => ":\<\<",
    :^         => ":^",
    :"`"       => ":`",     # for emacs, and justice!
    :~         => ":~",
    :|         => ":|",

    :"!"       => [":\"!\"",  ":!" ],
    :"!="      => [":\"!=\"", ":!="],
    :"!~"      => [":\"!~\"", ":!~"],
    :"\$"      => ":\"$\"", # for justice!
    :"&&"      => ":\"&&\"",
    :"'"       => ":\"\'\"",
    :","       => ":\",\"",
    :"."       => ":\".\"",
    :".."      => ":\"..\"",
    :"..."     => ":\"...\"",
    :":"       => ":\":\"",
    :"::"      => ":\"::\"",
    :";"       => ":\";\"",
    :"="       => ":\"=\"",
    :"=>"      => ":\"=>\"",
    :"\?"      => ":\"?\"", # rawr!
    :"@"       => ":\"@\"",
    :"||"      => ":\"||\"",
    :"|||"     => ":\"|||\"",
    :"++"      => ":\"++\"",

    :"\""      => ":\"\\\"\"",
    :"\"\""    => ":\"\\\"\\\"\"",

    :"9"       => ":\"9\"",
    :"foo bar" => ":\"foo bar\"",
    :"*foo"    => ":\"*foo\"",
    :"foo "    => ":\"foo \"",
    :" foo"    => ":\" foo\"",
    :" "       => ":\" \"",
  }

  ruby_version_is ""..."1.9" do
    symbols.each do |input, expected|
      expected = expected[0] if expected.is_a?(Array)
      it "returns self as a symbol literal for #{expected}" do
        input.inspect.should   == expected
      end
    end
  end

  ruby_version_is "1.9" do
    symbols.each do |input, expected|
      expected = expected[1] if expected.is_a?(Array)
      it "returns self as a symbol literal for #{expected}" do
        input.inspect.should   == expected
      end
    end
  end
end
