class Class
  def ===(other)
    if (other.class == Class)
      self == other
    else
      other.kind_of?(self)
    end
  end

  def to_s
    name
  end

  def define_method(name, &method)
    __c9_add_method__(name.to_s_prim.to_message_id, method.to_proc_prim)
  end
  def define_singleton_method(name, &method)
    __c9_make_singleton__.__c9_add_method__(name.to_s_prim.to_message_id, method.to_proc_prim)
  end
end
