// Copyright (c) 2018-2019 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "hevc2_parser.h"

using namespace BsReader2;

namespace BS_HEVC2
{
const Bs8u ScanOrder1[4][2 * 2][2] =
{
    {
        { 0, 0},{ 0, 1},
        { 1, 0},{ 1, 1},
    },
    {
        { 0, 0},{ 1, 0},
        { 0, 1},{ 1, 1},
    },
    {
        { 0, 0},{ 0, 1},
        { 1, 0},{ 1, 1},
    },
    {
        { 0, 0},{ 1, 0},
        { 1, 1},{ 0, 1},
    },
};
const Bs8u ScanPos1[4][2][2] =
{
    {
        {   0,   1,},
        {   2,   3,},
    },
    {
        {   0,   2,},
        {   1,   3,},
    },
    {
        {   0,   1,},
        {   2,   3,},
    },
    {
        {   0,   3,},
        {   1,   2,},
    },
};
const Bs8u ScanOrder2[4][4 * 4][2] =
{
    {
        { 0, 0},{ 0, 1},{ 1, 0},{ 0, 2},
        { 1, 1},{ 2, 0},{ 0, 3},{ 1, 2},
        { 2, 1},{ 3, 0},{ 1, 3},{ 2, 2},
        { 3, 1},{ 2, 3},{ 3, 2},{ 3, 3},
    },
    {
        { 0, 0},{ 1, 0},{ 2, 0},{ 3, 0},
        { 0, 1},{ 1, 1},{ 2, 1},{ 3, 1},
        { 0, 2},{ 1, 2},{ 2, 2},{ 3, 2},
        { 0, 3},{ 1, 3},{ 2, 3},{ 3, 3},
    },
    {
        { 0, 0},{ 0, 1},{ 0, 2},{ 0, 3},
        { 1, 0},{ 1, 1},{ 1, 2},{ 1, 3},
        { 2, 0},{ 2, 1},{ 2, 2},{ 2, 3},
        { 3, 0},{ 3, 1},{ 3, 2},{ 3, 3},
    },
    {
        { 0, 0},{ 1, 0},{ 2, 0},{ 3, 0},
        { 3, 1},{ 2, 1},{ 1, 1},{ 0, 1},
        { 0, 2},{ 1, 2},{ 2, 2},{ 3, 2},
        { 3, 3},{ 2, 3},{ 1, 3},{ 0, 3},
    },
};
const Bs8u ScanPos2[4][4][4] =
{
    {
        {   0,   1,   3,   6,},
        {   2,   4,   7,  10,},
        {   5,   8,  11,  13,},
        {   9,  12,  14,  15,},
    },
    {
        {   0,   4,   8,  12,},
        {   1,   5,   9,  13,},
        {   2,   6,  10,  14,},
        {   3,   7,  11,  15,},
    },
    {
        {   0,   1,   2,   3,},
        {   4,   5,   6,   7,},
        {   8,   9,  10,  11,},
        {  12,  13,  14,  15,},
    },
    {
        {   0,   7,   8,  15,},
        {   1,   6,   9,  14,},
        {   2,   5,  10,  13,},
        {   3,   4,  11,  12,},
    },
};
const Bs8u ScanOrder3[4][8 * 8][2] =
{
    {
        { 0, 0},{ 0, 1},{ 1, 0},{ 0, 2},{ 1, 1},{ 2, 0},{ 0, 3},{ 1, 2},
        { 2, 1},{ 3, 0},{ 0, 4},{ 1, 3},{ 2, 2},{ 3, 1},{ 4, 0},{ 0, 5},
        { 1, 4},{ 2, 3},{ 3, 2},{ 4, 1},{ 5, 0},{ 0, 6},{ 1, 5},{ 2, 4},
        { 3, 3},{ 4, 2},{ 5, 1},{ 6, 0},{ 0, 7},{ 1, 6},{ 2, 5},{ 3, 4},
        { 4, 3},{ 5, 2},{ 6, 1},{ 7, 0},{ 1, 7},{ 2, 6},{ 3, 5},{ 4, 4},
        { 5, 3},{ 6, 2},{ 7, 1},{ 2, 7},{ 3, 6},{ 4, 5},{ 5, 4},{ 6, 3},
        { 7, 2},{ 3, 7},{ 4, 6},{ 5, 5},{ 6, 4},{ 7, 3},{ 4, 7},{ 5, 6},
        { 6, 5},{ 7, 4},{ 5, 7},{ 6, 6},{ 7, 5},{ 6, 7},{ 7, 6},{ 7, 7},
    },
    {
        { 0, 0},{ 1, 0},{ 2, 0},{ 3, 0},{ 4, 0},{ 5, 0},{ 6, 0},{ 7, 0},
        { 0, 1},{ 1, 1},{ 2, 1},{ 3, 1},{ 4, 1},{ 5, 1},{ 6, 1},{ 7, 1},
        { 0, 2},{ 1, 2},{ 2, 2},{ 3, 2},{ 4, 2},{ 5, 2},{ 6, 2},{ 7, 2},
        { 0, 3},{ 1, 3},{ 2, 3},{ 3, 3},{ 4, 3},{ 5, 3},{ 6, 3},{ 7, 3},
        { 0, 4},{ 1, 4},{ 2, 4},{ 3, 4},{ 4, 4},{ 5, 4},{ 6, 4},{ 7, 4},
        { 0, 5},{ 1, 5},{ 2, 5},{ 3, 5},{ 4, 5},{ 5, 5},{ 6, 5},{ 7, 5},
        { 0, 6},{ 1, 6},{ 2, 6},{ 3, 6},{ 4, 6},{ 5, 6},{ 6, 6},{ 7, 6},
        { 0, 7},{ 1, 7},{ 2, 7},{ 3, 7},{ 4, 7},{ 5, 7},{ 6, 7},{ 7, 7},
    },
    {
        { 0, 0},{ 0, 1},{ 0, 2},{ 0, 3},{ 0, 4},{ 0, 5},{ 0, 6},{ 0, 7},
        { 1, 0},{ 1, 1},{ 1, 2},{ 1, 3},{ 1, 4},{ 1, 5},{ 1, 6},{ 1, 7},
        { 2, 0},{ 2, 1},{ 2, 2},{ 2, 3},{ 2, 4},{ 2, 5},{ 2, 6},{ 2, 7},
        { 3, 0},{ 3, 1},{ 3, 2},{ 3, 3},{ 3, 4},{ 3, 5},{ 3, 6},{ 3, 7},
        { 4, 0},{ 4, 1},{ 4, 2},{ 4, 3},{ 4, 4},{ 4, 5},{ 4, 6},{ 4, 7},
        { 5, 0},{ 5, 1},{ 5, 2},{ 5, 3},{ 5, 4},{ 5, 5},{ 5, 6},{ 5, 7},
        { 6, 0},{ 6, 1},{ 6, 2},{ 6, 3},{ 6, 4},{ 6, 5},{ 6, 6},{ 6, 7},
        { 7, 0},{ 7, 1},{ 7, 2},{ 7, 3},{ 7, 4},{ 7, 5},{ 7, 6},{ 7, 7},
    },
    {
        { 0, 0},{ 1, 0},{ 2, 0},{ 3, 0},{ 4, 0},{ 5, 0},{ 6, 0},{ 7, 0},
        { 7, 1},{ 6, 1},{ 5, 1},{ 4, 1},{ 3, 1},{ 2, 1},{ 1, 1},{ 0, 1},
        { 0, 2},{ 1, 2},{ 2, 2},{ 3, 2},{ 4, 2},{ 5, 2},{ 6, 2},{ 7, 2},
        { 7, 3},{ 6, 3},{ 5, 3},{ 4, 3},{ 3, 3},{ 2, 3},{ 1, 3},{ 0, 3},
        { 0, 4},{ 1, 4},{ 2, 4},{ 3, 4},{ 4, 4},{ 5, 4},{ 6, 4},{ 7, 4},
        { 7, 5},{ 6, 5},{ 5, 5},{ 4, 5},{ 3, 5},{ 2, 5},{ 1, 5},{ 0, 5},
        { 0, 6},{ 1, 6},{ 2, 6},{ 3, 6},{ 4, 6},{ 5, 6},{ 6, 6},{ 7, 6},
        { 7, 7},{ 6, 7},{ 5, 7},{ 4, 7},{ 3, 7},{ 2, 7},{ 1, 7},{ 0, 7},
    },
};
const Bs8u ScanPos3[4][8][8] =
{
    {
        {   0,   1,   3,   6,  10,  15,  21,  28,},
        {   2,   4,   7,  11,  16,  22,  29,  36,},
        {   5,   8,  12,  17,  23,  30,  37,  43,},
        {   9,  13,  18,  24,  31,  38,  44,  49,},
        {  14,  19,  25,  32,  39,  45,  50,  54,},
        {  20,  26,  33,  40,  46,  51,  55,  58,},
        {  27,  34,  41,  47,  52,  56,  59,  61,},
        {  35,  42,  48,  53,  57,  60,  62,  63,},
    },
    {
        {   0,   8,  16,  24,  32,  40,  48,  56,},
        {   1,   9,  17,  25,  33,  41,  49,  57,},
        {   2,  10,  18,  26,  34,  42,  50,  58,},
        {   3,  11,  19,  27,  35,  43,  51,  59,},
        {   4,  12,  20,  28,  36,  44,  52,  60,},
        {   5,  13,  21,  29,  37,  45,  53,  61,},
        {   6,  14,  22,  30,  38,  46,  54,  62,},
        {   7,  15,  23,  31,  39,  47,  55,  63,},
    },
    {
        {   0,   1,   2,   3,   4,   5,   6,   7,},
        {   8,   9,  10,  11,  12,  13,  14,  15,},
        {  16,  17,  18,  19,  20,  21,  22,  23,},
        {  24,  25,  26,  27,  28,  29,  30,  31,},
        {  32,  33,  34,  35,  36,  37,  38,  39,},
        {  40,  41,  42,  43,  44,  45,  46,  47,},
        {  48,  49,  50,  51,  52,  53,  54,  55,},
        {  56,  57,  58,  59,  60,  61,  62,  63,},
    },
    {
        {   0,  15,  16,  31,  32,  47,  48,  63,},
        {   1,  14,  17,  30,  33,  46,  49,  62,},
        {   2,  13,  18,  29,  34,  45,  50,  61,},
        {   3,  12,  19,  28,  35,  44,  51,  60,},
        {   4,  11,  20,  27,  36,  43,  52,  59,},
        {   5,  10,  21,  26,  37,  42,  53,  58,},
        {   6,   9,  22,  25,  38,  41,  54,  57,},
        {   7,   8,  23,  24,  39,  40,  55,  56,},
    },
};
const Bs8u ScanTraverse4[16 * 16][2] =
{
    { 0, 0},{ 1, 0},{ 2, 0},{ 3, 0},{ 4, 0},{ 5, 0},{ 6, 0},{ 7, 0},{ 8, 0},{ 9, 0},{10, 0},{11, 0},{12, 0},{13, 0},{14, 0},{15, 0},
    {15, 1},{14, 1},{13, 1},{12, 1},{11, 1},{10, 1},{ 9, 1},{ 8, 1},{ 7, 1},{ 6, 1},{ 5, 1},{ 4, 1},{ 3, 1},{ 2, 1},{ 1, 1},{ 0, 1},
    { 0, 2},{ 1, 2},{ 2, 2},{ 3, 2},{ 4, 2},{ 5, 2},{ 6, 2},{ 7, 2},{ 8, 2},{ 9, 2},{10, 2},{11, 2},{12, 2},{13, 2},{14, 2},{15, 2},
    {15, 3},{14, 3},{13, 3},{12, 3},{11, 3},{10, 3},{ 9, 3},{ 8, 3},{ 7, 3},{ 6, 3},{ 5, 3},{ 4, 3},{ 3, 3},{ 2, 3},{ 1, 3},{ 0, 3},
    { 0, 4},{ 1, 4},{ 2, 4},{ 3, 4},{ 4, 4},{ 5, 4},{ 6, 4},{ 7, 4},{ 8, 4},{ 9, 4},{10, 4},{11, 4},{12, 4},{13, 4},{14, 4},{15, 4},
    {15, 5},{14, 5},{13, 5},{12, 5},{11, 5},{10, 5},{ 9, 5},{ 8, 5},{ 7, 5},{ 6, 5},{ 5, 5},{ 4, 5},{ 3, 5},{ 2, 5},{ 1, 5},{ 0, 5},
    { 0, 6},{ 1, 6},{ 2, 6},{ 3, 6},{ 4, 6},{ 5, 6},{ 6, 6},{ 7, 6},{ 8, 6},{ 9, 6},{10, 6},{11, 6},{12, 6},{13, 6},{14, 6},{15, 6},
    {15, 7},{14, 7},{13, 7},{12, 7},{11, 7},{10, 7},{ 9, 7},{ 8, 7},{ 7, 7},{ 6, 7},{ 5, 7},{ 4, 7},{ 3, 7},{ 2, 7},{ 1, 7},{ 0, 7},
    { 0, 8},{ 1, 8},{ 2, 8},{ 3, 8},{ 4, 8},{ 5, 8},{ 6, 8},{ 7, 8},{ 8, 8},{ 9, 8},{10, 8},{11, 8},{12, 8},{13, 8},{14, 8},{15, 8},
    {15, 9},{14, 9},{13, 9},{12, 9},{11, 9},{10, 9},{ 9, 9},{ 8, 9},{ 7, 9},{ 6, 9},{ 5, 9},{ 4, 9},{ 3, 9},{ 2, 9},{ 1, 9},{ 0, 9},
    { 0,10},{ 1,10},{ 2,10},{ 3,10},{ 4,10},{ 5,10},{ 6,10},{ 7,10},{ 8,10},{ 9,10},{10,10},{11,10},{12,10},{13,10},{14,10},{15,10},
    {15,11},{14,11},{13,11},{12,11},{11,11},{10,11},{ 9,11},{ 8,11},{ 7,11},{ 6,11},{ 5,11},{ 4,11},{ 3,11},{ 2,11},{ 1,11},{ 0,11},
    { 0,12},{ 1,12},{ 2,12},{ 3,12},{ 4,12},{ 5,12},{ 6,12},{ 7,12},{ 8,12},{ 9,12},{10,12},{11,12},{12,12},{13,12},{14,12},{15,12},
    {15,13},{14,13},{13,13},{12,13},{11,13},{10,13},{ 9,13},{ 8,13},{ 7,13},{ 6,13},{ 5,13},{ 4,13},{ 3,13},{ 2,13},{ 1,13},{ 0,13},
    { 0,14},{ 1,14},{ 2,14},{ 3,14},{ 4,14},{ 5,14},{ 6,14},{ 7,14},{ 8,14},{ 9,14},{10,14},{11,14},{12,14},{13,14},{14,14},{15,14},
    {15,15},{14,15},{13,15},{12,15},{11,15},{10,15},{ 9,15},{ 8,15},{ 7,15},{ 6,15},{ 5,15},{ 4,15},{ 3,15},{ 2,15},{ 1,15},{ 0,15},
};
const Bs8u ScanTraverse5[32 * 32][2] =
{
    { 0, 0},{ 1, 0},{ 2, 0},{ 3, 0},{ 4, 0},{ 5, 0},{ 6, 0},{ 7, 0},{ 8, 0},{ 9, 0},{10, 0},{11, 0},{12, 0},{13, 0},{14, 0},{15, 0},{16, 0},{17, 0},{18, 0},{19, 0},{20, 0},{21, 0},{22, 0},{23, 0},{24, 0},{25, 0},{26, 0},{27, 0},{28, 0},{29, 0},{30, 0},{31, 0},
    {31, 1},{30, 1},{29, 1},{28, 1},{27, 1},{26, 1},{25, 1},{24, 1},{23, 1},{22, 1},{21, 1},{20, 1},{19, 1},{18, 1},{17, 1},{16, 1},{15, 1},{14, 1},{13, 1},{12, 1},{11, 1},{10, 1},{ 9, 1},{ 8, 1},{ 7, 1},{ 6, 1},{ 5, 1},{ 4, 1},{ 3, 1},{ 2, 1},{ 1, 1},{ 0, 1},
    { 0, 2},{ 1, 2},{ 2, 2},{ 3, 2},{ 4, 2},{ 5, 2},{ 6, 2},{ 7, 2},{ 8, 2},{ 9, 2},{10, 2},{11, 2},{12, 2},{13, 2},{14, 2},{15, 2},{16, 2},{17, 2},{18, 2},{19, 2},{20, 2},{21, 2},{22, 2},{23, 2},{24, 2},{25, 2},{26, 2},{27, 2},{28, 2},{29, 2},{30, 2},{31, 2},
    {31, 3},{30, 3},{29, 3},{28, 3},{27, 3},{26, 3},{25, 3},{24, 3},{23, 3},{22, 3},{21, 3},{20, 3},{19, 3},{18, 3},{17, 3},{16, 3},{15, 3},{14, 3},{13, 3},{12, 3},{11, 3},{10, 3},{ 9, 3},{ 8, 3},{ 7, 3},{ 6, 3},{ 5, 3},{ 4, 3},{ 3, 3},{ 2, 3},{ 1, 3},{ 0, 3},
    { 0, 4},{ 1, 4},{ 2, 4},{ 3, 4},{ 4, 4},{ 5, 4},{ 6, 4},{ 7, 4},{ 8, 4},{ 9, 4},{10, 4},{11, 4},{12, 4},{13, 4},{14, 4},{15, 4},{16, 4},{17, 4},{18, 4},{19, 4},{20, 4},{21, 4},{22, 4},{23, 4},{24, 4},{25, 4},{26, 4},{27, 4},{28, 4},{29, 4},{30, 4},{31, 4},
    {31, 5},{30, 5},{29, 5},{28, 5},{27, 5},{26, 5},{25, 5},{24, 5},{23, 5},{22, 5},{21, 5},{20, 5},{19, 5},{18, 5},{17, 5},{16, 5},{15, 5},{14, 5},{13, 5},{12, 5},{11, 5},{10, 5},{ 9, 5},{ 8, 5},{ 7, 5},{ 6, 5},{ 5, 5},{ 4, 5},{ 3, 5},{ 2, 5},{ 1, 5},{ 0, 5},
    { 0, 6},{ 1, 6},{ 2, 6},{ 3, 6},{ 4, 6},{ 5, 6},{ 6, 6},{ 7, 6},{ 8, 6},{ 9, 6},{10, 6},{11, 6},{12, 6},{13, 6},{14, 6},{15, 6},{16, 6},{17, 6},{18, 6},{19, 6},{20, 6},{21, 6},{22, 6},{23, 6},{24, 6},{25, 6},{26, 6},{27, 6},{28, 6},{29, 6},{30, 6},{31, 6},
    {31, 7},{30, 7},{29, 7},{28, 7},{27, 7},{26, 7},{25, 7},{24, 7},{23, 7},{22, 7},{21, 7},{20, 7},{19, 7},{18, 7},{17, 7},{16, 7},{15, 7},{14, 7},{13, 7},{12, 7},{11, 7},{10, 7},{ 9, 7},{ 8, 7},{ 7, 7},{ 6, 7},{ 5, 7},{ 4, 7},{ 3, 7},{ 2, 7},{ 1, 7},{ 0, 7},
    { 0, 8},{ 1, 8},{ 2, 8},{ 3, 8},{ 4, 8},{ 5, 8},{ 6, 8},{ 7, 8},{ 8, 8},{ 9, 8},{10, 8},{11, 8},{12, 8},{13, 8},{14, 8},{15, 8},{16, 8},{17, 8},{18, 8},{19, 8},{20, 8},{21, 8},{22, 8},{23, 8},{24, 8},{25, 8},{26, 8},{27, 8},{28, 8},{29, 8},{30, 8},{31, 8},
    {31, 9},{30, 9},{29, 9},{28, 9},{27, 9},{26, 9},{25, 9},{24, 9},{23, 9},{22, 9},{21, 9},{20, 9},{19, 9},{18, 9},{17, 9},{16, 9},{15, 9},{14, 9},{13, 9},{12, 9},{11, 9},{10, 9},{ 9, 9},{ 8, 9},{ 7, 9},{ 6, 9},{ 5, 9},{ 4, 9},{ 3, 9},{ 2, 9},{ 1, 9},{ 0, 9},
    { 0,10},{ 1,10},{ 2,10},{ 3,10},{ 4,10},{ 5,10},{ 6,10},{ 7,10},{ 8,10},{ 9,10},{10,10},{11,10},{12,10},{13,10},{14,10},{15,10},{16,10},{17,10},{18,10},{19,10},{20,10},{21,10},{22,10},{23,10},{24,10},{25,10},{26,10},{27,10},{28,10},{29,10},{30,10},{31,10},
    {31,11},{30,11},{29,11},{28,11},{27,11},{26,11},{25,11},{24,11},{23,11},{22,11},{21,11},{20,11},{19,11},{18,11},{17,11},{16,11},{15,11},{14,11},{13,11},{12,11},{11,11},{10,11},{ 9,11},{ 8,11},{ 7,11},{ 6,11},{ 5,11},{ 4,11},{ 3,11},{ 2,11},{ 1,11},{ 0,11},
    { 0,12},{ 1,12},{ 2,12},{ 3,12},{ 4,12},{ 5,12},{ 6,12},{ 7,12},{ 8,12},{ 9,12},{10,12},{11,12},{12,12},{13,12},{14,12},{15,12},{16,12},{17,12},{18,12},{19,12},{20,12},{21,12},{22,12},{23,12},{24,12},{25,12},{26,12},{27,12},{28,12},{29,12},{30,12},{31,12},
    {31,13},{30,13},{29,13},{28,13},{27,13},{26,13},{25,13},{24,13},{23,13},{22,13},{21,13},{20,13},{19,13},{18,13},{17,13},{16,13},{15,13},{14,13},{13,13},{12,13},{11,13},{10,13},{ 9,13},{ 8,13},{ 7,13},{ 6,13},{ 5,13},{ 4,13},{ 3,13},{ 2,13},{ 1,13},{ 0,13},
    { 0,14},{ 1,14},{ 2,14},{ 3,14},{ 4,14},{ 5,14},{ 6,14},{ 7,14},{ 8,14},{ 9,14},{10,14},{11,14},{12,14},{13,14},{14,14},{15,14},{16,14},{17,14},{18,14},{19,14},{20,14},{21,14},{22,14},{23,14},{24,14},{25,14},{26,14},{27,14},{28,14},{29,14},{30,14},{31,14},
    {31,15},{30,15},{29,15},{28,15},{27,15},{26,15},{25,15},{24,15},{23,15},{22,15},{21,15},{20,15},{19,15},{18,15},{17,15},{16,15},{15,15},{14,15},{13,15},{12,15},{11,15},{10,15},{ 9,15},{ 8,15},{ 7,15},{ 6,15},{ 5,15},{ 4,15},{ 3,15},{ 2,15},{ 1,15},{ 0,15},
    { 0,16},{ 1,16},{ 2,16},{ 3,16},{ 4,16},{ 5,16},{ 6,16},{ 7,16},{ 8,16},{ 9,16},{10,16},{11,16},{12,16},{13,16},{14,16},{15,16},{16,16},{17,16},{18,16},{19,16},{20,16},{21,16},{22,16},{23,16},{24,16},{25,16},{26,16},{27,16},{28,16},{29,16},{30,16},{31,16},
    {31,17},{30,17},{29,17},{28,17},{27,17},{26,17},{25,17},{24,17},{23,17},{22,17},{21,17},{20,17},{19,17},{18,17},{17,17},{16,17},{15,17},{14,17},{13,17},{12,17},{11,17},{10,17},{ 9,17},{ 8,17},{ 7,17},{ 6,17},{ 5,17},{ 4,17},{ 3,17},{ 2,17},{ 1,17},{ 0,17},
    { 0,18},{ 1,18},{ 2,18},{ 3,18},{ 4,18},{ 5,18},{ 6,18},{ 7,18},{ 8,18},{ 9,18},{10,18},{11,18},{12,18},{13,18},{14,18},{15,18},{16,18},{17,18},{18,18},{19,18},{20,18},{21,18},{22,18},{23,18},{24,18},{25,18},{26,18},{27,18},{28,18},{29,18},{30,18},{31,18},
    {31,19},{30,19},{29,19},{28,19},{27,19},{26,19},{25,19},{24,19},{23,19},{22,19},{21,19},{20,19},{19,19},{18,19},{17,19},{16,19},{15,19},{14,19},{13,19},{12,19},{11,19},{10,19},{ 9,19},{ 8,19},{ 7,19},{ 6,19},{ 5,19},{ 4,19},{ 3,19},{ 2,19},{ 1,19},{ 0,19},
    { 0,20},{ 1,20},{ 2,20},{ 3,20},{ 4,20},{ 5,20},{ 6,20},{ 7,20},{ 8,20},{ 9,20},{10,20},{11,20},{12,20},{13,20},{14,20},{15,20},{16,20},{17,20},{18,20},{19,20},{20,20},{21,20},{22,20},{23,20},{24,20},{25,20},{26,20},{27,20},{28,20},{29,20},{30,20},{31,20},
    {31,21},{30,21},{29,21},{28,21},{27,21},{26,21},{25,21},{24,21},{23,21},{22,21},{21,21},{20,21},{19,21},{18,21},{17,21},{16,21},{15,21},{14,21},{13,21},{12,21},{11,21},{10,21},{ 9,21},{ 8,21},{ 7,21},{ 6,21},{ 5,21},{ 4,21},{ 3,21},{ 2,21},{ 1,21},{ 0,21},
    { 0,22},{ 1,22},{ 2,22},{ 3,22},{ 4,22},{ 5,22},{ 6,22},{ 7,22},{ 8,22},{ 9,22},{10,22},{11,22},{12,22},{13,22},{14,22},{15,22},{16,22},{17,22},{18,22},{19,22},{20,22},{21,22},{22,22},{23,22},{24,22},{25,22},{26,22},{27,22},{28,22},{29,22},{30,22},{31,22},
    {31,23},{30,23},{29,23},{28,23},{27,23},{26,23},{25,23},{24,23},{23,23},{22,23},{21,23},{20,23},{19,23},{18,23},{17,23},{16,23},{15,23},{14,23},{13,23},{12,23},{11,23},{10,23},{ 9,23},{ 8,23},{ 7,23},{ 6,23},{ 5,23},{ 4,23},{ 3,23},{ 2,23},{ 1,23},{ 0,23},
    { 0,24},{ 1,24},{ 2,24},{ 3,24},{ 4,24},{ 5,24},{ 6,24},{ 7,24},{ 8,24},{ 9,24},{10,24},{11,24},{12,24},{13,24},{14,24},{15,24},{16,24},{17,24},{18,24},{19,24},{20,24},{21,24},{22,24},{23,24},{24,24},{25,24},{26,24},{27,24},{28,24},{29,24},{30,24},{31,24},
    {31,25},{30,25},{29,25},{28,25},{27,25},{26,25},{25,25},{24,25},{23,25},{22,25},{21,25},{20,25},{19,25},{18,25},{17,25},{16,25},{15,25},{14,25},{13,25},{12,25},{11,25},{10,25},{ 9,25},{ 8,25},{ 7,25},{ 6,25},{ 5,25},{ 4,25},{ 3,25},{ 2,25},{ 1,25},{ 0,25},
    { 0,26},{ 1,26},{ 2,26},{ 3,26},{ 4,26},{ 5,26},{ 6,26},{ 7,26},{ 8,26},{ 9,26},{10,26},{11,26},{12,26},{13,26},{14,26},{15,26},{16,26},{17,26},{18,26},{19,26},{20,26},{21,26},{22,26},{23,26},{24,26},{25,26},{26,26},{27,26},{28,26},{29,26},{30,26},{31,26},
    {31,27},{30,27},{29,27},{28,27},{27,27},{26,27},{25,27},{24,27},{23,27},{22,27},{21,27},{20,27},{19,27},{18,27},{17,27},{16,27},{15,27},{14,27},{13,27},{12,27},{11,27},{10,27},{ 9,27},{ 8,27},{ 7,27},{ 6,27},{ 5,27},{ 4,27},{ 3,27},{ 2,27},{ 1,27},{ 0,27},
    { 0,28},{ 1,28},{ 2,28},{ 3,28},{ 4,28},{ 5,28},{ 6,28},{ 7,28},{ 8,28},{ 9,28},{10,28},{11,28},{12,28},{13,28},{14,28},{15,28},{16,28},{17,28},{18,28},{19,28},{20,28},{21,28},{22,28},{23,28},{24,28},{25,28},{26,28},{27,28},{28,28},{29,28},{30,28},{31,28},
    {31,29},{30,29},{29,29},{28,29},{27,29},{26,29},{25,29},{24,29},{23,29},{22,29},{21,29},{20,29},{19,29},{18,29},{17,29},{16,29},{15,29},{14,29},{13,29},{12,29},{11,29},{10,29},{ 9,29},{ 8,29},{ 7,29},{ 6,29},{ 5,29},{ 4,29},{ 3,29},{ 2,29},{ 1,29},{ 0,29},
    { 0,30},{ 1,30},{ 2,30},{ 3,30},{ 4,30},{ 5,30},{ 6,30},{ 7,30},{ 8,30},{ 9,30},{10,30},{11,30},{12,30},{13,30},{14,30},{15,30},{16,30},{17,30},{18,30},{19,30},{20,30},{21,30},{22,30},{23,30},{24,30},{25,30},{26,30},{27,30},{28,30},{29,30},{30,30},{31,30},
    {31,31},{30,31},{29,31},{28,31},{27,31},{26,31},{25,31},{24,31},{23,31},{22,31},{21,31},{20,31},{19,31},{18,31},{17,31},{16,31},{15,31},{14,31},{13,31},{12,31},{11,31},{10,31},{ 9,31},{ 8,31},{ 7,31},{ 6,31},{ 5,31},{ 4,31},{ 3,31},{ 2,31},{ 1,31},{ 0,31},
};


inline bool isSLNonRefPic(NALU& nalu){
    switch(nalu.nal_unit_type){
    case TRAIL_N:
    case TSA_N:
    case STSA_N:
    case RADL_N:
    case RASL_N:
    case RSV_VCL_N10:
    case RSV_VCL_N12:
    case RSV_VCL_N14:
        return true;
    default:
        return false;
    }
}

void Info::decodeSSH(NALU& nalu, bool bNewSequence)
{
    Bs32s i = 0, j = 0, x = 0, y = 0, p = 0, m = 0, tileIdx = 0;
    auto& slice = *nalu.slice;
    auto& sps   = *slice.sps;
    auto& pps   = *slice.pps;

    const Bs8u _SubWidthC[5]  = {1,2,2,1,1};
    const Bs8u _SubHeightC[5] = {1,2,1,1,1};

    auto prevPicWidthInCtbsY  = PicWidthInCtbsY;
    auto prevPicHeightInCtbsY = PicHeightInCtbsY;
    auto prevMinTbLog2SizeY   = MinTbLog2SizeY;
    auto prevCtbLog2SizeY     = CtbLog2SizeY;

    NoRaslOutputFlag    = bNewSequence || isIDR(nalu) || isBLA(nalu);
    NoBackwardPredFlag  = true;
    ColPicSlices        = 0;
    NumColSlices        = 0;
    MaxPicOrderCntLsb   = (1 << (sps.log2_max_pic_order_cnt_lsb_minus4+4));

    SubWidthC            = _SubWidthC[sps.chroma_format_idc + sps.separate_colour_plane_flag];
    SubHeightC           = _SubHeightC[sps.chroma_format_idc + sps.separate_colour_plane_flag];
    MinCbLog2SizeY       = sps.log2_min_luma_coding_block_size_minus3 + 3;
    CtbLog2SizeY         = MinCbLog2SizeY + sps.log2_diff_max_min_luma_coding_block_size;
    CtbSizeY             = (1 << CtbLog2SizeY);
    PicWidthInCtbsY      = (sps.pic_width_in_luma_samples + CtbSizeY - 1) / CtbSizeY;
    PicHeightInCtbsY     = (sps.pic_height_in_luma_samples + CtbSizeY - 1) / CtbSizeY;
    MinCbSizeY           = (1 << MinCbLog2SizeY);
    PicWidthInMinCbsY    = (sps.pic_width_in_luma_samples + MinCbSizeY - 1) / MinCbSizeY;
    PicHeightInMinCbsY   = (sps.pic_height_in_luma_samples + MinCbSizeY - 1) / MinCbSizeY;
    PicSizeInMinCbsY     = PicWidthInMinCbsY * PicHeightInMinCbsY;
    PicSizeInCtbsY       = PicWidthInCtbsY * PicHeightInCtbsY;
    PicSizeInSamplesY    = sps.pic_width_in_luma_samples * sps.pic_height_in_luma_samples;
    PicWidthInSamplesC   = sps.pic_width_in_luma_samples / SubWidthC;
    PicHeightInSamplesC  = sps.pic_height_in_luma_samples / SubHeightC;
    MinTbLog2SizeY       = sps.log2_min_luma_transform_block_size_minus2 + 2;
    MaxTbLog2SizeY       = MinTbLog2SizeY + sps.log2_diff_max_min_luma_transform_block_size;
    Log2MinIpcmCbSizeY   = sps.log2_min_pcm_luma_coding_block_size_minus3 + 3;
    Log2MaxIpcmCbSizeY   = Log2MinIpcmCbSizeY + sps.log2_diff_max_min_pcm_luma_coding_block_size;
    Log2MinCuQpDeltaSize = CtbLog2SizeY - pps.diff_cu_qp_delta_depth;
    Log2ParMrgLevel      = pps.log2_parallel_merge_level_minus2 + 2;
    SliceQpY             = 26 + pps.init_qp_minus26 + slice.qp_delta;
    BitDepthY            = sps.bit_depth_luma_minus8 + 8;
    BitDepthC            = sps.bit_depth_chroma_minus8 + 8;
    QpBdOffsetY          = 6 * sps.bit_depth_luma_minus8;
    QpBdOffsetC          = 6 * sps.bit_depth_chroma_minus8;
    ChromaArrayType      = sps.separate_colour_plane_flag ? 0 : sps.chroma_format_idc;
    Log2MinCuChromaQpOffsetSize = CtbLog2SizeY - pps.diff_cu_chroma_qp_offset_depth;
    CtbWidthC  = CtbSizeY / SubWidthC;
    CtbHeightC = CtbSizeY / SubHeightC;
    RawCtuBits = (Bs32u)CtbSizeY * CtbSizeY * BitDepthY + 2ul * CtbWidthC * CtbHeightC * BitDepthC;
    PaletteMaxPredictorSize = sps.palette_max_size + sps.delta_palette_max_predictor_size;
    TwoVersionsOfCurrDecPicFlag =
        pps.curr_pic_ref_enabled_flag
        && (sps.sample_adaptive_offset_enabled_flag
            || !pps.deblocking_filter_disabled_flag
            || pps.deblocking_filter_override_enabled_flag);

    if (bNewSequence)
    {
        Bs32u sz = (sps.palette_max_size + PaletteMaxPredictorSize);
        PalettePredictorEntryReuseFlags.resize(sz);
        CurrentPaletteEntries[0].resize(sz);
        CurrentPaletteEntries[1].resize(sz);
        CurrentPaletteEntries[2].resize(sz);
    }

    if (m_cSlice && slice.dependent_slice_segment_flag)
    {
        Slice tmp = *m_cSlice;
        tmp.first_slice_segment_in_pic_flag = slice.first_slice_segment_in_pic_flag;
        tmp.no_output_of_prior_pics_flag    = slice.no_output_of_prior_pics_flag;
        tmp.dependent_slice_segment_flag    = 1;
        tmp.slice_segment_address           = slice.slice_segment_address;
        tmp.num_entry_point_offsets         = slice.num_entry_point_offsets;
        tmp.offset_len_minus1               = slice.offset_len_minus1;
        tmp.entry_point_offset_minus1       = slice.entry_point_offset_minus1;

        slice = tmp;
    }

    if (!m_bRPLDecoded)
        decodePOC(nalu);

    NewPicture = (bNewSequence || !m_cSlice || (m_cSlice->POC != slice.POC)
        || (m_cSlice->POC == slice.POC && CtuInRs[slice.slice_segment_address]));

    if (NewPicture)
    {
        if (!m_bRPLDecoded)
            m_DPB.swap(m_DPBafter);

        SliceAddrRsInTs.resize(PicSizeInCtbsY);
        std::fill(SliceAddrRsInTs.begin(), SliceAddrRsInTs.end(), -1);

        CtuInRs.resize(PicSizeInCtbsY);
        std::fill(CtuInRs.begin(), CtuInRs.end(), nullptr);
    }

    for (auto& sao : m_sao)
        sao.resize(PicSizeInCtbsY);

    if (!m_bRPLDecoded)
    {
        m_prevSlicePOC = slice.POC;
        decodeRPL(nalu);
    }

    m_bRPLDecoded = false;

    i = 0;
    while (NoBackwardPredFlag && i < (Bs32s)slice.num_ref_idx_l0_active)
        NoBackwardPredFlag = (slice.L0[i++].POC <= slice.POC);
    i = 0;
    while (NoBackwardPredFlag && i < (Bs32s)slice.num_ref_idx_l1_active)
        NoBackwardPredFlag = (slice.L1[i++].POC <= slice.POC);

    bool updateAddr = !m_cSlice || bNewSequence;

    if (!updateAddr)
    {
        auto& prevPPS = *m_cSlice->pps;

        updateAddr =
               (prevPicWidthInCtbsY != PicWidthInCtbsY)
            || (prevPicHeightInCtbsY != PicHeightInCtbsY)
            || (prevPPS.tiles_enabled_flag != pps.tiles_enabled_flag)
            || (prevPPS.num_tile_columns_minus1 != pps.num_tile_columns_minus1)
            || (prevPPS.num_tile_rows_minus1 != pps.num_tile_rows_minus1)
            || (prevPPS.uniform_spacing_flag != pps.uniform_spacing_flag);

        if (!updateAddr && pps.tiles_enabled_flag && !pps.uniform_spacing_flag)
        {
            updateAddr |= !!memcmp(pps.column_width_minus1, prevPPS.column_width_minus1, sizeof(pps.column_width_minus1[0]) * pps.num_tile_columns_minus1);
            updateAddr |= !!memcmp(pps.row_height_minus1, prevPPS.row_height_minus1, sizeof(pps.column_width_minus1[0]) * pps.num_tile_rows_minus1);
        }
    }

    if (updateAddr)
    {
        colWidth.resize(pps.num_tile_columns_minus1+1);
        if (pps.uniform_spacing_flag)
        {
            for (i = 0; i <= pps.num_tile_columns_minus1; i++)
            {
                colWidth[i] = ((i + 1) * PicWidthInCtbsY) / (pps.num_tile_columns_minus1 + 1) -
                               (i * PicWidthInCtbsY) / (pps.num_tile_columns_minus1 + 1);
            }
        }
        else
        {
            colWidth[pps.num_tile_columns_minus1] = PicWidthInCtbsY;

            for (i = 0; i < pps.num_tile_columns_minus1; i++)
            {
                colWidth[i] = pps.column_width_minus1[i] + 1;
                colWidth[pps.num_tile_columns_minus1] -= colWidth[i];
            }
        }

        rowHeight.resize(pps.num_tile_rows_minus1+1);
        if (pps.uniform_spacing_flag)
        {
            for (j = 0; j <= pps.num_tile_rows_minus1; j++)
            {
                rowHeight[j] = ((j + 1) * PicHeightInCtbsY) / (pps.num_tile_rows_minus1 + 1) -
                                (j * PicHeightInCtbsY) / (pps.num_tile_rows_minus1 + 1);
            }
        }
        else
        {
            rowHeight[pps.num_tile_rows_minus1] = PicHeightInCtbsY;
            for (j = 0; j < pps.num_tile_rows_minus1; j++)
            {
                rowHeight[j] = pps.row_height_minus1[j] + 1;
                rowHeight[pps.num_tile_rows_minus1] -= rowHeight[j];
            }
        }

        colBd.resize(pps.num_tile_columns_minus1+2);
        for (colBd[0] = 0, i = 0; i <= pps.num_tile_columns_minus1; i++)
            colBd[i + 1] = colBd[i] + colWidth[i];

        rowBd.resize(pps.num_tile_rows_minus1+2);
        for (rowBd[0] = 0, j = 0; j <= pps.num_tile_rows_minus1; j++)
            rowBd[j + 1] = rowBd[j] + rowHeight[j];

        CtbAddrRsToTs.resize(PicSizeInCtbsY);
        for (Bs16u ctbAddrRs = 0; ctbAddrRs < PicSizeInCtbsY; ctbAddrRs++)
        {
            Bs16u tbX = ctbAddrRs % PicWidthInCtbsY;
            Bs16u tbY = ctbAddrRs / PicWidthInCtbsY;
            Bs16u tileX = 0;
            Bs16u tileY = 0;

            for (i = 0; i <= pps.num_tile_columns_minus1; i++)
                if (tbX >= colBd[ i ])
                    tileX = i;

            for (j = 0; j  <=  pps.num_tile_rows_minus1; j++)
                if (tbY  >=  rowBd[ j ])
                    tileY = j;

            CtbAddrRsToTs[ctbAddrRs] = 0;

            for (i = 0; i < tileX; i++)
                CtbAddrRsToTs[ctbAddrRs] += rowHeight[tileY] * colWidth[i];

            for (j = 0; j < tileY; j++)
                CtbAddrRsToTs[ctbAddrRs] += PicWidthInCtbsY * rowHeight[j];

            CtbAddrRsToTs[ctbAddrRs] += (tbY - rowBd[tileY]) * colWidth[tileX] + tbX - colBd[tileX];
        }

        CtbAddrTsToRs.resize(PicSizeInCtbsY);
        for (Bs16u ctbAddrRs = 0; ctbAddrRs < PicSizeInCtbsY; ctbAddrRs++)
            CtbAddrTsToRs[CtbAddrRsToTs[ctbAddrRs]] = ctbAddrRs;


        TileId.resize(PicSizeInCtbsY);
        for (j = 0, tileIdx = 0; j <= pps.num_tile_rows_minus1; j++ )
            for (i = 0; i <= pps.num_tile_columns_minus1; i++, tileIdx++)
                for (y = rowBd[ j ]; y < rowBd[ j + 1 ]; y++)
                    for (x = colBd[ i ]; x < colBd[ i + 1 ]; x++)
                        TileId[CtbAddrRsToTs[y * PicWidthInCtbsY + x]] = tileIdx;
    }

    if (   updateAddr
        || prevMinTbLog2SizeY != MinTbLog2SizeY
        || prevCtbLog2SizeY   != CtbLog2SizeY)
    {
        m_MinTbAddrZsPitch = (PicWidthInCtbsY << (CtbLog2SizeY - MinTbLog2SizeY));
        j = (PicHeightInCtbsY << ( CtbLog2SizeY - MinTbLog2SizeY));
        m_MinTbAddrZs.resize(m_MinTbAddrZsPitch * j);

        for (y = 0; y < j; y++)
        {
            for (x = 0; x < (Bs32s)m_MinTbAddrZsPitch; x++)
            {
                Bs32u ctbAddrRs = PicWidthInCtbsY * ((y << MinTbLog2SizeY) >> CtbLog2SizeY)
                    + ((x << MinTbLog2SizeY) >> CtbLog2SizeY);

                MinTbAddrZs(x, y) = (CtbAddrRsToTs[ctbAddrRs] << ((CtbLog2SizeY - MinTbLog2SizeY) * 2));

                for (i = 0, p = 0; i < (CtbLog2SizeY - MinTbLog2SizeY); i++)
                {
                    m = (1 << i);
                    p += ((m & x) ? (m * m) : 0) + ((m & y) ? (2 * m * m) : 0);
                }

                MinTbAddrZs(x, y) += p;
            }
        }
    }

    m_cSlice = &slice;
}

void Info::decodePOC(NALU& nalu)
{
    auto& slice = *nalu.slice;
    PocInfo POC = {(Bs32s)slice.pic_order_cnt_lsb, 0};

    if (!(isIRAP(nalu) && NoRaslOutputFlag))
    {
        if ((POC.Lsb < m_prevPOC.Lsb) && ((m_prevPOC.Lsb - POC.Lsb) >= (MaxPicOrderCntLsb / 2)))
            POC.Msb = m_prevPOC.Msb + MaxPicOrderCntLsb;
        else if ((POC.Lsb > m_prevPOC.Lsb) && ((POC.Lsb - m_prevPOC.Lsb) > (MaxPicOrderCntLsb / 2)))
            POC.Msb = m_prevPOC.Msb - MaxPicOrderCntLsb;
        else
            POC.Msb = m_prevPOC.Msb;
    }

    slice.POC = POC.Msb + POC.Lsb;

    if(    nalu.nuh_temporal_id_plus1 == 1
        && !(isRASL(nalu) || isRADL(nalu) || isSLNonRefPic(nalu)))
    {
        m_prevPOC = POC;
    }
}

struct FindLt
{
    Bs32s m_max;
    Bs32s m_poc;

