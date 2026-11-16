#include <iostream>
#include <raylib.h>
#include <deque>
#include <raymath.h>

using namespace std;

// ---------- Config ----------
Color green = {173, 204, 96, 255};
Color DarkGreen = {43, 51, 24, 255};

int cellSize = 30;
int cellCount = 25;
int offset = 75;

double lastUpdateTime = 0;

// ---------- Helpers ----------
bool ElementInDeque(const Vector2 &element, const deque<Vector2> &body)
{
    for (unsigned int i = 0; i < body.size(); i++)
    {
        if (Vector2Equals(body[i], element))
        {
            return true;
        }
    }
    return false;
}

bool eventTriggered(double interval)
{
    double currentTime = GetTime();
    if (currentTime - lastUpdateTime >= interval)
    {
        lastUpdateTime = currentTime;
        return true;
    }
    return false;
}

void DrawCentered(const char *text, int y, int fontSize)
{
    int width = MeasureText(text, fontSize);
    DrawText(text, (GetScreenWidth() - width) / 2, y, fontSize, DarkGreen);
}

// ---------- Snake ----------
class Snake
{
public:
    deque<Vector2> body = {Vector2{6, 9}, Vector2{5, 9}, Vector2{4, 9}};
    Vector2 direction = {1, 0};     // direction the snake last moved in
    deque<Vector2> inputQueue;      // up to 2 buffered turns
    bool addSegment = false;

    void Draw()
    {
        for (unsigned int i = 0; i < body.size(); i++)
        {
            float x = body[i].x;
            float y = body[i].y;
            Rectangle segment = Rectangle{offset + x * cellSize, offset + y * cellSize, float(cellSize), float(cellSize)};
            DrawRectangleRounded(segment, 0.5, 6, DarkGreen);
        }
    }

    // Buffers a turn, ignoring reversals and duplicates
    void QueueDirection(Vector2 newDir)
    {
        Vector2 last = inputQueue.empty() ? direction : inputQueue.back();

        if (Vector2Equals(newDir, last))
            return;
        if (Vector2Equals(Vector2Add(newDir, last), Vector2{0, 0}))
            return; // 180 degree turn
        if (inputQueue.size() < 2)
            inputQueue.push_back(newDir);
    }

    void Update()
    {
        if (!inputQueue.empty())
        {
            direction = inputQueue.front();
            inputQueue.pop_front();
        }

        body.push_front(Vector2Add(body[0], direction));
        if (addSegment == true)
        {
            addSegment = false;
        }
        else
        {
            body.pop_back();
        }
    }

    void Reset()
    {
        body = {Vector2{6, 9}, Vector2{5, 9}, Vector2{4, 9}};
        direction = {1, 0};
        inputQueue.clear();
        addSegment = false;
    }
};

// ---------- Food ----------
class Food
{
public:
    Vector2 position;
    Texture2D texture;

    Food(const deque<Vector2> &snakeBody)
    {
        texture = LoadTexture("Graphics/food.png");
        if (texture.id == 0)
        {
            TraceLog(LOG_WARNING, "Could not load Graphics/food.png (check working directory)");
        }
        position = GenerateRandomPos(snakeBody);
    }

    ~Food()
    {
        UnloadTexture(texture);
    }

    // Food owns a texture, so copying it would unload the texture twice
    Food(const Food &) = delete;
    Food &operator=(const Food &) = delete;

    void Draw()
    {
        DrawTexture(texture, offset + position.x * cellSize, offset + position.y * cellSize, WHITE);
    }

    Vector2 GenerateRandomCell()
    {
        float x = GetRandomValue(0, cellCount - 1);
        float y = GetRandomValue(0, cellCount - 1);
        return Vector2{x, y};
    }

    // Only call this when the board is not full (Game checks for that)
    Vector2 GenerateRandomPos(const deque<Vector2> &snakeBody)
    {
        Vector2 pos = GenerateRandomCell();
        while (ElementInDeque(pos, snakeBody))
        {
            pos = GenerateRandomCell();
        }
        return pos;
    }
};

// ---------- Game ----------
enum GameState
{
    STATE_START,
    STATE_RUNNING,
    STATE_PAUSED,
    STATE_GAMEOVER,
    STATE_WON
};

class Game
{
public:
    Snake snake;
    Food food{snake.body};
    GameState state = STATE_START;
    int score = 0;
    int highScore = 0;
    Sound eatSound;
    Sound wallSound;

    Game()
    {
        InitAudioDevice();
        eatSound = LoadSound("Sounds/eat.mp3");
        wallSound = LoadSound("Sounds/wall.mp3");
        LoadHighScore();
    }

    ~Game()
    {
        UnloadSound(eatSound);
        UnloadSound(wallSound);
        CloseAudioDevice();
    }

    // Time between snake moves; gets faster as the score rises
    double GetInterval()
    {
        double interval = 0.2 - score * 0.005;
        if (interval < 0.05)
        {
            interval = 0.05;
        }
        return interval;
    }

