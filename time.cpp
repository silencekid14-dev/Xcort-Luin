#include "time.h"
#include <chrono>
#include <ctime>
#include <cstdio>
#include <cctype>
#include <stdexcept>
#include <string>

namespace luin {

namespace {

Value makeNative(std::string name, std::function<Value(std::vector<Value>&)> fn) {
    auto nf = std::make_shared<NativeFunction>();
    nf->name = std::move(name);
    nf->fn = std::move(fn);
    return nf;
}

// Parses an offset argument into total minutes east of GMT. Accepts:
//   - a plain number of hours: 3, -5, 5.5
//   - a string: "GMT+3", "GMT +0300", "GMT-05:30", "+0300", "-05:30"
// Returns 0 (no shift / GMT) if no argument was given.
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
        // "0300" style: last two digits are minutes
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

std::string formatTm(const std::tm& t, const char* fmt) {
    char buf[64];
    std::strftime(buf, sizeof(buf), fmt, &t);
    return std::string(buf);
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

    mod->members["today"] = makeNative("today", [](std::vector<Value>& args) -> Value {
        std::tm t = localNow();
        return formatTm(t, "%Y-%m-%d");
    });

    mod->members["clock"] = makeNative("clock", [](std::vector<Value>& args) -> Value {
        std::tm t = localNow();
        return formatTm(t, "%H:%M:%S");
    });

    mod->members["year"] = makeNative("year", [](std::vector<Value>& args) -> Value {
        return localNow().tm_year + 1900;
    });
    mod->members["month"] = makeNative("month", [](std::vector<Value>& args) -> Value {
        return localNow().tm_mon + 1;
    });
    mod->members["day"] = makeNative("day", [](std::vector<Value>& args) -> Value {
        return localNow().tm_mday;
    });
    mod->members["hour"] = makeNative("hour", [](std::vector<Value>& args) -> Value {
        return localNow().tm_hour;
    });
    mod->members["minute"] = makeNative("minute", [](std::vector<Value>& args) -> Value {
        return localNow().tm_min;
    });
    mod->members["second"] = makeNative("second", [](std::vector<Value>& args) -> Value {
        return localNow().tm_sec;
    });

    mod->members["stamp"] = makeNative("stamp", [](std::vector<Value>& args) -> Value {
        return static_cast<int>(std::time(nullptr));
    });

    return mod;
}

} // namespace luin
