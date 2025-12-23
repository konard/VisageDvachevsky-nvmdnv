set pagination off
set confirm off
set print demangle on
set print asm-demangle on
set logging file /tmp/novelmind_editor_gdb.log
set logging overwrite on
set logging on
break NovelMind::editor::EditorRuntimeHost::stop
commands
  silent
  printf "[gdb] EditorRuntimeHost::stop hit\n"
  bt
  continue
end
break NovelMind::editor::EditorRuntimeHost::pause
commands
  silent
  printf "[gdb] EditorRuntimeHost::pause hit\n"
  bt
  continue
end
break NovelMind::scripting::ScriptRuntime::onSay
commands
  silent
  printf "[gdb] ScriptRuntime::onSay hit\n"
  continue
end
break NovelMind::scripting::ScriptRuntime::onGotoScene
commands
  silent
  printf "[gdb] ScriptRuntime::onGotoScene hit\n"
  continue
end
run
