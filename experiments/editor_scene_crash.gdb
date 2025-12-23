set pagination off
set confirm off
set print demangle on
set print asm-demangle on
set logging file /tmp/novelmind_editor_crash.log
set logging overwrite on
set logging enabled on
break __cxa_throw
commands
  silent
  printf "[gdb] __cxa_throw\n"
  bt
  continue
end
break NovelMind::editor::qt::NMSceneViewPanel::createObject
commands
  silent
  printf "[gdb] createObject hit\n"
  bt
  continue
end
break NovelMind::editor::qt::NMSceneViewPanel::onSceneObjectSelected
commands
  silent
  printf "[gdb] onSceneObjectSelected hit\n"
  bt
  continue
end
break NovelMind::editor::qt::NMSceneGraphicsScene::addSceneObject
commands
  silent
  printf "[gdb] addSceneObject hit\n"
  bt
  continue
end
catch signal SIGSEGV
run
