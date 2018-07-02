/***---NATIVES: PROTOTYPES---***/
/***AUTOGENERATED DO NOT MANUALLY EDIT***/

NATIVE(add, VARIADIC);
NATIVE(and,VARIADIC);
NATIVE(break,0);
NATIVE(catch,3);
NATIVE(channel_close,1);
NATIVE(channel_control,2);
NATIVE(channel_flush,1);
NATIVE(channel_getline,1);
NATIVE(channel_read,2);
NATIVE(channel_read_hex,2);
NATIVE(channel_write,2);
NATIVE(channel_write_hex,2);
NATIVE(continue,0);
NATIVE(dict, VARIADIC);
NATIVE(dict_del,2);
NATIVE(dict_get,2);
NATIVE(dict_has,2);
NATIVE(dict_keys,1);
NATIVE(dict_set,3);
NATIVE(dict_set_all,2);
NATIVE(dict_size,1);
NATIVE(dir_create,1);
NATIVE(dir_delete,1);
NATIVE(dir_open,1);
NATIVE(disk_sync,0);
NATIVE(div, VARIADIC);
NATIVE(eq, 2);
NATIVE(error, 1);
NATIVE(eval,1);
NATIVE(eval_up,2);
NATIVE(file_copy,2);
NATIVE(file_delete,1);
NATIVE(file_getcwd,0);
NATIVE(file_open,2);
NATIVE(file_read,1);
NATIVE(file_read_hex,1);
NATIVE(file_run,1);
NATIVE(file_setcwd,1);
NATIVE(file_stat,1);
NATIVE(file_write,2);
NATIVE(file_write_hex,2);
NATIVE(foreach,3);
NATIVE(frame_cmd_up,1);
NATIVE(frame_cmds,0);
NATIVE(frame_count,0);
NATIVE(from_radix_i32,2);
NATIVE(global_delete, 1);
NATIVE(global_get, 1);
NATIVE(global_set, 2);
NATIVE(gt, 2);
NATIVE(gte, 2);
NATIVE(hex_decode,1);
NATIVE(hex_encode,1);
NATIVE(hex_reverse,1);
NATIVE(hex_zeroes,1);
NATIVE(if,VARIADIC);
NATIVE(info_about_cmd,1);
NATIVE(info_channel_drivers, 0);
NATIVE(info_channels, 0);
NATIVE(info_cmds, 0);
NATIVE(info_cmds_count, 0);
NATIVE(info_globals, 0);
NATIVE(info_vars, 0);
NATIVE(is_bool, 1);
NATIVE(is_false, 1);
NATIVE(is_hex,1);
NATIVE(is_int, 1);
NATIVE(is_null,1);
NATIVE(is_true, 1);
NATIVE(list, VARIADIC);
NATIVE(list_add, 2);
NATIVE(list_at, 2);
NATIVE(list_concat, VARIADIC);
NATIVE(list_find,2);
NATIVE(list_length, 1);
NATIVE(list_remove, 2);
NATIVE(list_reverse, 1);
NATIVE(list_shuffle,1);
NATIVE(list_sort, 1);
NATIVE(loop,1);
NATIVE(lt, 2);
NATIVE(lte, 2);
NATIVE(math_and_u32,2);
NATIVE(math_clz_u32,1);
NATIVE(math_not_u32,1);
NATIVE(math_or_u32,2);
NATIVE(math_shl_u32,2);
NATIVE(math_xor_u32,2);
NATIVE(mod, 2);
NATIVE(mul, VARIADIC);
NATIVE(null,0);
NATIVE(nvl, VARIADIC);
NATIVE(or,VARIADIC);
NATIVE(pass,1);
NATIVE(platform_type,0);
NATIVE(proc,3);
NATIVE(proc_delete,1);
NATIVE(proc_meta_delete,2);
NATIVE(proc_meta_get,2);
NATIVE(proc_meta_keys,1);
NATIVE(proc_meta_set,3);
NATIVE(puts, 1);
NATIVE(puts_nonewline, 1);
NATIVE(rand_hex,1);
NATIVE(rand_u32,0);
NATIVE(rand_u32_uniform,1);
NATIVE(repl_history_add,1);
NATIVE(repl_history_clear,0);
NATIVE(repl_history_get,0);
NATIVE(repl_history_max_get,0);
NATIVE(repl_history_max_set,1);
NATIVE(repl_prompt_get,0);
NATIVE(repl_prompt_set,1);
NATIVE(repl_read,0);
NATIVE(repl_read_script_get,0);
NATIVE(repl_read_script_set,1);
NATIVE(return,1);
NATIVE(set, 2);
NATIVE(set_up, 3);
NATIVE(string_casecmp, 2);
NATIVE(string_char_at,2);
NATIVE(string_chr,1);
NATIVE(string_cmp, 2);
NATIVE(string_concat, VARIADIC);
NATIVE(string_ends_with, 2);
NATIVE(string_find,2);
NATIVE(string_find_at,3);
NATIVE(string_find_rev,2);
NATIVE(string_is_blank, 1);
NATIVE(string_is_digits, 1);
NATIVE(string_left,2);
NATIVE(string_length, 1);
NATIVE(string_lower,1);
NATIVE(string_replace,3);
NATIVE(string_skip,2);
NATIVE(string_starts_with, 2);
NATIVE(string_sub,3);
NATIVE(string_trim,1);
NATIVE(string_upper,1);
NATIVE(sub, VARIADIC);
NATIVE(sys_at_exit,1);
NATIVE(sys_at_exit_list,0);
NATIVE(sys_at_exit_remove,1);
NATIVE(sys_delenv,1);
NATIVE(sys_environ,0);
NATIVE(sys_exit,VARIADIC);
NATIVE(sys_no_exit,1);
NATIVE(sys_run,1);
NATIVE(sys_setenv,2);
NATIVE(time_local,1);
NATIVE(time_make_local,1);
NATIVE(time_now,0);
NATIVE(time_startup,0);
NATIVE(time_utc,1);
NATIVE(to_hex,1);
NATIVE(to_radix_i32,2);
NATIVE(token_classify,1);
NATIVE(token_quote,1);
NATIVE(token_unquote,1);
NATIVE(unsupported_parse_dump,1);
NATIVE(var_del, 1);
NATIVE(var_get, 1);
NATIVE(var_get_up, 2);
NATIVE(var_has, 1);
NATIVE(while,2);

/***AUTOGENERATED DO NOT MANUALLY EDIT***/
