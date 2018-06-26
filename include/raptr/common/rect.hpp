#pragma once

namespace raptr {

constexpr double EPS = 1e-5;

struct Point {
  double x, y;
};

struct Rect {
  double x, y, w, h;
};

inline bool SDL_RectEmpty(const Rect *r)
{
  return ((!r) || (r->w <= 0) || (r->h <= 0)) ? true : false;
}

inline bool SDL_RectEquals(const Rect *a, const Rect *b)
{
  return (a && b && (a->x - b->x) < EPS && (a->y - b->y) < EPS &&
    (a->w - b->w) < EPS && (a->h - b->h) < EPS) ? true : false;
}

inline bool SDL_IntersectRect(const Rect * A, const Rect * B, Rect * result)
{
  double Amin, Amax, Bmin, Bmax;

  /* Horizontal intersection */
  Amin = A->x;
  Amax = Amin + A->w;
  Bmin = B->x;
  Bmax = Bmin + B->w;
  if (Bmin > Amin)
    Amin = Bmin;
  result->x = Amin;
  if (Bmax < Amax)
    Amax = Bmax;
  result->w = Amax - Amin;

  /* Vertical intersection */
  Amin = A->y;
  Amax = Amin + A->h;
  Bmin = B->y;
  Bmax = Bmin + B->h;
  if (Bmin > Amin)
    Amin = Bmin;
  result->y = Amin;
  if (Bmax < Amax)
    Amax = Bmax;
  result->h = Amax - Amin;

  return !SDL_RectEmpty(result);
}

inline bool
SDL_HasIntersection(const Rect * A, const Rect * B)
{
  double Amin, Amax, Bmin, Bmax;

  /* Horizontal intersection */
  Amin = A->x;
  Amax = Amin + A->w;
  Bmin = B->x;
  Bmax = Bmin + B->w;
  if (Bmin > Amin)
    Amin = Bmin;
  if (Bmax < Amax)
    Amax = Bmax;
  if (Amax <= Amin)
    return false;

  /* Vertical intersection */
  Amin = A->y;
  Amax = Amin + A->h;
  Bmin = B->y;
  Bmax = Bmin + B->h;
  if (Bmin > Amin)
    Amin = Bmin;
  if (Bmax < Amax)
    Amax = Bmax;
  if (Amax <= Amin)
    return false;

  return true;
}

}
