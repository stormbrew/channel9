class Module
  def include(o)
    __c9_include__(o)
  end
  def alias_method(new, old)
    __c9_alias_method__(new.to_message_id, old.to_message_id)
  end

  def name
    __c9_name__
  end
  def to_sym
    __c9_name__
  end

  def const_set(const, val)
    __c9_add_constant__(const.to_s_prim, val)
  end
  def const_get(const)
    __c9_get_constant__(const.to_s_prim)
  end
  def const_defined?(const)
    __c9_get_constant__(const.to_s_prim) != undefined
  end

  def method_defined?(name)
    __c9_lookup__(name.to_s_prim.to_message_id) != undefined
  end

  def remove_method(name)
    __c9_add_method__(name.to_s_prim.to_message_id, undefined)
  end
  def undef_method(name)
    __c9_add_method__(name.to_s_prim.to_message_id, nil)
  end
end