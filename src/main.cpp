#include<iostream>
#include "raylib.h"

int main() {
    InitWindow(800, 600, "Snake CPP");

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("Raylib is working!", 250, 280, 30, GREEN);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}