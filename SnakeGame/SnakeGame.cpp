#include <iostream>
#include <vector>
#include <deque>
#include <Windows.h>
#include <iterator>
#include <random>
#include <conio.h>
#include <chrono>
#include <thread>
#include <map>

using namespace std;
using namespace this_thread;
using namespace chrono;

const char CHAR_APPLE = '$';
const char CHAR_FILL_FIELD = ' ';
const char CHAR_SNAKE = 'o';
const char HEAD_SNAKE_CHAR = '@';
const char WALL_CHAR = '#';
const int COUNT_ROW = 25;
const int COUNT_COLUMN = 80;
const char UP_DIRECTION = 'w';
const char LEFT_DIRECTION = 'a';
const char DOWN_DIRECTION = 's';
const char RIGHT_DIRECTION = 'd';
const char EXIT_CHAR = 'q';
const int TARGET_FPS = 10; // Частота кадров
const int FRAME_DURACTION = 1000 / TARGET_FPS; // Длительность одного кадра в миллисекундах

std::map<char, char> LAYOUT_MAP = {
    {'w', 'w'}, {'a', 'a'}, {'s', 's'}, {'d', 'd'}, // Английские
    {'ц', 'w'}, {'ф', 'a'}, {'ы', 's'}, {'в', 'd'}, // Русские
    {'q', 'q'}, {'й', 'q'}                          // Выход
};

struct Apple 
{
    int x;
    int y;
};

struct SnakeSegment 
{
    int x;
    int y;
};

using Snake = deque<SnakeSegment>;

HANDLE hConsole;
CHAR_INFO screenBuffer[COUNT_ROW][COUNT_COLUMN];

void InitScreenBuffer() 
{
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT windowSize = { 0, 0, COUNT_COLUMN - 1, COUNT_ROW - 1 };
    COORD bufferSize = { COUNT_COLUMN, COUNT_ROW };
    SetConsoleWindowInfo(hConsole, TRUE, &windowSize);
    SetConsoleScreenBufferSize(hConsole, bufferSize);
}

void DrawToScreenBuffer(int x, int y, char symbol) 
{
    screenBuffer[y][x].Char.AsciiChar = symbol;
    screenBuffer[y][x].Attributes = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE;
}

void RenderScreen() 
{
    SMALL_RECT writeRegion = { 0, 0, COUNT_COLUMN - 1, COUNT_ROW - 1 };
    COORD bufferSize = { COUNT_COLUMN, COUNT_ROW };
    COORD bufferCoord = { 0, 0 };
    WriteConsoleOutput(hConsole, (CHAR_INFO*)screenBuffer, bufferSize, bufferCoord, &writeRegion);
}

void PrintField(const vector<vector<char>>& field) 
{
    for (const auto& row : field) 
    {
        for (const auto& cell : row) 
        {
            cout << cell;
        }
        cout << endl;
    }
}

void FillField(vector<vector<char>>& field) 
{
    for (int y = 0; y < COUNT_ROW; ++y) 
    {
        for (int x = 0; x < COUNT_COLUMN; ++x) 
        {
            if (y == 0 || y == COUNT_ROW - 1 || x == 0 || x == COUNT_COLUMN - 1) 
            {
                field[y][x] = WALL_CHAR;
            }
            else 
            {
                field[y][x] = CHAR_FILL_FIELD;
            }
        }
    }
}

int GetRandomValue(int minValue, int maxValue) 
{
    static random_device dev;
    static mt19937 rng(dev());
    uniform_int_distribution<int> dist(minValue, maxValue);
    return dist(rng);
}

void CreateApple(Apple& apple, const Snake& snake, const vector<vector<char>>& field) 
{
    do 
    {
        apple.y = GetRandomValue(1, COUNT_ROW - 2); // Исключаем стены
        apple.x = GetRandomValue(1, COUNT_COLUMN - 2);
    } while (field[apple.y][apple.x] != CHAR_FILL_FIELD);
}

void AddAppleToField(vector<vector<char>>& field, const Apple& apple) 
{
    field[apple.y][apple.x] = CHAR_APPLE;
}

void CreateSnake(Snake& snake) 
{
    snake.push_back({ COUNT_COLUMN / 2, COUNT_ROW / 2 });
    snake.push_back({ COUNT_COLUMN / 2 + 1, COUNT_ROW / 2 });
}

void AddSnakeToField(vector<vector<char>>& field, const Snake& snake) 
{
    field[snake.front().y][snake.front().x] = HEAD_SNAKE_CHAR;
    for (size_t i = 1; i < snake.size(); ++i) {
        field[snake[i].y][snake[i].x] = CHAR_SNAKE;
    }
}

void ClearSnakeOnField(vector<vector<char>>& field, const Snake& snake) 
{
    for (const auto& segment : snake) 
    {
        field[segment.y][segment.x] = CHAR_FILL_FIELD;
    }
}

void GameOver() 
{
    cout << "\nGame Over!" << endl;
    exit(0);
}

void Win() 
{
    cout << "\nYou Win!" << endl;
    exit(0);
}

bool SnakeEatApple(const SnakeSegment& head, const Apple& apple) 
{
    return head.x == apple.x && head.y == apple.y;
}

void MoveSnake(Snake& snake, char direction, bool grow) 
{
    SnakeSegment newHead = snake.front();
    switch (direction) {
    case UP_DIRECTION: --newHead.y; break;
    case DOWN_DIRECTION: ++newHead.y; break;
    case LEFT_DIRECTION: --newHead.x; break;
    case RIGHT_DIRECTION: ++newHead.x; break;
    }
    snake.push_front(newHead);
    if (!grow) 
    {
        snake.pop_back();
    }
}

