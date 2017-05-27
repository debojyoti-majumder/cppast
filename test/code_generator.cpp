// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/code_generator.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("code_generator")
{
    // no need to check much here, as each entity check separately
    auto code = R"(using type=int;

type* var;

template<template<typename>class... T>
struct templated{
};

struct foo{
  int a;

  auto func(int)->int(*(*)(int))[42];

private:
  int const b=42;
};

int(*(*(foo::* mptr)(int))(int))[42];

enum class bar
:int{
  a,
  b=42
};

void func(int(*)(int));

extern void(* ptr)(int(*)(int))=&func;)";
    auto file = parse({}, "code_generator.cpp", code);

    SECTION("basic")
    {
        REQUIRE(get_code(*file) == code);
    }
    SECTION("formatting")
    {
        auto synopsis = R"(using type = int;

type* var;

template <template <typename> class ... T>
struct templated
{
};

struct foo
{
  int a;

  auto func(int) -> int(*(*)(int))[42];

private:
  int const b = 42;
};

int(*(*(foo::* mptr)(int))(int))[42];

enum class bar
: int
{
  a,
  b = 42
};

void func(int(*)(int));

extern void(* ptr)(int(*)(int)) = &func;
)";

        class formatted_generator : public test_generator
        {
        public:
            using test_generator::test_generator;

        private:
            formatting do_get_formatting() const override
            {
                return formatting_flags::brace_nl | formatting_flags::operator_ws
                       | formatting_flags::comma_ws;
            }
        } generator(code_generator::generation_options{});
        generate_code(generator, *file);
        REQUIRE(generator.str() == synopsis);
    }
    SECTION("exclude target")
    {
        auto code = R"(
namespace a {}

namespace b = a;

using c = int*;
typedef int d;)";

        auto synopsis = R"(namespace a{
}

namespace b=excluded;

using c=excluded;

using d=excluded;)";

        auto file = parse({}, "code_generator_exclude_target.cpp", code);
        REQUIRE(get_code(*file, code_generator::exclude_target) == synopsis);
    }
    SECTION("exclude return")
    {
        auto code = R"(
void a();
template <typename T>
auto b() -> int*;

struct foo
{
    int c() const&;
    operator const int ();
};
)";

        auto synopsis = R"(excluded a();

template<typename T>
excluded b();

struct foo{
  excluded c()const&;

  operator excluded();
};)";

        auto file = parse({}, "code_generator_exclude_return.cpp", code);
        REQUIRE(get_code(*file, code_generator::exclude_return) == synopsis);
    }
    SECTION("exclude")
    {
        // exclude all entities starting with `e`
        class exclude_generator : public test_generator
        {
        public:
            using test_generator::test_generator;

        private:
            generation_options do_get_options(const cpp_entity& e) override
            {
                if (e.name().front() == 'e')
                    return code_generator::exclude;
                return {};
            }
        };

        auto code = R"(
void e();

void func(int a, int e, int c);

template <typename e1, typename e2>
void tfunc(int a);

struct base {};
struct e_t {};

struct bar : e_t, base {};

class foo : e_t
{
    int a;

public:
    int e1;

private:
    int b;

public:
    int c;
    int e2;

private:
    int e3;
};
)";

        auto synopsis = R"(void func(int a,int c);

void tfunc(int a);

struct base{
};

struct bar
:base{
};

class foo{
  int a;

  int b;

public:
  int c;
};
)";

        auto file = parse({}, "code_generator_exclude.cpp", code);

        exclude_generator generator(code_generator::generation_options{});
        generate_code(generator, *file);
        REQUIRE(generator.str() == synopsis);
    }
}
