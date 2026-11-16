# 🐍 Snake-CPP

A classic Snake game built using **C++17** and **Raylib**, featuring object-oriented programming, collision detection, sound effects, and a simple retro-inspired design.

## 🎮 Overview

Snake-CPP is a recreation of the classic Snake arcade game. Control the snake, collect food, increase your score, and avoid colliding with walls or yourself.

This project was created as part of my journey into C++ programming, object-oriented programming, and game development.

## ✨ Features

- Classic Snake gameplay
- Object-oriented design using C++ classes
- Smooth, grid-based movement
- Food generation at random positions
- Wall and self-collision detection
- Score tracking
- Sound effects for eating food and hitting walls
- Keyboard controls using WASD or arrow keys

## 🎮 Controls

| Key | Action |
|---|---|
| ↑ / W | Move Up |
| ↓ / S | Move Down |
| ← / A | Move Left |
| → / D | Move Right |

## 🛠️ Technologies Used

- **C++17** – Core programming language
- **Raylib** – Graphics, audio, and game window
- **Raymath** – Vector mathematics
- **vcpkg** – Dependency management

## 📁 Project Structure

```text
Snake-CPP/
├── .vscode/
│   ├── c_cpp_properties.json
│   ├── launch.json
│   └── tasks.json
├── src/
│   ├── main.cpp
│   ├── Graphics/
│   │   └── food.png
│   └── Sounds/
│       ├── eat.mp3
│       └── wall.mp3
├── .gitignore
├── README.md
└── vcpkg.json
```

## ⚙️ Requirements

- Windows
- Visual Studio Code
- C++17-compatible compiler (MinGW)
- Raylib
- vcpkg

## 🚀 Build and Run

1. Clone the repository:

   ```bash
   git clone https://github.com/ApexPraxis/Snake-CPP.git
   ```

2. Open the project folder in Visual Studio Code.

3. Install the project dependencies through vcpkg using the `vcpkg.json` manifest and the `x64-mingw-dynamic` triplet.

4. Ensure your Raylib and MinGW toolchain paths match the configuration in `.vscode/tasks.json`.

5. Press `Ctrl + Shift + B` to run the **Build Snake** task.

6. Run the generated executable using the configured working directory.

## 🧠 Concepts Practiced

- Classes and objects
- Encapsulation
- Constructors and destructors
- STL containers (`std::deque`)
- Functions and references
- Collision detection
- Game loops and frame timing
- Resource management
- Vector operations

## 🔮 Future Improvements

- [ ] Separate classes into individual header and source files
- [ ] Add a main menu and game-over screen
- [ ] Implement a persistent high-score system
- [ ] Add difficulty levels
- [ ] Improve animations and visual effects
- [ ] Introduce CMake support
- [ ] Add automated build testing with GitHub Actions

## 👨‍💻 Author

**ApexPraxis**

GitHub: https://github.com/ApexPraxis

---

*Built with C++, curiosity, and a willingness to learn by creating.*
