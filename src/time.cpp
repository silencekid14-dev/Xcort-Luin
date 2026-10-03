#include "time.h"
#include <chrono>
#include <ctime>
#include <cstdio>
#include <cctype>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace luin {

namespace {

using Clock = std::chrono::steady_clock;
Clock::time_point g_timerStart = Clock::now();

Value makeNative(std::string name, std::function<Value(std::vector<Value>&)> fn) {
    auto nf = std::make_shared<NativeFunction>();
    nf->name = std::move(name);
    nf->fn = std::move(fn);
    return nf;
}

int parseOffsetMinutes(std::vector<Value>& args) {
    if (args.empty()) return 0;
    if (std::holds_alternative<int>(args[0]))
        return static_cast<int>(std::get<int>(args[0]) * 60);
    if (std::holds_alternative<double>(args[0]))
        return static_cast<int>(std::get<double>(args[0]) * 60);
    if (!std::holds_alternative<std::string>(args[0]))
        throw std::runtime_error("time.now(): offset must be a number or a string like \"GMT+3\"");
    std::string raw = std::get<std::string>(args[0]);
    std::string digits;
    bool sawSign = false;
    int sign = 1;
    for (char c : raw) {
        if (c == '+' && !sawSign) { sign = 1; sawSign = true; continue; }
        if (c == '-' && !sawSign) { sign = -1; sawSign = true; continue; }
        if (std::isdigit(static_cast<unsigned char>(c)) || c == ':') digits += c;
    }
    if (digits.empty()) return 0;
    int hours = 0, minutes = 0;
    auto colon = digits.find(':');
    if (colon != std::string::npos) {
        hours = std::stoi(digits.substr(0, colon));
        minutes = std::stoi(digits.substr(colon + 1));
    } else if (digits.size() > 2) {
        hours = std::stoi(digits.substr(0, digits.size() - 2));
        minutes = std::stoi(digits.substr(digits.size() - 2));
    } else {
        hours = std::stoi(digits);
    }
    return sign * (hours * 60 + minutes);
}

std::tm shiftedTime(int offsetMinutes) {
    std::time_t now = std::time(nullptr);
    std::time_t shifted = now + static_cast<std::time_t>(offsetMinutes) * 60;
    std::tm out{};
#if defined(_WIN32)
    gmtime_s(&out, &shifted);
#else
    gmtime_r(&shifted, &out);
#endif
    return out;
}

std::tm localNow() {
    std::time_t now = std::time(nullptr);
    std::tm out{};
#if defined(_WIN32)
    localtime_s(&out, &now);
#else
    localtime_r(&now, &out);
#endif
    return out;
}

std::tm gmNow() {
    std::time_t now = std::time(nullptr);
    std::tm out{};
#if defined(_WIN32)
    gmtime_s(&out, &now);
#else
    gmtime_r(&now, &out);
#endif
    return out;
}

std::string formatTm(const std::tm& t, const char* fmt) {
    char buf[128];
    std::strftime(buf, sizeof(buf), fmt, &t);
    return std::string(buf);
}

int asIntArg(const Value& v, const std::string& fn) {
    if (std::holds_alternative<int>(v)) return std::get<int>(v);
    if (std::holds_alternative<double>(v)) return static_cast<int>(std::get<double>(v));
    throw std::runtime_error("time." + fn + "(): expected number");
}

} // namespace

