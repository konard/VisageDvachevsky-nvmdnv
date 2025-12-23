#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "NovelMind/scripting/lexer.hpp"

using namespace NovelMind::scripting;

TEST_CASE("Lexer tokenizes basic tokens", "[lexer]")
{
    Lexer lexer;

    SECTION("tokenizes keywords")
    {
        auto result = lexer.tokenize("character scene show hide say choice");
        REQUIRE(result.isOk());

        const auto& tokens = result.value();
        REQUIRE(tokens.size() == 7); // 6 keywords + EOF

        REQUIRE(tokens[0].type == TokenType::Character);
        REQUIRE(tokens[1].type == TokenType::Scene);
        REQUIRE(tokens[2].type == TokenType::Show);
        REQUIRE(tokens[3].type == TokenType::Hide);
        REQUIRE(tokens[4].type == TokenType::Say);
        REQUIRE(tokens[5].type == TokenType::Choice);
        REQUIRE(tokens[6].type == TokenType::EndOfFile);
    }

    SECTION("tokenizes identifiers")
    {
        auto result = lexer.tokenize("Hero myVariable _private");
        REQUIRE(result.isOk());

        const auto& tokens = result.value();
        REQUIRE(tokens.size() == 4);

        REQUIRE(tokens[0].type == TokenType::Identifier);
        REQUIRE(tokens[0].lexeme == "Hero");
        REQUIRE(tokens[1].type == TokenType::Identifier);
        REQUIRE(tokens[1].lexeme == "myVariable");
        REQUIRE(tokens[2].type == TokenType::Identifier);
        REQUIRE(tokens[2].lexeme == "_private");
    }

    SECTION("tokenizes integers")
    {
        auto result = lexer.tokenize("0 42 12345");
        REQUIRE(result.isOk());

        const auto& tokens = result.value();
        REQUIRE(tokens.size() == 4);

        REQUIRE(tokens[0].type == TokenType::Integer);
        REQUIRE(tokens[0].intValue == 0);
        REQUIRE(tokens[1].type == TokenType::Integer);
        REQUIRE(tokens[1].intValue == 42);
        REQUIRE(tokens[2].type == TokenType::Integer);
        REQUIRE(tokens[2].intValue == 12345);
    }

    SECTION("tokenizes floats")
    {
        auto result = lexer.tokenize("0.0 3.14 123.456");
        REQUIRE(result.isOk());

        const auto& tokens = result.value();
        REQUIRE(tokens.size() == 4);

        REQUIRE(tokens[0].type == TokenType::Float);
        REQUIRE(tokens[0].floatValue == 0.0f);
        REQUIRE(tokens[1].type == TokenType::Float);
        REQUIRE(tokens[1].floatValue == Catch::Approx(3.14f));
        REQUIRE(tokens[2].type == TokenType::Float);
        REQUIRE(tokens[2].floatValue == Catch::Approx(123.456f));
    }

    SECTION("tokenizes strings")
    {
        auto result = lexer.tokenize(R"("hello" "world" "with spaces")");
        REQUIRE(result.isOk());

        const auto& tokens = result.value();
        REQUIRE(tokens.size() == 4);

        REQUIRE(tokens[0].type == TokenType::String);
        REQUIRE(tokens[0].lexeme == "hello");
        REQUIRE(tokens[1].type == TokenType::String);
        REQUIRE(tokens[1].lexeme == "world");
        REQUIRE(tokens[2].type == TokenType::String);
        REQUIRE(tokens[2].lexeme == "with spaces");
    }

    SECTION("handles escape sequences in strings")
    {
        auto result = lexer.tokenize(R"("line1\nline2" "tab\there" "quote\"here")");
        REQUIRE(result.isOk());

        const auto& tokens = result.value();
        REQUIRE(tokens.size() == 4);

        REQUIRE(tokens[0].lexeme == "line1\nline2");
        REQUIRE(tokens[1].lexeme == "tab\there");
        REQUIRE(tokens[2].lexeme == "quote\"here");
    }

    SECTION("tokenizes operators")
    {
        auto result = lexer.tokenize("= + - * / % == != < <= > >= ->");
        REQUIRE(result.isOk());

        const auto& tokens = result.value();
        REQUIRE(tokens.size() == 14);

        REQUIRE(tokens[0].type == TokenType::Assign);
        REQUIRE(tokens[1].type == TokenType::Plus);
        REQUIRE(tokens[2].type == TokenType::Minus);
        REQUIRE(tokens[3].type == TokenType::Star);
        REQUIRE(tokens[4].type == TokenType::Slash);
        REQUIRE(tokens[5].type == TokenType::Percent);
        REQUIRE(tokens[6].type == TokenType::Equal);
        REQUIRE(tokens[7].type == TokenType::NotEqual);
        REQUIRE(tokens[8].type == TokenType::Less);
        REQUIRE(tokens[9].type == TokenType::LessEqual);
        REQUIRE(tokens[10].type == TokenType::Greater);
        REQUIRE(tokens[11].type == TokenType::GreaterEqual);
        REQUIRE(tokens[12].type == TokenType::Arrow);
    }

    SECTION("tokenizes delimiters")
    {
        auto result = lexer.tokenize("( ) { } [ ] , : ; .");
        REQUIRE(result.isOk());

        const auto& tokens = result.value();
        REQUIRE(tokens.size() == 11);

        REQUIRE(tokens[0].type == TokenType::LeftParen);
        REQUIRE(tokens[1].type == TokenType::RightParen);
        REQUIRE(tokens[2].type == TokenType::LeftBrace);
        REQUIRE(tokens[3].type == TokenType::RightBrace);
        REQUIRE(tokens[4].type == TokenType::LeftBracket);
        REQUIRE(tokens[5].type == TokenType::RightBracket);
        REQUIRE(tokens[6].type == TokenType::Comma);
        REQUIRE(tokens[7].type == TokenType::Colon);
        REQUIRE(tokens[8].type == TokenType::Semicolon);
        REQUIRE(tokens[9].type == TokenType::Dot);
    }
}

