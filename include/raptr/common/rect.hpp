/*!
  \file rect.hpp
  This module provides overloads to SDL_Rect functions for double Rect and Point objects
 */

#pragma once

namespace raptr {

//! A double precision point
struct Point {
  double x, y;
};

//! A double precision rectangle
struct Rect {
  double x, y, w, h;
};

/*! If a Rect has no width or has no height, then it is considered empty

  \param r - The rectangle to check if empty
  \param EPS - The epsilon to use when checking if a value is 0
  \return Whether or not the rectangle has 0-width or 0-height
*/
inline bool SDL_RectEmpty(const Rect *r, double EPS = 1e-5)
{
  return ((!r) || (r->w <= EPS) || (r->h <= EPS)) ? true : false;
}

/*! Checks for equality between two double-precision rectangles

  \param a - The first rectangle to check
  \param b - The second rectangle to check
  \param EPS - The epsilon to use when checking if a value is 0
  \return Whether or not the rectangles are equal
*/
inline bool SDL_RectEquals(const Rect *a, const Rect *b, double EPS = 1e-5)
{
  return (a && b && (a->x - b->x) < EPS && (a->y - b->y) < EPS &&
    (a->w - b->w) < EPS && (a->h - b->h) < EPS) ? true : false;
}

/*! Horizontally and vertically intersect two rectangles and store the result

  \param A - The first rectangle in the intersection
  \param B - The second rectangle in the intersection
  \param result - The Rectangle to store the resulting intersection in
  \return Whether an intersection happened
*/
inline bool SDL_IntersectRect(const Rect* A, const Rect* B, Rect* result)
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

/*! Returns true if there was a horizontal or vertical intersection of two rectangles

  \param A - The first rectangle in the intersection
  \param B - The second rectangle in the intersection
  \return Whether an intersection happened
*/
inline bool SDL_HasIntersection(const Rect* A, const Rect* B)
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

} // namespace raptr
