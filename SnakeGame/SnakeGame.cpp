#include <iostream>
#include <vector>
#include <deque>
#include <Windows.h>
#include <random>
#include <map>
#include <chrono>
#include <thread>
#include <conio.h>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

// Константы
const char CHAR_APPLE = '$';
const char CHAR_FILL_FIELD = ' ';
const char CHAR_SNAKE = 'o';
const char HEAD_SNAKE_CHAR = '@';
const char WALL_CHAR = '#';
const char UP_DIRECTION = 'w';
const char LEFT_DIRECTION = 'a';
const char DOWN_DIRECTION = 's';
const char RIGHT_DIRECTION = 'd';
const char EXIT_CHAR = 'q';

const int COUNT_ROW = 25;
const int COUNT_COLUMN = 80;
const int TARGET_FPS = 10;
const int FRAME_DURATION = 1000 / TARGET_FPS;

std::map<char, char> LAYOUT_MAP = {
    {'w', 'w'}, {'a', 'a'}, {'s', 's'}, {'d', 'd'}, // Английские
    {'ц', 'w'}, {'ф', 'a'}, {'ы', 's'}, {'в', 'd'}, // Русские
    {'q', 'q'}, {'й', 'q'}                          // Выход
};

// Класс для управления экраном
class ScreenBuffer {
    HANDLE hConsole;
    CHAR_INFO buffer[COUNT_ROW][COUNT_COLUMN];

public:
    ScreenBuffer() {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SMALL_RECT windowSize = { 0, 0, COUNT_COLUMN - 1, COUNT_ROW - 1 };
        COORD bufferSize = { COUNT_COLUMN, COUNT_ROW };
        SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
        SetConsoleScreenBufferSize(hConsole, bufferSize);
    }

    void draw(int x, int y, char symbol) {
        buffer[y][x].Char.AsciiChar = symbol;
        buffer[y][x].Attributes = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE;
    }

    void render() {
        SMALL_RECT writeRegion = { 0, 0, COUNT_COLUMN - 1, COUNT_ROW - 1 };
        COORD bufferSize = { COUNT_COLUMN, COUNT_ROW };
        COORD bufferCoord = { 0, 0 };
        WriteConsoleOutput(hConsole, (CHAR_INFO*)buffer, bufferSize, bufferCoord, &writeRegion);
    }

    void clear() {
        for (int y = 0; y < COUNT_ROW; ++y) {
            for (int x = 0; x < COUNT_COLUMN; ++x) {
                draw(x, y, CHAR_FILL_FIELD);
            }
        }
    }
};

// Класс для змейки
class Snake {
    deque<pair<int, int>> segments;

public:
    Snake() {
        segments.push_back({ COUNT_COLUMN / 2, COUNT_ROW / 2 });
        segments.push_back({ COUNT_COLUMN / 2 + 1, COUNT_ROW / 2 });
    }

    const auto& getHead() const { return segments.front(); }
    const auto& getSegments() const { return segments; }

    void move(char direction, bool grow) {
        auto [headX, headY] = segments.front();
        switch (direction) {
        case UP_DIRECTION:    --headY; break;
        case DOWN_DIRECTION:  ++headY; break;
        case LEFT_DIRECTION:  --headX; break;
        case RIGHT_DIRECTION: ++headX; break;
        }
        segments.push_front({ headX, headY });
        if (!grow) {
            segments.pop_back();
        }
    }

    bool checkCollision() const {
        const auto& head = segments.front();
        return count(segments.begin() + 1, segments.end(), head) > 0;
    }
};

// Класс для яблока
class Apple {
    int x, y;

public:
    void spawn(const vector<vector<char>>& field) {
        static random_device dev;
        static mt19937 rng(dev());
        uniform_int_distribution<int> distX(1, COUNT_COLUMN - 2);
        uniform_int_distribution<int> distY(1, COUNT_ROW - 2);

        do {
            x = distX(rng);
            y = distY(rng);
        } while (field[y][x] != CHAR_FILL_FIELD);
    }

    bool isEaten(const Snake& snake) const {
        auto [headX, headY] = snake.getHead();
        return headX == x && headY == y;
    }

    int getX() const { return x; }
    int getY() const { return y; }
};

// Класс для игрового поля
class Field {
    vector<vector<char>> field;

public:
    Field() : field(COUNT_ROW, vector<char>(COUNT_COLUMN, CHAR_FILL_FIELD)) {
        for (int y = 0; y < COUNT_ROW; ++y) {
            for (int x = 0; x < COUNT_COLUMN; ++x) {
                if (y == 0 || y == COUNT_ROW - 1 || x == 0 || x == COUNT_COLUMN - 1) {
                    field[y][x] = WALL_CHAR;
                }
            }
        }
    }

