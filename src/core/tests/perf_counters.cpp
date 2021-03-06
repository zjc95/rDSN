/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Microsoft Corporation
 * 
 * -=- Robust Distributed System Nucleus (rDSN) -=- 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Description:
 *     Unit-test for perf_counters.
 *
 * Revision history:
 *     Nov., 2015, @qinzuoyan (Zuoyan Qin), first version
 *     xxxx-xx-xx, author, fix bug about xxx
 */

# include <dsn/internal/perf_counters.h>
# include <gtest/gtest.h>

using namespace ::dsn;

TEST(core, perf_counters)
{
    utils::perf_counters& c = utils::perf_counters::instance();
    perf_counter_ptr p;

    p = c.get_counter("test", "number_counter", COUNTER_TYPE_NUMBER, false);
    ASSERT_EQ(nullptr, p.get());
    p = c.get_counter("test", "number_counter", COUNTER_TYPE_NUMBER, true);
    ASSERT_NE(nullptr, p.get());
    p = c.get_counter("test", "number_counter", COUNTER_TYPE_NUMBER, false);
    ASSERT_NE(nullptr, p.get());

    p = c.get_counter("test", "rate_counter", COUNTER_TYPE_RATE, false);
    ASSERT_EQ(nullptr, p.get());
    p = c.get_counter("test", "rate_counter", COUNTER_TYPE_RATE, true);
    ASSERT_NE(nullptr, p.get());
    p = c.get_counter("test", "rate_counter", COUNTER_TYPE_RATE, false);
    ASSERT_NE(nullptr, p.get());

    ASSERT_FALSE(c.remove_counter("unexist_section", "number_counter"));
    ASSERT_FALSE(c.remove_counter("test", "unexist_counter"));

    ASSERT_TRUE(c.remove_counter("test", "number_counter"));
    p = c.get_counter("test", "number_counter", COUNTER_TYPE_NUMBER, false);
    ASSERT_EQ(nullptr, p.get());

    ASSERT_TRUE(c.remove_counter("test", "rate_counter"));
    p = c.get_counter("test", "rate_counter", COUNTER_TYPE_RATE, false);
    ASSERT_EQ(nullptr, p.get());

    p = c.get_counter("unexist_counter", COUNTER_TYPE_NUMBER, false);
    ASSERT_EQ(nullptr, p.get());
    ASSERT_FALSE(c.remove_counter("unexist_counter"));
}