    FindLt(Bs32s poc, Bs32s max)
    {
        m_poc = poc;
        m_max = max;
    }

    inline bool operator () (const std::pair<Bs32s, bool>& v)
    {
        return ((v.first & (m_max - 1)) == m_poc) && v.second;
    }
};

void Info::decodeRPL(NALU& nalu)
{
    auto& slice = *nalu.slice;
    auto& pps = *slice.pps;
    std::vector<RefPic> RPS, TmpL0, TmpL1, CurrBefore, CurrAfter, CurrLt;
    size_t NumPicTotalCurr = 0, TmpL0Sz, TmpL1Sz;
    RefPic currPic = {};

    currPic.POC = slice.POC;
    currPic.used = pps.curr_pic_ref_enabled_flag;
    currPic.lost = !pps.curr_pic_ref_enabled_flag;
    currPic.long_term = true;

    for (Bs32u i = 0; i < slice.strps.NumDeltaPocs; i++)
    {
        RefPic ref = {};

        ref.POC  = slice.POC + slice.strps.DeltaPoc[i];
        ref.used = GetBit(slice.strps.UsedByCurrPicFlags, i);
        ref.lost = !m_DPB[ref.POC];

        RPS.push_back(ref);

        if (ref.used)
        {
            NumPicTotalCurr++;

            if (ref.POC < slice.POC)
                CurrBefore.push_back(ref);
            else
                CurrAfter.push_back(ref);
        }
    }

    for (Bs32u i = 0; i < (Bs32u)(slice.num_long_term_sps + slice.num_long_term_pics); i++)
    {
        RefPic ref = {1, 0, 0, 0};

        ref.POC = slice.poc_lsb_lt[i];

        if (slice.DeltaPocMsbCycleLt[i])
        {
            ref.POC += slice.POC - slice.DeltaPocMsbCycleLt[ i ] * MaxPicOrderCntLsb - (slice.POC & (MaxPicOrderCntLsb - 1));
            ref.lost = !m_DPB[ref.POC];
        }
        else
        {
            auto picX = std::find_if(m_DPB.begin(), m_DPB.end(), FindLt(ref.POC, MaxPicOrderCntLsb));

            ref.lost = (m_DPB.end() == picX);

            if (!ref.lost)
                ref.POC = picX->first;
        }

        ref.used = GetBit(slice.used_by_curr_pic_lt_flags, i);

        RPS.push_back(ref);

        if (ref.used)
        {
            NumPicTotalCurr++;
            CurrLt.push_back(ref);
        }
    }

    if (pps.curr_pic_ref_enabled_flag)
        NumPicTotalCurr++;

    if (RPS.size())
        memcpy(slice.DPB, &RPS[0], RPS.size() * sizeof(RefPic));

    TmpL0Sz = BS_MAX(NumPicTotalCurr, slice.num_ref_idx_l0_active);
    TmpL1Sz = BS_MAX(NumPicTotalCurr, slice.num_ref_idx_l1_active);

    m_DPBafter.erase(m_DPBafter.begin(), m_DPBafter.end());

    for (auto& ref : RPS)
        m_DPBafter[ref.POC] = !ref.lost;

    m_DPBafter[slice.POC] = 1;//!isSLNonRefPic(nalu);

#define _ADD_REFS(DST, SRC, MAX)                 \
{                                                \
    size_t _N = BS_MIN(MAX, SRC.size());         \
    if (_N > 0)                                  \
    {                                            \
        auto end = SRC.begin();                  \
        std::advance(end, _N);                   \
        DST.insert(DST.end(), SRC.begin(), end); \
        MAX -= _N;                               \
    }                                            \
}

    while (TmpL0Sz > 0)
    {
        _ADD_REFS(TmpL0, CurrBefore, TmpL0Sz);
        _ADD_REFS(TmpL0, CurrAfter,  TmpL0Sz);
        _ADD_REFS(TmpL0, CurrLt,     TmpL0Sz);

        if (pps.curr_pic_ref_enabled_flag && TmpL0Sz)
        {
            TmpL0.push_back(currPic);
            TmpL0Sz--;
        }
    }

    while (TmpL1Sz > 0)
    {
        _ADD_REFS(TmpL1, CurrAfter,  TmpL1Sz);
        _ADD_REFS(TmpL1, CurrBefore, TmpL1Sz);
        _ADD_REFS(TmpL1, CurrLt,     TmpL1Sz);

        if (pps.curr_pic_ref_enabled_flag && TmpL1Sz)
        {
            TmpL1.push_back(currPic);
            TmpL1Sz--;
        }
    }

#undef _ADD_REFS

    if (slice.num_ref_idx_l0_active > 0)
    {
        if (slice.ref_pic_list_modification_flag_l0)
        {
            for (Bs32u i = 0; i < slice.num_ref_idx_l0_active; i++)
                slice.L0[i] = TmpL0[slice.list_entry_lx[0][i]];
        }
        else
        {
            memcpy(slice.L0, &TmpL0[0], slice.num_ref_idx_l0_active * sizeof(RefPic));
        }
    }

    if (   pps.curr_pic_ref_enabled_flag
        && slice.type != SLICE_TYPE::I
        && !slice.ref_pic_list_modification_flag_l0
        && TmpL0.size() > slice.num_ref_idx_l0_active)
        slice.L0[slice.num_ref_idx_l0_active - 1] = currPic;

    if (slice.num_ref_idx_l1_active > 0)
    {
        if (slice.ref_pic_list_modification_flag_l1)
        {
            for (Bs32u i = 0; i < slice.num_ref_idx_l1_active; i++)
                slice.L1[i] = TmpL1[slice.list_entry_lx[1][i]];
        }
        else
        {
            memcpy(slice.L1, &TmpL1[0], slice.num_ref_idx_l1_active * sizeof(RefPic));
        }
    }
}

bool Info::AvailableZs(Bs16s xCurr, Bs16s yCurr, Bs16s xNbY, Bs16s yNbY)
{
    auto& sps = *m_cSlice->sps;
    Bs32s minBlockAddrCurr = MinTbAddrZs(xCurr >> MinTbLog2SizeY, yCurr >> MinTbLog2SizeY);
    Bs32s minBlockAddrN = -1;
    Bs32u CtbAddrInRsCurr = (yCurr >> CtbLog2SizeY) * PicWidthInCtbsY + (xCurr >> CtbLog2SizeY);
    Bs32u CtbAddrInRsN    = 0;
    Bs32u CtbAddrInTsCurr = CtbAddrRsToTs[CtbAddrInRsCurr];
    Bs32u CtbAddrInTsN    = CtbAddrRsToTs[CtbAddrInRsN];

    if (   xNbY >= 0
        && yNbY >= 0
        && xNbY < sps.pic_width_in_luma_samples
        && yNbY < sps.pic_height_in_luma_samples)
    {
        minBlockAddrN = MinTbAddrZs(xNbY >> MinTbLog2SizeY, yNbY >> MinTbLog2SizeY);
        CtbAddrInRsN  = (yNbY>>CtbLog2SizeY) * PicWidthInCtbsY + (xNbY>>CtbLog2SizeY);
        CtbAddrInTsN  = CtbAddrRsToTs[CtbAddrInRsN];
    }

    if (   minBlockAddrN < 0
        || minBlockAddrN > minBlockAddrCurr
        || SliceAddrRsInTs[CtbAddrInTsN] != SliceAddrRsInTs[CtbAddrInTsCurr]
        || TileId[CtbAddrInTsN] != TileId[CtbAddrInTsCurr])
    {
        return false;
    }

    return true;
}

bool Info::AvailablePb(Bs16s xCb, Bs16s yCb, Bs16u nCbS, Bs16s xPb, Bs16s yPb,
        Bs16s nPbW, Bs16s nPbH, Bs16u partIdx, Bs16s xNbY, Bs16s yNbY)
{
    bool sameCb =
           xCb <= xNbY
        && yCb <= yNbY
        && (xCb + nCbS) > xNbY
        && (yCb + nCbS) > yNbY;

    if (!sameCb)
    {
        if (AvailableZs(xPb, yPb, xNbY, yNbY))
        {
            CU* p = GetCU(xNbY, yNbY);

            if (!p)
                throw InvalidSyntax();

            return (p->PredMode != MODE_INTRA);
        }
        return false;
    }

    if (   (nPbW << 1) == nCbS
        && (nPbH << 1) == nCbS
        && partIdx == 1
        && (yCb + nPbH) <= yNbY
        && (xCb + nPbW) > xNbY)
        return false;

    CU* p = GetCU(xNbY, yNbY);

    if (!p)
        throw InvalidSyntax();

    return (p->PredMode != MODE_INTRA);
}

CU* GetUnit(CU& cu, Bs16s x, Bs16s y)
{
    for (CU* p = &cu; p; p = p->Next)
    {
        if (   x >= p->x
            && y >= p->y
            && x < (p->x + (1 << p->log2CbSize))
            && y < (p->y + (1 << p->log2CbSize)))
            return p;
    }
    return 0;
}

PU* GetUnit(PU& pu, Bs16s x, Bs16s y)
{
    for (PU* p = &pu; p; p = p->Next)
    {
        if (   x >= p->x
            && y >= p->y
            && x < (p->x + p->w)
            && y < (p->y + p->h))
            return p;
    }
    return 0;
}

TU* GetUnit(TU& cu, Bs16s x, Bs16s y)
{
    for (TU* p = &cu; p; p = p->Next)
    {
        if (   x >= p->x
            && y >= p->y
            && x < (p->x + (1 << p->log2TrafoSize))
            && y < (p->y + (1 << p->log2TrafoSize)))
            return p;
    }
    return 0;
}

CU* Info::GetCU(Bs16s x, Bs16s y)
{
    CTU* p = CtuInRs[(y >> CtbLog2SizeY) * PicWidthInCtbsY + (x >> CtbLog2SizeY)];
    if (p)
        return GetUnit(*p->Cu, x, y);
    return 0;
}

TU* Info::GetTU(CU& cu, Bs16s x, Bs16s y)
{
    if (!cu.Tu)
        return 0;
    return GetUnit(*cu.Tu, x, y);
}

PU* Info::GetPU(Bs16s x, Bs16s y)
{
    CU* cu = GetCU(x, y);
    if (!cu || !cu->Pu)
        return 0;
    return GetUnit(*cu->Pu, x, y);
}

#define Clip3(_min, _max, _x) std::min(std::max(_min, _x), _max)
const Bs16s QpC_trans[14] = { 29, 30, 31, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37};

Bs16s Info::GetQpCb(Bs16s QpY, Bs16s CuQpOffsetCb)
{
    Bs16s QpCb = Clip3(-QpBdOffsetC, 57, QpY + m_cSlice->pps->cb_qp_offset + m_cSlice->cb_qp_offset + CuQpOffsetCb);

    if (ChromaArrayType == 1)
    {
        if (QpCb > 43)
            return QpCb - 6;

        if (QpCb >= 30)
            return QpC_trans[QpCb - 30];

        return QpCb;
    }

    return BS_MIN(51, QpCb);
}

Bs16s Info::GetQpCr(Bs16s QpY, Bs16s CuQpOffsetCr)
{
    Bs16s QpCr = Clip3(-QpBdOffsetC, 57, QpY + m_cSlice->pps->cr_qp_offset + m_cSlice->cr_qp_offset + CuQpOffsetCr);

    if (ChromaArrayType == 1)
    {
        if (QpCr > 43)
            return QpCr - 6;

        if (QpCr >= 30)
            return QpC_trans[QpCr - 30];

        return QpCr;
    }

    return BS_MIN(51, QpCr);
}

inline bool PredFlagLX(PU&pu, Bs16u X) { return pu.inter_pred_idc == PRED_BI || pu.inter_pred_idc == X; }
template<class T> inline T Sign(T x) { return (x > 0) ? 1 : (x < 0) ? -1 : 0; }
template<class T> inline bool EQ(const T& l, const T& r) { return !memcmp(&l, &r, sizeof(T)); }
template<class T> inline bool CP(T& d, const T& s) { return !memmove(&d, &s, sizeof(T)); }

const Bs8u lXCandIdx[12][2] =
{
    { 0, 1 }, { 1, 0 }, { 0, 2 }, { 2, 0 },
    { 1, 2 }, { 2, 1 }, { 0, 3 }, { 3, 0 },
    { 1, 3 }, { 3, 1 }, { 2, 3 }, { 3, 2 }
};

void Info::decodeMvLX(CU& cu, PU&pu, Bs16u partIdx)
{
    if (!pu.merge_flag)
        throw InvalidSyntax();

    //8.5.3.2.2
    const Bs16u A0 = 0, A1 = 1, B0 = 2, B1 = 3, B2 = 4, Col = 5;
    const Bs16u MaxCand = 6 * 3;
    Bs16s xPb = pu.x;
    Bs16s yPb = pu.y;
    Bs16s nPbW = pu.w;
    Bs16s nPbH = pu.h;
    Bs16u nCbS = (1 << cu.log2CbSize);
    bool  availableFlag[MaxCand] = {};
    bool  predFlagLX[MaxCand][2] = {};
    Bs16s mvLX[MaxCand][2][2] = {};
    Bs16s refIdxLX[MaxCand][2];
    Bs16u mergeCandList[MaxCand] = {};
    Bs16u numCurrMergeCand = 0
        , numOrigMergeCand = 0
        , MaxNumMergeCand;
    auto& slice = *m_cSlice;

    memset(refIdxLX, -1, sizeof(refIdxLX));

    if (Log2ParMrgLevel > 2 && nCbS == 8)
    {
        xPb = cu.x;
        yPb = cu.y;
        nPbW = nCbS;
        nPbH = nCbS;
        partIdx = 0;
    }

    //8.5.3.2.3
    {
        PU* puN;
        Bs16s xNbN = xPb - 1;
        Bs16s yNbN = yPb + nPbH - 1;
        bool availableA1, availableB1, availableB0, availableA0, availableB2;
        auto CopyPred = [&] (Bs16u N)
        {
            predFlagLX[N][0] = (puN->inter_pred_idc == PRED_L0 || puN->inter_pred_idc == PRED_BI);
            predFlagLX[N][1] = (puN->inter_pred_idc == PRED_L1 || puN->inter_pred_idc == PRED_BI);
            if (predFlagLX[N][0])
            {
                CP(mvLX[N][0], puN->MvLX[0]);
                refIdxLX[N][0] = puN->ref_idx_l0;
            }
            if (predFlagLX[N][1])
            {
                CP(mvLX[N][1], puN->MvLX[1]);
                refIdxLX[N][1] = puN->ref_idx_l1;
            }
        };

        if (   (   (xPb >> Log2ParMrgLevel) == (xNbN >> Log2ParMrgLevel)
                && (yPb >> Log2ParMrgLevel) == (yNbN >> Log2ParMrgLevel))
            || (partIdx == 1 && (cu.PartMode == PART_Nx2N || cu.PartMode == PART_nLx2N || cu.PartMode == PART_nRx2N)))
            availableA1 = false;
        else
            availableA1 = AvailablePb(cu.x, cu.y, nCbS, xPb, yPb, nPbW, nPbH, partIdx, xNbN, yNbN);

        if (availableA1)
        {
            puN = GetPU(xNbN, yNbN);

            CopyPred(A1);
            availableFlag[A1] = true;

            mergeCandList[numCurrMergeCand++] = A1;
            if (numCurrMergeCand > pu.merge_idx)
                goto l_done;
        }

        xNbN = xPb + nPbW - 1;
        yNbN = yPb - 1;

        if (   (   (xPb >> Log2ParMrgLevel) == (xNbN >> Log2ParMrgLevel)
                && (yPb >> Log2ParMrgLevel) == (yNbN >> Log2ParMrgLevel))
            || (partIdx == 1 && (cu.PartMode == PART_2NxN || cu.PartMode == PART_2NxnU || cu.PartMode == PART_2NxnD)))
            availableB1 = false;
        else
            availableB1 = AvailablePb(cu.x, cu.y, nCbS, xPb, yPb, nPbW, nPbH, partIdx, xNbN, yNbN);

        if (availableB1)
        {
            puN = GetPU(xNbN, yNbN);

            CopyPred(B1);

            if (!availableA1 || !(EQ(mvLX[B1], mvLX[A1]) && EQ(refIdxLX[B1], refIdxLX[A1])))
            {
                availableFlag[B1] = true;

                mergeCandList[numCurrMergeCand++] = B1;
                if (numCurrMergeCand > pu.merge_idx)
                    goto l_done;
            }
        }

        xNbN = xPb + nPbW;
        yNbN = yPb - 1;

        if (   (xPb >> Log2ParMrgLevel) == (xNbN >> Log2ParMrgLevel)
            && (yPb >> Log2ParMrgLevel) == (yNbN >> Log2ParMrgLevel))
            availableB0 = false;
        else
            availableB0 = AvailablePb(cu.x, cu.y, nCbS, xPb, yPb, nPbW, nPbH, partIdx, xNbN, yNbN);

        if (availableB0)
        {
            puN = GetPU(xNbN, yNbN);

            CopyPred(B0);

            if (!availableB1 || !(EQ(mvLX[B0], mvLX[B1]) && EQ(refIdxLX[B0], refIdxLX[B1])))
            {
                availableFlag[B0] = true;

                mergeCandList[numCurrMergeCand++] = B0;
                if (numCurrMergeCand > pu.merge_idx)
                    goto l_done;
            }
        }

        xNbN = xPb - 1;
        yNbN = yPb + nPbH;

        if (   (xPb >> Log2ParMrgLevel) == (xNbN >> Log2ParMrgLevel)
            && (yPb >> Log2ParMrgLevel) == (yNbN >> Log2ParMrgLevel))
            availableA0 = false;
        else
            availableA0 = AvailablePb(cu.x, cu.y, nCbS, xPb, yPb, nPbW, nPbH, partIdx, xNbN, yNbN);

        if (availableA0)
        {
            puN = GetPU(xNbN, yNbN);

            CopyPred(A0);

            if (!availableA1 || !(EQ(mvLX[A0], mvLX[A1]) && EQ(refIdxLX[A0], refIdxLX[A1])))
            {
                availableFlag[A0] = true;

                mergeCandList[numCurrMergeCand++] = A0;
                if (numCurrMergeCand > pu.merge_idx)
                    goto l_done;
            }
        }

        xNbN = xPb - 1;
        yNbN = yPb - 1;

        if (   (xPb >> Log2ParMrgLevel) == (xNbN >> Log2ParMrgLevel)
            && (yPb >> Log2ParMrgLevel) == (yNbN >> Log2ParMrgLevel))
            availableB2 = false;
        else
            availableB2 = AvailablePb(cu.x, cu.y, nCbS, xPb, yPb, nPbW, nPbH, partIdx, xNbN, yNbN);

        if (availableB2)
        {
            puN = GetPU(xNbN, yNbN);

            CopyPred(B2);

            if (   (!availableA1 || !(EQ(mvLX[B2], mvLX[A1]) && EQ(refIdxLX[B2], refIdxLX[A1])))
                && (!availableB1 || !(EQ(mvLX[B2], mvLX[B1]) && EQ(refIdxLX[B2], refIdxLX[B1])))
                && !(availableFlag[A0] && availableFlag[A1] && availableFlag[B0] && availableFlag[B1]))
            {
                availableFlag[B2] = true;

                mergeCandList[numCurrMergeCand++] = B2;
                if (numCurrMergeCand > pu.merge_idx)
                    goto l_done;
            }
        }
    }

    {
        refIdxLX[Col][0] = 0;
        refIdxLX[Col][1] = 0;

        bool  availableFlagL0Col = false
            , availableFlagL1Col = false;

        //8.5.3.2.8
        if (m_cSlice->temporal_mvp_enabled_flag)
            availableFlagL0Col = decodeMvLXCol(xPb, yPb, nPbW, nPbH, 0, refIdxLX[Col][0], mvLX[Col][0]);

        availableFlag[Col] = availableFlagL0Col;
        predFlagLX[Col][0] = availableFlagL0Col;
        predFlagLX[Col][1] = 0;

        if (m_cSlice->type == SLICE_TYPE::B)
        {
            //8.5.3.2.8
            if (m_cSlice->temporal_mvp_enabled_flag)
                availableFlagL1Col = decodeMvLXCol(xPb, yPb, nPbW, nPbH, 1, refIdxLX[Col][1], mvLX[Col][1]);

            availableFlag[Col] = availableFlagL0Col || availableFlagL1Col;
            predFlagLX[Col][1] = availableFlagL1Col;
        }

        if (availableFlag[Col])
        {
            mergeCandList[numCurrMergeCand++] = Col;
            if (numCurrMergeCand > pu.merge_idx)
                goto l_done;
        }

    }

    numOrigMergeCand = numCurrMergeCand;
    MaxNumMergeCand = std::min<Bs16u>(m_cSlice->MaxNumMergeCand, pu.merge_idx + 1);

    if (   m_cSlice->type == SLICE_TYPE::B
        && numOrigMergeCand > 1
        && numOrigMergeCand < MaxNumMergeCand)
    {
        //8.5.3.2.4
        Bs16u numInputMergeCand = numCurrMergeCand;
        Bs16u combIdx = 0;
        auto RefPicList0 = m_cSlice->L0;
        auto RefPicList1 = m_cSlice->L1;

        while (combIdx < (numOrigMergeCand * (numOrigMergeCand - 1))
            && numCurrMergeCand < MaxNumMergeCand)
        {
            auto l0Cand = mergeCandList[lXCandIdx[combIdx][0]];
            auto l1Cand = mergeCandList[lXCandIdx[combIdx][1]];

            if (   predFlagLX[l0Cand][0]
                && predFlagLX[l1Cand][1]
                && (   RefPicList0[refIdxLX[l0Cand][0]].POC != RefPicList1[refIdxLX[l1Cand][1]].POC
                    || !EQ(mvLX[l0Cand][0], mvLX[l1Cand][1])))
            {
                Bs16u combCandK = numCurrMergeCand - numInputMergeCand + 6;

                refIdxLX[combCandK][0]   = refIdxLX[l0Cand][0];
                refIdxLX[combCandK][1]   = refIdxLX[l1Cand][1];
                predFlagLX[combCandK][0] = true;
                predFlagLX[combCandK][1] = true;
                mvLX[combCandK][0][0]    = mvLX[l0Cand][0][0];
                mvLX[combCandK][0][1]    = mvLX[l0Cand][0][1];
                mvLX[combCandK][1][0]    = mvLX[l1Cand][1][0];
                mvLX[combCandK][1][1]    = mvLX[l1Cand][1][1];

                mergeCandList[numCurrMergeCand] = combCandK;
                numCurrMergeCand++;
            }
            combIdx++;
        }
    }

    //8.5.3.2.5
    if (numCurrMergeCand < MaxNumMergeCand)
    {
        Bs16u numRefIdx = (m_cSlice->type == P)
            ? m_cSlice->num_ref_idx_l0_active
            : BS_MIN(m_cSlice->num_ref_idx_l0_active, m_cSlice->num_ref_idx_l1_active);
        Bs16u numInputMergeCand = numCurrMergeCand;

        if (m_cSlice->type == P)
        {
            for (Bs16u zeroIdx = 0; numCurrMergeCand < MaxNumMergeCand; zeroIdx++)
            {
                Bs16u zeroCandM = numCurrMergeCand - numInputMergeCand + 12;

                refIdxLX  [zeroCandM][0] = (zeroIdx < numRefIdx) ? zeroIdx : 0;
                refIdxLX  [zeroCandM][1] = -1;
                predFlagLX[zeroCandM][0] = 1;
                predFlagLX[zeroCandM][1] = 0;

                mergeCandList[numCurrMergeCand] = zeroCandM;
                numCurrMergeCand++;
            }
        }
        else
        {
            for (Bs16u zeroIdx = 0; numCurrMergeCand < MaxNumMergeCand; zeroIdx++)
            {
                Bs16u zeroCandM = numCurrMergeCand - numInputMergeCand + 12;

                refIdxLX  [zeroCandM][0] = (zeroIdx < numRefIdx) ? zeroIdx : 0;
                refIdxLX  [zeroCandM][1] = (zeroIdx < numRefIdx) ? zeroIdx : 0;
                predFlagLX[zeroCandM][0] = 1;
                predFlagLX[zeroCandM][1] = 1;

                mergeCandList[numCurrMergeCand] = zeroCandM;
                numCurrMergeCand++;
            }
        }
    }

l_done:
    Bs16u N = mergeCandList[pu.merge_idx];

    CP(pu.MvLX, mvLX[N]);
    pu.ref_idx_l0 = refIdxLX[N][0];
    pu.ref_idx_l1 = refIdxLX[N][1];

    if (   slice.use_integer_mv_flag
        || (predFlagLX[N][0] && slice.POC == slice.L0[pu.ref_idx_l0].POC))
    {
        pu.MvLX[0][0] = ((pu.MvLX[0][0] >> 2) << 2);
        pu.MvLX[0][1] = ((pu.MvLX[0][1] >> 2) << 2);
    }

    if (   slice.use_integer_mv_flag
        || (predFlagLX[N][1] && slice.POC == slice.L1[pu.ref_idx_l1].POC))
    {
        pu.MvLX[1][0] = ((pu.MvLX[1][0] >> 2) << 2);
        pu.MvLX[1][1] = ((pu.MvLX[1][1] >> 2) << 2);
    }

    if (predFlagLX[N][0] && predFlagLX[N][1])
        pu.inter_pred_idc = PRED_BI;
    else
        pu.inter_pred_idc = predFlagLX[N][0] ? PRED_L0 : PRED_L1;

    if (pu.inter_pred_idc == PRED_BI && (pu.w + pu.h == 12))
    {
        pu.inter_pred_idc = PRED_L0;
    }
}

void Info::decodeMvLX(CU& cu, PU&pu, Bs16u X, Bs32s (&mvdLX)[2], Bs16u partIdx)
{
    if (pu.merge_flag)
        throw InvalidSyntax();

    if (!PredFlagLX(pu, X))
        return;

    auto refIdxLX = (X ? pu.ref_idx_l1 : pu.ref_idx_l0);
    auto& RefPicListX = (X ? m_cSlice->L1 : m_cSlice->L0);
    auto& RefPicListY = (!X ? m_cSlice->L1 : m_cSlice->L0);

    Bs32s uLX[2], mvpLX[2] = {};
    //8.5.3.2.6:
    {
        bool  availableFlagLXA = false
            , availableFlagLXB = false
            , availableFlagLXCol = false;
        Bs32s mvLXA[2] = {}, mvLXB[2] = {}, i, mvpListLX[2][2];
        Bs16s mvLXCol[2];

        //8.5.3.2.7
        {
            bool  isScaledFlagLX = false;
            Bs16s xNbA[2], yNbA[2], refIdxA = -1
                , xNbB[3], yNbB[3], refIdxB = -1;
            RefPic* refPicListA = 0;
            RefPic* refPicListB = 0;
            Bs16u nCbS = (1 << cu.log2CbSize);

            xNbA[0] = pu.x - 1;
            yNbA[0] = pu.y + pu.h;
            xNbA[1] = xNbA[0];
            yNbA[1] = yNbA[0] - 1;

            xNbB[0] = pu.x + pu.w;
            yNbB[0] = pu.y - 1;
            xNbB[1] = pu.x + pu.w - 1;
            yNbB[1] = pu.y - 1;
            xNbB[2] = pu.x - 1;
            yNbB[2] = pu.y - 1;

            bool availableA[2] =
            {
                AvailablePb(cu.x, cu.y, nCbS, pu.x, pu.y, pu.w, pu.h, partIdx, xNbA[0], yNbA[0]),
                AvailablePb(cu.x, cu.y, nCbS, pu.x, pu.y, pu.w, pu.h, partIdx, xNbA[1], yNbA[1])
            };
            bool availableB[3] =
            {
                AvailablePb(cu.x, cu.y, nCbS, pu.x, pu.y, pu.w, pu.h, partIdx, xNbB[0], yNbB[0]),
                AvailablePb(cu.x, cu.y, nCbS, pu.x, pu.y, pu.w, pu.h, partIdx, xNbB[1], yNbB[1]),
                AvailablePb(cu.x, cu.y, nCbS, pu.x, pu.y, pu.w, pu.h, partIdx, xNbB[2], yNbB[2])
            };

            if (availableA[0] || availableA[1])
                isScaledFlagLX = true;

            for (Bs16u k = 0; k < 2; k++)
            {
                if (!availableA[k] || availableFlagLXA)
                    continue;

                PU* p = GetPU(xNbA[k], yNbA[k]);
                if (!p)
                    throw InvalidSyntax();
                PU& puA = *p;

                Bs16u refIdxAX = ( X ? puA.ref_idx_l1 : puA.ref_idx_l0);
                Bs16u refIdxAY = (!X ? puA.ref_idx_l1 : puA.ref_idx_l0);

                if (   PredFlagLX(puA, X)
                    && RefPicListX[refIdxAX].POC == RefPicListX[refIdxLX].POC)
                {
                    availableFlagLXA = true;
                    mvLXA[0] = puA.MvLX[X][0];
                    mvLXA[1] = puA.MvLX[X][1];
                }
                else if (PredFlagLX(puA, !X)
                      && RefPicListY[refIdxAY].POC == RefPicListX[refIdxLX].POC)
                {
                    availableFlagLXA = true;
                    mvLXA[0] = puA.MvLX[!X][0];
                    mvLXA[1] = puA.MvLX[!X][1];
                }
            }

            for (Bs16u k = 0; k < 2; k++)
            {
                if (!availableA[k] || availableFlagLXA)
                    continue;

                PU* p = GetPU(xNbA[k], yNbA[k]);
                if (!p)
                    throw InvalidSyntax();
                PU& puA = *p;

                Bs16u refIdxAX = ( X ? puA.ref_idx_l1 : puA.ref_idx_l0);
                Bs16u refIdxAY = (!X ? puA.ref_idx_l1 : puA.ref_idx_l0);

                if (   PredFlagLX(puA, X)
                    && RefPicListX[refIdxAX].long_term == RefPicListX[refIdxLX].long_term)
                {
                    availableFlagLXA = true;
                    mvLXA[0] = puA.MvLX[X][0];
                    mvLXA[1] = puA.MvLX[X][1];
                    refIdxA = refIdxAX;
                    refPicListA = RefPicListX;
                }
                else if (   PredFlagLX(puA, !X)
                    && RefPicListY[refIdxAY].long_term == RefPicListX[refIdxLX].long_term)
                {
                    availableFlagLXA = true;
                    mvLXA[0] = puA.MvLX[!X][0];
                    mvLXA[1] = puA.MvLX[!X][1];
                    refIdxA = refIdxAY;
                    refPicListA = RefPicListY;
                }

                if (   availableFlagLXA
                    && refPicListA[refIdxA].POC != RefPicListX[refIdxLX].POC
                    && !refPicListA[refIdxA].long_term
                    && !RefPicListX[refIdxLX].long_term)
                {
                    Bs32s td = Clip3(-128, 127, m_cSlice->POC - refPicListA[refIdxA].POC);
                    Bs32s tb = Clip3(-128, 127, m_cSlice->POC - RefPicListX[refIdxLX].POC);
                    Bs32s tx = (16384 + (BS_ABS(td) >> 1)) / td;
                    Bs32s distScaleFactor = Clip3(-4096, 4095, (tb * tx + 32) >> 6);

                    mvLXA[0] = Clip3(-32768, 32767, Sign(distScaleFactor * mvLXA[0])
                        * ((abs(distScaleFactor * mvLXA[0]) + 127) >> 8));
                    mvLXA[1] = Clip3(-32768, 32767, Sign(distScaleFactor * mvLXA[1])
                        * ((abs(distScaleFactor * mvLXA[1]) + 127) >> 8));
                }
            }

            for (Bs16u k = 0; k < 3; k++)
            {
                if (!availableB[k] || availableFlagLXB)
                    continue;

                PU* p = GetPU(xNbB[k], yNbB[k]);
                if (!p)
                    throw InvalidSyntax();
                PU& puB = *p;

                Bs16u refIdxBX = ( X ? puB.ref_idx_l1 : puB.ref_idx_l0);
                Bs16u refIdxBY = (!X ? puB.ref_idx_l1 : puB.ref_idx_l0);

                if (   PredFlagLX(puB, X)
                    && RefPicListX[refIdxBX].POC == RefPicListX[refIdxLX].POC)
                {
                    availableFlagLXB = true;
                    mvLXB[0] = puB.MvLX[X][0];
                    mvLXB[1] = puB.MvLX[X][1];
                }
                else if (PredFlagLX(puB, !X)
                      && RefPicListY[refIdxBY].POC == RefPicListX[refIdxLX].POC)
                {
                    availableFlagLXB = true;
                    mvLXB[0] = puB.MvLX[!X][0];
                    mvLXB[1] = puB.MvLX[!X][1];
                }
            }

            if (!isScaledFlagLX && availableFlagLXB)
            {
                availableFlagLXA = true;
                mvLXA[0] = mvLXB[0];
                mvLXA[1] = mvLXB[1];
            }

            if (!isScaledFlagLX)
            {
                availableFlagLXB = false;

                for (Bs16u k = 0; k < 3; k++)
                {
                    if (!availableB[k] || availableFlagLXB)
                        continue;

                    PU* p = GetPU(xNbB[k], yNbB[k]);
                    if (!p)
                        throw InvalidSyntax();
                    PU& puB = *p;

                    Bs16u refIdxBX = ( X ? puB.ref_idx_l1 : puB.ref_idx_l0);
                    Bs16u refIdxBY = (!X ? puB.ref_idx_l1 : puB.ref_idx_l0);

                    if (   PredFlagLX(puB, X)
                        && RefPicListX[refIdxBX].long_term == RefPicListX[refIdxLX].long_term)
                    {
                        availableFlagLXB = true;
                        mvLXB[0] = puB.MvLX[X][0];
                        mvLXB[1] = puB.MvLX[X][1];
                        refIdxB = refIdxBX;
                        refPicListB = RefPicListX;
                    }
                    else if (   PredFlagLX(puB, !X)
                        && RefPicListY[refIdxBY].long_term == RefPicListX[refIdxLX].long_term)
                    {
                        availableFlagLXB = true;
                        mvLXB[0] = puB.MvLX[!X][0];
                        mvLXB[1] = puB.MvLX[!X][1];
                        refIdxB = refIdxBY;
                        refPicListB = RefPicListY;
                    }

                    if (   availableFlagLXB
                        && refPicListB[refIdxB].POC != RefPicListX[refIdxLX].POC
                        && !refPicListB[refIdxB].long_term
                        && !RefPicListX[refIdxLX].long_term)
                    {
                        Bs32s td = Clip3(-128, 127, m_cSlice->POC - refPicListB[refIdxB].POC);
                        Bs32s tb = Clip3(-128, 127, m_cSlice->POC - RefPicListX[refIdxLX].POC);
                        Bs32s tx = (16384 + (BS_ABS(td) >> 1)) / td;
                        Bs32s distScaleFactor = Clip3(-4096, 4095, (tb * tx + 32) >> 6);

                        mvLXB[0] = Clip3(-32768, 32767, Sign(distScaleFactor * mvLXB[0])
                            * ((abs(distScaleFactor * mvLXB[0]) + 127) >> 8));
                        mvLXB[1] = Clip3(-32768, 32767, Sign(distScaleFactor * mvLXB[1])
                            * ((abs(distScaleFactor * mvLXB[1]) + 127) >> 8));
                    }
                }
            }

        }

        bool mvLXA_NE_mvLXB = !EQ(mvLXA, mvLXB);

        if (   availableFlagLXA
            && availableFlagLXB
            && mvLXA_NE_mvLXB)
        {
            availableFlagLXCol = false;
        }
        else if (m_cSlice->temporal_mvp_enabled_flag)
        {
            //8.5.3.2.8
            availableFlagLXCol = decodeMvLXCol(pu.x, pu.y, pu.w, pu.h, X, refIdxLX, mvLXCol);
        }

        i = 0;
        if (availableFlagLXA)
        {
            CP(mvpListLX[i++], mvLXA);

            if (availableFlagLXB && mvLXA_NE_mvLXB)
                CP(mvpListLX[i++], mvLXB);
        }
        else if (availableFlagLXB)
            CP(mvpListLX[i++], mvLXB);

        if (i < 2 && availableFlagLXCol)
        {
            mvpListLX[i][0] = mvLXCol[0];
            mvpListLX[i][1] = mvLXCol[1];
            i++;
        }

        while (i < 2)
        {
            mvpListLX[i][0] = 0;
            mvpListLX[i][1] = 0;
            i++;
        }

        CP(mvpLX, mvpListLX[X ? pu.mvp_l1_flag : pu.mvp_l0_flag]);
    }

    auto& slice = *m_cSlice;

    if (slice.use_integer_mv_flag || slice.POC == (X ? slice.L1 : slice.L0)[refIdxLX].POC)
    {
        uLX[0]        = ((((mvpLX[0] >> 2) + mvdLX[0]) << 2) + (1 << 16)) % (1 << 16);
        pu.MvLX[X][0] = (uLX[0] >= (1 << 15)) ? (uLX[0] - (1 << 16)) : uLX[0];
        uLX[1]        = ((((mvpLX[1] >> 2) + mvdLX[1]) << 2) + (1 << 16)) % (1 << 16);
        pu.MvLX[X][1] = (uLX[1] >= (1 << 15)) ? (uLX[1] - (1 << 16)) : uLX[1];
    }
    else
    {
        uLX[0]        = (mvpLX[0] + mvdLX[0] + (1 << 16)) % (1 << 16);
        pu.MvLX[X][0] = (uLX[0] >= (1 << 15)) ? (uLX[0] - (1 << 16)) : uLX[0];
        uLX[1]        = (mvpLX[1] + mvdLX[1] + (1 << 16)) % (1 << 16);
        pu.MvLX[X][1] = (uLX[1] >= (1 << 15)) ? (uLX[1] - (1 << 16)) : uLX[1];
    }

    if (   pu.inter_pred_idc == PRED_BI
        && X == 1
        && TwoVersionsOfCurrDecPicFlag
        && pu.w == 8
        && pu.h == 8)
    {
        bool noIntegerMvFlag =
            !(((pu.MvLX[0][0] & 0x3) == 0 && (pu.MvLX[0][1] & 0x3) == 0)
                || ((pu.MvLX[1][0] & 0x3) == 0 && (pu.MvLX[1][1] & 0x3) == 0));
        bool identicalMvs = EQ(pu.MvLX[1], pu.MvLX[0])
            && slice.L0[pu.ref_idx_l0].POC == slice.L1[pu.ref_idx_l1].POC;

        if (noIntegerMvFlag && !identicalMvs)
            pu.inter_pred_idc = PRED_L0;
    }
}

bool Info::decodeMvLXCol(Bs16u xPb, Bs16u yPb, Bs16u nPbW, Bs16u nPbH, Bs16u X, Bs16u refIdxLX, Bs16s (&mvLXCol)[2])
{
    //8.5.3.2.8
    if (!ColPicSlices || !m_cSlice->temporal_mvp_enabled_flag)
        throw InvalidSyntax();

    Slice* ColPic;
    CU* colCU;
    PU* colPb;
    Bs16u ColTs, ColRs, refIdxCol;
    Bs16s mvCol[2];
    auto& sps = *m_cSlice->sps;
    RefPic *listCol, *LX = (X ? m_cSlice->L1 : m_cSlice->L0);
    bool availableFlagLXCol = false;
    Bs16s xCol = xPb + nPbW, xColPb;
    Bs16s yCol = yPb + nPbH, yColPb;
    bool  bCentral =
        !(   (yPb >> CtbLog2SizeY) == (yCol >> CtbLog2SizeY)
          && yCol < sps.pic_height_in_luma_samples
          && xCol < sps.pic_width_in_luma_samples);

l_start:
    if (bCentral)
    {
        xCol = xPb + (nPbW >> 1);
        yCol = yPb + (nPbH >> 1);
    }

    xColPb = ((xCol >> 4) << 4);
    yColPb = ((yCol >> 4) << 4);

    ColRs = (yColPb >> CtbLog2SizeY) * PicWidthInCtbsY + (xColPb >> CtbLog2SizeY);

    for (Bs32s i = 0;; i++)
    {
        ColPic = ColPicSlices[i];
        ColTs  = ColPic->pps->CtbAddrRsToTs[ColRs];

        if (   ColTs >= ColPic->ctu->CtbAddrInTs
            && ColTs <  (ColPic->ctu->CtbAddrInTs + ColPic->NumCTU))
            break;
        if (i + 1 >= NumColSlices)
            throw InvalidSyntax();
    }

    if (ColPic->Split)
    {
        colCU = 0;
        auto pCTU = ColPic->ctu;

        while (pCTU && pCTU->CtbAddrInTs != ColTs)
            pCTU = pCTU->Next;

        if (pCTU)
            colCU = pCTU->Cu;
    }
    else
    {
        colCU = ColPic->ctu[ColTs - ColPic->ctu[0].CtbAddrInTs].Cu;
    }
    if (!colCU)
        throw InvalidSyntax();

    colCU = GetUnit(*colCU, xColPb, yColPb);
    if (!colCU)
        throw InvalidSyntax();
    if (!colCU->Pu)
        goto l_end;

    colPb = GetUnit(*colCU->Pu, xColPb, yColPb);
    if (!colPb)
        throw InvalidSyntax();

    if (PredFlagLX(*colPb, 0) == 0)
    {
        CP(mvCol, colPb->MvLX[1]);
        refIdxCol = colPb->ref_idx_l1;
        listCol   = ColPic->L1;
    }
    else if (PredFlagLX(*colPb, 1) == 0)
    {
        CP(mvCol, colPb->MvLX[0]);
        refIdxCol = colPb->ref_idx_l0;
        listCol   = ColPic->L0;
    }
    else if (NoBackwardPredFlag)
    {
        CP(mvCol, colPb->MvLX[X]);
        refIdxCol = X ? colPb->ref_idx_l1 : colPb->ref_idx_l0;
        listCol   = X ? ColPic->L1 : ColPic->L0;
    }
    else
    {
        CP(mvCol, colPb->MvLX[!m_cSlice->collocated_from_l0_flag]);
        refIdxCol = m_cSlice->collocated_from_l0_flag ? colPb->ref_idx_l0 : colPb->ref_idx_l1;
        listCol   = m_cSlice->collocated_from_l0_flag ? ColPic->L0 : ColPic->L1;
    }

    if (LX[refIdxLX].long_term != listCol[refIdxCol].long_term)
        goto l_end;

    availableFlagLXCol = true;

    {
        Bs32s colPocDiff = ColPic->POC - listCol[refIdxCol].POC;
        Bs32s currPocDiff = m_cSlice->POC - LX[refIdxLX].POC;

        if (LX[refIdxLX].long_term || colPocDiff == currPocDiff)
        {
            mvLXCol[0] = mvCol[0];
            mvLXCol[1] = mvCol[1];
            return true;
        }

        Bs32s td = Clip3(-128, 127, colPocDiff);
        Bs32s tb = Clip3(-128, 127, currPocDiff);
        Bs32s tx = (16384 + (BS_ABS(td) >> 1)) / td;
        Bs32s distScaleFactor = Clip3(-4096, 4095, (tb * tx + 32) >> 6);

        mvLXCol[0] = Clip3(-32768, 32767, Sign(distScaleFactor * mvCol[0]) * ((abs(distScaleFactor * mvCol[0]) + 127) >> 8));
        mvLXCol[1] = Clip3(-32768, 32767, Sign(distScaleFactor * mvCol[1]) * ((abs(distScaleFactor * mvCol[1]) + 127) >> 8));
    }

l_end:
    if (!availableFlagLXCol && !bCentral)
    {
        bCentral = true;
        goto l_start;
    }

    return availableFlagLXCol;
}

};