bool IsCollision(const Snake& snake, const vector<vector<char>>& field) 
{
    const auto& head = snake.front();
    if (field[head.y][head.x] == WALL_CHAR) 
    {
        return true; // Столкновение со стеной
    }
    for (size_t i = 1; i < snake.size(); ++i) 
    {
        if (snake[i].x == head.x && snake[i].y == head.y) 
        {
            return true; // Столкновение с собой
        }
    }
    return false;
}

void UpdateScreenBuffer(const vector<vector<char>>& field, const Snake& snake, const Apple& apple) 
{
    for (int y = 0; y < COUNT_ROW; ++y) 
    {
        for (int x = 0; x < COUNT_COLUMN; ++x) {
            DrawToScreenBuffer(x, y, field[y][x]);
        }
    }

    for (size_t i = 1; i < snake.size(); ++i) 
    {
        DrawToScreenBuffer(snake[i].x, snake[i].y, CHAR_SNAKE);
    }

    DrawToScreenBuffer(snake.front().x, snake.front().y, HEAD_SNAKE_CHAR);
    DrawToScreenBuffer(apple.x, apple.y, CHAR_APPLE);
}

char GetDirectionInput() {
    char input = static_cast<char>(_getch());
    if (LAYOUT_MAP.find(input) != LAYOUT_MAP.end()) {
        return LAYOUT_MAP[input]; // Преобразуем символ в соответствующий
    }
    return input; // Если символ не найден в карте, возвращаем как есть
}

int main() 
{
    vector<vector<char>> field(COUNT_ROW, vector<char>(COUNT_COLUMN));
    FillField(field);

    Snake snake;
    CreateSnake(snake);

    Apple apple;
    CreateApple(apple, snake, field);

    char direction = LEFT_DIRECTION;
    bool grow = false;

    InitScreenBuffer();

    while (true) 
    {
        // Начало кадра
        auto frameStart = high_resolution_clock::now();

        // Обрабатываем ввод
        if (_kbhit()) 
        {
            char input = GetDirectionInput();

            if (input == EXIT_CHAR) {
                break;
            }

            // Обрабатываем только допустимые направления
            if ((input == UP_DIRECTION || input == DOWN_DIRECTION || input == LEFT_DIRECTION || input == RIGHT_DIRECTION) &&
                !(direction == UP_DIRECTION && input == DOWN_DIRECTION) &&
                !(direction == DOWN_DIRECTION && input == UP_DIRECTION) &&
                !(direction == LEFT_DIRECTION && input == RIGHT_DIRECTION) &&
                !(direction == RIGHT_DIRECTION && input == LEFT_DIRECTION)) 
            {
                direction = input;

                // Очищаем оставшийся буфер
                while (_kbhit()) 
                {
                    _getch(); // Читаем и игнорируем символы
                }
            }
        }

        // Обновление логики игры
        MoveSnake(snake, direction, grow);
        grow = SnakeEatApple(snake.front(), apple);
        if (grow) 
        {
            CreateApple(apple, snake, field);
        }
        if (IsCollision(snake, field)) 
        {
            GameOver();
        }

        // Обновляем буфер экрана
        UpdateScreenBuffer(field, snake, apple);

        // Отображение экрана
        RenderScreen();

        if (snake.size() == (COUNT_ROW - 2) * (COUNT_COLUMN - 2)) 
        {
            Win();
        }

        // Завершение кадра
        auto frameEnd = high_resolution_clock::now();
        auto elapsedTime = duration_cast<milliseconds>(frameEnd - frameStart).count();
        if (elapsedTime < FRAME_DURACTION) 
        {
            this_thread::sleep_for(milliseconds(FRAME_DURACTION - elapsedTime));
        }
    }

    return 0;
}

//int main() {
//    vector<vector<char>> field(countRow, vector<char>(COUNT_COLUMN));
//    FillField(field);
//
//    Snake snake;
//    CreateSnake(snake);
//    Apple apple;
//    CreateApple(apple, snake, field);
//
//    char direction = LEFT_DIRECTION;
//    bool grow = false;
//
//    while (true) {
//
//        // Обрабатываем ввод
//        if (_kbhit()) {
//            char input = _getch();
//
//            // Обрабатываем только допустимые направления
//            if ((input == UP_DIRECTION || input == DOWN_DIRECTION || input == LEFT_DIRECTION || input == RIGHT_DIRECTION) &&
//                !(direction == UP_DIRECTION && input == DOWN_DIRECTION) &&
//                !(direction == DOWN_DIRECTION && input == UP_DIRECTION) &&
//                !(direction == LEFT_DIRECTION && input == RIGHT_DIRECTION) &&
//                !(direction == RIGHT_DIRECTION && input == LEFT_DIRECTION)) {
//                direction = input;
//
//                // Очищаем оставшийся буфер
//                while (_kbhit()) {
//                    _getch(); // Читаем и игнорируем символы
//                }
//            }
//        }
//
//        ClearSnakeOnField(field, snake);
//        MoveSnake(snake, direction, grow);
//
//        grow = SnakeEatApple(snake.front(), apple);
//        if (grow) {
//            CreateApple(apple, snake, field);
//        }
//
//        if (IsCollision(snake, field)) {
//            GameOver();
//        }
//
//        FillField(field);
//        AddAppleToField(field, apple);
//        AddSnakeToField(field, snake);
//
//        system("cls");
//        PrintField(field);
//
//        if (snake.size() == (countRow - 2) * (COUNT_COLUMN - 2)) {
//            Win();
//        }
//
//        sleep_for(milliseconds(100));
//    }
//
//    return 0;
//}