#pragma once

#include <notcurses/notcurses.h>

#include <list>

class Game
{
public:
  struct Point
  {
    unsigned x = 0;
    unsigned y = 0;

    bool operator==(const Point& other) { return x == other.x && y == other.y; }
    bool operator!=(const Point& other) { return x != other.x || y != other.y; }
  };

  struct Size
  {
    unsigned width  = 0;
    unsigned height = 0;
  };

  struct Palette
  {
    uint32_t snake;
    uint32_t food;
    uint32_t empty;
  };

  enum Direction
  {
    up,
    down,
    right,
    left
  };

public:
  Game();
  ~Game();

  void run();

private:
  void addFood();
  void cleanup();
  void gameOver();
  void init();
  bool isKeyValid(const uint32_t c);
  void moveSnakeHead(const Point& pos);
  void pause();

private:
  struct notcurses* _nc = nullptr;
  struct ncplane* _stdp = nullptr;
  struct ncvisual* _ncv = nullptr;
  ncblitter_e _blitter  = NCBLIT_2x1;

  Size _termSize;
  Size _gameSize;

  enum Direction _snakedir   = right;
  struct Segment *snake_head = nullptr, *snake_tail = nullptr;
  std::list<Point> _snake;

  Palette palette {.snake = ncpixel(0, 255, 0), .food = ncpixel(255, 0, 0), .empty = 0};
};
