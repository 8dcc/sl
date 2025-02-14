/*
 * Copyright 2025 8dcc
 *
 * This file is part of SL.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LISP_TYPES_H_
#define LISP_TYPES_H_ 1

/*
 * C types used for representing integer and floating-point numbers in Lisp.
 */
typedef long long LispInt;
typedef double LispFlt;

/*
 * Generic numeric type, used when performing operations between numeric
 * expressions that don't (necessarily) share a type.
 *
 * If this gets updated, you should also change the 'expr_*_generic_num'
 * functions below.
 */
typedef LispFlt GenericNum;

#endif /* LISP_TYPES_H_ */
