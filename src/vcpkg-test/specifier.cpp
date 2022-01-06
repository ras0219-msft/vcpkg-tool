#include <catch2/catch.hpp>

#include <vcpkg/base/system.print.h>
#include <vcpkg/base/util.h>

#include <vcpkg/packagespec.h>

#include <vcpkg-test/util.h>

using namespace vcpkg;

TEST_CASE ("specifier parsing", "[specifier]")
{
    SECTION ("parsed specifier from string")
    {
        auto maybe_spec = vcpkg::parse_qualified_specifier("zlib");
        REQUIRE(maybe_spec.has_value());

        auto& spec = *maybe_spec.get();
        REQUIRE(spec.name == "zlib");
        REQUIRE(!spec.features);
        REQUIRE(!spec.triplet);
    }

    SECTION ("parsed specifier from string with triplet")
    {
        auto maybe_spec = vcpkg::parse_qualified_specifier("zlib:x64-uwp");
        REQUIRE(maybe_spec);

        auto& spec = *maybe_spec.get();
        REQUIRE(spec.name == "zlib");
        REQUIRE(spec.triplet.value_or("") == "x64-uwp");
    }

    SECTION ("parsed specifier from string with colons")
    {
        auto s = vcpkg::parse_qualified_specifier("zlib:x86-uwp:");
        REQUIRE(!s);
    }

    SECTION ("parsed specifier from string with feature")
    {
        auto maybe_spec = vcpkg::parse_qualified_specifier("zlib[feature]:x64-uwp");
        REQUIRE(maybe_spec);

        auto& spec = *maybe_spec.get();
        REQUIRE(spec.name == "zlib");
        REQUIRE(spec.features.value_or(std::vector<std::string>{}) == std::vector<std::string>{"feature"});
        REQUIRE(spec.triplet.value_or("") == "x64-uwp");
    }

    SECTION ("parsed specifier from string with many features")
    {
        auto maybe_spec = vcpkg::parse_qualified_specifier("zlib[0, 1,2]");
        REQUIRE(maybe_spec);

        auto& spec = *maybe_spec.get();
        REQUIRE(spec.features.value_or(std::vector<std::string>{}) == std::vector<std::string>{"0", "1", "2"});
    }

    SECTION ("parsed specifier wildcard feature")
    {
        auto maybe_spec = vcpkg::parse_qualified_specifier("zlib[*]");
        REQUIRE(maybe_spec);

        auto& spec = *maybe_spec.get();
        REQUIRE(spec.features.value_or(std::vector<std::string>{}) == std::vector<std::string>{"*"});
    }
}
TEST_CASE ("specifier conversion", "[specifier]")
{
    SECTION ("basic feature implicitdefault yes")
    {
        auto maybe_spec = vcpkg::parse_qualified_specifier("zlib[feature]:x86-uwp");
        REQUIRE(maybe_spec);

        auto spec = Test::unwrap(maybe_spec.get()->to_full_spec(Test::X64_ANDROID, ImplicitDefault::yes));
        REQUIRE(spec.package_spec.name() == "zlib");
        REQUIRE(spec.constraint == DependencyConstraint{});
        REQUIRE(spec.features == std::vector<std::string>{"feature", "core", "default"});
        REQUIRE(spec.package_spec.triplet() == Test::X86_UWP);
    }
    SECTION ("basic feature implicitdefault no")
    {
        auto maybe_spec = vcpkg::parse_qualified_specifier("zlib[feature]:x86-uwp");
        REQUIRE(maybe_spec);

        auto spec = Test::unwrap(maybe_spec.get()->to_full_spec(Test::X64_ANDROID, ImplicitDefault::no));
        REQUIRE(spec.package_spec.name() == "zlib");
        REQUIRE(spec.constraint == DependencyConstraint{});
        REQUIRE(spec.features == std::vector<std::string>{"feature", "core"});
        REQUIRE(spec.package_spec.triplet() == Test::X86_UWP);
    }
    SECTION ("core implicitdefault yes")
    {
        auto maybe_spec = vcpkg::parse_qualified_specifier("zlib[core]:x86-uwp");
        REQUIRE(maybe_spec);

        auto spec = Test::unwrap(maybe_spec.get()->to_full_spec(Test::X64_ANDROID, ImplicitDefault::yes));
        REQUIRE(spec.features == std::vector<std::string>{"core"});
    }
    SECTION ("core implicitdefault no")
    {
        auto maybe_spec = vcpkg::parse_qualified_specifier("zlib[core]:x86-uwp");
        REQUIRE(maybe_spec);

        auto spec = Test::unwrap(maybe_spec.get()->to_full_spec(Test::X64_ANDROID, ImplicitDefault::no));
        REQUIRE(spec.features == std::vector<std::string>{"core"});
    }
}

#if defined(_WIN32)
TEST_CASE ("ascii to utf16", "[utf16]")
{
    SECTION ("ASCII to utf16")
    {
        auto str = vcpkg::Strings::to_utf16("abc");
        REQUIRE(str == L"abc");
    }

    SECTION ("ASCII to utf16 with whitespace")
    {
        auto str = vcpkg::Strings::to_utf16("abc -x86-windows");
        REQUIRE(str == L"abc -x86-windows");
    }
}
#endif
