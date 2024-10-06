define sl_print_expr
  print expr_println(stdout, $arg0)
end

define sl_print_expr_debug
  print expr_print_debug(stdout, $arg0)
end

define sl_continue_and_print_expr
  continue
  sl_print_expr $arg0
end

define sl_print_env
  print env_print(stdout, $arg0)
end