    void HandleInput()
    {
        Vector2 dir = {0, 0};
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
            dir = {0, -1};
        else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
            dir = {0, 1};
        else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
            dir = {-1, 0};
        else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
            dir = {1, 0};

        bool hasDirection = (dir.x != 0 || dir.y != 0);

        if (state == STATE_START)
        {
            if (hasDirection || IsKeyPressed(KEY_SPACE))
            {
                if (hasDirection)
                    snake.QueueDirection(dir);
                StartRunning();
            }
        }
        else if (state == STATE_RUNNING)
        {
            if (hasDirection)
                snake.QueueDirection(dir);
            if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE))
                state = STATE_PAUSED;
        }
        else if (state == STATE_PAUSED)
        {
            if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE))
                StartRunning();
        }
        else // game over or won
        {
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))
                Reset();
        }
    }

    void Update()
    {
        if (state != STATE_RUNNING)
            return;

        if (!eventTriggered(GetInterval()))
            return;

        snake.Update();
        CheckCollisionWithFood();
        CheckCollisionWithEdge();
        CheckCollisionWithSelf();
    }

    void Draw()
    {
        food.Draw();
        snake.Draw();
        DrawOverlay();
    }

    void StartRunning()
    {
        state = STATE_RUNNING;
        lastUpdateTime = GetTime(); // avoid an instant first move
    }

    void Reset()
    {
        snake.Reset();
        food.position = food.GenerateRandomPos(snake.body);
        score = 0;
        state = STATE_START;
    }

    void CheckCollisionWithFood()
    {
        if (Vector2Equals(snake.body[0], food.position))
        {
            snake.addSegment = true;
            score++;
            PlaySound(eatSound);

            // The snake grows by one on the next move; if that fills the board, you win
            if (snake.body.size() + 1 >= (unsigned int)(cellCount * cellCount))
            {
                Win();
            }
            else
            {
                food.position = food.GenerateRandomPos(snake.body);
            }
        }
    }

    void CheckCollisionWithEdge()
    {
        if (state != STATE_RUNNING)
            return;

        if (snake.body[0].x >= cellCount || snake.body[0].x < 0)
        {
            GameOver();
        }
        else if (snake.body[0].y >= cellCount || snake.body[0].y < 0)
        {
            GameOver();
        }
    }

    void CheckCollisionWithSelf()
    {
        if (state != STATE_RUNNING)
            return;

        for (unsigned int i = 1; i < snake.body.size(); i++)
        {
            if (Vector2Equals(snake.body[0], snake.body[i]))
            {
                GameOver();
                return;
            }
        }
    }

    void GameOver()
    {
        state = STATE_GAMEOVER;
        UpdateHighScore();
        PlaySound(wallSound);
    }

    void Win()
    {
        state = STATE_WON;
        UpdateHighScore();
    }

    void UpdateHighScore()
    {
        if (score > highScore)
        {
            highScore = score;
            SaveHighScore();
        }
    }

    // High score is stored with raylib's own file functions
    void LoadHighScore()
    {
        if (FileExists("highscore.txt"))
        {
            char *text = LoadFileText("highscore.txt");
            if (text != NULL)
            {
                highScore = TextToInteger(text);
                UnloadFileText(text);
            }
        }
    }

    void SaveHighScore()
    {
        SaveFileText("highscore.txt", (char *)TextFormat("%i", highScore));
    }

    void DrawOverlay()
    {
        if (state == STATE_RUNNING)
            return;

        int boardSize = cellSize * cellCount;
        int midY = offset + boardSize / 2;
        DrawRectangle(offset, midY - 70, boardSize, 140, Fade(green, 0.85f));

        if (state == STATE_START)
        {
            DrawCentered("Press a direction key to start", midY - 35, 40);
            DrawCentered("WASD / Arrows to move, P to pause", midY + 15, 25);
        }
        else if (state == STATE_PAUSED)
        {
            DrawCentered("Paused", midY - 35, 50);
            DrawCentered("Press P or Space to resume", midY + 25, 25);
        }
        else if (state == STATE_GAMEOVER)
        {
            DrawCentered("Game Over", midY - 50, 50);
            DrawCentered(TextFormat("Final score: %i", score), midY + 5, 30);
            DrawCentered("Press Space to play again", midY + 40, 25);
        }
        else if (state == STATE_WON)
        {
            DrawCentered("You Win!", midY - 50, 50);
            DrawCentered(TextFormat("Final score: %i", score), midY + 5, 30);
            DrawCentered("Press Space to play again", midY + 40, 25);
        }
    }
};

// ---------- Main ----------
int main()
{
    cout << "Starting the game..." << endl;
    InitWindow(2 * offset + cellSize * cellCount, 2 * offset + cellSize * cellCount, "Retro Snake");
    SetTargetFPS(60);

    { // Scope so Game (and its texture/sounds) is destroyed before CloseWindow()
        Game game;

        while (WindowShouldClose() == false)
        {
            // input and logic
            game.HandleInput();
            game.Update();

            // drawing
            BeginDrawing();
            ClearBackground(green);
            DrawRectangleLinesEx(Rectangle{float(offset) - 5, float(offset) - 5, float(cellSize * cellCount + 10), float(cellSize * cellCount + 10)}, 5, DarkGreen);
            DrawText("Retro Snake", offset - 5, 20, 40, DarkGreen);
            DrawText(TextFormat("Score: %i", game.score), offset - 5, offset + cellSize * cellCount + 10, 40, DarkGreen);

            const char *best = TextFormat("Best: %i", game.highScore);
            DrawText(best, offset + cellSize * cellCount + 5 - MeasureText(best, 40), offset + cellSize * cellCount + 10, 40, DarkGreen);

            game.Draw();
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}