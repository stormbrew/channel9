class Module
  def include(o)
    __c9_include__(o)
  end
  def alias_method(new, old)
    __c9_alias_method__(new, old)
  end

  def name
    __c9_name__
  end

  def const_set(const, val)
    __c9_add_constant__(const.to_s_prim, val)
  end
  def const_get(const)
    __c9_get_constant__(const.to_s_prim)
  end
end