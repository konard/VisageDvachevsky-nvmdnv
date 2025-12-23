#include <catch2/catch_test_macros.hpp>
#include "NovelMind/scripting/vm.hpp"

using namespace NovelMind::scripting;

TEST_CASE("VM initial state", "[scripting]")
{
    VirtualMachine vm;

    REQUIRE_FALSE(vm.isRunning());
    REQUIRE_FALSE(vm.isPaused());
    REQUIRE_FALSE(vm.isWaiting());
}

TEST_CASE("VM load empty program fails", "[scripting]")
{
    VirtualMachine vm;

    auto result = vm.load({}, {});
    REQUIRE(result.isError());
}

TEST_CASE("VM load and run simple program", "[scripting]")
{
    VirtualMachine vm;

    std::vector<Instruction> program = {
        {OpCode::PUSH_INT, 42},
        {OpCode::HALT, 0}
    };

    auto result = vm.load(program, {});
    REQUIRE(result.isOk());

    vm.run();
    REQUIRE(vm.isHalted());
}

TEST_CASE("VM arithmetic operations", "[scripting]")
{
    VirtualMachine vm;

    std::vector<Instruction> program = {
        {OpCode::PUSH_INT, 10},
        {OpCode::PUSH_INT, 5},
        {OpCode::ADD, 0},
        {OpCode::STORE_VAR, 0},  // Store to "result"
        {OpCode::HALT, 0}
    };

    std::vector<std::string> strings = {"result"};

    auto result = vm.load(program, strings);
    REQUIRE(result.isOk());

    vm.run();

    auto val = vm.getVariable("result");
    REQUIRE(std::holds_alternative<NovelMind::i32>(val));
    REQUIRE(std::get<NovelMind::i32>(val) == 15);
}

TEST_CASE("VM subtraction", "[scripting]")
{
    VirtualMachine vm;

    std::vector<Instruction> program = {
        {OpCode::PUSH_INT, 20},
        {OpCode::PUSH_INT, 8},
        {OpCode::SUB, 0},
        {OpCode::STORE_VAR, 0},
        {OpCode::HALT, 0}
    };

    vm.load(program, {"result"});
    vm.run();

    auto val = vm.getVariable("result");
    REQUIRE(std::get<NovelMind::i32>(val) == 12);
}

TEST_CASE("VM multiplication", "[scripting]")
{
    VirtualMachine vm;

    std::vector<Instruction> program = {
        {OpCode::PUSH_INT, 6},
        {OpCode::PUSH_INT, 7},
        {OpCode::MUL, 0},
        {OpCode::STORE_VAR, 0},
        {OpCode::HALT, 0}
    };

    vm.load(program, {"result"});
    vm.run();

    auto val = vm.getVariable("result");
    REQUIRE(std::get<NovelMind::i32>(val) == 42);
}

TEST_CASE("VM comparison operations", "[scripting]")
{
    VirtualMachine vm;

    std::vector<Instruction> program = {
        {OpCode::PUSH_INT, 5},
        {OpCode::PUSH_INT, 5},
        {OpCode::EQ, 0},
        {OpCode::STORE_VAR, 0},
        {OpCode::HALT, 0}
    };

    vm.load(program, {"equal"});
    vm.run();

    auto val = vm.getVariable("equal");
    REQUIRE(std::holds_alternative<bool>(val));
    REQUIRE(std::get<bool>(val) == true);
}

TEST_CASE("VM conditional jump", "[scripting]")
{
    VirtualMachine vm;

    std::vector<Instruction> program = {
        {OpCode::PUSH_BOOL, 1},      // true
        {OpCode::JUMP_IF, 4},        // Jump to instruction 4 if true
        {OpCode::PUSH_INT, 0},       // This should be skipped
        {OpCode::JUMP, 5},
        {OpCode::PUSH_INT, 1},       // This should execute
        {OpCode::STORE_VAR, 0},
        {OpCode::HALT, 0}
    };

    vm.load(program, {"result"});
    vm.run();

    auto val = vm.getVariable("result");
    REQUIRE(std::get<NovelMind::i32>(val) == 1);
}

TEST_CASE("VM flags", "[scripting]")
{
    VirtualMachine vm;

    vm.load({{OpCode::HALT, 0}}, {});

    vm.setFlag("test_flag", true);
    REQUIRE(vm.getFlag("test_flag") == true);

    vm.setFlag("test_flag", false);
    REQUIRE(vm.getFlag("test_flag") == false);
}

TEST_CASE("VM variables", "[scripting]")
{
    VirtualMachine vm;

    vm.load({{OpCode::HALT, 0}}, {});

    vm.setVariable("int_var", NovelMind::i32{100});
    vm.setVariable("str_var", std::string{"hello"});
    vm.setVariable("bool_var", true);

    REQUIRE(std::get<NovelMind::i32>(vm.getVariable("int_var")) == 100);
    REQUIRE(std::get<std::string>(vm.getVariable("str_var")) == "hello");
    REQUIRE(std::get<bool>(vm.getVariable("bool_var")) == true);
}

TEST_CASE("VM pause and resume", "[scripting]")
{
    VirtualMachine vm;

    std::vector<Instruction> program = {
        {OpCode::NOP, 0},
        {OpCode::NOP, 0},
        {OpCode::HALT, 0}
    };

    vm.load(program, {});
    vm.step();
    vm.pause();

    REQUIRE(vm.isPaused());

    vm.resume();
    REQUIRE_FALSE(vm.isPaused());
}

TEST_CASE("VM reset", "[scripting]")
{
    VirtualMachine vm;

    std::vector<Instruction> program = {
        {OpCode::PUSH_INT, 1},
        {OpCode::HALT, 0}
    };

    vm.load(program, {});
    vm.run();
    REQUIRE(vm.isHalted());

    vm.reset();
    REQUIRE_FALSE(vm.isHalted());
    REQUIRE_FALSE(vm.isRunning());
}
