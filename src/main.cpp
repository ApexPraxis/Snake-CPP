#include <raylib.h>
#include <raymath.h>
#include <algorithm>
#include <deque>
#include <fstream>
#include <vector>

// ---------- Config ----------
constexpr int cellSize = 30;
constexpr int cellCount = 25;
constexpr int offset = 75;
constexpr int boardSize = cellSize * cellCount;

const Color lightGreen = {173, 204, 96, 255};
const Color darkGreen = {43, 51, 24, 255};

double lastUpdateTime = 0;

// ---------- Helpers ----------
bool ElementInDeque(const Vector2 &element, const std::deque<Vector2> &body)
{
    for (const Vector2 &segment : body)
    {
        if (Vector2Equals(segment, element))
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
    DrawText(text, (GetScreenWidth() - width) / 2, y, fontSize, darkGreen);
}

// ---------- Snake ----------
class Snake
{
public:
    std::deque<Vector2> body;
    Vector2 direction;                // direction the snake last moved in
    std::deque<Vector2> inputQueue;   // up to 2 buffered turns
    bool addSegment = false;

    Snake() { Reset(); }

    void Draw()
    {
        for (const Vector2 &segmentPos : body)
        {
            Rectangle segment = Rectangle{
                offset + segmentPos.x * cellSize,
                offset + segmentPos.y * cellSize,
                float(cellSize), float(cellSize)};
            DrawRectangleRounded(segment, 0.5f, 6, darkGreen);
        }
    }

    // Buffers a turn, ignoring reversals and duplicates.
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
        if (addSegment)
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
    Vector2 position = {0, 0};
    Texture2D texture;

    Food(const std::deque<Vector2> &snakeBody)
    {
        texture = LoadTexture("Graphics/food.png");
        if (texture.id == 0)
        {
            TraceLog(LOG_WARNING, "Could not load Graphics/food.png (check working directory)");
        }
        GenerateRandomPos(snakeBody);
    }

    ~Food() { UnloadTexture(texture); }

    // A Food owns a texture, so copying it would unload the texture twice.
    Food(const Food &) = delete;
    Food &operator=(const Food &) = delete;

    void Draw()
    {
        DrawTexture(texture,
                    int(offset + position.x * cellSize),
                    int(offset + position.y * cellSize),
                    WHITE);
    }

    // Picks a random free cell. Returns false if the board is full.
    bool GenerateRandomPos(const std::deque<Vector2> &snakeBody)
    {
        std::vector<Vector2> freeCells;
        freeCells.reserve(cellCount * cellCount);
        for (int y = 0; y < cellCount; y++)
        {
            for (int x = 0; x < cellCount; x++)
            {
                Vector2 cell = {float(x), float(y)};
                if (!ElementInDeque(cell, snakeBody))
                {
                    freeCells.push_back(cell);
                }
            }
        }

        if (freeCells.empty())
            return false;

        position = freeCells[GetRandomValue(0, int(freeCells.size()) - 1)];
        return true;
    }
};

// ---------- Game ----------
enum class State
{
    Start,
    Running,
    Paused,
    GameOver,
    Won
};

class Game
{
public:
    Snake snake;
    Food food{snake.body};
    State state = State::Start;
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

    Game(const Game &) = delete;
    Game &operator=(const Game &) = delete;

    // Time between snake moves; gets faster as the score rises.
    double GetInterval() const
    {
        return std::max(0.05, 0.2 - score * 0.005);
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

        switch (state)
        {
        case State::Start:
            if (hasDirection || IsKeyPressed(KEY_SPACE))
            {
                if (hasDirection)
                    snake.QueueDirection(dir);
                StartRunning();
            }
            break;

        case State::Running:
            if (hasDirection)
                snake.QueueDirection(dir);
            if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE))
                state = State::Paused;
            break;

        case State::Paused:
            if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_SPACE))
                StartRunning();
            break;

        case State::GameOver:
        case State::Won:
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))
                Reset();
            break;
        }
    }

    void Update()
    {
        if (state != State::Running)
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

private:
    void StartRunning()
    {
        state = State::Running;
        lastUpdateTime = GetTime(); // avoid an instant first move
    }

    void Reset()
{
    snake.Reset();
    food.GenerateRandomPos(snake.body);
    score = 0;
    lastUpdateTime = GetTime();
    state = State::Start;
}

    void CheckCollisionWithFood()
    {
        if (!Vector2Equals(snake.body[0], food.position))
            return;

        snake.addSegment = true;
        score++;
        PlaySound(eatSound);

        // The snake grows by one on the next move; if that fills the board, you win.
        if (snake.body.size() + 1 >= size_t(cellCount) * cellCount)
        {
            Win();
        }
        else
        {
            food.GenerateRandomPos(snake.body);
        }
    }

    void CheckCollisionWithEdge()
    {
        if (state != State::Running)
            return;

        const Vector2 &head = snake.body[0];
        if (head.x < 0 || head.x >= cellCount || head.y < 0 || head.y >= cellCount)
        {
            GameOver();
        }
    }

    void CheckCollisionWithSelf()
    {
        if (state != State::Running)
            return;

        for (size_t i = 1; i < snake.body.size(); i++)
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
        state = State::GameOver;
        UpdateHighScore();
        PlaySound(wallSound);
    }

    void Win()
    {
        state = State::Won;
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

    void LoadHighScore()
    {
        std::ifstream file("highscore.txt");
        if (file)
        {
            file >> highScore;
        }
    }

    void SaveHighScore() const
    {
        std::ofstream file("highscore.txt");
        if (file)
        {
            file << highScore;
        }
    }

    void DrawOverlay()
    {
        if (state == State::Running)
            return;

        int midY = offset + boardSize / 2;
        DrawRectangle(offset, midY - 70, boardSize, 140, Fade(lightGreen, 0.85f));

        switch (state)
        {
        case State::Start:
            DrawCentered("Press a direction key to start", midY - 35, 40);
            DrawCentered("WASD / Arrows to move, P to pause", midY + 15, 25);
            break;
        case State::Paused:
            DrawCentered("Paused", midY - 35, 50);
            DrawCentered("Press P or Space to resume", midY + 25, 25);
            break;
        case State::GameOver:
            DrawCentered("Game Over", midY - 50, 50);
            DrawCentered(TextFormat("Final score: %i", score), midY + 5, 30);
            DrawCentered("Press Space to play again", midY + 40, 25);
            break;
        case State::Won:
            DrawCentered("You Win!", midY - 50, 50);
            DrawCentered(TextFormat("Final score: %i", score), midY + 5, 30);
            DrawCentered("Press Space to play again", midY + 40, 25);
            break;
        default:
            break;
        }
    }
};

// ---------- Main ----------
int main()
{
    InitWindow(2 * offset + boardSize, 2 * offset + boardSize, "Retro Snake");
    SetTargetFPS(60);

    { // Scope so Game (and its texture/sounds) is destroyed before CloseWindow()
        Game game;

        while (!WindowShouldClose())
        {
            // input and logic
            game.HandleInput();
            game.Update();

            // drawing
            BeginDrawing();
            ClearBackground(lightGreen);
            DrawRectangleLinesEx(
                Rectangle{float(offset) - 5, float(offset) - 5,
                          float(boardSize + 10), float(boardSize + 10)},
                5, darkGreen);
            DrawText("Retro Snake", offset - 5, 20, 40, darkGreen);
            DrawText(TextFormat("Score: %i", game.score), offset - 5, offset + boardSize + 10, 40, darkGreen);

            const char *best = TextFormat("Best: %i", game.highScore);
            DrawText(best, offset + boardSize + 5 - MeasureText(best, 40), offset + boardSize + 10, 40, darkGreen);

            game.Draw();
            EndDrawing();
        }
    }

    CloseWindow();
    return 0;
}