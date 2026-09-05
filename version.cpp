#include "version.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cctype>

namespace fs = std::filesystem;

namespace luin {
namespace {

// ---------------------------------------------------------------------------
// Tiny helpers (no external JSON library)
// ---------------------------------------------------------------------------

std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}

// Extremely small JSON object reader for our known flat shape.
// Looks for "key": value  (string or number). Returns empty string on miss.
std::string jsonGetString(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return "";
    ++pos;
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
    if (pos >= json.size()) return "";

    if (json[pos] == '"') {
        ++pos;
        std::string val;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                char n = json[pos + 1];
                if (n == '"' || n == '\\' || n == '/') val += n;
                else if (n == 'n') val += '\n';
                else if (n == 'r') val += '\r';
                else if (n == 't') val += '\t';
                else val += n;
                pos += 2;
            } else {
                val += json[pos++];
            }
        }
        return val;
    }

    // number / bare token until comma or }
    size_t end = pos;
    while (end < json.size() && json[end] != ',' && json[end] != '}' &&
           !std::isspace(static_cast<unsigned char>(json[end]))) {
        ++end;
    }
    return json.substr(pos, end - pos);
}

long long jsonGetInt(const std::string& json, const std::string& key, long long fallback = 0) {
    std::string s = jsonGetString(json, key);
    if (s.empty()) return fallback;
    try {
        return std::stoll(s);
    } catch (...) {
        return fallback;
    }
}

// ---------------------------------------------------------------------------
// Version comparison: "2.4" < "2.10" < "3.0"  (numeric per segment)
// Accepts optional leading 'v' / 'V'.
// Returns -1 if a < b, 0 if equal, +1 if a > b.
// ---------------------------------------------------------------------------

std::vector<int> parseVersionParts(std::string v) {
    while (!v.empty() && (v[0] == 'v' || v[0] == 'V')) v.erase(0, 1);
    std::vector<int> parts;
    std::string cur;
    for (char c : v) {
        if (c == '.') {
            if (!cur.empty()) {
                try { parts.push_back(std::stoi(cur)); } catch (...) { parts.push_back(0); }
                cur.clear();
            }
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            cur += c;
        } else {
            break; // stop at first non-numeric / non-dot
        }
    }
    if (!cur.empty()) {
        try { parts.push_back(std::stoi(cur)); } catch (...) { parts.push_back(0); }
    }
    return parts;
}

