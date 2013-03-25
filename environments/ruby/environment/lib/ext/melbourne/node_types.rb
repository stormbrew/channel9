node_types = %w{
  method
  fbody
  cfunc
  scope
  block
  if
  case
  when
  opt_n
  while
  until
  iter
  for
  break
  next
  redo
  retry
  begin
  rescue
  resbody
  ensure
  and
  or
  not
  masgn
  lasgn
  dasgn
  dasgn_curr
  gasgn
  iasgn
  cdecl
  cvasgn
  cvdecl
  op_asgn1
  op_asgn2
  op_asgn_and
  op_asgn_or
  call
  fcall
  vcall
  super
  zsuper
  array
  zarray
  hash
  return
  yield
  lvar
  dvar
  gvar
  ivar
  const
  cvar
  nth_ref
  back_ref
  match
  match2
  match3
  lit
  str
  dstr
  xstr
  dxstr
  evstr
  dregx
  dregx_once
  args
  argscat
  argspush
  splat
  to_ary
  svalue
  block_arg
  block_pass
  defn
  defs
  alias
  valias
  undef
  class
  module
  sclass
  colon2
  colon3
  cref
  dot2
  dot3
  flip2
  flip3
  attrset
  self
  nil
  true
  false
  defined
  newline
  postexe
  dmethod
  bmethod
  memo
  ifunc
  dsym
  attrasgn
  regex
  fixnum
  number
  hexnum
  binnum
  octnum
  float
  negate
  last
  file
}

File.open("node_types.cpp", "w") do |f|
  f.puts <<EOF
/* This file is generated by node_types.rb. Do not edit. */

#include "node_types.hpp"

namespace melbourne {

  static const char node_types[] = {
EOF

  node_types.each do |type|
    f.puts("    \"#{type}\\0\"")
  end

  f.puts("  };")
  f.puts

  f.puts("  static const unsigned short node_types_offsets[] = {")
  offset = 0

  node_types.each_with_index do |type, index|
    f.puts(",") if index > 0
    f.write("    #{offset}")
    offset += node_types[index].length + 1
  end

  f.puts <<EOF

  };

  const char *get_node_type_string(enum node_type nt) {
    return node_types + node_types_offsets[nt];
  }

};  // namespace melbourne
EOF
end

File.open("node_types.hpp", "w") do |f|
  f.puts <<EOF
#ifndef MEL_NODE_TYPES_HPP
#define MEL_NODE_TYPES_HPP
/* This file is generated by node_types.rb. Do not edit. */

#ifdef __cplusplus
extern "C" {
#endif

namespace melbourne {

  enum node_type {
EOF

  node_types.each_with_index do |type, index|
    f.puts(",") if index > 0
    f.write("    NODE_#{type.upcase}")
  end

  f.puts <<EOF

  };

  const char *get_node_type_string(enum node_type nt);

};  // namespace melbourne

#ifdef __cplusplus
}  /* extern "C" */
#endif
#endif
EOF
end