TEST_CASE("Lexer handles comments", "[lexer]")
{
    Lexer lexer;

    SECTION("skips line comments")
    {
        auto result = lexer.tokenize("show // this is a comment\nhide");
        REQUIRE(result.isOk());

        const auto& tokens = result.value();
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].type == TokenType::Show);
        REQUIRE(tokens[1].type == TokenType::Hide);
    }

    SECTION("skips block comments")
    {
        auto result = lexer.tokenize("show /* block comment */ hide");
        REQUIRE(result.isOk());

        const auto& tokens = result.value();
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].type == TokenType::Show);
        REQUIRE(tokens[1].type == TokenType::Hide);
    }

    SECTION("skips nested block comments")
    {
        auto result = lexer.tokenize("show /* outer /* inner */ outer */ hide");
        REQUIRE(result.isOk());

        const auto& tokens = result.value();
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].type == TokenType::Show);
        REQUIRE(tokens[1].type == TokenType::Hide);
    }
}

TEST_CASE("Lexer tracks source locations", "[lexer]")
{
    Lexer lexer;

    SECTION("tracks line and column for single line")
    {
        auto result = lexer.tokenize("show Hero");
        REQUIRE(result.isOk());

        const auto& tokens = result.value();
        REQUIRE(tokens[0].location.line == 1);
        REQUIRE(tokens[0].location.column == 1);
        REQUIRE(tokens[1].location.line == 1);
        REQUIRE(tokens[1].location.column == 6);
    }

    SECTION("tracks line for multiple lines")
    {
        auto result = lexer.tokenize("show\nhide\ngoto");
        REQUIRE(result.isOk());

        const auto& tokens = result.value();
        REQUIRE(tokens[0].location.line == 1);
        REQUIRE(tokens[1].location.line == 2);
        REQUIRE(tokens[2].location.line == 3);
    }
}

TEST_CASE("Lexer handles color literals", "[lexer]")
{
    Lexer lexer;

    SECTION("parses hex colors")
    {
        auto result = lexer.tokenize("#FFCC00 #FF0000");
        REQUIRE(result.isOk());

        const auto& tokens = result.value();
        REQUIRE(tokens.size() == 3);
        REQUIRE(tokens[0].type == TokenType::String);
        REQUIRE(tokens[0].lexeme == "#FFCC00");
        REQUIRE(tokens[1].type == TokenType::String);
        REQUIRE(tokens[1].lexeme == "#FF0000");
    }
}

TEST_CASE("Lexer reports errors", "[lexer]")
{
    Lexer lexer;

    SECTION("reports unterminated string")
    {
        auto result = lexer.tokenize("\"unterminated");
        REQUIRE(result.isError());
    }
}