    const auto& getField() const { return field; }

    void update(const Snake& snake, const Apple& apple) {
        clear();
        for (const auto& segment : snake.getSegments()) {
            auto [x, y] = segment;
            field[y][x] = CHAR_SNAKE;
        }
        field[snake.getHead().second][snake.getHead().first] = HEAD_SNAKE_CHAR;
        field[apple.getY()][apple.getX()] = CHAR_APPLE;
    }

private:
    void clear() {
        for (int y = 1; y < COUNT_ROW - 1; ++y) {
            for (int x = 1; x < COUNT_COLUMN - 1; ++x) {
                field[y][x] = CHAR_FILL_FIELD;
            }
        }
    }
};

// Класс для игры
class Game {
    ScreenBuffer screen;
    Field field;
    Snake snake;
    Apple apple;
    char direction = LEFT_DIRECTION;

public:
    void run() {
        apple.spawn(field.getField());
        while (true) {
            auto frameStart = high_resolution_clock::now();
            if (processInput()) break;
            update();
            render();
            limitFPS(frameStart);
        }
    }

private:
    bool processInput() {
        if (_kbhit()) { // Проверка, нажата ли клавиша
            char input = static_cast<char>(_getch()); // Чтение нажатой клавиши
            if (LAYOUT_MAP.find(input) != LAYOUT_MAP.end()) {
                char newDirection = LAYOUT_MAP[input]; // Преобразуем ввод в допустимое направление
                if (newDirection == EXIT_CHAR) return true; // Выход из игры
                if (!isOppositeDirection(newDirection)) {
                    direction = newDirection; // Устанавливаем новое направление
                }
            }

            // Очищаем буфер клавиатуры, чтобы исключить "зависания"
            while (_kbhit()) {
                _getch(); // Читаем и игнорируем оставшиеся символы
            }
        }
        return false;
    }

    void update() {
        bool grow = apple.isEaten(snake);
        snake.move(direction, grow);
        if (grow) apple.spawn(field.getField());
        if (snake.checkCollision()) {
            cout << "Game Over!" << endl;
            exit(0);
        }
        field.update(snake, apple);
    }

    void render() {
        screen.clear();
        const auto& f = field.getField();
        for (int y = 0; y < COUNT_ROW; ++y) {
            for (int x = 0; x < COUNT_COLUMN; ++x) {
                screen.draw(x, y, f[y][x]);
            }
        }
        screen.render();
    }

    void limitFPS(const time_point<high_resolution_clock>& frameStart) {
        auto frameEnd = high_resolution_clock::now();
        auto elapsed = duration_cast<milliseconds>(frameEnd - frameStart).count();
        if (elapsed < FRAME_DURATION) {
            sleep_for(milliseconds(FRAME_DURATION - elapsed));
        }
    }

    bool isOppositeDirection(char newDirection) const {
        return (direction == UP_DIRECTION && newDirection == DOWN_DIRECTION) ||
            (direction == DOWN_DIRECTION && newDirection == UP_DIRECTION) ||
            (direction == LEFT_DIRECTION && newDirection == RIGHT_DIRECTION) ||
            (direction == RIGHT_DIRECTION && newDirection == LEFT_DIRECTION);
    }
};

// Главная функция
int main() {
    Game game;
    game.run();
    return 0;
}



