#include "NovelMind/scripting/vm.hpp"
#include "NovelMind/scripting/value.hpp"
#include <csignal>
#include <iostream>

using namespace NovelMind::scripting;

static int g_args_size = -1;
static std::string g_arg0;
static std::string g_arg1;

int main() {
  VirtualMachine vm;

  std::vector<Instruction> program = {
      {OpCode::PUSH_STRING, 1},
      {OpCode::SAY, 0},
      {OpCode::HALT, 0},
  };
  std::vector<std::string> strings = {"Hello", "Hero"};

  auto loadResult = vm.load(program, strings);
  if (!loadResult.isOk()) {
    std::cerr << "Failed to load VM program: " << loadResult.error() << "\n";
    return 1;
  }

  vm.registerCallback(OpCode::SAY, [](const std::vector<Value> &args) {
    g_args_size = static_cast<int>(args.size());
    if (args.size() > 0) {
      g_arg0 = asString(args[0]);
    }
    if (args.size() > 1) {
      g_arg1 = asString(args[1]);
    }
    raise(SIGTRAP);
  });

  vm.step(); // PUSH_STRING
  vm.step(); // SAY

  std::cout << "args.size=" << g_args_size << " arg0=" << g_arg0
            << " arg1=" << g_arg1 << "\n";
  return 0;
}
