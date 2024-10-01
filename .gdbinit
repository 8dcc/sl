define print_expr
  print expr_println($arg0)
end

define continue_and_printexpr
  continue
  print_expr $arg0
end