std::shared_ptr<Module> createTimeModule() {
    auto mod = std::make_shared<Module>();
    mod->name = "time";

    mod->members["now"] = makeNative("now", [](std::vector<Value>& args) -> Value {
        if (args.empty()) {
            std::tm t = localNow();
            return formatTm(t, "%Y-%m-%d %H:%M:%S");
        }
        int offsetMin = parseOffsetMinutes(args);
        std::tm t = shiftedTime(offsetMin);
        return formatTm(t, "%Y-%m-%d %H:%M:%S") + " GMT" +
               (offsetMin >= 0 ? "+" : "-") +
               std::to_string(std::abs(offsetMin) / 60);
    });
    mod->members["today"] = makeNative("today", [](std::vector<Value>&) -> Value {
        return formatTm(localNow(), "%Y-%m-%d");
    });
    mod->members["clock"] = makeNative("clock", [](std::vector<Value>&) -> Value {
        return formatTm(localNow(), "%H:%M:%S");
    });
    mod->members["year"] = makeNative("year", [](std::vector<Value>&) -> Value { return localNow().tm_year + 1900; });
    mod->members["month"] = makeNative("month", [](std::vector<Value>&) -> Value { return localNow().tm_mon + 1; });
    mod->members["day"] = makeNative("day", [](std::vector<Value>&) -> Value { return localNow().tm_mday; });
    mod->members["hour"] = makeNative("hour", [](std::vector<Value>&) -> Value { return localNow().tm_hour; });
    mod->members["minute"] = makeNative("minute", [](std::vector<Value>&) -> Value { return localNow().tm_min; });
    mod->members["second"] = makeNative("second", [](std::vector<Value>&) -> Value { return localNow().tm_sec; });
    mod->members["stamp"] = makeNative("stamp", [](std::vector<Value>&) -> Value {
        return static_cast<int>(std::time(nullptr));
    });
    mod->members["utc"] = makeNative("utc", [](std::vector<Value>&) -> Value {
        return formatTm(gmNow(), "%Y-%m-%d %H:%M:%S UTC");
    });
    mod->members["weekday"] = makeNative("weekday", [](std::vector<Value>&) -> Value { return localNow().tm_wday; });
    mod->members["weekday_name"] = makeNative("weekday_name", [](std::vector<Value>&) -> Value {
        static const char* names[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
        return std::string(names[localNow().tm_wday]);
    });
    mod->members["month_name"] = makeNative("month_name", [](std::vector<Value>&) -> Value {
        static const char* names[] = {"January","February","March","April","May","June",
                                      "July","August","September","October","November","December"};
        return std::string(names[localNow().tm_mon]);
    });
    mod->members["yearday"] = makeNative("yearday", [](std::vector<Value>&) -> Value { return localNow().tm_yday + 1; });
    mod->members["is_leap"] = makeNative("is_leap", [](std::vector<Value>& args) -> Value {
        int y = args.empty() ? (localNow().tm_year + 1900) : asIntArg(args[0], "is_leap");
        return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
    });
    mod->members["days_in_month"] = makeNative("days_in_month", [](std::vector<Value>& args) -> Value {
        int y = args.size() >= 1 ? asIntArg(args[0], "days_in_month") : (localNow().tm_year + 1900);
        int m = args.size() >= 2 ? asIntArg(args[1], "days_in_month") : (localNow().tm_mon + 1);
        static int dim[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        if (m < 1 || m > 12) throw std::runtime_error("time.days_in_month(): month 1-12");
        int d = dim[m - 1];
        if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))) d = 29;
        return d;
    });
    mod->members["format"] = makeNative("format", [](std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0]))
            throw std::runtime_error("time.format() expects a format string");
        return formatTm(localNow(), std::get<std::string>(args[0]).c_str());
    });
    mod->members["iso"] = makeNative("iso", [](std::vector<Value>&) -> Value {
        return formatTm(localNow(), "%Y-%m-%dT%H:%M:%S");
    });
    mod->members["sleep_ms"] = makeNative("sleep_ms", [](std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("time.sleep_ms() expects milliseconds");
        int ms = asIntArg(args[0], "sleep_ms");
        if (ms < 0) ms = 0;
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        return std::string("");
    });
    mod->members["sleep"] = makeNative("sleep", [](std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("time.sleep() expects seconds");
        double s = std::holds_alternative<double>(args[0]) ? std::get<double>(args[0])
                 : static_cast<double>(asIntArg(args[0], "sleep"));
        if (s < 0) s = 0;
        std::this_thread::sleep_for(std::chrono::duration<double>(s));
        return std::string("");
    });
    mod->members["timer_start"] = makeNative("timer_start", [](std::vector<Value>&) -> Value {
        g_timerStart = Clock::now();
        return std::string("");
    });
    mod->members["timer_ms"] = makeNative("timer_ms", [](std::vector<Value>&) -> Value {
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - g_timerStart).count();
        return static_cast<int>(ms);
    });
    mod->members["timer_s"] = makeNative("timer_s", [](std::vector<Value>&) -> Value {
        return std::chrono::duration<double>(Clock::now() - g_timerStart).count();
    });
    mod->members["millis"] = makeNative("millis", [](std::vector<Value>&) -> Value {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return static_cast<int>(ms % 1000000000LL);
    });
    mod->members["unix_ms"] = makeNative("unix_ms", [](std::vector<Value>&) -> Value {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        return static_cast<double>(ms);
    });
    mod->members["from_stamp"] = makeNative("from_stamp", [](std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("time.from_stamp() expects unix stamp");
        std::time_t t = static_cast<std::time_t>(asIntArg(args[0], "from_stamp"));
        std::tm out{};
#if defined(_WIN32)
        localtime_s(&out, &t);
#else
        localtime_r(&t, &out);
#endif
        return formatTm(out, "%Y-%m-%d %H:%M:%S");
    });
    mod->members["parse"] = makeNative("parse", [](std::vector<Value>& args) -> Value {
        if (args.empty() || !std::holds_alternative<std::string>(args[0]))
            throw std::runtime_error("time.parse() expects \"YYYY-MM-DD\" string");
        std::string s = std::get<std::string>(args[0]);
        int y = 0, m = 0, d = 0;
        if (std::sscanf(s.c_str(), "%d-%d-%d", &y, &m, &d) != 3)
            throw std::runtime_error("time.parse(): expected YYYY-MM-DD");
        std::tm t{};
        t.tm_year = y - 1900; t.tm_mon = m - 1; t.tm_mday = d; t.tm_hour = 12; t.tm_isdst = -1;
        return static_cast<int>(std::mktime(&t));
    });
    mod->members["add_days"] = makeNative("add_days", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("time.add_days() expects (stamp, days)");
        return asIntArg(args[0], "add_days") + asIntArg(args[1], "add_days") * 86400;
    });
    mod->members["add_hours"] = makeNative("add_hours", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("time.add_hours() expects (stamp, hours)");
        return asIntArg(args[0], "add_hours") + asIntArg(args[1], "add_hours") * 3600;
    });
    mod->members["add_minutes"] = makeNative("add_minutes", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("time.add_minutes() expects (stamp, minutes)");
        return asIntArg(args[0], "add_minutes") + asIntArg(args[1], "add_minutes") * 60;
    });
    mod->members["diff_days"] = makeNative("diff_days", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("time.diff_days() expects (stamp_a, stamp_b)");
        return (asIntArg(args[0], "diff_days") - asIntArg(args[1], "diff_days")) / 86400;
    });
    mod->members["diff_seconds"] = makeNative("diff_seconds", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("time.diff_seconds() expects (stamp_a, stamp_b)");
        return asIntArg(args[0], "diff_seconds") - asIntArg(args[1], "diff_seconds");
    });
    mod->members["is_weekend"] = makeNative("is_weekend", [](std::vector<Value>&) -> Value {
        int w = localNow().tm_wday; return w == 0 || w == 6;
    });
    mod->members["quarter"] = makeNative("quarter", [](std::vector<Value>&) -> Value {
        return (localNow().tm_mon / 3) + 1;
    });
    mod->members["week_number"] = makeNative("week_number", [](std::vector<Value>&) -> Value {
        return (localNow().tm_yday / 7) + 1;
    });
    mod->members["ampm"] = makeNative("ampm", [](std::vector<Value>&) -> Value {
        return localNow().tm_hour < 12 ? std::string("AM") : std::string("PM");
    });
    mod->members["hour12"] = makeNative("hour12", [](std::vector<Value>&) -> Value {
        int h = localNow().tm_hour % 12; return h == 0 ? 12 : h;
    });
    mod->members["date_parts"] = makeNative("date_parts", [](std::vector<Value>&) -> Value {
        std::tm t = localNow();
        auto arr = std::make_shared<ValueArray>();
        arr->elements.push_back(t.tm_year + 1900);
        arr->elements.push_back(t.tm_mon + 1);
        arr->elements.push_back(t.tm_mday);
        arr->elements.push_back(t.tm_hour);
        arr->elements.push_back(t.tm_min);
        arr->elements.push_back(t.tm_sec);
        return arr;
    });
    mod->members["elapsed_since"] = makeNative("elapsed_since", [](std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("time.elapsed_since() expects stamp");
        return static_cast<int>(std::time(nullptr)) - asIntArg(args[0], "elapsed_since");
    });
    return mod;
}

} // namespace luin
