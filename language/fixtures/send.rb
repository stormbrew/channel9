module LangSendSpecs
  module_function

  def fooM0; 100 end
  def fooM1(a); [a]; end
  def fooM2(a,b); [a,b]; end
  def fooM3(a,b,c); [a,b,c]; end
  def fooM4(a,b,c,d); [a,b,c,d]; end
  def fooM5(a,b,c,d,e); [a,b,c,d,e]; end
  def fooM0O1(a=1); [a]; end
  def fooM1O1(a,b=1); [a,b]; end
  def fooM2O1(a,b,c=1); [a,b,c]; end
  def fooM3O1(a,b,c,d=1); [a,b,c,d]; end
  def fooM4O1(a,b,c,d,e=1); [a,b,c,d,e]; end
  def fooM0O2(a=1,b=2); [a,b]; end
  def fooM0R(*r); r; end
  def fooM1R(a, *r); [a, r]; end
  def fooM0O1R(a=1, *r); [a, r]; end
  def fooM1O1R(a, b=1, *r); [a, b, r]; end

  def one(a); a; end
  def oneb(a,&b); [a,yield(b)]; end
  def twob(a,b,&c); [a,b,yield(c)]; end
  def makeproc(&b) b end

  def yield_now; yield; end

  def double(x); x * 2 end
  def weird_parens
    # means double((5).to_s)
    # NOT   (double(5)).to_s
    double (5).to_s
  end

  def rest_len(*a); a.size; end

  def self.twos(a,b,*c)
    [c.size, c.last]
  end

  class PrivateSetter
    attr_reader :foo
    attr_writer :foo
    private :foo=

      def call_self_foo_equals(value)
        self.foo = value
      end

    def call_self_foo_equals_masgn(value)
      a, self.foo = 1, value
    end
  end
  
  class PrivateGetter
    attr_reader :foo
    private :foo

    def call_self_foo
      self.foo
    end

    def call_self_foo_or_equals(value)
      self.foo ||= 6
    end
  end

  class AttrSet
    attr_reader :result
    def []=(a, b, c, d); @result = [a,b,c,d]; end
  end

  class ToProc
    def initialize(val)
      @val = val
    end

    def to_proc
      Proc.new { @val }
    end
  end

  class ToAry
    def initialize(obj)
      @obj = obj
    end

    def to_ary
      @obj
    end
  end
end

def lang_send_rest_len(*a)
  a.size
end
