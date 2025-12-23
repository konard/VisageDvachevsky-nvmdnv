#include <catch2/catch_test_macros.hpp>
#include "NovelMind/core/types.hpp"
#include "NovelMind/scripting/value.hpp"
#include "NovelMind/scripting/vm.hpp"
#include <cstring>

using namespace NovelMind::scripting;
using NovelMind::i32;
using NovelMind::u32;

TEST_CASE("VM VN opcodes pass args", "[scripting]") {
  SECTION("SAY uses operand text and speaker from stack") {
    VirtualMachine vm;
    std::vector<Instruction> program = {
        {OpCode::PUSH_STRING, 1},
        {OpCode::SAY, 0},
        {OpCode::HALT, 0},
    };
    std::vector<std::string> strings = {"Hello", "Hero"};
    REQUIRE(vm.load(program, strings).isOk());

    std::vector<Value> args;
    vm.registerCallback(OpCode::SAY,
                        [&args](const std::vector<Value> &in) { args = in; });

    vm.step();
    vm.step();

    REQUIRE(args.size() == 2);
    REQUIRE(asString(args[0]) == "Hello");
    REQUIRE(asString(args[1]) == "Hero");
  }

  SECTION("SHOW_CHARACTER uses id and position from stack") {
    VirtualMachine vm;
    std::vector<Instruction> program = {
        {OpCode::PUSH_STRING, 0},
        {OpCode::PUSH_INT, 2},
        {OpCode::SHOW_CHARACTER, 0},
        {OpCode::HALT, 0},
    };
    std::vector<std::string> strings = {"Alex"};
    REQUIRE(vm.load(program, strings).isOk());

    std::vector<Value> args;
    vm.registerCallback(OpCode::SHOW_CHARACTER,
                        [&args](const std::vector<Value> &in) { args = in; });

    vm.step();
    vm.step();
    vm.step();

    REQUIRE(args.size() == 2);
    REQUIRE(asString(args[0]) == "Alex");
    REQUIRE(asInt(args[1]) == 2);
  }

  SECTION("CHOICE collects count and options") {
    VirtualMachine vm;
    std::vector<Instruction> program = {
        {OpCode::PUSH_INT, 2},
        {OpCode::PUSH_STRING, 0},
        {OpCode::PUSH_STRING, 1},
        {OpCode::CHOICE, 2},
        {OpCode::HALT, 0},
    };
    std::vector<std::string> strings = {"Left", "Right"};
    REQUIRE(vm.load(program, strings).isOk());

    std::vector<Value> args;
    vm.registerCallback(OpCode::CHOICE,
                        [&args](const std::vector<Value> &in) { args = in; });

    vm.step();
    vm.step();
    vm.step();
    vm.step();

    REQUIRE(args.size() == 3);
    REQUIRE(asInt(args[0]) == 2);
    REQUIRE(asString(args[1]) == "Left");
    REQUIRE(asString(args[2]) == "Right");
  }

  SECTION("TRANSITION uses type and duration") {
    VirtualMachine vm;
    float duration = 0.5f;
    u32 durBits = 0;
    std::memcpy(&durBits, &duration, sizeof(float));
    std::vector<Instruction> program = {
        {OpCode::PUSH_INT, durBits},
        {OpCode::TRANSITION, 0},
        {OpCode::HALT, 0},
    };
    std::vector<std::string> strings = {"fade"};
    REQUIRE(vm.load(program, strings).isOk());

    std::vector<Value> args;
    vm.registerCallback(OpCode::TRANSITION,
                        [&args](const std::vector<Value> &in) { args = in; });

    vm.step();
    vm.step();

    REQUIRE(args.size() == 2);
    REQUIRE(asString(args[0]) == "fade");
    REQUIRE(asInt(args[1]) == static_cast<i32>(durBits));
  }

  SECTION("STOP_MUSIC passes optional fade duration") {
    VirtualMachine vm;
    float duration = 1.0f;
    u32 durBits = 0;
    std::memcpy(&durBits, &duration, sizeof(float));
    std::vector<Instruction> program = {
        {OpCode::PUSH_INT, durBits},
        {OpCode::STOP_MUSIC, 0},
        {OpCode::HALT, 0},
    };
    REQUIRE(vm.load(program, {}).isOk());

    std::vector<Value> args;
    vm.registerCallback(OpCode::STOP_MUSIC,
                        [&args](const std::vector<Value> &in) { args = in; });

    vm.step();
    vm.step();

    REQUIRE(args.size() == 1);
    REQUIRE(asInt(args[0]) == static_cast<i32>(durBits));
  }

  SECTION("GOTO_SCENE passes entry point") {
    VirtualMachine vm;
    std::vector<Instruction> program = {
        {OpCode::GOTO_SCENE, 123},
        {OpCode::HALT, 0},
    };
    REQUIRE(vm.load(program, {}).isOk());

    std::vector<Value> args;
    vm.registerCallback(OpCode::GOTO_SCENE,
                        [&args](const std::vector<Value> &in) { args = in; });

    vm.step();

    REQUIRE(args.size() == 1);
    REQUIRE(asInt(args[0]) == 123);
  }
}
