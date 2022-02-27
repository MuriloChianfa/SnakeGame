/*
 * Snake game in matrix 8x8
 *
 * SnakeGame.c
 *
 * Author:     Murilo Chianfa
 * Build date: 2021-06-27 20:00
 */

#include <SoftwareSerial.h>
#include <IRremote.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Rows ports
#define ROW_1 13
#define ROW_2 12
#define ROW_3 11
#define ROW_4 10
#define ROW_5 9
#define ROW_6 8
#define ROW_7 7
#define ROW_8 6

// Cols ports
#define COL_1 5
#define COL_2 4
#define COL_3 3
#define COL_4 2
#define COL_5 A4
#define COL_6 A3
#define COL_7 A2
#define COL_8 A1

// Matrix constants
#define MATRIX_WIDTH 8
#define MATRIX_HEIGHT 8

// Snake constants
#define INITIAL_SNAKE_LEN 2
#define MAX_SNAKE_LEN 12

// Controll port
#define CONTROLL_PORT A5

// Controll buttons
#define BUTTON_VOLUME_PLUS 0xFD807F 
#define BUTTON_LEFT_ARROW 0xFD20DF
#define BUTTON_RIGHT_ARROW 0xFD609F
#define BUTTON_VOLUME_MINUS 0xFD906F

IRrecv controll(CONTROLL_PORT);
decode_results command;

int row[8] = {
    ROW_1, ROW_2, ROW_3, ROW_4,
    ROW_5, ROW_6, ROW_7, ROW_8
};

int col[8] = {
    COL_1, COL_2, COL_3, COL_4,
    COL_5, COL_6, COL_7, COL_8
};

struct Position_t {
    int x;
    int y;
} __attribute__((__packed__));

enum Direction {
    left,
    right,
    up,
    down
};

struct Snake_t {
    struct Position_t snakeBody[MAX_SNAKE_LEN];
    int length;
    int head;
    int tail;
    int points;
    int direction;
} __attribute__((__packed__));

struct Fruit_t {
    struct Position_t position;
};

/*
 * Function: randomNumber
 * ----------------
 *   return a random number between received args
 *
 *   min: minimun number of possible random int
 *   max: maximun number of possible random int
 *
 *   returns: int
 */
int randomNumber(int min = 1, int max = 8)
{
    return ((rand() % max) + min);
}

/*
 * Function: randomPosition
 * ----------------
 *   Generate a random position struct
 *
 *   returns: struct Position_t
 */
struct Position_t randomPosition()
{
    return { randomNumber(), randomNumber() };
}

/*
 * Function: isSnake
 * ----------------
 *   Verify if received position is snake
 *
 *   snake: current snake on board
 *   x: row in matrix
 *   y: col in matrix
 *
 *   returns: int
 */
int isSnake(struct Snake_t *snake, int *x, int *y)
{
    for (unsigned int i = snake->tail; i <= snake->head; i++)
    {
        if (
            snake->snakeBody[i].x == *x &&
            snake->snakeBody[i].y == *y
        )
        {
            return 1;
        }
    }

    if (snake->head < snake->tail)
    {
        for (unsigned int i = 0; i <= snake->head; i++)
        {
            if (
                snake->snakeBody[i].x == *x &&
                snake->snakeBody[i].y == *y
            )
            {
                return 1;
            }
        }
    }

    return 0;
}

/*
 * Function: randomFruit
 * ----------------
 *   generate a new fruit in a position that is not snake
 *
 *   snake: current snake on board
 *
 *   returns: struct Fruit_t
 */
struct Fruit_t randomFruit(struct Snake_t *snake)
{
    int x = 0;
    int y = 0;

    for (;;)
    {
        x = randomNumber(0, 7);
        y = randomNumber(0, 7);

        if (!isSnake(snake, &x, &y))
        {
            break;
        }                          
    }

    return { x, y };
}

/*
 * Function: cleanMatrix
 * ----------------
 *   Disable all leds of matrix
 *
 *   returns: void
 */
void cleanMatrix()
{
    for (unsigned int register c = 0; c < MATRIX_WIDTH; c++)
    {
        digitalWrite(col[c], LOW);

        for (unsigned int register r = 0; r < MATRIX_HEIGHT; r++)
        {
            digitalWrite(row[r], HIGH);
        }
    }
}

/*
 * Function: pixelController
 * ----------------
 *   enable the led by traverse matrix
 *
 *   x: row in matrix
 *   y: col in matrix
 *
 *   returns: void
 */
void pixelController(int x, int y)
{
    cleanMatrix();

    for (unsigned int register c = 0; c < MATRIX_WIDTH; c++)
    {
        for (unsigned int register r = 0; r < MATRIX_HEIGHT; r++)
        {
            digitalWrite(row[r], (r == x) ? LOW : HIGH);
        }

        digitalWrite(col[c], (c == y) ? HIGH : LOW);
    }
}

/*
 * Function: createSnake
 * ----------------
 *   create a initial snake on fixed position and direction
 *
 *   returns: struct Snake_t
 */
struct Snake_t createSnake()
{
    return { {{ 2, 2 }, { 2, 3 }}, INITIAL_SNAKE_LEN, INITIAL_SNAKE_LEN - 1, 0, 0, Direction::right };
}

/*
 * Function: drawBoard
 * ----------------
 *   draw snake and fruit in board
 *
 *   snake: snake instance to get your position
 *   fruit: fruit instance to get your position
 *
 *   returns: void
 */
