#include <iostream>
#include <vector>
#include <deque>
#include <iterator>
#include <random>
#include <conio.h>
#include "windows.h"

using namespace std;

const char CHAR_APPLE = '*';
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

struct Apple {
    int x;
    int y;
};

struct SnakeSegment {
    int x;
    int y;
};

using Snake = deque<SnakeSegment>;

void PrintField(const vector<vector<char>>& field) {
    for (const auto& row : field) {
        for (const auto& cell : row) {
            cout << cell;
        }
        cout << endl;
    }
}

void InitField(vector<vector<char>>& field) {
    for (int y = 0; y < COUNT_ROW; ++y) {
        for (int x = 0; x < COUNT_COLUMN; ++x) {
            if (y == 0 || y == COUNT_ROW - 1 || x == 0 || x == COUNT_COLUMN - 1) {
                field[y][x] = WALL_CHAR;
            }
            else {
                field[y][x] = CHAR_FILL_FIELD;
            }
        }
    }
}

int GetRandomValue(int minValue, int maxValue) {
    static random_device dev;
    static mt19937 rng(dev());
    uniform_int_distribution<int> dist(minValue, maxValue);
    return dist(rng);
}

void CreateApple(Apple& apple, const Snake& snake, const vector<vector<char>>& field) {
    do {
        apple.y = GetRandomValue(1, COUNT_ROW - 2); // Исключаем стены
        apple.x = GetRandomValue(1, COUNT_COLUMN - 2);
    } while (field[apple.y][apple.x] != CHAR_FILL_FIELD);
}

void AddAppleToField(vector<vector<char>>& field, const Apple& apple) {
    field[apple.y][apple.x] = CHAR_APPLE;
}

void CreateSnake(Snake& snake) {
    snake.push_back({ COUNT_COLUMN / 2, COUNT_ROW / 2 });
    snake.push_back({ COUNT_COLUMN / 2 + 1, COUNT_ROW / 2 });
}

void AddSnakeToField(vector<vector<char>>& field, const Snake& snake) {
    field[snake.front().y][snake.front().x] = HEAD_SNAKE_CHAR;
    for (size_t i = 1; i < snake.size(); ++i) {
        field[snake[i].y][snake[i].x] = CHAR_SNAKE;
    }
}

void ClearSnakeOnField(vector<vector<char>>& field, const Snake& snake) {
    for (const auto& segment : snake) {
        field[segment.y][segment.x] = CHAR_FILL_FIELD;
    }
}

void GameOver() {
    cout << "\nGame Over!" << endl;
    exit(0);
}

void Win() {
    cout << "\nYou Win!" << endl;
    exit(0);
}

bool SnakeEatApple(const SnakeSegment& head, const Apple& apple) {
    return head.x == apple.x && head.y == apple.y;
}

void MoveSnake(Snake& snake, char direction, bool grow) {
    SnakeSegment newHead = snake.front();

    switch (direction) {
    case UP_DIRECTION: --newHead.y; break;
    case DOWN_DIRECTION: ++newHead.y; break;
    case LEFT_DIRECTION: --newHead.x; break;
    case RIGHT_DIRECTION: ++newHead.x; break;
    }

    snake.push_front(newHead);
    if (!grow) {
        snake.pop_back();
    }
}

bool IsCollision(const Snake& snake, const vector<vector<char>>& field) {
    const auto& head = snake.front();
    if (field[head.y][head.x] == WALL_CHAR) {
        return true; // Столкновение со стеной
    }
    for (size_t i = 1; i < snake.size(); ++i) {
        if (snake[i].x == head.x && snake[i].y == head.y) {
            return true; // Столкновение с собой
        }
    }
    return false;
}

int main() {
    vector<vector<char>> field(COUNT_ROW, vector<char>(COUNT_COLUMN));
    InitField(field);

    Snake snake;
    CreateSnake(snake);

    Apple apple;
    CreateApple(apple, snake, field);

    char direction = LEFT_DIRECTION;
    bool grow = false;

    while (true) {
        if (_kbhit()) {
            char input = _getch();
            if ((input == UP_DIRECTION || input == DOWN_DIRECTION ||
                input == LEFT_DIRECTION || input == RIGHT_DIRECTION) &&
                !(direction == UP_DIRECTION && input == DOWN_DIRECTION) &&
                !(direction == DOWN_DIRECTION && input == UP_DIRECTION) &&
                !(direction == LEFT_DIRECTION && input == RIGHT_DIRECTION) &&
                !(direction == RIGHT_DIRECTION && input == LEFT_DIRECTION)) {
                direction = input;
            }
        }

        ClearSnakeOnField(field, snake);

        MoveSnake(snake, direction, grow);
        grow = SnakeEatApple(snake.front(), apple);

        if (grow) {
            CreateApple(apple, snake, field);
        }

        if (IsCollision(snake, field)) {
            GameOver();
        }

        InitField(field);
        AddAppleToField(field, apple);
        AddSnakeToField(field, snake);

        system("cls");
        PrintField(field);

        if (snake.size() == (COUNT_ROW - 2) * (COUNT_COLUMN - 2)) {
            Win();
        }

        Sleep(100);
    }

    return 0;
}
