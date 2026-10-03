# Luin v2.4 → v2.5

## GUI via Raylib

New files: `gui.h` / `gui.cpp` / `gui.sx`

## Features
- Luin can now write Graphics code has shown below:
 ```sx
# gui.sx — visible window demo (stays open until ESC)
import gui

show("Opening window... press ESC to close")

gui.init(800, 600, "Luin GUI Demo")

x = 100
y = 200
dx = 4
dy = 3
frames = 0

while gui.is_open() {
  # close on ESC
  if gui.key_pressed(gui.KEY_ESC) {
    sloop()
  }

  # bounce the circle
  x = x + dx
  y = y + dy
  if x < 40 or x > 760 {
    dx = 0 - dx
  }
  if y < 40 or y > 560 {
    dy = 0 - dy
  }

  gui.begin()
  gui.clear(18, 22, 35)

  # title
  gui.text(20, 20, 32, "Luin GUI is working!", 255, 255, 255)
  gui.text(20, 60, 20, "Press ESC to close", 180, 190, 210)

  # static shapes
  gui.rect(50, 120, 200, 100, 70, 140, 255)
  gui.rect(300, 120, 150, 100, 255, 120, 80)
  gui.line(0, 300, 800, 300, 80, 90, 120)

  # bouncing circle
  gui.circle(x, y, 40, 100, 255, 160)

  # frame counter
  gui.text(20, 560, 18, f"frames: {frames}", 150, 160, 180)

  gui.end()
  frames = frames + 1
}

gui.close()
show("Window closed. Bye!")
```

## Other Features
- Include local pkg, switch for solving long if/els chains of code.

## Compatibility

Existing v2.3 & v2.4 scripts continue to run unchanged in v2.5
