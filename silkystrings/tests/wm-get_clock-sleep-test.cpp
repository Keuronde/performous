/* This file is part of SilkyStrings 
 * Copyright (C) 2006  Olli Salli, Tuomas Per�l�, Ville Virkkala
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <WM.h>

#include <cassert>

int main()
{
  using namespace SilkyStrings;
  double t0;

  WM wm (640, 480, false);

  assert ((t0 = wm.get_clock ()) >= 0.0);

  wm.sleep (1);

  assert (wm.get_clock () >= 1.0);

  wm.sleep (1);

  assert (wm.get_clock () >= 2.0);

  return 0;
}
