#include "gui.h"

#include <cmath>
#include "raylib.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

namespace luin {

namespace {

bool g_windowOpen = false;
std::unordered_map<int, Texture2D> g_textures;
std::unordered_map<int, Sound> g_sounds;
int g_nextTexId = 1;
int g_nextSndId = 1;
Camera2D g_cam2d{};
bool g_cam2dActive = false;

Value makeNative(std::string name, std::function<Value(std::vector<Value>&)> fn) {
    auto nf = std::make_shared<NativeFunction>();
    nf->name = std::move(name);
    nf->fn = std::move(fn);
    return nf;
}

int asInt(const Value& v, const std::string& fn, size_t i = 0) {
    if (std::holds_alternative<int>(v)) return std::get<int>(v);
    if (std::holds_alternative<double>(v)) return static_cast<int>(std::get<double>(v));
    throw std::runtime_error("gui." + fn + "(): argument " + std::to_string(i + 1) + " must be a number");
}

double asNum(const Value& v, const std::string& fn, size_t i = 0) {
    if (std::holds_alternative<int>(v)) return static_cast<double>(std::get<int>(v));
    if (std::holds_alternative<double>(v)) return std::get<double>(v);
    throw std::runtime_error("gui." + fn + "(): argument " + std::to_string(i + 1) + " must be a number");
}

const std::string& asStr(const Value& v, const std::string& fn, size_t i = 0) {
    if (!std::holds_alternative<std::string>(v))
        throw std::runtime_error("gui." + fn + "(): argument " + std::to_string(i + 1) + " must be a string");
    return std::get<std::string>(v);
}

Color rgba(int r, int g, int b, int a = 255) {
    return Color{
        static_cast<unsigned char>(r < 0 ? 0 : (r > 255 ? 255 : r)),
        static_cast<unsigned char>(g < 0 ? 0 : (g > 255 ? 255 : g)),
        static_cast<unsigned char>(b < 0 ? 0 : (b > 255 ? 255 : b)),
        static_cast<unsigned char>(a < 0 ? 0 : (a > 255 ? 255 : a))
    };
}

void requireWindow(const std::string& fn) {
    if (!g_windowOpen)
        throw std::runtime_error("gui." + fn + "(): window not open — call gui.init() first");
}

} // namespace

std::shared_ptr<Module> createGuiModule() {
    auto mod = std::make_shared<Module>();
    mod->name = "gui";

    // 1. init(width, height, title)
    mod->members["init"] = makeNative("init", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2)
            throw std::runtime_error("gui.init() expects (width, height [, title])");
        int w = asInt(args[0], "init", 0);
        int h = asInt(args[1], "init", 1);
        std::string title = args.size() >= 3 ? asStr(args[2], "init", 2) : "Luin GUI";
        if (g_windowOpen) CloseWindow();
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
        InitWindow(w, h, title.c_str());
        InitAudioDevice();
        SetTargetFPS(60);
        g_windowOpen = true;
        g_cam2d = {0};
        g_cam2d.zoom = 1.0f;
        g_cam2dActive = false;
        return true;
    });

    // 2. close()
    mod->members["close"] = makeNative("close", [](std::vector<Value>&) -> Value {
        if (g_windowOpen) {
            for (auto& p : g_textures) UnloadTexture(p.second);
            g_textures.clear();
            for (auto& p : g_sounds) UnloadSound(p.second);
            g_sounds.clear();
            CloseAudioDevice();
            CloseWindow();
            g_windowOpen = false;
        }
        return std::string("");
    });

    // 3. is_open()
    mod->members["is_open"] = makeNative("is_open", [](std::vector<Value>&) -> Value {
        if (!g_windowOpen) return false;
        return !WindowShouldClose();
    });

    // 4. begin() — BeginDrawing
    mod->members["begin"] = makeNative("begin", [](std::vector<Value>&) -> Value {
        requireWindow("begin");
        BeginDrawing();
        if (g_cam2dActive) BeginMode2D(g_cam2d);
        return std::string("");
    });

    // 5. end()
    mod->members["end"] = makeNative("end", [](std::vector<Value>&) -> Value {
        requireWindow("end");
        if (g_cam2dActive) EndMode2D();
        EndDrawing();
        return std::string("");
    });

    // 6. clear(r,g,b [,a])
    mod->members["clear"] = makeNative("clear", [](std::vector<Value>& args) -> Value {
        requireWindow("clear");
        if (args.size() < 3)
            throw std::runtime_error("gui.clear() expects (r, g, b [, a])");
        int a = args.size() >= 4 ? asInt(args[3], "clear", 3) : 255;
        ClearBackground(rgba(asInt(args[0],"clear",0), asInt(args[1],"clear",1), asInt(args[2],"clear",2), a));
        return std::string("");
    });

    // 7. fps([target])
    mod->members["fps"] = makeNative("fps", [](std::vector<Value>& args) -> Value {
        if (!args.empty()) {
            SetTargetFPS(asInt(args[0], "fps", 0));
            return std::string("");
        }
        return GetFPS();
    });

    // 8. delta()
    mod->members["delta"] = makeNative("delta", [](std::vector<Value>&) -> Value {
        return static_cast<double>(GetFrameTime());
    });

    // 9. time()
    mod->members["time"] = makeNative("time", [](std::vector<Value>&) -> Value {
        return static_cast<double>(GetTime());
    });

    // 10. width()
    mod->members["width"] = makeNative("width", [](std::vector<Value>&) -> Value {
        requireWindow("width");
        return GetScreenWidth();
    });

    // 11. height()
    mod->members["height"] = makeNative("height", [](std::vector<Value>&) -> Value {
        requireWindow("height");
        return GetScreenHeight();
    });

    // 12. set_title(s)
    mod->members["set_title"] = makeNative("set_title", [](std::vector<Value>& args) -> Value {
        requireWindow("set_title");
        if (args.empty()) throw std::runtime_error("gui.set_title() expects a string");
        SetWindowTitle(asStr(args[0], "set_title").c_str());
        return std::string("");
    });

    // 13. rect(x,y,w,h,r,g,b [,a])
    mod->members["rect"] = makeNative("rect", [](std::vector<Value>& args) -> Value {
        requireWindow("rect");
        if (args.size() < 7)
            throw std::runtime_error("gui.rect() expects (x,y,w,h,r,g,b [,a])");
        int a = args.size() >= 8 ? asInt(args[7],"rect",7) : 255;
        DrawRectangle(
            asInt(args[0],"rect",0), asInt(args[1],"rect",1),
            asInt(args[2],"rect",2), asInt(args[3],"rect",3),
            rgba(asInt(args[4],"rect",4), asInt(args[5],"rect",5), asInt(args[6],"rect",6), a));
        return std::string("");
    });

    // 14. rect_lines(x,y,w,h,r,g,b [,a])
    mod->members["rect_lines"] = makeNative("rect_lines", [](std::vector<Value>& args) -> Value {
        requireWindow("rect_lines");
        if (args.size() < 7)
            throw std::runtime_error("gui.rect_lines() expects (x,y,w,h,r,g,b [,a])");
        int a = args.size() >= 8 ? asInt(args[7],"rect_lines",7) : 255;
        DrawRectangleLines(
            asInt(args[0],"rect_lines",0), asInt(args[1],"rect_lines",1),
            asInt(args[2],"rect_lines",2), asInt(args[3],"rect_lines",3),
            rgba(asInt(args[4],"rect_lines",4), asInt(args[5],"rect_lines",5), asInt(args[6],"rect_lines",6), a));
        return std::string("");
    });

    // 15. circle(x,y,radius,r,g,b [,a])
    mod->members["circle"] = makeNative("circle", [](std::vector<Value>& args) -> Value {
        requireWindow("circle");
        if (args.size() < 6)
            throw std::runtime_error("gui.circle() expects (x,y,radius,r,g,b [,a])");
        int a = args.size() >= 7 ? asInt(args[6],"circle",6) : 255;
        DrawCircle(
            asInt(args[0],"circle",0), asInt(args[1],"circle",1),
            static_cast<float>(asNum(args[2],"circle",2)),
            rgba(asInt(args[3],"circle",3), asInt(args[4],"circle",4), asInt(args[5],"circle",5), a));
        return std::string("");
    });

    // 16. circle_lines(x,y,radius,r,g,b [,a])
    mod->members["circle_lines"] = makeNative("circle_lines", [](std::vector<Value>& args) -> Value {
        requireWindow("circle_lines");
        if (args.size() < 6)
            throw std::runtime_error("gui.circle_lines() expects (x,y,radius,r,g,b [,a])");
        int a = args.size() >= 7 ? asInt(args[6],"circle_lines",6) : 255;
        DrawCircleLines(
            asInt(args[0],"circle_lines",0), asInt(args[1],"circle_lines",1),
            static_cast<float>(asNum(args[2],"circle_lines",2)),
            rgba(asInt(args[3],"circle_lines",3), asInt(args[4],"circle_lines",4), asInt(args[5],"circle_lines",5), a));
        return std::string("");
    });

    // 17. line(x1,y1,x2,y2,r,g,b [,a])
    mod->members["line"] = makeNative("line", [](std::vector<Value>& args) -> Value {
        requireWindow("line");
        if (args.size() < 7)
            throw std::runtime_error("gui.line() expects (x1,y1,x2,y2,r,g,b [,a])");
        int a = args.size() >= 8 ? asInt(args[7],"line",7) : 255;
        DrawLine(
            asInt(args[0],"line",0), asInt(args[1],"line",1),
            asInt(args[2],"line",2), asInt(args[3],"line",3),
            rgba(asInt(args[4],"line",4), asInt(args[5],"line",5), asInt(args[6],"line",6), a));
        return std::string("");
    });

    // 18. pixel(x,y,r,g,b [,a])
    mod->members["pixel"] = makeNative("pixel", [](std::vector<Value>& args) -> Value {
        requireWindow("pixel");
        if (args.size() < 5)
            throw std::runtime_error("gui.pixel() expects (x,y,r,g,b [,a])");
        int a = args.size() >= 6 ? asInt(args[5],"pixel",5) : 255;
        DrawPixel(
            asInt(args[0],"pixel",0), asInt(args[1],"pixel",1),
            rgba(asInt(args[2],"pixel",2), asInt(args[3],"pixel",3), asInt(args[4],"pixel",4), a));
        return std::string("");
    });

    // 19. text(x,y,size,str,r,g,b [,a])
    mod->members["text"] = makeNative("text", [](std::vector<Value>& args) -> Value {
        requireWindow("text");
        if (args.size() < 7)
            throw std::runtime_error("gui.text() expects (x,y,size,str,r,g,b [,a])");
        int a = args.size() >= 8 ? asInt(args[7],"text",7) : 255;
        DrawText(
            asStr(args[3],"text",3).c_str(),
            asInt(args[0],"text",0), asInt(args[1],"text",1),
            asInt(args[2],"text",2),
            rgba(asInt(args[4],"text",4), asInt(args[5],"text",5), asInt(args[6],"text",6), a));
        return std::string("");
    });

    // 20. text_width(str, size)
    mod->members["text_width"] = makeNative("text_width", [](std::vector<Value>& args) -> Value {
        if (args.size() < 2)
            throw std::runtime_error("gui.text_width() expects (str, size)");
        return MeasureText(asStr(args[0],"text_width").c_str(), asInt(args[1],"text_width",1));
    });

    // 21. key_down(keyCode)  — key as int (raylib KEY_*)
    mod->members["key_down"] = makeNative("key_down", [](std::vector<Value>& args) -> Value {
        requireWindow("key_down");
        if (args.empty()) throw std::runtime_error("gui.key_down() expects key code");
        return IsKeyDown(asInt(args[0],"key_down"));
    });

    // 22. key_pressed(keyCode)
    mod->members["key_pressed"] = makeNative("key_pressed", [](std::vector<Value>& args) -> Value {
        requireWindow("key_pressed");
        if (args.empty()) throw std::runtime_error("gui.key_pressed() expects key code");
        return IsKeyPressed(asInt(args[0],"key_pressed"));
    });

    // 23. key_released(keyCode)
    mod->members["key_released"] = makeNative("key_released", [](std::vector<Value>& args) -> Value {
        requireWindow("key_released");
        if (args.empty()) throw std::runtime_error("gui.key_released() expects key code");
        return IsKeyReleased(asInt(args[0],"key_released"));
    });

    // 24. mouse_x()
    mod->members["mouse_x"] = makeNative("mouse_x", [](std::vector<Value>&) -> Value {
        requireWindow("mouse_x");
        return GetMouseX();
    });

    // 25. mouse_y()
    mod->members["mouse_y"] = makeNative("mouse_y", [](std::vector<Value>&) -> Value {
        requireWindow("mouse_y");
        return GetMouseY();
    });

    // 26. mouse_down(button)  0=left 1=right 2=middle
    mod->members["mouse_down"] = makeNative("mouse_down", [](std::vector<Value>& args) -> Value {
        requireWindow("mouse_down");
        int btn = args.empty() ? 0 : asInt(args[0],"mouse_down");
        return IsMouseButtonDown(btn);
    });

    // 27. mouse_pressed(button)
    mod->members["mouse_pressed"] = makeNative("mouse_pressed", [](std::vector<Value>& args) -> Value {
        requireWindow("mouse_pressed");
        int btn = args.empty() ? 0 : asInt(args[0],"mouse_pressed");
        return IsMouseButtonPressed(btn);
    });

    // 28. mouse_wheel()
    mod->members["mouse_wheel"] = makeNative("mouse_wheel", [](std::vector<Value>&) -> Value {
        requireWindow("mouse_wheel");
        return static_cast<double>(GetMouseWheelMove());
    });

    // 29. triangle(x1,y1,x2,y2,x3,y3,r,g,b [,a])
    mod->members["triangle"] = makeNative("triangle", [](std::vector<Value>& args) -> Value {
        requireWindow("triangle");
        if (args.size() < 9)
            throw std::runtime_error("gui.triangle() expects (x1,y1,x2,y2,x3,y3,r,g,b [,a])");
        int a = args.size() >= 10 ? asInt(args[9],"triangle",9) : 255;
        DrawTriangle(
            Vector2{static_cast<float>(asNum(args[0],"triangle",0)), static_cast<float>(asNum(args[1],"triangle",1))},
            Vector2{static_cast<float>(asNum(args[2],"triangle",2)), static_cast<float>(asNum(args[3],"triangle",3))},
            Vector2{static_cast<float>(asNum(args[4],"triangle",4)), static_cast<float>(asNum(args[5],"triangle",5))},
            rgba(asInt(args[6],"triangle",6), asInt(args[7],"triangle",7), asInt(args[8],"triangle",8), a));
        return std::string("");
    });

    // 30. ellipse(x,y,rx,ry,r,g,b [,a])
    mod->members["ellipse"] = makeNative("ellipse", [](std::vector<Value>& args) -> Value {
        requireWindow("ellipse");
        if (args.size() < 7)
            throw std::runtime_error("gui.ellipse() expects (x,y,rx,ry,r,g,b [,a])");
        int a = args.size() >= 8 ? asInt(args[7],"ellipse",7) : 255;
        DrawEllipse(
            asInt(args[0],"ellipse",0), asInt(args[1],"ellipse",1),
            static_cast<float>(asNum(args[2],"ellipse",2)), static_cast<float>(asNum(args[3],"ellipse",3)),
            rgba(asInt(args[4],"ellipse",4), asInt(args[5],"ellipse",5), asInt(args[6],"ellipse",6), a));
        return std::string("");
    });

    // 31. load_texture(path) -> id
    mod->members["load_texture"] = makeNative("load_texture", [](std::vector<Value>& args) -> Value {
        requireWindow("load_texture");
        if (args.empty()) throw std::runtime_error("gui.load_texture() expects path");
        Texture2D tex = LoadTexture(asStr(args[0],"load_texture").c_str());
        int id = g_nextTexId++;
        g_textures[id] = tex;
        return id;
    });

    // 32. draw_texture(id, x, y [,tint_r,g,b,a])
    mod->members["draw_texture"] = makeNative("draw_texture", [](std::vector<Value>& args) -> Value {
        requireWindow("draw_texture");
        if (args.size() < 3)
            throw std::runtime_error("gui.draw_texture() expects (id, x, y [,r,g,b,a])");
        int id = asInt(args[0],"draw_texture");
        auto it = g_textures.find(id);
        if (it == g_textures.end())
            throw std::runtime_error("gui.draw_texture(): invalid texture id");
        Color tint = WHITE;
        if (args.size() >= 7)
            tint = rgba(asInt(args[3],"draw_texture",3), asInt(args[4],"draw_texture",4),
                        asInt(args[5],"draw_texture",5), asInt(args[6],"draw_texture",6));
        DrawTexture(it->second, asInt(args[1],"draw_texture",1), asInt(args[2],"draw_texture",2), tint);
        return std::string("");
    });

    // 33. unload_texture(id)
    mod->members["unload_texture"] = makeNative("unload_texture", [](std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("gui.unload_texture() expects id");
        int id = asInt(args[0],"unload_texture");
        auto it = g_textures.find(id);
        if (it != g_textures.end()) {
            UnloadTexture(it->second);
            g_textures.erase(it);
        }
        return std::string("");
    });

    // 34. load_sound(path) -> id
    mod->members["load_sound"] = makeNative("load_sound", [](std::vector<Value>& args) -> Value {
        requireWindow("load_sound");
        if (args.empty()) throw std::runtime_error("gui.load_sound() expects path");
        Sound s = LoadSound(asStr(args[0],"load_sound").c_str());
        int id = g_nextSndId++;
        g_sounds[id] = s;
        return id;
    });

    // 35. play_sound(id)
    mod->members["play_sound"] = makeNative("play_sound", [](std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("gui.play_sound() expects id");
        int id = asInt(args[0],"play_sound");
        auto it = g_sounds.find(id);
        if (it != g_sounds.end()) PlaySound(it->second);
        return std::string("");
    });

    // 36. set_camera(offset_x, offset_y, target_x, target_y, rotation, zoom)
    mod->members["set_camera"] = makeNative("set_camera", [](std::vector<Value>& args) -> Value {
        if (args.size() < 6)
            throw std::runtime_error("gui.set_camera() expects (ox,oy,tx,ty,rot,zoom)");
        g_cam2d.offset = Vector2{static_cast<float>(asNum(args[0],"set_camera",0)),
                                 static_cast<float>(asNum(args[1],"set_camera",1))};
        g_cam2d.target = Vector2{static_cast<float>(asNum(args[2],"set_camera",2)),
                                 static_cast<float>(asNum(args[3],"set_camera",3))};
        g_cam2d.rotation = static_cast<float>(asNum(args[4],"set_camera",4));
        g_cam2d.zoom = static_cast<float>(asNum(args[5],"set_camera",5));
        g_cam2dActive = true;
        return std::string("");
    });

    // 37. camera_off()
    mod->members["camera_off"] = makeNative("camera_off", [](std::vector<Value>&) -> Value {
        g_cam2dActive = false;
        return std::string("");
    });

    // 38. set_cursor(visible)  true/false
    mod->members["set_cursor"] = makeNative("set_cursor", [](std::vector<Value>& args) -> Value {
        requireWindow("set_cursor");
        bool vis = true;
        if (!args.empty()) {
            if (std::holds_alternative<bool>(args[0])) vis = std::get<bool>(args[0]);
            else vis = asInt(args[0],"set_cursor") != 0;
        }
        if (vis) ShowCursor(); else HideCursor();
        return std::string("");
    });

    // 39. screenshot(path)
    mod->members["screenshot"] = makeNative("screenshot", [](std::vector<Value>& args) -> Value {
        requireWindow("screenshot");
        if (args.empty()) throw std::runtime_error("gui.screenshot() expects path");
        TakeScreenshot(asStr(args[0],"screenshot").c_str());
        return std::string("");
    });

    // 40. random_color() -> [r,g,b]
    mod->members["random_color"] = makeNative("random_color", [](std::vector<Value>&) -> Value {
        auto arr = std::make_shared<ValueArray>();
        arr->elements.push_back(GetRandomValue(0, 255));
        arr->elements.push_back(GetRandomValue(0, 255));
        arr->elements.push_back(GetRandomValue(0, 255));
        return arr;
    });

    // Bonus constants for keys (common ones)
    mod->members["KEY_SPACE"] = 32;
    mod->members["KEY_ENTER"] = 257;
    mod->members["KEY_ESC"] = 256;
    mod->members["KEY_LEFT"] = 263;
    mod->members["KEY_RIGHT"] = 262;
    mod->members["KEY_UP"] = 265;
    mod->members["KEY_DOWN"] = 264;
    mod->members["KEY_A"] = 65;
    mod->members["KEY_W"] = 87;
    mod->members["KEY_S"] = 83;
    mod->members["KEY_D"] = 68;

    return mod;
}

} // namespace luin
