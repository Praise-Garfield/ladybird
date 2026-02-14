/*
 * Copyright (c) 2024-2025, Tim Flynn <trflynn89@ladybird.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibTest/TestCase.h>

#include <AK/GenericLexer.h>
#include <AK/String.h>
#include <LibHTTP/Cache/Utilities.h>
#include <LibHTTP/HTTP.h>
#include <LibHTTP/HeaderList.h>

TEST_CASE(collect_an_http_quoted_string)
{
    {
        auto test = "\"\""_string;
        GenericLexer lexer { test };

        auto result = HTTP::collect_an_http_quoted_string(lexer);
        EXPECT_EQ(result, "\"\""_string);
    }
    {
        auto test = "\"abc\""_string;
        GenericLexer lexer { test };

        auto result = HTTP::collect_an_http_quoted_string(lexer);
        EXPECT_EQ(result, "\"abc\""_string);
    }
    {
        auto test = "foo \"abc\""_string;

        GenericLexer lexer { test };
        lexer.ignore(4);

        auto result = HTTP::collect_an_http_quoted_string(lexer);
        EXPECT_EQ(result, "\"abc\""_string);
    }
    {
        auto test = "foo=\"abc\""_string;

        GenericLexer lexer { test };
        lexer.ignore(4);

        auto result = HTTP::collect_an_http_quoted_string(lexer);
        EXPECT_EQ(result, "\"abc\""_string);
    }
    {
        auto test = "foo=\"abc\" bar"_string;

        GenericLexer lexer { test };
        lexer.ignore(4);

        auto result = HTTP::collect_an_http_quoted_string(lexer);
        EXPECT_EQ(result, "\"abc\""_string);
    }
    {
        auto test = "\"abc\" bar"_string;
        GenericLexer lexer { test };

        auto result = HTTP::collect_an_http_quoted_string(lexer);
        EXPECT_EQ(result, "\"abc\""_string);
    }
}

TEST_CASE(extract_cache_control_directive)
{
    EXPECT(!HTTP::contains_cache_control_directive({}, "no-cache"sv));
    EXPECT(!HTTP::contains_cache_control_directive(","sv, "no-cache"sv));

    EXPECT(!HTTP::contains_cache_control_directive("no-cache"sv, "no"sv));
    EXPECT(!HTTP::contains_cache_control_directive("no-cache"sv, "cache"sv));
    EXPECT(!HTTP::contains_cache_control_directive("no-cache"sv, "no cache"sv));

    EXPECT(!HTTP::contains_cache_control_directive("abno-cache"sv, "no-cache"sv));
    EXPECT(!HTTP::contains_cache_control_directive("no-cachecd"sv, "no-cache"sv));
    EXPECT(!HTTP::contains_cache_control_directive("abno-cachecd"sv, "no-cache"sv));

    EXPECT_EQ(HTTP::extract_cache_control_directive("no-cache"sv, "no-cache"sv), ""sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=4"sv, "max-age"sv), "4"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age = 4"sv, "max-age"sv), "4"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age= 4"sv, "max-age"sv), "4"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age =4"sv, "max-age"sv), "4"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age = 4 , no-cache"sv, "max-age"sv), "4"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("no-cache , max-age = 4"sv, "max-age"sv), "4"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("s-maxage=4, max-age=5"sv, "max-age"sv), "5"sv);

    EXPECT_EQ(HTTP::extract_cache_control_directive("Max-Age=4"sv, "max-age"sv), "4"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("MAX-AGE=4"sv, "max-age"sv), "4"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=4"sv, "MAX-AGE"sv), "4"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("No-Cache"sv, "no-cache"sv), ""sv);

    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=4,"sv, "max-age"sv), "4"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("no-cache,"sv, "no-cache"sv), ""sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("no-cache, "sv, "no-cache"sv), ""sv);

    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=4, max-age=5"sv, "max-age"sv), "4"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("no-cache, max-age=4, max-age=5"sv, "max-age"sv), "4"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=4, no-cache"sv, "no-cache"sv), ""sv);

    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=\"4\""sv, "max-age"sv), "\"4\""sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=\"004\""sv, "max-age"sv), "\"004\""sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=\"4\", no-cache"sv, "max-age"sv), "\"4\""sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("foo=\"bar\", max-age=\"4\""sv, "max-age"sv), "\"4\""sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=\"4,5\", no-cache"sv, "max-age"sv), "\"4,5\""sv);

    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=\"4\\5\""sv, "max-age"sv), "\"4\\5\""sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=\"4\\\"5\""sv, "max-age"sv), "\"4\\\"5\""sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=\"4\\\\5\""sv, "max-age"sv), "\"4\\\\5\""sv);

    EXPECT(!HTTP::contains_cache_control_directive("max-age\"4\""sv, "max-age"sv));
    EXPECT(!HTTP::contains_cache_control_directive("max-age=\"4"sv, "max-age"sv));
    EXPECT(!HTTP::contains_cache_control_directive("foo=\"bar, max-age=4"sv, "max-age"sv));
    EXPECT(!HTTP::contains_cache_control_directive("\"unterminated"sv, "max-age"sv));
    EXPECT(!HTTP::contains_cache_control_directive("max-age=\"4, no-cache"sv, "max-age"sv));
    EXPECT(!HTTP::contains_cache_control_directive("max-age=\"4, no-cache"sv, "no-cache"sv));

    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=\"4, no-cache\", foo=bar"sv, "max-age"sv), "\"4, no-cache\""sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=\"4, no-cache\", foo=bar"sv, "foo"sv), "bar"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("foo=\"bar,baz\", max-age=4"sv, "foo"sv), "\"bar,baz\""sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("foo=\"bar,baz\", max-age=4"sv, "max-age"sv), "4"sv);

    EXPECT_EQ(HTTP::extract_cache_control_directive(",,max-age=4"sv, "max-age"sv), "4"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age==4"sv, "max-age"sv), "=4"sv);
    EXPECT_EQ(HTTP::extract_cache_control_directive("max-age=4="sv, "max-age"sv), "4="sv);
    EXPECT(!HTTP::contains_cache_control_directive("=4"sv, "max-age"sv));
}

TEST_CASE(extract_length)
{
    // No Content-Length header returns null.
    {
        auto headers = HTTP::HeaderList::create();
        auto result = headers->extract_length();
        EXPECT(result.has<Empty>());
    }

    // Valid Content-Length returns the parsed value.
    {
        auto headers = HTTP::HeaderList::create();
        headers->append({ "Content-Length", "42" });
        auto result = headers->extract_length();
        EXPECT(result.has<u64>());
        EXPECT_EQ(result.get<u64>(), 42u);
    }

    // Content-Length of zero.
    {
        auto headers = HTTP::HeaderList::create();
        headers->append({ "Content-Length", "0" });
        auto result = headers->extract_length();
        EXPECT(result.has<u64>());
        EXPECT_EQ(result.get<u64>(), 0u);
    }

    // Empty Content-Length value returns null.
    {
        auto headers = HTTP::HeaderList::create();
        headers->append({ "Content-Length", "" });
        auto result = headers->extract_length();
        EXPECT(result.has<Empty>());
    }

    // Non-digit characters return null.
    {
        auto headers = HTTP::HeaderList::create();
        headers->append({ "Content-Length", "abc" });
        auto result = headers->extract_length();
        EXPECT(result.has<Empty>());
    }

    // Mixed digit and non-digit characters return null.
    {
        auto headers = HTTP::HeaderList::create();
        headers->append({ "Content-Length", "42abc" });
        auto result = headers->extract_length();
        EXPECT(result.has<Empty>());
    }

    // Negative value returns null.
    {
        auto headers = HTTP::HeaderList::create();
        headers->append({ "Content-Length", "-1" });
        auto result = headers->extract_length();
        EXPECT(result.has<Empty>());
    }

    // Overflow value returns failure.
    {
        auto headers = HTTP::HeaderList::create();
        headers->append({ "Content-Length", "99999999999999999999999" });
        auto result = headers->extract_length();
        EXPECT(result.has<HTTP::HeaderList::ExtractLengthFailure>());
    }

    // Conflicting Content-Length values return failure.
    {
        auto headers = HTTP::HeaderList::create();
        headers->append({ "Content-Length", "42" });
        headers->append({ "Content-Length", "43" });
        auto result = headers->extract_length();
        EXPECT(result.has<HTTP::HeaderList::ExtractLengthFailure>());
    }

    // Duplicate identical Content-Length values return the value.
    {
        auto headers = HTTP::HeaderList::create();
        headers->append({ "Content-Length", "42" });
        headers->append({ "Content-Length", "42" });
        auto result = headers->extract_length();
        EXPECT(result.has<u64>());
        EXPECT_EQ(result.get<u64>(), 42u);
    }

    // Maximum u64 value parses successfully.
    {
        auto headers = HTTP::HeaderList::create();
        headers->append({ "Content-Length", "18446744073709551615" });
        auto result = headers->extract_length();
        EXPECT(result.has<u64>());
        EXPECT_EQ(result.get<u64>(), 18446744073709551615ULL);
    }

    // One past maximum u64 value returns failure.
    {
        auto headers = HTTP::HeaderList::create();
        headers->append({ "Content-Length", "18446744073709551616" });
        auto result = headers->extract_length();
        EXPECT(result.has<HTTP::HeaderList::ExtractLengthFailure>());
    }
}