//#include <iostream>
//#include <vector>
//#include <deque>
//#include <Windows.h>
//#include <iterator>
//#include <random>
//#include <conio.h>
//#include <chrono>
//#include <thread>
//#include <map>
//
//using namespace std;
//using namespace this_thread;
//using namespace chrono;
//
//const char CHAR_APPLE = '$';
//const char CHAR_FILL_FIELD = ' ';
//const char CHAR_SNAKE = 'o';
//const char HEAD_SNAKE_CHAR = '@';
//const char WALL_CHAR = '#';
//const int COUNT_ROW = 25;
//const int COUNT_COLUMN = 80;
//const char UP_DIRECTION = 'w';
//const char LEFT_DIRECTION = 'a';
//const char DOWN_DIRECTION = 's';
//const char RIGHT_DIRECTION = 'd';
//const char EXIT_CHAR = 'q';
//const int TARGET_FPS = 10; // Частота кадров
//const int FRAME_DURACTION = 1000 / TARGET_FPS; // Длительность одного кадра в миллисекундах
//
//std::map<char, char> LAYOUT_MAP = {
//    {'w', 'w'}, {'a', 'a'}, {'s', 's'}, {'d', 'd'}, // Английские
//    {'ц', 'w'}, {'ф', 'a'}, {'ы', 's'}, {'в', 'd'}, // Русские
//    {'q', 'q'}, {'й', 'q'}                          // Выход
//};
//
//struct Apple 
//{
//    int x;
//    int y;
//};
//
//struct SnakeSegment 
//{
//    int x;
//    int y;
//};
//
//using Snake = deque<SnakeSegment>;
//using Field = vector<vector<char>>;
//
//HANDLE hConsole;
//CHAR_INFO screenBuffer[COUNT_ROW][COUNT_COLUMN];
//
//void InitScreenBuffer() 
//{
//    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
//    SMALL_RECT windowSize = { 0, 0, COUNT_COLUMN - 1, COUNT_ROW - 1 };
//    COORD bufferSize = { COUNT_COLUMN, COUNT_ROW };
//    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
//    SetConsoleScreenBufferSize(hConsole, bufferSize);
//}
//
//void DrawToScreenBuffer(int x, int y, char symbol) 
//{
//    screenBuffer[y][x].Char.AsciiChar = symbol;
//    screenBuffer[y][x].Attributes = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE;
//}
//
//void RenderScreen() 
//{
//    SMALL_RECT writeRegion = { 0, 0, COUNT_COLUMN - 1, COUNT_ROW - 1 };
//    COORD bufferSize = { COUNT_COLUMN, COUNT_ROW };
//    COORD bufferCoord = { 0, 0 };
//    WriteConsoleOutput(hConsole, (CHAR_INFO*)screenBuffer, bufferSize, bufferCoord, &writeRegion);
//}
//
//void PrintField(const vector<vector<char>>& field) 
//{
//    for (const auto& row : field) 
//    {
//        for (const auto& cell : row) 
//        {
//            cout << cell;
//        }
//        cout << endl;
//    }
//}
//
//Field FillField() 
//{
//    Field field(COUNT_ROW, vector<char>(COUNT_COLUMN, CHAR_FILL_FIELD));
//    for (int y = 0; y < COUNT_ROW; ++y) 
//    {
//        for (int x = 0; x < COUNT_COLUMN; ++x) 
//        {
//            if (y == 0 || y == COUNT_ROW - 1 || x == 0 || x == COUNT_COLUMN - 1) 
//            {
//                field[y][x] = WALL_CHAR;
//            }
//            else 
//            {
//                field[y][x] = CHAR_FILL_FIELD;
//            }
//        }
//    }
//    return field;
//}
//
//int GetRandomValue(int minValue, int maxValue) 
//{
//    static random_device dev;
//    static mt19937 rng(dev());
//    uniform_int_distribution<int> dist(minValue, maxValue);
//    return dist(rng);
//}
//
//Apple CreateApple(const Snake& snake, const vector<vector<char>>& field) 
//{
//    Apple apple;
//    do 
//    {
//        apple.y = GetRandomValue(1, COUNT_ROW - 2); // Исключаем стены
//        apple.x = GetRandomValue(1, COUNT_COLUMN - 2);
//    } while (field[apple.y][apple.x] != CHAR_FILL_FIELD);
//    return apple;
//}
//
//Field AddAppleToField(Field field, const Apple& apple) 
//{
//    field[apple.y][apple.x] = CHAR_APPLE;
//    return field;
//}
//
//Snake CreateSnake() 
//{
//    Snake snake;
//    snake.push_back({ COUNT_COLUMN / 2, COUNT_ROW / 2 });
//    snake.push_back({ COUNT_COLUMN / 2 + 1, COUNT_ROW / 2 });
//    return snake;
//}
//
//Field AddSnakeToField(Field field, const Snake& snake) 
//{
//    field[snake.front().y][snake.front().x] = HEAD_SNAKE_CHAR;
//    for (size_t i = 1; i < snake.size(); ++i) {
//        field[snake[i].y][snake[i].x] = CHAR_SNAKE;
//    }
//    return field;
//}
//
//void ClearSnakeOnField(vector<vector<char>>& field, const Snake& snake) 
//{
//    for (const auto& segment : snake) 
//    {
//        field[segment.y][segment.x] = CHAR_FILL_FIELD;
//    }
//}
//
//void GameOver() 
//{
//    cout << "\nGame Over!" << endl;
//    exit(0);
//}
//
//void Win() 
//{
//    cout << "\nYou Win!" << endl;
//    exit(0);
//}
//
//bool SnakeEatApple(const SnakeSegment& head, const Apple& apple) 
//{
//    return head.x == apple.x && head.y == apple.y;
//}
//
//Snake MoveSnake(const Snake& snake, char direction, bool grow) 
//{
//    Snake newSnake = snake;
//    SnakeSegment newHead = snake.front();
//    switch (direction) {
//    case UP_DIRECTION: --newHead.y; break;
//    case DOWN_DIRECTION: ++newHead.y; break;
//    case LEFT_DIRECTION: --newHead.x; break;
//    case RIGHT_DIRECTION: ++newHead.x; break;
//    }
//    newSnake.push_front(newHead);
//    if (!grow) 
//    {
//        newSnake.pop_back();
//    }
//    return newSnake;
//}
//
//bool IsCollision(const Snake& snake, const vector<vector<char>>& field) 
//{
//    const auto& head = snake.front();
//    if (field[head.y][head.x] == WALL_CHAR) 
//    {
//        return true; // Столкновение со стеной
//    }
//    for (size_t i = 1; i < snake.size(); ++i) 
//    {
//        if (snake[i].x == head.x && snake[i].y == head.y) 
//        {
//            return true; // Столкновение с собой
//        }
//    }
//    return false;
//}
//
//void UpdateScreenBuffer(const vector<vector<char>>& field, const Snake& snake, const Apple& apple) 
//{
//    for (int y = 0; y < COUNT_ROW; ++y) 
//    {
//        for (int x = 0; x < COUNT_COLUMN; ++x) {
//            DrawToScreenBuffer(x, y, field[y][x]);
//        }
//    }
//
//    for (size_t i = 1; i < snake.size(); ++i) 
//    {
//        DrawToScreenBuffer(snake[i].x, snake[i].y, CHAR_SNAKE);
//    }
//
//    DrawToScreenBuffer(snake.front().x, snake.front().y, HEAD_SNAKE_CHAR);
//    DrawToScreenBuffer(apple.x, apple.y, CHAR_APPLE);
//}
//
//char GetDirectionInput(char currentDirection) {
//    if (_kbhit()) {
//        char input = static_cast<char>(_getch());
//        if (LAYOUT_MAP.find(input) != LAYOUT_MAP.end()) {
//            char newDirection = LAYOUT_MAP[input];
//            // Проверка на обратное направление
//            if ((currentDirection == UP_DIRECTION && newDirection == DOWN_DIRECTION) ||
//                (currentDirection == DOWN_DIRECTION && newDirection == UP_DIRECTION) ||
//                (currentDirection == LEFT_DIRECTION && newDirection == RIGHT_DIRECTION) ||
//                (currentDirection == RIGHT_DIRECTION && newDirection == LEFT_DIRECTION)) {
//                return currentDirection; // Игнорируем обратное направление
//            }
//            return newDirection;
//        }
//    }
//    return currentDirection;
//}
//
//void RunGame() {
//    Field field = FillField();
//    Snake snake = CreateSnake();
//    Apple apple = CreateApple(snake, field);
//    char direction = LEFT_DIRECTION;
//    bool grow = false;
//
//    InitScreenBuffer();
//
//    while (true) {
//        // Время начала кадра
//        auto frameStart = high_resolution_clock::now();
//
//        // Обработка ввода
//        direction = GetDirectionInput(direction);
//        if (direction == EXIT_CHAR) break;
//
//        // Перемещение змейки
//        snake = MoveSnake(snake, direction, grow);
//        grow = SnakeEatApple(snake.front(), apple);
//
//        if (grow) {
//            apple = CreateApple(snake, field);
//        }
//
//        if (IsCollision(snake, field)) {
//            cout << "Game Over!" << endl;
//            break;
//        }
//
//        // Обновляем поле
//        field = FillField();
//        field = AddSnakeToField(field, snake);
//        field = AddAppleToField(field, apple);
//        UpdateScreenBuffer(field, snake, apple);
//
//        RenderScreen();
//
//        // Ограничение FPS
//        auto frameEnd = high_resolution_clock::now();
//        auto elapsedTime = duration_cast<milliseconds>(frameEnd - frameStart).count();
//        if (elapsedTime < FRAME_DURACTION) {
//            this_thread::sleep_for(milliseconds(FRAME_DURACTION - elapsedTime));
//        }
//    }
//}
//
//int main() 
//{
//    RunGame();
//    return 0;
//}