void drawBoard(struct Snake_t *snake, struct Fruit_t *fruit)
{
    for (unsigned int i = snake->tail; i <= snake->head; i++)
    {
        pixelController(snake->snakeBody[i].x, snake->snakeBody[i].y);
    }

    if (snake->head < snake->tail)
    {
        for (unsigned int i = 0; i <= snake->head; i++)
        {
            pixelController(snake->snakeBody[i].x, snake->snakeBody[i].y);
        }
    }

    pixelController(fruit->position.x, fruit->position.y);
}

/*
 * Function: calculatePhysics
 * ----------------
 *   calculate movement as at time pass
 *
 *   snake: snake instance to calculate your next waypoint
 *
 *   returns: void
 */
void calculatePhysics(struct Snake_t *snake)
{
    int iteratorX = 0;
    int iteratorY = 0;

    switch (snake->direction)
    {
        case Direction::right: iteratorY =  1;  break;
        case Direction::left:  iteratorY = -1;  break;
        case Direction::up:    iteratorX = -1;  break;
        case Direction::down:  iteratorX =  1;  break;
    }

    int previousPosition = snake->head;

    snake->tail = ((snake->tail + 1) == MAX_SNAKE_LEN) ? 0 : snake->tail + 1;
    snake->head = ((snake->head + 1) == MAX_SNAKE_LEN) ? 0 : snake->head + 1;

    snake->snakeBody[snake->head].x = snake->snakeBody[previousPosition].x + iteratorX;
    snake->snakeBody[snake->head].y = snake->snakeBody[previousPosition].y + iteratorY;

    if (snake->snakeBody[snake->head].x >= MATRIX_WIDTH)
    {
        snake->snakeBody[snake->head].x = 0;
    }
    else if (snake->snakeBody[snake->head].x < 0)
    {
        snake->snakeBody[snake->head].x = MATRIX_WIDTH -1;
    }

    if (snake->snakeBody[snake->head].y >= MATRIX_HEIGHT)
    {
        snake->snakeBody[snake->head].y = 0;
    }
    else if (snake->snakeBody[snake->head].y < 0)
    {
        snake->snakeBody[snake->head].y = MATRIX_HEIGHT -1;
    }
}

/*
 * Function: checkIfIsFruit
 * ----------------
 *   check if snake head is in fruit and regenerates ifits
 *
 *   snake: snake instance to get your position
 *   fruit: fruit instance to get your position
 *
 *   returns: void
 */
void checkIfIsFruit(struct Snake_t *snake, struct Fruit_t *fruit)
{
    if (
        snake->snakeBody[snake->head].x == fruit->position.x &&
        snake->snakeBody[snake->head].y == fruit->position.y    
    )
    {
        if (snake->length < MAX_SNAKE_LEN)
        {
            snake->length++;
            snake->tail--;
            if (snake->tail < 0)
            {
                snake->tail = MAX_SNAKE_LEN - 1;
            }
        }

        snake->points++;
        *fruit = randomFruit(snake);
    }
}

/*
 * Function: checkGameover
 * ----------------
 *   check if snake len is higher them permited
 *
 *   snake: snake instance to get your len
 *
 *   returns: int|bool
 */
int checkGameover(struct Snake_t *snake)
{
    if (snake->length >= MAX_SNAKE_LEN)
    {
        return 1;
    }

    return 0;
}

/*
 * Function: dispatchButtonAction
 * ----------------
 *   set new direction on controll click
 *
 *   snake: snake instance to set a new direction
 *
 *   returns: void
 */
void dispatchButtonAction(struct Snake_t *snake)
{
    if (!controll.decode(&command))
    {
        return;
    }

    Serial.println(command.value, HEX);

    switch (command.value)
    {
        case BUTTON_VOLUME_PLUS:  snake->direction = Direction::up;    break;
        case BUTTON_VOLUME_MINUS: snake->direction = Direction::down;  break;
        case BUTTON_LEFT_ARROW:   snake->direction = Direction::left;  break;
        case BUTTON_RIGHT_ARROW:  snake->direction = Direction::right; break;
    }

    controll.resume();
}

int gameover = 1;
unsigned long previousTime = 0;
unsigned long elapsedTime = 0;
struct Snake_t snake = createSnake();
struct Fruit_t fruit = randomFruit(&snake);

/*
 * Function: setup
 * ----------------
 *   init the basics ports mode
 *
 *   returns: void
 */
void setup()
{
    Serial.begin(9600);

    srand(time(NULL));

    for (unsigned int register i = 2; i < 14; i++)
    {
        pinMode(i, OUTPUT);
    }

    controll.enableIRIn();
}

/*
 * Function: loop
 * ----------------
 *   main loop to calculate phisycs and controll elapsed time 
 *
 *   returns: void
 */
void loop()
{
    if (gameover)
    {
        snake = createSnake();
        fruit = randomFruit(&snake);

        gameover = 0;
        return;
    }

    unsigned long currentTime = millis();

    drawBoard(&snake, &fruit);

    elapsedTime += currentTime - previousTime;

    if (elapsedTime > 100)
    {
        calculatePhysics(&snake);
        checkIfIsFruit(&snake, &fruit);
        gameover = checkGameover(&snake);

        elapsedTime = 0;
    }

    dispatchButtonAction(&snake);
    
    previousTime = currentTime;
}

