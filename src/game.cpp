#include "game.h"

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#include <fmt/format.h>

using namespace std::chrono_literals;

#define TEXT_CHANNELS NCCHANNELS_INITIALIZER(255, 255, 255, 20, 20, 20)

void Game::addFood()
{
  int x = 0;
  int y = 0;

  uint32_t pix = 0;

  do
  {
    x = rand() % _gameSize.width;
    y = rand() % _gameSize.height;
    ncvisual_at_yx(_ncv, y, x, &pix);
  } while (pix != palette.empty);
  ncvisual_set_yx(_ncv, y, x, palette.food);
}

Game::~Game()
{
  ncvisual_destroy(_ncv);
  notcurses_stop(_nc);
}

void Game::gameOver()
{
  std::string score = fmt::format("Score: {}", _snake.size() - 1);

  static const int h = 3;
  struct ncplane_options ncp_opt
  {
    .y = ((int)_termSize.height - h) / 2, .x = 0, .rows = h, .cols = _termSize.width
  };
  struct ncplane* n = ncplane_create(_stdp, &ncp_opt);
  ncplane_set_base(n, " ", 0, TEXT_CHANNELS);
  ncplane_set_channels(n, TEXT_CHANNELS);
  ncplane_puttext(n, 0, NCALIGN_CENTER, "Game Over!", nullptr);
  ncplane_home(n);
  ncplane_puttext(n, 1, NCALIGN_CENTER, score.c_str(), nullptr);
  ncplane_home(n);
  ncplane_puttext(n, 2, NCALIGN_CENTER, "(Press any key to exit)", nullptr);

  notcurses_render(_nc);

  notcurses_get_blocking(_nc, nullptr);
  ncplane_destroy(n);
}

Game::Game()
{
  srand(time(nullptr));

  struct notcurses_options nc_opt
  {
    .flags = NCOPTION_SUPPRESS_BANNERS
  };
  _nc   = notcurses_core_init(&nc_opt, stdout);
  _stdp = notcurses_stddim_yx(_nc, &_termSize.height, &_termSize.width);

  _blitter         = NCBLIT_2x1;
  _gameSize.width  = _termSize.width;
  _gameSize.height = _termSize.height * 2;

  uint32_t* buf = (uint32_t*)malloc(_gameSize.height * _gameSize.width);
  memset(buf, 0, _gameSize.height * _gameSize.width);
  _ncv = ncvisual_from_rgba(
      buf, _gameSize.height, _gameSize.width * sizeof(uint32_t), _gameSize.width);
  free(buf);

  _snake = {{_gameSize.width / 2, _gameSize.height / 2}};

  addFood();

  ncvisual_set_yx(_ncv, _snake.front().y, _snake.front().x, palette.snake);
}

bool Game::isKeyValid(const uint32_t c)
{
  switch (c)
  {
    case NCKEY_LEFT:
    case NCKEY_RIGHT:
      return _snakedir == Direction::down || _snakedir == Direction::up;
    case NCKEY_UP:
    case NCKEY_DOWN:
      return _snakedir == Direction::left || _snakedir == Direction::right;
    case 'p':
    case 'q':
      return true;
    default:
      return false;
  }
}

void Game::run()
{
  struct ncinput ni = {.id = 0};

  uint32_t c = 0;
  std::chrono::steady_clock::time_point begin;
  do
  {
    if (c && !isKeyValid(c))
      continue;

    begin = std::chrono::steady_clock::now();

    // Render
    if (!c || isKeyValid(c))
    {
      ncplane_erase(_stdp);
      struct ncvisual_options ncv
      {
        .n = _stdp, .scaling = NCSCALE_NONE, .blitter = _blitter
      };
      ncvisual_blit(_nc, _ncv, &ncv);
      ncplane_set_channels(_stdp, TEXT_CHANNELS);
      ncplane_printf_yx(_stdp, 0, 0, " Score: %zu ", _snake.size());
      notcurses_render(_nc);
    }

    // Handle keys
    if (c)
      switch (c)
      {
        case NCKEY_LEFT:
          if (_snakedir == Direction::down || _snakedir == Direction::up)
            _snakedir = Direction::left;
          break;
        case NCKEY_RIGHT:
          if (_snakedir == Direction::down || _snakedir == Direction::up)
            _snakedir = Direction::right;
          break;
        case NCKEY_UP:
          if (_snakedir == Direction::left || _snakedir == Direction::right)
            _snakedir = Direction::up;
          break;
        case NCKEY_DOWN:
          if (_snakedir == Direction::left || _snakedir == Direction::right)
            _snakedir = Direction::down;
          break;
        case 'p':
          pause();
          continue;
      }

    // Next Head position
    Point head = _snake.front();
    switch (_snakedir)
    {
      case Direction::up:
        head.y = head.y > 0 ? head.y - 1 : _gameSize.height - 1;
        break;
      case Direction::down:
        head.y = head.y < _gameSize.height - 1 ? head.y + 1 : 0;
        break;
      case Direction::left:
        head.x = head.x > 0 ? head.x - 1 : _gameSize.width - 1;
        break;
      case Direction::right:
        head.x = head.x < _gameSize.width - 1 ? head.x + 1 : 0;
        break;
    }

    // Check new Head position
    uint32_t pix = 0;
    Point tail   = _snake.back();
    ncvisual_at_yx(_ncv, head.y, head.x, &pix);

    // Snake => Dead
    if (pix == palette.snake && head != tail)
    {
      gameOver();
      return;
    }

    // Food => Grow
    if (pix == palette.food)
    {
      _snake.push_back(tail);
      addFood();
    }

    // Move
    moveSnakeHead(head);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    // Wait
    if (!c)
      std::this_thread::sleep_for(
          60ms - std::chrono::duration_cast<std::chrono::milliseconds>(end - begin));
  } while ((c = notcurses_get_nblock(_nc, &ni)) != 'q');
}

void Game::moveSnakeHead(const Point& pos)
{
  _snake.push_front(pos);
  ncvisual_set_yx(_ncv, pos.y, pos.x, palette.snake);
  ncvisual_set_yx(_ncv, _snake.back().y, _snake.back().x, palette.empty);
  _snake.pop_back();
}

void Game::pause()
{
  static const int h = 2;
  struct ncplane_options ncp
  {
    .y = ((int)_termSize.height - h) / 2, .x = 0, .rows = h, .cols = _termSize.width
  };
  struct ncplane* n = ncplane_create(_stdp, &ncp);
  ncplane_set_base(n, " ", 0, TEXT_CHANNELS);
  ncplane_set_channels(n, TEXT_CHANNELS);
  ncplane_puttext(n, 0, NCALIGN_CENTER, "Game Paused", nullptr);
  ncplane_home(n);
  ncplane_puttext(n, 1, NCALIGN_CENTER, "(Press any key to resume)", nullptr);
  notcurses_render(_nc);
  notcurses_get_blocking(_nc, nullptr);
  ncplane_destroy(n);
}