int compareVersions(const std::string& a, const std::string& b) {
    auto pa = parseVersionParts(a);
    auto pb = parseVersionParts(b);
    size_t n = std::max(pa.size(), pb.size());
    for (size_t i = 0; i < n; ++i) {
        int va = i < pa.size() ? pa[i] : 0;
        int vb = i < pb.size() ? pb[i] : 0;
        if (va < vb) return -1;
        if (va > vb) return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// State file location
// ---------------------------------------------------------------------------

fs::path stateFilePath() {
    // Prefer a per-user config location so multiple Luin installs share state.
    const char* home = nullptr;
#if defined(_WIN32)
    home = std::getenv("USERPROFILE");
    if (!home) home = std::getenv("HOME");
#else
    home = std::getenv("HOME");
#endif
    if (home && home[0] != '\0') {
        fs::path dir = fs::path(home) / ".luin";
        std::error_code ec;
        fs::create_directories(dir, ec);
        return dir / "version_state.json";
    }
    // Fallback: next to the current working directory.
    return fs::path("luin_version_state.json");
}

struct State {
    long long last_check = 0;           // unix seconds
    std::string last_remote_version;    // e.g. "2.5"
    std::string last_notified_version;  // last version we already printed news for
    std::string platform;               // windows / macos / linux
    std::string news;                   // optional multi-line news from remote
};

State loadState() {
    State s;
    s.platform = LUIN_PLATFORM;
    std::ifstream in(stateFilePath());
    if (!in) return s;
    std::ostringstream buf;
    buf << in.rdbuf();
    std::string json = buf.str();
    s.last_check = jsonGetInt(json, "last_check", 0);
    s.last_remote_version = jsonGetString(json, "last_remote_version");
    s.last_notified_version = jsonGetString(json, "last_notified_version");
    std::string plat = jsonGetString(json, "platform");
    if (!plat.empty()) s.platform = plat;
    s.news = jsonGetString(json, "news");
    return s;
}

void saveState(const State& s) {
    std::ofstream out(stateFilePath(), std::ios::trunc);
    if (!out) return;
    out << "{\n"
        << "  \"last_check\": " << s.last_check << ",\n"
        << "  \"last_remote_version\": \"" << jsonEscape(s.last_remote_version) << "\",\n"
        << "  \"last_notified_version\": \"" << jsonEscape(s.last_notified_version) << "\",\n"
        << "  \"platform\": \"" << jsonEscape(s.platform) << "\",\n"
        << "  \"news\": \"" << jsonEscape(s.news) << "\"\n"
        << "}\n";
}

// ---------------------------------------------------------------------------
// Network fetch via the system `curl` binary (available on the build host
// and on virtually every developer machine). Timeouts keep the process
// snappy when offline.
// ---------------------------------------------------------------------------

bool fetchRemoteVersionText(std::string& outBody) {
    // -sS : silent but show errors on stderr
    // -L  : follow redirects
    // --max-time 8 : hard timeout
    // -A  : polite User-Agent
    std::string cmd = std::string("curl -sS -L --max-time 8 ")
                    + "-A \"Luin/" + LUIN_VERSION + " (" + LUIN_PLATFORM + ")\" "
                    + "\"https://xluin.netlify.app/version.txt\" 2>/dev/null";

#if defined(_WIN32)
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    if (!pipe) return false;

    char buffer[512];
    std::string result;
    while (std::fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }

#if defined(_WIN32)
    int rc = _pclose(pipe);
#else
    int rc = pclose(pipe);
#endif
    if (rc != 0 || result.empty()) return false;

    outBody = result;
    return true;
}

// True if `s` looks like a version token: optional leading 'v', then digits
// and dots only (e.g. "2.4", "v2.10.1"). Rejects HTML / garbage.
bool looksLikeVersion(const std::string& s) {
    if (s.empty() || s.size() > 32) return false;
    size_t i = 0;
    if (s[0] == 'v' || s[0] == 'V') ++i;
    if (i >= s.size() || !std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    bool sawDigit = false;
    for (; i < s.size(); ++i) {
        char c = s[i];
        if (std::isdigit(static_cast<unsigned char>(c))) {
            sawDigit = true;
        } else if (c == '.') {
            // ok
        } else {
            return false;
        }
    }
    return sawDigit;
}

// Parse the remote file.
// Expected shapes (first non-empty line is the version):
//
//   2.5
//
//   v2.5
//   Optional news line 1
//   Optional news line 2
//
// Returns the version string; remaining lines become `news` (trimmed).
// Returns empty string if the body does not start with a valid version
// (e.g. an HTML 404 page).
std::string parseRemote(const std::string& body, std::string& newsOut) {
    // Hard reject obvious HTML error pages.
    if (body.find("<!DOCTYPE") != std::string::npos ||
        body.find("<html") != std::string::npos ||
        body.find("<HTML") != std::string::npos) {
        newsOut.clear();
        return "";
    }

    std::istringstream iss(body);
    std::string line;
    std::string version;
    std::ostringstream news;

    while (std::getline(iss, line)) {
        // strip trailing \r
        if (!line.empty() && line.back() == '\r') line.pop_back();
        // skip pure whitespace lines until we have a version
        bool onlyWs = true;
        for (char c : line) {
            if (!std::isspace(static_cast<unsigned char>(c))) { onlyWs = false; break; }
        }
        if (version.empty()) {
            if (onlyWs) continue;
            std::string token = line;
            size_t sp = token.find_first_of(" \t");
            if (sp != std::string::npos) token = token.substr(0, sp);
            if (!looksLikeVersion(token)) {
                // First non-empty line is not a version → treat whole body as invalid.
                newsOut.clear();
                return "";
            }
            version = token;
        } else {
            if (!onlyWs) {
                if (news.tellp() > 0) news << '\n';
                news << line;
            }
        }
    }
    newsOut = news.str();
    return version;
}

void printUpdateNotice(const std::string& remoteVer, const std::string& news) {
    std::cerr << "\n";
    std::cerr << "============================================================\n";
    std::cerr << "  Luin update available\n";
    std::cerr << "  You are running  : v" << LUIN_VERSION << " (" << LUIN_PLATFORM << ")\n";
    std::cerr << "  Latest release   : v" << remoteVer << "\n";
    std::cerr << "------------------------------------------------------------\n";
    if (!news.empty()) {
        std::cerr << news << "\n";
        std::cerr << "------------------------------------------------------------\n";
    }
    std::cerr << "  Please update to version " << remoteVer << ".\n";
    std::cerr << "  Download: https://xluin.netlify.app\n";
    std::cerr << "============================================================\n";
    std::cerr << "\n";
}

} // namespace

void checkForUpdates() {
    constexpr long long kCheckIntervalSec = 3600; // 1 hour

    auto now = std::chrono::system_clock::now();
    long long nowSec = std::chrono::duration_cast<std::chrono::seconds>(
                           now.time_since_epoch())
                           .count();

    State state = loadState();
    bool didNetwork = false;

    // Network only when the cooldown has elapsed (or first run).
    if (state.last_check == 0 || (nowSec - state.last_check) >= kCheckIntervalSec) {
        std::string body;
        if (fetchRemoteVersionText(body)) {
            std::string news;
            std::string remote = parseRemote(body, news);
            // Always advance the cooldown after a successful HTTP response,
            // even if the body was not a valid version file (e.g. 404 HTML).
            state.last_check = nowSec;
            state.platform = LUIN_PLATFORM;
            if (!remote.empty()) {
                state.last_remote_version = remote;
                state.news = news;
            }
            didNetwork = true;
        } else {
            // Offline / curl failed: still record the attempt so we don't
            // hammer a dead network path every launch.
            state.last_check = nowSec;
            didNetwork = true;
        }
    }

    // Decide whether to show the notice.
    // Show when we know a remote version that is strictly newer than this binary.
    // While this binary is still outdated the notice is repeated every run.
    bool shouldNotify = false;
    if (!state.last_remote_version.empty() &&
        compareVersions(state.last_remote_version, LUIN_VERSION) > 0) {
        shouldNotify = true;
    }

    if (shouldNotify) {
        printUpdateNotice(state.last_remote_version, state.news);
        state.last_notified_version = state.last_remote_version;
    }

    // Persist whenever we did a network round-trip or we updated the
    // "last notified" marker.
    if (didNetwork || shouldNotify) {
        saveState(state);
    }
}

} // namespace luin
