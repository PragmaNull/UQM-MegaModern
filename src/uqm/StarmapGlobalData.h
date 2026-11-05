//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "gendef.h"
#include "resinst.h"
#include "hyper.h" // JMS: For ARILOU_HOME_X and Y
#include "planets/planets.h"
#include "planets/elemdata.h"


// JSD Changed the way RAINBOW#_DEFINED works
inline constexpr auto VortexScale {20};

static constexpr STAR_DESC StarmapArray[] =
	{
		//                           postfix name index (like 'Normae')
		//                         prefix name index (like 'Alpha')   |
		//                                        alien presence  |   |
		//                                     owner (unused)  |  |   |
		//   x,    y              star type   colour       |   |  |   |
		{{5007, 35}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 6, 74},
		{{708, 41}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 7, 91},
		{{4714, 78}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 7, 74},
		{{2187, 83}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 0, 126},
		{{2814, 89}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 82},
		{{4244, 91}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 0, 125},
		{{5652, 98}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 0, 124},
		{{2939, 116}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 3, 82},
		{{2771, 146}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 82},
		{{5313, 150}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 6, 73},
		{{265, 156}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 5, 92},
		{{4529, 169}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 8, 74},
		{{4911, 180}, MakeStar(GIANT_STAR, ORANGE_BODY, -1), 0, 1, 74},
		{{4747, 221}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 4, 74},
		{{9708, 250}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 0, 112},
		{{4861, 262}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 2, 74},
		{{2908, 269}, MakeStar(DWARF_STAR, BLUE_BODY, -1), SHOFIXTI_DEFINED, 4, 82},
		{{1855, 270}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 5, 81},
		{{7958, 270}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 2, 8},
		{{5160, 280}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 4, 73},
		{{570, 289}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 4, 92},
		{{4923, 294}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), YEHAT_DEFINED, 3, 74},
		{{2820, 301}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 5, 82},
		{{7934, 318}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 8},
		{{8062, 318}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 8},
		{{1116, 334}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 6, 91},
		{{803, 337}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 3, 91},
		{{1787, 338}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 6, 81},
		{{877, 340}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 4, 91},
		{{5338, 355}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 5, 73},
		{{5039, 373}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 1, 73},
		{{843, 380}, MakeStar(GIANT_STAR, ORANGE_BODY, -1), 0, 1, 91},
		{{4872, 408}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 5, 74},
		{{1740, 423}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 7, 81},
		{{4596, 429}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 9, 74},
		{{843, 431}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 2, 91},
		{{2156, 440}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 4, 81},
		{{2004, 441}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 81},
		{{530, 442}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 2, 92},
		{{958, 468}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 5, 91},
		{{2058, 475}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 2, 81},
		{{304, 477}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 1, 92},
		{{522, 525}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), PKUNK_DEFINED, 3, 92},
		{{2100, 554}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 3, 81},
		{{134, 565}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 6, 92},
		{{6858, 577}, MakeStar(DWARF_STAR, GREEN_BODY, -1), MYCON_TRAP_DEFINED, 0, 123},
		{{5014, 584}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 73},
		{{5256, 608}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 3, 73},
		{{2411, 718}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 1, 9},
		{{2589, 741}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 9},
		{{675, 742}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 8, 91},
		{{9292, 750}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 4, 5},
		{{1463, 779}, MakeStar(GIANT_STAR, RED_BODY, -1), 0, 6, 80},
		{{3089, 782}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 4, 9},
		{{2854, 787}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 9},
		{{3333, 801}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 5, 9},
		{{9237, 821}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 5, 5},
		{{9339, 843}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 1, 5},
		{{242, 857}, MakeStar(GIANT_STAR, ORANGE_BODY, -1), 0, 3, 90},
		{{1515, 866}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 5, 80},
		{{4770, 895}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 5, 75},
		{{1412, 905}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 2, 80},
		{{4681, 916}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), RAINBOW0_DEFINED, 6, 75},
		{{9333, 937}, MakeStar(SUPER_GIANT_STAR, YELLOW_BODY, -1), MELNORME0_DEFINED, 2, 5},
		{{9419, 942}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 3, 5},
		{{230, 952}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 2, 90},
		{{146, 955}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 1, 90},
		{{4873, 968}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 4, 75},
		{{1559, 993}, MakeStar(SUPER_GIANT_STAR, RED_BODY, -1), MELNORME1_DEFINED, 1, 80},
		{{1895, 1041}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 93},
		{{4337, 1066}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 75},
		{{3732, 1067}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 0, 122},
		{{1579, 1115}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 3, 80},
		{{4875, 1145}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 3, 75},
		{{4604, 1187}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 75},
		{{5812, 1208}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 4, 72},
		{{1312, 1260}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 4, 80},
		{{1916, 1270}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 1, 93},
		{{6562, 1270}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 0, 121},
		{{416, 1301}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 0, 120},
		{{3958, 1354}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 2, 89},
		{{4000, 1363}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 1, 89},
		{{1752, 1450}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), SOL_DEFINED, 0, 129},
		{{2187, 1500}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 0, 127},
		{{1806, 1507}, MakeStar(GIANT_STAR, WHITE_BODY, -1), 0, 0, 128},
		{{5708, 1520}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 5, 72},
		{{9469, 1548}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 2, 6},
		{{4333, 1562}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 88},
		{{6041, 1562}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 72},
		{{9375, 1583}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 6},
		{{2881, 1614}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 2, 96},
		{{6083, 1625}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 3, 72},
		{{4250, 1645}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 1, 88},
		{{650, 1646}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 7, 85},
		{{9477, 1670}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 3, 6},
		{{2840, 1676}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 96},
		{{9541, 1687}, MakeStar(GIANT_STAR, RED_BODY, -1), 0, 4, 6},
		{{7395, 1687}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 3, 69},
		{{4333, 1687}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), VUX_DEFINED, 2, 88},
		{{9559, 1735}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 5, 6},
		{{736, 1737}, MakeStar(GIANT_STAR, BLUE_BODY, -1), 0, 6, 85},
		{{1601, 1746}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 1, 94},
		{{7395, 1750}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 69},
		{{951, 1770}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 1, 85},
		{{1666, 1812}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 2, 94},
		{{7187, 1833}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 2, 69},
		{{705, 1838}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 5, 85},
		{{1140, 1847}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 2, 85},
		{{6467, 1878}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 71},
		{{2791, 1895}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 96},
		{{6500, 1916}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 2, 71},
		{{5458, 1916}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 0, 119},
		{{1048, 1919}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 3, 85},
		{{3678, 1926}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 2, 99},
		{{3345, 1931}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), START_COLONY_DEFINED, 0, 98},
		{{8187, 1937}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 4, 7},
		{{3352, 1940}, MakeStar(SUPER_GIANT_STAR, WHITE_BODY, -1), MELNORME2_DEFINED, 0, 97},
		{{977, 1953}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 4, 85},
		{{4221, 1986}, MakeStar(DWARF_STAR, BLUE_BODY, -1), MAIDENS_DEFINED, 1, 100},
		{{4500, 2000}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 100},
		{{6833, 2000}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 70},
		{{8163, 2009}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 3, 7},
		{{8080, 2011}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 7},
		{{6036, 2035}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 4, 71},
		{{6479, 2062}, MakeStar(DWARF_STAR, GREEN_BODY, -1), EGG_CASE1_DEFINED, 3, 71},
		{{2104, 2083}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), ZOQ_SCOUT_DEFINED, 0, 118},
		{{8062, 2083}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 1, 7},
		{{270, 2187}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 79},
		{{6500, 2208}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 6, 71},
		{{6291, 2208}, MakeStar(DWARF_STAR, GREEN_BODY, -1), MYCON_DEFINED, 5, 71},
		{{125, 2229}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 2, 79},
		{{312, 2250}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 3, 79},
		{{3884, 2262}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 3, 99},
		{{742, 2268}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), CHMMR_DEFINED, 0, 117},
		{{2306, 2285}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 1, 95},
		{{2402, 2309}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 95},
		{{6395, 2312}, MakeStar(DWARF_STAR, GREEN_BODY, -1), SUN_DEVICE_DEFINED, 2, 12},
		{{8875, 2312}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 9, 61},
		{{3551, 2320}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 1, 99},
		{{6208, 2333}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 1, 12},
		{{3354, 2354}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 4, 99},
		{{9909, 2359}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 0, 111},
		{{2298, 2385}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 95},
		{{7020, 2395}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 2, 70},
		{{9038, 2407}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 5, 61},
		{{9375, 2416}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 8, 61},
		{{6500, 2458}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 6, 12},
		{{217, 2509}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 4, 78},
		{{3641, 2512}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 2, 86},
		{{5625, 2520}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 14},
		{{3713, 2537}, MakeStar(DWARF_STAR, GREEN_BODY, -1), ORZ_DEFINED, 3, 86},
		{{3587, 2566}, MakeStar(DWARF_STAR, GREEN_BODY, -1), ANDROSYNTH_DEFINED, 7, 86},
		{{9291, 2583}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 61},
		{{3654, 2587}, MakeStar(SUPER_GIANT_STAR, GREEN_BODY, -1), MELNORME3_DEFINED, 1, 86},
		{{3721, 2619}, MakeStar(DWARF_STAR, GREEN_BODY, -1), TAALO_PROTECTOR_DEFINED, 4, 86},
		{{5791, 2625}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 14},
		{{6416, 2625}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 5, 12},
		{{6008, 2631}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), EGG_CASE0_DEFINED, 2, 14},
		{{3608, 2637}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 6, 86},
		{{3499, 2648}, MakeStar(DWARF_STAR, BLUE_BODY, -1), EXCAVATION_SITE_DEFINED, 1, 87},
		{{9479, 2666}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 2, 61},
		{{3668, 2666}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 5, 86},
		{{229, 2666}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 78},
		{{8895, 2687}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 7, 61},
		{{138, 2696}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 5, 78},
		{{5375, 2729}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 0, 116},
		{{6354, 2729}, MakeStar(DWARF_STAR, WHITE_BODY, -1), EGG_CASE2_DEFINED, 3, 12},
		{{6458, 2750}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 4, 12},
		{{2458, 2750}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 6, 106},
		{{351, 2758}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 2, 78},
		{{7083, 2770}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 70},
		{{3759, 2778}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 4, 87},
		{{9333, 2791}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 61},
		{{3400, 2804}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 2, 87},
		{{9469, 2806}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), DRUUGE_DEFINED, 6, 61},
		{{3619, 2830}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 3, 87},
		{{2208, 2854}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 7, 106},
		{{9250, 2854}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 4, 61},
		{{672, 2863}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 7, 78},
		{{167, 2875}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 6, 78},
		{{4030, 2887}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 105},
		{{384, 2900}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 3, 78},
		{{2727, 2951}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 5, 106},
		{{4645, 2958}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 105},
		{{5625, 2958}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 2, 13},
		{{8270, 2958}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 3, 66},
		{{8291, 2979}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 66},
		{{6020, 2979}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), RAINBOW1_DEFINED, 3, 13},
		{{6562, 3020}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 4, 70},
		{{2011, 3043}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 8, 106},
		{{8125, 3083}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 1, 66},
		{{2354, 3166}, MakeStar(GIANT_STAR, YELLOW_BODY, -1), 0, 4, 106},
		{{3833, 3187}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 1, 105},
		{{5812, 3208}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 1, 13},
		{{9000, 3250}, MakeStar(DWARF_STAR, RED_BODY, -1), ALGOLITES_DEFINED, 0, 113},
		{{291, 3250}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 84},
		{{501, 3259}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 2, 84},
		{{791, 3270}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 84},
		{{2354, 3291}, MakeStar(SUPER_GIANT_STAR, RED_BODY, -1), MELNORME4_DEFINED, 1, 106},
		{{1104, 3333}, MakeStar(DWARF_STAR, WHITE_BODY, -1), MOTHER_ARK_DEFINED, 4, 84},
		{{2687, 3333}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 3, 106},
		{{3187, 3375}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 2, 107},
		{{1758, 3418}, MakeStar(DWARF_STAR, BLUE_BODY, -1), SPATHI_MONUMENT_DEFINED, 2, 108},
		{{2520, 3437}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 106},
		{{8437, 3458}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 64},
		{{8770, 3458}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 4, 64},
		{{3000, 3500}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 107},
		{{149, 3519}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 5, 76},
		{{8791, 3541}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 64},
		{{2148, 3551}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 2, 109},
		{{7375, 3562}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 0, 115},
		{{9312, 3562}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 2, 63},
		{{9599, 3583}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 4, 63},
		{{9375, 3604}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 3, 63},
		{{90, 3614}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 6, 76},
		{{2770, 3625}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 4, 107},
		{{8708, 3625}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 3, 64},
		{{267, 3645}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 3, 76},
		{{1604, 3645}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 1, 108},
		{{2274, 3663}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 1, 109},
		{{229, 3666}, MakeStar(DWARF_STAR, GREEN_BODY, -1), ILWRATH_DEFINED, 1, 76},
		{{3083, 3674}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 107},
		{{2416, 3687}, MakeStar(GIANT_STAR, ORANGE_BODY, -1), SPATHI_DEFINED, 5, 109},
		{{9333, 3708}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 63},
		{{2250, 3708}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 3, 109},
		{{288, 3735}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 2, 76},
		{{2354, 3741}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 4, 109},
		{{2583, 3750}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 6, 109},
		{{4125, 3770}, MakeStar(DWARF_STAR, BLUE_BODY, -1), SYREEN_DEFINED, 0, 114},
		{{166, 3770}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 4, 76},
		{{6270, 3833}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 4, 10},
		{{2145, 3916}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 110},
		{{6125, 3937}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 3, 10},
		{{6291, 3937}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 9, 10},
		{{5937, 3937}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), SHIP_VAULT_DEFINED, 5, 10},
		{{2479, 3958}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 7, 109},
		{{926, 3972}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 83},
		{{2062, 3991}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 110},
		{{5895, 4020}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 8, 10},
		{{285, 4020}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 1, 77},
		{{6062, 4041}, MakeStar(GIANT_STAR, YELLOW_BODY, -1), 0, 1, 10},
		{{2875, 4041}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 2, 20},
		{{8645, 4062}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 65},
		{{860, 4065}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 83},
		{{5958, 4083}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 2, 10},
		{{3038, 4083}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 3, 20},
		{{291, 4104}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 77},
		{{6166, 4125}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 6, 10},
		{{9812, 4145}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 3, 62},
		{{8520, 4166}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 1, 65},
		{{9573, 4182}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 62},
		{{500, 4187}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 3, 77},
		{{2145, 4208}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 110},
		{{6208, 4229}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 7, 10},
		{{2812, 4250}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 7, 20},
		{{2937, 4306}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 4, 20},
		{{9416, 4395}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 62},
		{{2875, 4479}, MakeStar(GIANT_STAR, WHITE_BODY, -1), 0, 1, 20},
		{{250, 4583}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 5, 26},
		{{7250, 4583}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 4, 68},
		{{479, 4583}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 26},
		{{5708, 4604}, MakeStar(DWARF_STAR, RED_BODY, -1), URQUAN0_DEFINED, 0, 104},
		{{479, 4645}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 2, 26},
		{{2895, 4687}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 6, 20},
		{{2708, 4708}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 5, 20},
		{{562, 4708}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 3, 26},
		{{416, 4717}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 4, 26},
		{{5094, 4931}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 4, 11},
		{{9000, 5000}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 67},
		{{8958, 5000}, MakeStar(GIANT_STAR, BLUE_BODY, -1), 0, 1, 67},
		{{5006, 5011}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), URQUAN1_DEFINED, 3, 11},
		{{7312, 5062}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 1, 68},
		{{3679, 5068}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), ZOQ_COLONY0_DEFINED, 3, 17},
		{{9062, 5083}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 3, 67},
		{{7416, 5083}, MakeStar(DWARF_STAR, GREEN_BODY, -1), RAINBOW2_DEFINED, 3, 68},
		{{5155, 5122}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 2, 11},
		{{3875, 5145}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 4, 17},
		{{4937, 5145}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 11},
		{{2979, 5166}, MakeStar(GIANT_STAR, ORANGE_BODY, -1), 0, 1, 15},
		{{3035, 5178}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 15},
		{{3994, 5185}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 5, 17},
		{{3541, 5187}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 17},
		{{5977, 5246}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 102},
		{{3770, 5250}, MakeStar(DWARF_STAR, GREEN_BODY, -1), ZOQ_COLONY1_DEFINED, 2, 17},
		{{1520, 5261}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 55},
		{{1613, 5279}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 4, 55},
		{{7020, 5291}, MakeStar(DWARF_STAR, BLUE_BODY, -1), KOHRAH0_DEFINED, 2, 68},
		{{1416, 5315}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 2, 55},
		{{2993, 5318}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 15},
		{{1425, 5404}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 55},
		{{1854, 5416}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 5, 55},
		{{3625, 5437}, MakeStar(GIANT_STAR, GREEN_BODY, -1), 0, 1, 16},
		{{3416, 5437}, MakeStar(DWARF_STAR, BLUE_BODY, -1), ZOQ_COLONY2_DEFINED, 2, 16},
		{{4000, 5437}, MakeStar(DWARF_STAR, GREEN_BODY, -1), ZOQFOT_DEFINED, 1, 18},
		{{6270, 5479}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 102},
		{{3583, 5479}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 3, 16},
		{{4083, 5513}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 18},
		{{2159, 5614}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 6, 55},
		{{3937, 5625}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), ZOQ_COLONY3_DEFINED, 2, 18},
		{{6014, 5632}, MakeStar(GIANT_STAR, BLUE_BODY, -1), 0, 1, 21},
		{{250, 5687}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 25},
		{{3625, 5750}, MakeStar(GIANT_STAR, RED_BODY, -1), 0, 2, 19},
		{{371, 5772}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 25},
		{{6107, 5785}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 2, 21},
		{{9645, 5791}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), BURVIXESE_DEFINED, 0, 130},
		{{1545, 5818}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 5, 54},
		{{3750, 5833}, MakeStar(GIANT_STAR, GREEN_BODY, -1), 0, 1, 19},
		{{6301, 5875}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 5, 21},
		{{1923, 5878}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 3, 54},
		{{4625, 5895}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 2, 131},
		{{152, 5900}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 25},
		{{5437, 5916}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 4, 33},
		{{1714, 5926}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 4, 54},
		{{6200, 5935}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), SAMATRA_DEFINED, 4, 21},
		{{6429, 5958}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 7, 21},
		{{4729, 5958}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 4, 131},
		{{1978, 5968}, MakeStar(DWARF_STAR, GREEN_BODY, -1), TALKING_PET_DEFINED, 2, 54},
		{{395, 5979}, MakeStar(GIANT_STAR, GREEN_BODY, -1), 0, 1, 22},
		{{563, 5980}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 5, 22},
		{{456, 5989}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 6, 22},
		{{4625, 6000}, MakeStar(DWARF_STAR, BLUE_BODY, -1), URQUAN2_DEFINED, 1, 131},
		{{6166, 6000}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 3, 21},
		{{6496, 6032}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 6, 21},
		{{2228, 6038}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 12, 54},
		{{4583, 6041}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 3, 131},
		{{1558, 6058}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 6, 54},
		{{1902, 6065}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 54},
		{{2159, 6073}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 11, 54},
		{{365, 6093}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 2, 22},
		{{541, 6145}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 4, 22},
		{{2200, 6176}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 10, 54},
		{{729, 6208}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 3, 23},
		{{5250, 6229}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 3, 33},
		{{8166, 6250}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 40},
		{{6215, 6255}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 8, 21},
		{{437, 6270}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 3, 22},
		{{5583, 6291}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 1, 33},
		{{1881, 6308}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 8, 54},
		{{1795, 6329}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 7, 54},
		{{2118, 6379}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 9, 54},
		{{750, 6458}, MakeStar(GIANT_STAR, WHITE_BODY, -1), 0, 1, 23},
		{{3716, 6458}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 1, 30},
		{{1360, 6489}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 2, 56},
		{{7333, 6500}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 1, 40},
		{{3770, 6500}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 2, 30},
		{{4500, 6500}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 0, 37},
		{{187, 6520}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 4, 24},
		{{125, 6541}, MakeStar(GIANT_STAR, RED_BODY, -1), 0, 1, 24},
		{{7812, 6562}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 40},
		{{770, 6602}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 23},
		{{5910, 6624}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 29},
		{{208, 6625}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 3, 24},
		{{2604, 6645}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 1, 101},
		{{1578, 6668}, MakeStar(SUPER_GIANT_STAR, GREEN_BODY, -1), MELNORME5_DEFINED, 1, 56},
		{{5479, 6687}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 2, 33},
		{{375, 6716}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 2, 24},
		{{312, 6728}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 5, 24},
		{{6020, 6729}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 2, 29},
		{{5062, 6750}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 10, 28},
		{{4208, 6854}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 1, 31},
		{{5145, 6875}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 9, 28},
		{{4291, 6937}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 31},
		{{5145, 6958}, MakeStar(DWARF_STAR, BLUE_BODY, -1), KOHRAH1_DEFINED, 8, 28},
		{{7208, 7000}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 39},
		{{8625, 7000}, MakeStar(DWARF_STAR, GREEN_BODY, -1), RAINBOW3_DEFINED, 1, 41},
		{{4955, 7034}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 28},
		{{4895, 7041}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 3, 28},
		{{4971, 7104}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 1, 28},
		{{8666, 7104}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 2, 41},
		{{4854, 7125}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 4, 28},
		{{5083, 7145}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 7, 28},
		{{7360, 7184}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 4, 39},
		{{1020, 7187}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 58},
		{{3875, 7187}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 1, 32},
		{{4879, 7201}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 5, 28},
		{{4958, 7229}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 6, 28},
		{{7125, 7250}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 39},
		{{7532, 7258}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 5, 39},
		{{2416, 7291}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 101},
		{{3854, 7291}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 2, 32},
		{{9687, 7333}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 3, 44},
		{{395, 7458}, MakeStar(DWARF_STAR, BLUE_BODY, -1), RAINBOW4_DEFINED, 2, 60},
		{{4895, 7458}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 3, 36},
		{{4645, 7479}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 4, 36},
		{{6940, 7514}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 11, 39},
		{{7443, 7538}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 6, 39},
		{{6479, 7541}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), KOHRAH2_DEFINED, 0, 38},
		{{7208, 7541}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 1, 39},
		{{5791, 7583}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 3, 34},
		{{333, 7625}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 60},
		{{5958, 7645}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 2, 34},
		{{1041, 7708}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 58},
		{{5875, 7729}, MakeStar(SUPER_GIANT_STAR, YELLOW_BODY, -1), MELNORME6_DEFINED, 1, 34},
		{{1125, 7791}, MakeStar(GIANT_STAR, BLUE_BODY, -1), 0, 1, 58},
		{{4979, 7791}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 2, 36},
		{{4958, 7791}, MakeStar(GIANT_STAR, WHITE_BODY, -1), 0, 1, 36},
		{{6889, 7803}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 10, 39},
		{{7200, 7849}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 8, 39},
		{{7395, 7854}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 7, 39},
		{{9437, 7854}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 44},
		{{2836, 7857}, MakeStar(DWARF_STAR, WHITE_BODY, -1), RAINBOW5_DEFINED, 5, 53},
		{{5375, 7875}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 1, 35},
		{{6187, 7875}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 7, 35},
		{{6041, 7916}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 5, 35},
		{{5979, 7979}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 4, 35},
		{{7083, 7993}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 9, 39},
		{{3270, 8000}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 8, 53},
		{{6104, 8000}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 6, 35},
		{{687, 8000}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 59},
		{{562, 8000}, MakeStar(GIANT_STAR, GREEN_BODY, -1), URQUAN_WRECK_DEFINED, 1, 59},
		{{5645, 8020}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 2, 35},
		{{1395, 8041}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 4, 58},
		{{8229, 8041}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 43},
		{{2518, 8056}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 3, 53},
		{{5875, 8062}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 3, 35},
		{{8416, 8083}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 43},
		{{9000, 8229}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 44},
		{{3562, 8250}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 9, 53},
		{{5437, 8270}, MakeStar(DWARF_STAR, RED_BODY, -1), RAINBOW6_DEFINED, 5, 48},
		{{1520, 8333}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 5, 58},
		{{2771, 8351}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 53},
		{{2535, 8358}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), THRADD_DEFINED, 4, 53},
		{{3151, 8390}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 7, 53},
		{{2362, 8395}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 11, 53},
		{{2822, 8395}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 53},
		{{5500, 8395}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 48},
		{{2536, 8504}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 4, 2},
		{{2582, 8507}, MakeStar(SUPER_GIANT_STAR, YELLOW_BODY, -1), MELNORME7_DEFINED, 1, 2},
		{{8625, 8562}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 6, 3},
		{{4375, 8562}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 0, 50},
		{{2593, 8569}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 2},
		{{2562, 8572}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 3, 2},
		{{8492, 8578}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 7, 3},
		{{1125, 8583}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 6, 58},
		{{8073, 8588}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 1, 46},
		{{8560, 8638}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 3},
		{{8750, 8645}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 5, 3},
		{{5562, 8645}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 1, 48},
		{{2588, 8653}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 14, 53},
		{{2458, 8666}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 10, 53},
		{{7666, 8666}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), RAINBOW7_DEFINED, 2, 46},
		{{2776, 8673}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), AQUA_HELIX_DEFINED, 6, 53},
		{{8630, 8693}, MakeStar(DWARF_STAR, GREEN_BODY, -1), UTWIG_DEFINED, 2, 3},
		{{2310, 8702}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 12, 53},
		{{437, 8770}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 1, 57},
		{{8534, 8797}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), RAINBOW8_DEFINED, 3, 3},
		{{8588, 8812}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 4, 3},
		{{7187, 8812}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 3, 46},
		{{5475, 8823}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 3, 48},
		{{3050, 8833}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 4, 1},
		{{2831, 8854}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 3, 1},
		{{2300, 8861}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 13, 53},
		{{479, 8875}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 57},
		{{2706, 8910}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 2, 1},
		{{333, 8916}, MakeStar(DWARF_STAR, GREEN_BODY, -1), DESTROYED_STARBASE_DEFINED, 3, 57}, // here
		{{2535, 8917}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 5, 1},
		{{8322, 8934}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 1, 45},
		{{8249, 8958}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 4, 45},
		{{8375, 8958}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 2, 45},
		{{5645, 8979}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 4, 48},
		{{2687, 9000}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 1},
		{{8375, 9041}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 3, 45},
		{{9960, 9042}, MakeStar(GIANT_STAR, WHITE_BODY, -1), RAINBOW9_DEFINED, 0, 42},
		{{7354, 9062}, MakeStar(DWARF_STAR, BLUE_BODY, -1), 0, 1, 47},
		{{7833, 9083}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 6, 47},
		{{2581, 9105}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 6, 1},
		{{7545, 9107}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 3, 47},
		{{7414, 9124}, MakeStar(DWARF_STAR, GREEN_BODY, -1), SUPOX_DEFINED, 2, 47},
		{{8500, 9125}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 5, 45},
		{{104, 9125}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 5, 27},
		{{7889, 9181}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 7, 47},
		{{7791, 9187}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 4, 47},
		{{7791, 9229}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 5, 47},
		{{4812, 9270}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 0, 51},
		{{8500, 9372}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), BOMB_DEFINED, 6, 45},
		{{7255, 9374}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 11, 45},
		{{8458, 9393}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 7, 45},
		{{1000, 9395}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 4, 27},
		{{5711, 9475}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 2, 49},
		{{62, 9479}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 3, 27},
		{{5989, 9496}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 7, 49},
		{{8000, 9505}, MakeStar(DWARF_STAR, WHITE_BODY, -1), 0, 9, 45},
		{{5329, 9538}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 5, 49},
		{{2916, 9541}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 103},
		{{8296, 9548}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 8, 45},
		{{5600, 9552}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 1, 49},
		{{7664, 9589}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 10, 45},
		{{6125, 9604}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 8, 49},
		{{9144, 9686}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 4, 4},
		{{5781, 9711}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 3, 49},
		{{5229, 9729}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 6, 49},
		{{9120, 9741}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 3, 4},
		{{9186, 9741}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 2, 4},
		{{9159, 9745}, MakeStar(SUPER_GIANT_STAR, BLUE_BODY, -1), MELNORME8_DEFINED, 1, 4},
		{{333, 9750}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 0, 0},
		{{9147, 9790}, MakeStar(DWARF_STAR, ORANGE_BODY, -1), 0, 5, 4},
		{{5704, 9795}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), VUX_BEAST_DEFINED, 4, 49},
		{{333, 9812}, MakeStar(DWARF_STAR, GREEN_BODY, -1), SLYLANDRO_DEFINED, 2, 27},
		{{1020, 9937}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 7, 27},
		{{83, 9979}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 6, 27},
		{{1937, 9979}, MakeStar(DWARF_STAR, RED_BODY, -1), 0, 1, 103},
		{{4395, 9979}, MakeStar(DWARF_STAR, GREEN_BODY, -1), 0, 0, 52},

		{{MaxXUniverse << 1, MaxYUniverse << 1}, 0, 0, 0, 0},

		// QuasiSpace locations
		{{(-12 * VortexScale) + 5000, (-21 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 133},
		{{(1 * VortexScale) + 5000, (-20 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 134},
		{{(-16 * VortexScale) + 5000, (-18 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 135},
		{{(8 * VortexScale) + 5000, (-17 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 136},
		{{(3 * VortexScale) + 5000, (-13 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 137},
		{{(-21 * VortexScale) + 5000, (-4 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 138},
		{{(-4 * VortexScale) + 5000, (-4 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 139},
		{{(-12 * VortexScale) + 5000, (-2 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 140},
		{{(-26 * VortexScale) + 5000, (2 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 141},
		{{(-17 * VortexScale) + 5000, (7 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 142},
		{{(10 * VortexScale) + 5000, (7 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 143},
		{{(15 * VortexScale) + 5000, (14 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 144},
		{{(22 * VortexScale) + 5000, (16 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 145},
		{{(-6 * VortexScale) + 5000, (19 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 146},
		{{(10 * VortexScale) + 5000, (20 * VortexScale) + 5000},
		 MakeStar(DWARF_STAR, WHITE_BODY, -1),
		 0,
		 0,
		 147},

		{{ArilouHomeX, ArilouHomeY}, MakeStar(DWARF_STAR, YELLOW_BODY, -1), 0, 0, 148},

		{{MaxXUniverse << 1, MaxYUniverse << 1}, 0, 0, 0, 0},
};

static constexpr GFXPOINT ConstellationsArray[] =
	{
		/* Crateris */
		{6014,			   5632			   },
		{6107,			   5785			   },
		{6107,			   5785			   },
		{6200,			   5935			   },
		{6200,			   5935			   },
		{6166,			   6000			   },
		{6166,			   6000			   },
		{6215,			   6255			   },
		{6200,			   5935			   },
		{6301,			   5875			   },
		{6301,			   5875			   },
		{6429,			   5958			   },
		{6429,			   5958			   },
		{6496,			   6032			   },

		/* Horologii */
		{7208,			   7541			   },
		{7125,			   7250			   },
		{7125,			   7250			   },
		{7208,			   7000			   },
		{7208,			   7000			   },
		{7360,			   7184			   },
		{7360,			   7184			   },
		{7532,			   7258			   },
		{7532,			   7258			   },
		{7443,			   7538			   },
		{7443,			   7538			   },
		{7208,			   7541			   },
		{7208,			   7541			   },
		{6940,			   7514			   },
		{6940,			   7514			   },
		{6889,			   7803			   },
		{6889,			   7803			   },
		{7083,			   7993			   },
		{7083,			   7993			   },
		{7200,			   7849			   },
		{7200,			   7849			   },
		{7395,			   7854			   },
		{7395,			   7854			   },
		{7208,			   7541			   },

		/* Equulei */
		{5910,			   6624			   },
		{6020,			   6729			   },

		/* Maksutov */
		{5977,			   5246			   },
		{6270,			   5479			   },

		/* Crucis */
		{5583,			   6291			   },
		{5479,			   6687			   },
		{5479,			   6687			   },
		{5250,			   6229			   },
		{5250,			   6229			   },
		{5437,			   5916			   },
		{5437,			   5916			   },
		{5583,			   6291			   },

		/* Hydrae */
		{7333,			   6500			   },
		{7812,			   6562			   },
		{7812,			   6562			   },
		{8166,			   6250			   },

		/* Reticuli */
		{7020,			   5291			   },
		{7312,			   5062			   },
		{7312,			   5062			   },
		{7416,			   5083			   },
		{7312,			   5062			   },
		{7250,			   4583			   },

		/* Cancri */
		{5006,			   5011			   },
		{4937,			   5145			   },
		{5006,			   5011			   },
		{5155,			   5122			   },
		{5006,			   5011			   },
		{5094,			   4931			   },
		{5094,			   4931			   },
		{5155,			   5122			   },

		/* Lentilis */
		{4625,			   6000			   },
		{4583,			   6041			   },
		{4625,			   6000			   },
		{4729,			   5958			   },
		{4625,			   6000			   },
		{4625,			   5895			   },
		{4729,			   5958			   },
		{4625,			   5895			   },

		/* Delphini */
		{3716,			   6458			   },
		{3770,			   6500			   },

		/* Cephei */
		{3625,			   5750			   },
		{3750,			   5833			   },

		/* Tucanae */
		{4000,			   5437			   },
		{3937,			   5625			   },
		{3937,			   5625			   },
		{4083,			   5513			   },
		{4083,			   5513			   },
		{4000,			   5437			   },

		/* Carinae */
		{3625,			   5437			   },
		{3583,			   5479			   },
		{3583,			   5479			   },
		{3416,			   5437			   },

		/* Cassiopeiae */
		{3541,			   5187			   },
		{3679,			   5068			   },
		{3679,			   5068			   },
		{3875,			   5145			   },
		{3875,			   5145			   },
		{3770,			   5250			   },
		{3875,			   5145			   },
		{3994,			   5185			   },
		{3541,			   5187			   },
		{3770,			   5250			   },

		/* Doradus */
		{4208,			   6854			   },
		{4291,			   6937			   },

		/* Monocerotis */
		{3875,			   7187			   },
		{3854,			   7291			   },

		/* Chamaeleonis */
		{4971,			   7104			   },
		{4955,			   7034			   },
		{4955,			   7034			   },
		{4895,			   7041			   },
		{4895,			   7041			   },
		{4854,			   7125			   },
		{4854,			   7125			   },
		{4879,			   7201			   },
		{4879,			   7201			   },
		{4958,			   7229			   },
		{4958,			   7229			   },
		{5083,			   7145			   },
		{5083,			   7145			   },
		{5145,			   6958			   },
		{5145,			   6958			   },
		{5145,			   6875			   },
		{5145,			   6875			   },
		{5062,			   6750			   },

		/* Geminorum */
		{4958,			   7791			   },
		{4979,			   7791			   },
		{4979,			   7791			   },
		{4895,			   7458			   },
		{4895,			   7458			   },
		{4645,			   7479			   },
		{4645,			   7479			   },
		{4958,			   7791			   },

		/* Eridani */
		{5875,			   7729			   },
		{5958,			   7645			   },
		{5958,			   7645			   },
		{5791,			   7583			   },
		{5791,			   7583			   },
		{5875,			   7729			   },

		/* Fornacis */
		{5375,			   7875			   },
		{5645,			   8020			   },
		{5645,			   8020			   },
		{5875,			   8062			   },
		{5875,			   8062			   },
		{5979,			   7979			   },
		{5979,			   7979			   },
		{6041,			   7916			   },
		{6041,			   7916			   },
		{6104,			   8000			   },
		{6041,			   7916			   },
		{6187,			   7875			   },

		/* Lipi */
		{5437,			   8270			   },
		{5500,			   8395			   },
		{5500,			   8395			   },
		{5562,			   8645			   },
		{5562,			   8645			   },
		{5475,			   8823			   },
		{5475,			   8823			   },
		{5645,			   8979			   },

		/* Lyncis */
		{5229,			   9729			   },
		{5329,			   9538			   },
		{5329,			   9538			   },
		{5600,			   9552			   },
		{5600,			   9552			   },
		{5704,			   9795			   },
		{5600,			   9552			   },
		{5711,			   9475			   },
		{5704,			   9795			   },
		{5781,			   9711			   },
		{5781,			   9711			   },
		{5711,			   9475			   },
		{5711,			   9475			   },
		{5989,			   9496			   },
		{5989,			   9496			   },
		{6125,			   9604			   },

		/* Hyades */
		{8322,			   8934			   },
		{8375,			   8958			   },
		{8322,			   8934			   },
		{8249,			   8958			   },
		{8249,			   8958			   },
		{8375,			   9041			   },
		{8375,			   8958			   },
		{8375,			   9041			   },
		{8375,			   9041			   },
		{8500,			   9125			   },
		{8500,			   9125			   },
		{8500,			   9372			   },
		{8500,			   9372			   },
		{8458,			   9393			   },
		{8458,			   9393			   },
		{8296,			   9548			   },
		{8296,			   9548			   },
		{8000,			   9505			   },
		{8000,			   9505			   },
		{7664,			   9589			   },
		{7664,			   9589			   },
		{7255,			   9374			   },

		/* Librae */
		{7354,			   9062			   },
		{7414,			   9124			   },
		{7414,			   9124			   },
		{7545,			   9107			   },
		{7545,			   9107			   },
		{7791,			   9187			   },
		{7791,			   9187			   },
		{7833,			   9083			   },
		{7833,			   9083			   },
		{7889,			   9181			   },
		{7889,			   9181			   },
		{7791,			   9229			   },
		{7791,			   9229			   },
		{7791,			   9187			   },

		/* Leporis */
		{7187,			   8812			   },
		{7666,			   8666			   },
		{7666,			   8666			   },
		{8073,			   8588			   },

		/* Aquarii */
		{8492,			   8578			   },
		{8560,			   8638			   },
		{8560,			   8638			   },
		{8625,			   8562			   },
		{8625,			   8562			   },
		{8750,			   8645			   },
		{8750,			   8645			   },
		{8630,			   8693			   },
		{8630,			   8693			   },
		{8588,			   8812			   },
		{8588,			   8812			   },
		{8534,			   8797			   },
		{8534,			   8797			   },
		{8560,			   8638			   },

		/* Aquilae */
		{9159,			   9745			   },
		{9144,			   9686			   },
		{9159,			   9745			   },
		{9120,			   9741			   },
		{9159,			   9745			   },
		{9147,			   9790			   },
		{9159,			   9745			   },
		{9186,			   9741			   },

		/* Lacertae */
		{8229,			   8041			   },
		{8416,			   8083			   },

		/* Leonis */
		{9000,			   8229			   },
		{9437,			   7854			   },
		{9437,			   7854			   },
		{9687,			   7333			   },

		/* Andromedae */
		{8666,			   7104			   },
		{8625,			   7000			   },

		/* Pyxidis */
		{8958,			   5000			   },
		{9000,			   5000			   },
		{9000,			   5000			   },
		{9062,			   5083			   },

		/* Hyginus */
		{8520,			   4166			   },
		{8645,			   4062			   },

		/* Phoenicis */
		{9416,			   4395			   },
		{9573,			   4182			   },
		{9573,			   4182			   },
		{9812,			   4145			   },

		/* Camelopardalis */
		{6208,			   4229			   },
		{6166,			   4125			   },
		{6166,			   4125			   },
		{6062,			   4041			   },
		{6062,			   4041			   },
		{5937,			   3937			   },
		{5937,			   3937			   },
		{5895,			   4020			   },
		{5895,			   4020			   },
		{5958,			   4083			   },
		{5958,			   4083			   },
		{6062,			   4041			   },
		{6062,			   4041			   },
		{6125,			   3937			   },
		{6125,			   3937			   },
		{6270,			   3833			   },
		{6270,			   3833			   },
		{6291,			   3937			   },

		/* Pictoris */
		{9333,			   3708			   },
		{9312,			   3562			   },
		{9312,			   3562			   },
		{9375,			   3604			   },
		{9375,			   3604			   },
		{9333,			   3708			   },
		{9375,			   3604			   },
		{9599,			   3583			   },

		/* Piscium */
		{8437,			   3458			   },
		{8708,			   3625			   },
		{8708,			   3625			   },
		{8770,			   3458			   },
		{8770,			   3458			   },
		{8791,			   3541			   },
		{8791,			   3541			   },
		{8708,			   3625			   },

		/* Puppis */
		{8125,			   3083			   },
		{8270,			   2958			   },
		{8270,			   2958			   },
		{8291,			   2979			   },
		{8291,			   2979			   },
		{8125,			   3083			   },

		/* Persei */
		{8875,			   2312			   },
		{9038,			   2407			   },
		{9038,			   2407			   },
		{8895,			   2687			   },
		{9038,			   2407			   },
		{9291,			   2583			   },
		{9291,			   2583			   },
		{9375,			   2416			   },
		{9291,			   2583			   },
		{9479,			   2666			   },
		{9479,			   2666			   },
		{9469,			   2806			   },
		{9469,			   2806			   },
		{9333,			   2791			   },
		{9333,			   2791			   },
		{9250,			   2854			   },
		{9333,			   2791			   },
		{9291,			   2583			   },

		/* Aurigae */
		{8062,			   2083			   },
		{8080,			   2011			   },
		{8080,			   2011			   },
		{8163,			   2009			   },
		{8163,			   2009			   },
		{8187,			   1937			   },

		/* Arietis */
		{9559,			   1735			   },
		{9541,			   1687			   },
		{9541,			   1687			   },
		{9477,			   1670			   },
		{9477,			   1670			   },
		{9469,			   1548			   },
		{9469,			   1548			   },
		{9375,			   1583			   },

		/* Arae */
		{9419,			   942			  },
		{9333,			   937			  },
		{9333,			   937			  },
		{9339,			   843			  },
		{9339,			   843			  },
		{9419,			   942			  },
		{9339,			   843			  },
		{9292,			   750			  },
		{9292,			   750			  },
		{9237,			   821			  },
		{9237,			   821			  },
		{9339,			   843			  },

		/* Trianguli */
		{7934,			   318			  },
		{7958,			   270			  },
		{7958,			   270			  },
		{8061,			   318			  },
		{8061,			   318			  },
		{7934,			   318			  },

		/* Arianni */
		{7187,			   1833			   },
		{7395,			   1750			   },
		{7395,			   1750			   },
		{7395,			   1687			   },
		{7395,			   1687			   },
		{7187,			   1833			   },

		/* Sagittarii */
		{6562,			   3020			   },
		{7083,			   2770			   },
		{7083,			   2770			   },
		{7020,			   2395			   },
		{7020,			   2395			   },
		{6833,			   2000			   },

		/* Scorpii */
		{6036,			   2035			   },
		{6291,			   2208			   },
		{6291,			   2208			   },
		{6500,			   2208			   },
		{6500,			   2208			   },
		{6479,			   2062			   },
		{6479,			   2062			   },
		{6500,			   1916			   },
		{6500,			   1916			   },
		{6467,			   1878			   },

		/* Sculptoris */
		{6041,			   1562			   },
		{6083,			   1625			   },
		{6041,			   1562			   },
		{5708,			   1520			   },
		{6041,			   1562			   },
		{5812,			   1208			   },

		/* Brahe */
		{6208,			   2333			   },
		{6395,			   2312			   },
		{6395,			   2312			   },
		{6500,			   2458			   },
		{6500,			   2458			   },
		{6416,			   2625			   },
		{6416,			   2625			   },
		{6354,			   2729			   },
		{6354,			   2729			   },
		{6458,			   2750			   },
		{6458,			   2750			   },
		{6416,			   2625			   },

		/* Copernicus */
		{5625,			   2520			   },
		{5791,			   2625			   },
		{5791,			   2625			   },
		{6008,			   2631			   },

		/* Kepler */
		{5625,			   2958			   },
		{5812,			   3208			   },
		{5812,			   3208			   },
		{6020,			   2979			   },
		{6020,			   2979			   },
		{5625,			   2958			   },

		/* Scuti */
		{5014,			   584			  },
		{5039,			   373			  },
		{5039,			   373			  },
		{5160,			   280			  },
		{5160,			   280			  },
		{5313,			   150			  },
		{5313,			   150			  },
		{5338,			   355			  },
		{5338,			   355			  },
		{5256,			   608			  },

		/* Serpentis */
		{4596,			   429			  },
		{4529,			   169			  },
		{4529,			   169			  },
		{4714,			   78				 },
		{4714,			   78				 },
		{4747,			   221			  },
		{4747,			   221			  },
		{4861,			   262			  },
		{4861,			   262			  },
		{4872,			   408			  },
		{4872,			   408			  },
		{4923,			   294			  },
		{4923,			   294			  },
		{4911,			   180			  },
		{4911,			   180			  },
		{5007,			   35				 },

		/* Sextantis */
		{4337,			   1066			   },
		{4604,			   1187			   },
		{4604,			   1187			   },
		{4681,			   916			  },
		{4681,			   916			  },
		{4770,			   895			  },
		{4770,			   895			  },
		{4873,			   968			  },
		{4873,			   968			  },
		{4875,			   1145			   },

		/* Luyten */
		{4333,			   1562			   },
		{4333,			   1687			   },
		{4333,			   1687			   },
		{4250,			   1645			   },
		{4250,			   1645			   },
		{4333,			   1562			   },

		/* Indi */
		{3958,			   1354			   },
		{4000,			   1363			   },

		/* Cerenkov */
		{4221,			   1986			   },
		{4500,			   2000			   },

		/* Mira */
		{3354,			   2354			   },
		{3551,			   2320			   },
		{3551,			   2320			   },
		{3884,			   2262			   },
		{3884,			   2262			   },
		{3678,			   1926			   },
		{3678,			   1926			   },
		{3551,			   2320			   },

		/* Vulpeculae */
		{3654,			   2587			   },
		{3641,			   2512			   },
		{3654,			   2587			   },
		{3713,			   2537			   },
		{3654,			   2587			   },
		{3721,			   2619			   },
		{3654,			   2587			   },
		{3668,			   2666			   },
		{3654,			   2587			   },
		{3608,			   2637			   },
		{3654,			   2587			   },
		{3587,			   2566			   },

		/* Lalande */
		{3499,			   2648			   },
		{3400,			   2804			   },
		{3400,			   2804			   },
		{3619,			   2830			   },
		{3619,			   2830			   },
		{3499,			   2648			   },
		{3619,			   2830			   },
		{3759,			   2778			   },

		/* Mensae */
		{3833,			   3187			   },
		{4030,			   2887			   },
		{4030,			   2887			   },
		{4645,			   2958			   },

		/* Saurus */
		{2306,			   2285			   },
		{2402,			   2309			   },
		{2402,			   2309			   },
		{2298,			   2385			   },
		{2298,			   2385			   },
		{2306,			   2285			   },

		/* Raynet */
		{2791,			   1895			   },
		{2840,			   1676			   },
		{2840,			   1676			   },
		{2881,			   1614			   },

		/* Caeli */
		{2411,			   718			  },
		{2589,			   741			  },
		{2589,			   741			  },
		{2854,			   787			  },
		{2854,			   787			  },
		{3089,			   782			  },
		{3089,			   782			  },
		{3333,			   801			  },

		/* Gorno */
		{2820,			   301			  },
		{2771,			   146			  },
		{2771,			   146			  },
		{2814,			   89				 },
		{2814,			   89				 },
		{2939,			   116			  },
		{2939,			   116			  },
		{2908,			   269			  },
		{2908,			   269			  },
		{2820,			   301			  },

		/* Ptolemae */
		{1740,			   423			  },
		{1787,			   338			  },
		{1787,			   338			  },
		{1855,			   270			  },
		{1787,			   338			  },
		{2004,			   441			  },
		{2004,			   441			  },
		{2058,			   475			  },
		{2058,			   475			  },
		{2100,			   554			  },
		{2100,			   554			  },
		{2156,			   440			  },
		{2156,			   440			  },
		{2058,			   475			  },

		/* Lyrae */
		{1916,			   1270			   },
		{1895,			   1041			   },

		/* Wolf */
		{1601,			   1746			   },
		{1666,			   1812			   },

		/* Centauri */
		{1579,			   1115			   },
		{1559,			   993			  },
		{1559,			   993			  },
		{1515,			   866			  },
		{1515,			   866			  },
		{1463,			   779			  },
		{1463,			   779			  },
		{1412,			   905			  },
		{1412,			   905			  },
		{1312,			   1260			   },

		/* Giclas */
		{675,			   742				 },
		{958,			   468				 },
		{958,			   468				 },
		{843,			   431				 },
		{843,			   431				 },
		{843,			   380				 },
		{843,			   380				 },
		{877,			   340				 },
		{877,			   340				 },
		{1116,			   334			  },
		{843,			   380				 },
		{803,			   337				 },
		{803,			   337				 },
		{708,			   41				},

		/* Krueger */
		{134,			   565				 },
		{304,			   477				 },
		{304,			   477				 },
		{522,			   525				 },
		{522,			   525				 },
		{530,			   442				 },
		{530,			   442				 },
		{570,			   289				 },
		{570,			   289				 },
		{265,			   156				 },
		{265,			   156				 },
		{304,			   477				 },

		/* Lacaille */
		{146,			   955				 },
		{230,			   952				 },
		{230,			   952				 },
		{242,			   857				 },
		{242,			   857				 },
		{146,			   955				 },

		/* Volantis */
		{650,			   1646			  },
		{736,			   1737			  },
		{736,			   1737			  },
		{705,			   1838			  },
		{705,			   1838			  },
		{650,			   1646			  },
		{705,			   1838			  },
		{977,			   1953			  },
		{977,			   1953			  },
		{1048,			   1919			   },
		{1048,			   1919			   },
		{1140,			   1847			   },
		{1140,			   1847			   },
		{951,			   1770			  },
		{951,			   1770			  },
		{1048,			   1919			   },

		/* Olber */
		{125,			   2229			  },
		{312,			   2250			  },
		{312,			   2250			  },
		{270,			   2187			  },
		{270,			   2187			  },
		{125,			   2229			  },

		/* Bootis */
		{217,			   2509			  },
		{229,			   2666			  },
		{229,			   2666			  },
		{138,			   2696			  },
		{138,			   2696			  },
		{167,			   2875			  },
		{167,			   2875			  },
		{384,			   2900			  },
		{384,			   2900			  },
		{672,			   2863			  },
		{384,			   2900			  },
		{351,			   2758			  },
		{351,			   2758			  },
		{229,			   2666			  },

		/* Virginis */
		{291,			   3250			  },
		{501,			   3259			  },
		{501,			   3259			  },
		{791,			   3270			  },
		{791,			   3270			  },
		{1104,			   3333			   },

		/* Herculis */
		{1604,			   3645			   },
		{1758,			   3418			   },

		/* Illuminati */
		{2354,			   3166			   },
		{2354,			   3291			   },
		{2354,			   3291			   },
		{2520,			   3437			   },
		{2520,			   3437			   },
		{2687,			   3333			   },
		{2687,			   3333			   },
		{2727,			   2951			   },
		{2727,			   2951			   },
		{2458,			   2750			   },
		{2458,			   2750			   },
		{2208,			   2854			   },
		{2208,			   2854			   },
		{2011,			   3043			   },

		/* Vitalis */
		{3187,			   3375			   },
		{3000,			   3500			   },
		{3000,			   3500			   },
		{2770,			   3625			   },
		{2770,			   3625			   },
		{3083,			   3674			   },
		{3083,			   3674			   },
		{3000,			   3500			   },

		/* Gruis */
		{2148,			   3551			   },
		{2274,			   3663			   },
		{2274,			   3663			   },
		{2250,			   3708			   },
		{2250,			   3708			   },
		{2354,			   3741			   },
		{2354,			   3741			   },
		{2479,			   3958			   },
		{2479,			   3958			   },
		{2583,			   3750			   },
		{2583,			   3750			   },
		{2416,			   3687			   },
		{2416,			   3687			   },
		{2354,			   3741			   },

		/* Squidi */
		{2145,			   3916			   },
		{2145,			   4208			   },
		{2145,			   4208			   },
		{2062,			   3991			   },
		{2062,			   3991			   },
		{2145,			   3916			   },

		/* Ceti */
		{2812,			   4250			   },
		{2875,			   4041			   },
		{2875,			   4041			   },
		{3038,			   4083			   },
		{3038,			   4083			   },
		{2937,			   4306			   },
		{2937,			   4306			   },
		{2812,			   4250			   },
		{2937,			   4306			   },
		{2875,			   4479			   },
		{2875,			   4479			   },
		{2895,			   4687			   },
		{2895,			   4687			   },
		{2708,			   4708			   },

		/* Capricorni */
		{2979,			   5166			   },
		{2993,			   5318			   },
		{2993,			   5318			   },
		{3035,			   5178			   },
		{3035,			   5178			   },
		{2979,			   5166			   },

		/* Tauri */
		{166,			   3770			  },
		{229,			   3666			  },
		{229,			   3666			  },
		{90,				3614			 },
		{90,				3614			 },
		{149,			   3519			  },
		{149,			   3519			  },
		{267,			   3645			  },
		{267,			   3645			  },
		{229,			   3666			  },
		{267,			   3645			  },
		{288,			   3735			  },

		/* Telescopii */
		{285,			   4020			  },
		{291,			   4104			  },
		{291,			   4104			  },
		{500,			   4187			  },
		{500,			   4187			  },
		{285,			   4020			  },

		/* Velorum */
		{860,			   4065			  },
		{926,			   3972			  },

		/* Cygnus */
		{250,			   4583			  },
		{479,			   4645			  },
		{479,			   4645			  },
		{416,			   4717			  },
		{416,			   4717			  },
		{562,			   4708			  },
		{562,			   4708			  },
		{479,			   4583			  },
		{479,			   4583			  },
		{479,			   4645			  },

		/* Normae */
		{1425,			   5404			   },
		{1416,			   5315			   },
		{1416,			   5315			   },
		{1520,			   5261			   },
		{1520,			   5261			   },
		{1613,			   5279			   },
		{1613,			   5279			   },
		{1854,			   5416			   },
		{1854,			   5416			   },
		{2159,			   5614			   },

		/* Orionis */
		{1558,			   6058			   },
		{1714,			   5926			   },
		{1714,			   5926			   },
		{1545,			   5818			   },
		{1714,			   5926			   },
		{1923,			   5878			   },
		{1923,			   5878			   },
		{1978,			   5968			   },
		{1978,			   5968			   },
		{1902,			   6065			   },
		{1902,			   6065			   },
		{1714,			   5926			   },
		{1902,			   6065			   },
		{1881,			   6308			   },
		{1881,			   6308			   },
		{1795,			   6329			   },
		{1881,			   6308			   },
		{2118,			   6379			   },
		{2118,			   6379			   },
		{2200,			   6176			   },
		{2200,			   6176			   },
		{2159,			   6073			   },
		{2159,			   6073			   },
		{2228,			   6038			   },
		{2159,			   6073			   },
		{1978,			   5968			   },

		/* Octantis */
		{1360,			   6489			   },
		{1578,			   6668			   },

		/* Sagittae */
		{250,			   5687			  },
		{371,			   5772			  },
		{371,			   5772			  },
		{152,			   5900			  },
		{152,			   5900			  },
		{250,			   5687			  },

		/* Circini */
		{395,			   5979			  },
		{456,			   5989			  },
		{456,			   5989			  },
		{563,			   5980			  },
		{563,			   5980			  },
		{541,			   6145			  },
		{541,			   6145			  },
		{437,			   6270			  },
		{437,			   6270			  },
		{365,			   6093			  },
		{365,			   6093			  },
		{395,			   5979			  },

		/* Columbae */
		{729,			   6208			  },
		{750,			   6458			  },
		{750,			   6458			  },
		{770,			   6602			  },

		/* Chandrasekhar */
		{125,			   6541			  },
		{208,			   6625			  },
		{208,			   6625			  },
		{187,			   6520			  },
		{208,			   6625			  },
		{312,			   6728			  },
		{208,			   6625			  },
		{375,			   6716			  },
		{375,			   6716			  },
		{312,			   6728			  },

		/* Mersenne */
		{2416,			   7291			   },
		{2604,			   6645			   },

		/* Pegasi */
		{395,			   7458			  },
		{333,			   7625			  },

		/* Pavonis */
		{562,			   8000			  },
		{687,			   8000			  },

		/* Muscae */
		{1020,			   7187			   },
		{1041,			   7708			   },
		{1041,			   7708			   },
		{1125,			   7791			   },
		{1125,			   7791			   },
		{1396,			   8041			   },
		{1396,			   8041			   },
		{1520,			   8333			   },
		{1520,			   8333			   },
		{1125,			   8583			   },
		{1125,			   8583			   },
		{1396,			   8041			   },

		/* Draconis */
		{3151,			   8390			   },
		{3562,			   8250			   },
		{3562,			   8250			   },
		{3270,			   8000			   },
		{3270,			   8000			   },
		{3151,			   8390			   },
		{3270,			   8000			   },
		{2836,			   7857			   },
		{2836,			   7857			   },
		{2518,			   8056			   },
		{2518,			   8056			   },
		{2362,			   8395			   },
		{2362,			   8395			   },
		{2535,			   8358			   },
		{2535,			   8358			   },
		{2771,			   8351			   },
		{2771,			   8351			   },
		{2822,			   8395			   },
		{2822,			   8395			   },
		{2776,			   8673			   },
		{2776,			   8673			   },
		{2588,			   8653			   },
		{2588,			   8653			   },
		{2458,			   8666			   },
		{2458,			   8666			   },
		{2310,			   8702			   },
		{2310,			   8702			   },
		{2300,			   8861			   },
		{2300,			   8861			   },
		{2458,			   8666			   },

		/* Apodis */
		{2536,			   8504			   },
		{2582,			   8507			   },
		{2582,			   8507			   },
		{2593,			   8569			   },
		{2593,			   8569			   },
		{2562,			   8572			   },
		{2562,			   8572			   },
		{2536,			   8504			   },

		/* Antliae */
		{2535,			   8917			   },
		{2581,			   9105			   },
		{2581,			   9105			   },
		{2687,			   9000			   },
		{2687,			   9000			   },
		{2706,			   8910			   },
		{2706,			   8910			   },
		{2535,			   8917			   },
		{2706,			   8910			   },
		{2831,			   8854			   },
		{2831,			   8854			   },
		{3050,			   8833			   },

		/* Klystron */
		{2916,			   9541			   },
		{1937,			   9979			   },

		/* Ophiuchi */
		{437,			   8770			  },
		{479,			   8875			  },
		{479,			   8875			  },
		{333,			   8916			  },
		{333,			   8916			  },
		{437,			   8770			  },

		/* Corvi */
		{104,			   9125			  },
		{62,				9479			 },
		{62,				9479			 },
		{83,				9979			 },
		{83,				9979			 },
		{333,			   9812			  },
		{333,			   9812			  },
		{1020,			   9937			   },
		{1020,			   9937			   },
		{1000,			   9395			   },

		/* End */
		{MaxXUniverse << 1, MaxYUniverse << 1}
};

static constexpr uint8_t ElementsArray[NUMBER_OF_ELEMENTS] =
	{
		COMMON,		 /* HYDROGEN */
		COMMON,		 /* HELIUM */
		COMMON,		 /* LITHIUM */
		BASE_METAL,	 /* BERYLLIUM */
		BASE_METAL,	 /* BORON */
		COMMON,		 /* CARBON */
		COMMON,		 /* NITROGEN */
		CORROSIVE,	 /* OXYGEN */
		CORROSIVE,	 /* FLUORINE */
		NOBLE,		 /* NEON */
		BASE_METAL,	 /* SODIUM */
		BASE_METAL,	 /* MAGNESIUM */
		BASE_METAL,	 /* ALUMINUM */
		COMMON,		 /* SILICON */
		COMMON,		 /* PHOSPHORUS */
		CORROSIVE,	 /* SULFUR */
		CORROSIVE,	 /* CHLORINE */
		NOBLE,		 /* ARGON */
		BASE_METAL,	 /* POTASSIUM */
		BASE_METAL,	 /* CALCIUM */
		BASE_METAL,	 /* SCANDIUM */
		BASE_METAL,	 /* TITANIUM */
		BASE_METAL,	 /* VANADIUM */
		BASE_METAL,	 /* CHROMIUM */
		BASE_METAL,	 /* MANGANESE */
		BASE_METAL,	 /* IRON */
		BASE_METAL,	 /* COBALT */
		BASE_METAL,	 /* NICKEL */
		BASE_METAL,	 /* COPPER */
		BASE_METAL,	 /* ZINC */
		BASE_METAL,	 /* GALLIUM */
		BASE_METAL,	 /* GERMANIUM */
		COMMON,		 /* ARSENIC */
		COMMON,		 /* SELENIUM */
		CORROSIVE,	 /* BROMINE */
		NOBLE,		 /* KRYPTON */
		BASE_METAL,	 /* RUBIDIUM */
		BASE_METAL,	 /* STRONTIUM */
		BASE_METAL,	 /* YTTRIUM */
		BASE_METAL,	 /* ZIRCONIUM */
		BASE_METAL,	 /* NIOBIUM */
		BASE_METAL,	 /* MOLYBDENUM */
		RADIOACTIVE, /* TECHNETIUM */
		BASE_METAL,	 /* RUTHENIUM */
		BASE_METAL,	 /* RHODIUM */
		PRECIOUS,	 /* PALLADIUM */
		PRECIOUS,	 /* SILVER */
		BASE_METAL,	 /* CADMIUM */
		BASE_METAL,	 /* INDIUM */
		BASE_METAL,	 /* TIN */
		BASE_METAL,	 /* ANTIMONY */
		BASE_METAL,	 /* TELLURIUM */
		CORROSIVE,	 /* IODINE */
		NOBLE,		 /* XENON */
		BASE_METAL,	 /* CESIUM */
		BASE_METAL,	 /* BARIUM */
		RARE_EARTH,	 /* LANTHANUM */
		RARE_EARTH,	 /* CERIUM */
		RARE_EARTH,	 /* PRASEODYMIUM */
		RARE_EARTH,	 /* NEODYMIUM */
		RARE_EARTH,	 /* PROMETHIUM */
		RARE_EARTH,	 /* SAMARIUM */
		RARE_EARTH,	 /* EUROPIUM */
		RARE_EARTH,	 /* GADOLINIUM */
		RARE_EARTH,	 /* TERBIUM */
		RARE_EARTH,	 /* DYPROSIUM */
		RARE_EARTH,	 /* HOLMIUM */
		RARE_EARTH,	 /* ERBIUM */
		RARE_EARTH,	 /* THULIUM */
		RARE_EARTH,	 /* YTTERBIUM */
		RARE_EARTH,	 /* LUTETIUM */
		BASE_METAL,	 /* HAFNIUM */
		BASE_METAL,	 /* TANTALUM */
		BASE_METAL,	 /* TUNGSTEN */
		BASE_METAL,	 /* RHENIUM */
		BASE_METAL,	 /* OSMIUM */
		PRECIOUS,	 /* IRIDIUM */
		PRECIOUS,	 /* PLATINUM */
		PRECIOUS,	 /* GOLD */
		BASE_METAL,	 /* MERCURY */
		BASE_METAL,	 /* THALLIUM */
		BASE_METAL,	 /* LEAD */
		BASE_METAL,	 /* BISMUTH */
		RADIOACTIVE, /* POLONIUM */
		RADIOACTIVE, /* ASTATINE */
		NOBLE,		 /* RADON */
		RADIOACTIVE, /* FRANCIUM */
		RADIOACTIVE, /* RADIUM */
		RADIOACTIVE, /* ACTINIUM */
		RADIOACTIVE, /* THORIUM */
		RADIOACTIVE, /* PROTACTINIUM */
		RADIOACTIVE, /* URANIUM */
		RADIOACTIVE, /* NEPTUNIUM */
		RADIOACTIVE, /* PLUTONIUM */

		COMMON,		 /* OZONE */
		COMMON,		 /* FREE RADICALS */
		COMMON,		 /* CARBON DIOXIDE */
		COMMON,		 /* CARBON MONOXIDE */
		COMMON,		 /* AMMONIA */
		COMMON,		 /* METHANE */
		COMMON,		 /* SULFURIC ACID */
		COMMON,		 /* HYDROCHLORIC ACID */
		COMMON,		 /* HYDROCYANIC ACID */
		COMMON,		 /* FORMIC ACID */
		COMMON,		 /* PHOSPHORIC ACID */
		COMMON,		 /* FORMALDEHYDE */
		COMMON,		 /* CYANOACETYLENE */
		COMMON,		 /* METHANOL */
		COMMON,		 /* ETHANOL */
		COMMON,		 /* SILICON MONOXIDE */
		COMMON,		 /* TITANIUM OXIDE */
		COMMON,		 /* ZIRCONIUM OXIDE */
		COMMON,		 /* WATER */
		COMMON,		 /* SILICON COMPOUNDS */
		COMMON,		 /* METAL OXIDES */
		EXOTIC,		 /* QUANTUM BLACK HOLES */
		EXOTIC,		 /* NEUTRONIUM */
		EXOTIC,		 /* MAGNETIC MONOPOLES */
		EXOTIC,		 /* DEGENERATE MATTER */
		EXOTIC,		 /* SUPER FLUIDS */
		EXOTIC,		 /* AGUUTI NODULES */
		COMMON,		 /* IRON COMPOUNDS */
		COMMON,		 /* ALUMINUM COMPOUNDS */
		COMMON,		 /* NITROUS OXIDE */
		RADIOACTIVE, /* RADIOACTIVES */
		COMMON,		 /* HYDROCARBONS */
		COMMON,		 /* CARBON COMPOUNDS */
		EXOTIC,		 /* ANTIMATTER */
		EXOTIC,		 /* CHARON DUST */
		EXOTIC,		 /* REISBURG HELICES */
		EXOTIC,		 /* TZO CRYSTALS */
		COMMON,		 /* CALCIUM COMPOUNDS */
		COMMON,		 /* NITRIC ACID */
};

/*------------------------------ Global Data ------------------------------ */

inline constexpr auto NoDeposit {0};

inline constexpr auto TraceUseful {MineralDeposit(Few, LIGHT)};
inline constexpr auto LightUseful {MineralDeposit(Moderate, LIGHT)};
inline constexpr auto MediumUseful {MineralDeposit(Moderate, MEDIUM)};
inline constexpr auto HeavyUseful {MineralDeposit(Moderate, HEAVY)};
inline constexpr auto HugeUseful {MineralDeposit(Numerous, HEAVY)};

static constexpr PlanetFrame PlanetsArray[NUMBER_OF_PLANET_TYPES] =
	{
		{
			/* OOLITE_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  VIOLET_BODY),													  /* Color and type/size of planet */
			LowTectonics,										/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(HIGH_DENSITY,				HEAVY),																					  /* Atmosphere and density */
			{
				{HOLMIUM, MediumUseful},
				{ERBIUM, MediumUseful},
				{THULIUM, MediumUseful},
				{YTTERBIUM, MediumUseful},
				{LUTETIUM, MediumUseful},
				{PALLADIUM, MediumUseful},
				{SILVER, MediumUseful},
				{IRIDIUM, MediumUseful},
			},
			OoliteColorTabRes,
			OoliteXlatTabRes,
			230,
			2,
			200,
			150,
		 },
		{
			/* YTTRIC_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  VIOLET_BODY),																		   /* Color and type/size of planet */
			LowTectonics,							 /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(HIGH_DENSITY,																			 MEDIUM),																																				 /* Atmosphere and density */
			{
				{YTTERBIUM, HugeUseful},
				{YTTRIUM, HugeUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			YttricColorTabRes,
			YttricXlatTabRes,
			250,
			2,
			80,
			200,
		 },
		{
			/* QUASI_DEGENERATE_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + TopoAlgo,
		  GREEN_BODY),															  /* Color and type/size of planet */
			MedTectonics,																		  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(HIGH_DENSITY,																								  Nothing),																																		 /* Atmosphere and density */
			{
				{DEGENERATE_MATTER, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			QuasiDegenerateColorTabRes,
			QuasiDegenerateXlatTabRes,
			500,
			1,
			0,
			160,
		 },
		{
			/* LANTHANIDE_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  YELLOW_BODY),																 /* Color and type/size of planet */
			HighTectonics,																		  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(HIGH_DENSITY,																					 LIGHT),																																													 /* Atmosphere and density */
			{
				{LANTHANUM, MediumUseful},
				{CERIUM, MediumUseful},
				{PRASEODYMIUM, MediumUseful},
				{NEODYMIUM, MediumUseful},
				{PROMETHIUM, MediumUseful},
				{SAMARIUM, MediumUseful},
				{GADOLINIUM, MediumUseful},
				{TERBIUM, MediumUseful},
			},
			LanthanideColorTabRes,
			LanthanideXlatTabRes,
			250,
			2,
			80,
			200,
		 },
		{
			/* TREASURE_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + TopoAlgo,
		  YELLOW_BODY),												 /* Color and type/size of planet */
			NoTectonics,																		/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(SUPER_DENSITY,																						LIGHT),																																													/* Atmosphere and density */
			{
				{PALLADIUM, HeavyUseful},
				{SILVER, HeavyUseful},
				{SILVER, HeavyUseful},
				{IRIDIUM, HeavyUseful},
				{GOLD, HeavyUseful},
				{PLATINUM, HeavyUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			TreasureColorTabRes,
			TreasureXlatTabRes,
			500,
			1,
			0,
			160,
		 },
		{
			/* UREA_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  YELLOW_BODY),											   /* Color and type/size of planet */
			HighTectonics,														/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LOW_DENSITY,																	   HEAVY),																																												   /* Atmosphere and density */
			{
				{AMMONIA, LightUseful},
				{FORMALDEHYDE, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			UreaColorTabRes,
			UreaXlatTabRes,
			230,
			2,
			200,
			150,
		 },
		{
			/* METAL_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  ORANGE_BODY),	/* Color and type/size of planet */
			LowTectonics,	/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(HIGH_DENSITY,HEAVY),																																									   /* Atmosphere and density */
			{
				{IRON, HugeUseful},
				{NICKEL, HeavyUseful},
				{VANADIUM, MediumUseful},
				{SILVER, MediumUseful},
				{URANIUM, HeavyUseful},
				{SULFUR, MediumUseful},
				{COPPER, HeavyUseful},
				{Nothing, NoDeposit},
			},
			MetalColorTabRes,
			MetalXlatTabRes,
			230,
			2,
			200,
			150,
		 },
		{
			/* RADIOACTIVE_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  ORANGE_BODY),													  /* Color and type/size of planet */
			NoTectonics,										 /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(SUPER_DENSITY,			 LIGHT),																																									 /* Atmosphere and density */
			{
				{ASTATINE, MediumUseful},
				{FRANCIUM, MediumUseful},
				{RADIUM, MediumUseful},
				{ACTINIUM, MediumUseful},
				{THORIUM, MediumUseful},
				{PROTACTINIUM, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			RadioactiveColorTabRes,
			RadioactiveXlatTabRes,
			250,
			2,
			80,
			200,
		 },
		{
			/* OPALESCENT_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  CYAN_BODY),																		   /* Color and type/size of planet */
			LowTectonics,							 /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LOW_DENSITY,																			 Nothing),																																				  /* Atmosphere and density */
			{
				{SAMARIUM, LightUseful},
				{GADOLINIUM, LightUseful},
				{ARGON, LightUseful},
				{LITHIUM, LightUseful},
				{SILICON, LightUseful},
				{ARSENIC, LightUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			OpalescentColorTabRes,
			OpalescentXlatTabRes,
			400,
			1,
			100,
			190,
		 },
		{
			/* CYANIC_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + TopoAlgo,
		  GREEN_BODY),															  /* Color and type/size of planet */
			NoTectonics,																		  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LIGHT_DENSITY,																								  LIGHT),																																	 /* Atmosphere and density */
			{
				{CYANOACETYLENE, HugeUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			CyanicColorTabRes,
			CyanicXlatTabRes,
			500,
			1,
			0,
			160,
		 },
		{
			/* ACID_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  GREEN_BODY),																 /* Color and type/size of planet */
			LowTectonics,																		/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,																					 LIGHT),																																													 /* Atmosphere and density */
			{
				{SULFURIC_ACID, HeavyUseful},
				{HYDROCHLORIC_ACID, HeavyUseful},
				{FORMIC_ACID, HeavyUseful},
				{HYDROCYANIC_ACID, HeavyUseful},
				{PHOSPHORIC_ACID, HeavyUseful},
				{NITRIC_ACID, HeavyUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			AcidColorTabRes,
			AcidXlatTabRes,
			250,
			2,
			80,
			200,
		 },
		{
			/* ALKALI_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  GREEN_BODY),												   /* Color and type/size of planet */
			LowTectonics,																		/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,																						MEDIUM),																																													 /* Atmosphere and density */
			{
				{CALCIUM, MediumUseful},
				{BARIUM, MediumUseful},
				{STRONTIUM, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			AlkaliColorTabRes,
			AlkaliXlatTabRes,
			250,
			2,
			80,
			200,
		 },
		{
			/* HALIDE_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  GREEN_BODY),											   /* Color and type/size of planet */
			NoTectonics,															/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LOW_DENSITY,																		   LIGHT),																																												   /* Atmosphere and density */
			{
				{FLUORINE, MediumUseful},
				{BROMINE, MediumUseful},
				{BROMINE, MediumUseful},
				{ASTATINE, MediumUseful},
				{IODINE, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			HalideColorTabRes,
			HalideXlatTabRes,
			250,
			2,
			80,
			200,
		 },
		{
			/* GREEN_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  GREEN_BODY), /* Color and type/size of planet */
			MedTectonics, /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,HEAVY),																																									 /* Atmosphere and density */
			{
				{PRASEODYMIUM, HeavyUseful},
				{NEODYMIUM, HeavyUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			GreenColorTabRes,
			GreenXlatTabRes,
			230,
			2,
			200,
			150,
		 },
		{
			/* COPPER_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + TopoAlgo,
		  GREEN_BODY),													   /* Color and type/size of planet */
			NoTectonics,										 /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(HIGH_DENSITY,				MEDIUM),							 /* Atmosphere and density */
			{
				{COPPER, HugeUseful},
				{COPPER, HugeUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			CopperColorTabRes,
			CopperXlatTabRes,
			500,
			1,
			0,
			160,
		 },
		{
			/* CARBIDE_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  RED_BODY),																		   /* Color and type/size of planet */
			LowTectonics,						  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,																			  LIGHT),																																				  /* Atmosphere and density */
			{
				{CARBON, HeavyUseful},
				{CARBON, HeavyUseful},
				{CARBON, HeavyUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			CarbideColorTabRes,
			CarbideXlatTabRes,
			400,
			1,
			100,
			190,
		 },
		{
			/* ULTRAMARINE_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  BLUE_BODY),															  /* Color and type/size of planet */
			NoTectonics,																		  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LIGHT_DENSITY,																								  HEAVY),																																		 /* Atmosphere and density */
			{
				{KRYPTON, MediumUseful},
				{COBALT, MediumUseful},
				{HOLMIUM, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			UltramarineColorTabRes,
			UltramarineXlatTabRes,
			200,
			2,
			100,
			100,
		 },
		{
			/* NOBLE_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  BLUE_BODY),																 /* Color and type/size of planet */
			NoTectonics,																		  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LIGHT_DENSITY,																					 LIGHT),																																													 /* Atmosphere and density */
			{
				{NEON, LightUseful},
				{RADON, LightUseful},
				{ARGON, LightUseful},
				{KRYPTON, LightUseful},
				{XENON, LightUseful},
				{HELIUM, LightUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			NobleColorTabRes,
			NobleXlatTabRes,
			250,
			2,
			80,
			200,
		 },
		{
			/* AZURE_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  BLUE_BODY),												 /* Color and type/size of planet */
			MedTectonics,																		/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LOW_DENSITY,																						MEDIUM),																																												   /* Atmosphere and density */
			{
				{INDIUM, LightUseful},
				{MOLYBDENUM, LightUseful},
				{VANADIUM, LightUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			AzureColorTabRes,
			AzureXlatTabRes,
			230,
			2,
			200,
			150,
		 },
		{
			/* CHONDRITE_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  PURPLE_BODY),												/* Color and type/size of planet */
			HighTectonics,													 /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,																	Nothing),																																												   /* Atmosphere and density */
			{
				{ETHANOL, HeavyUseful},
				{FREE_RADICALS, HeavyUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			ChondriteColorTabRes,
			ChondriteXlatTabRes,
			500,
			1,
			100,
			190,
		 },
		{
			/* PURPLE_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  PURPLE_BODY),   /* Color and type/size of planet */
			LowTectonics,	   /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LOW_DENSITY,LIGHT),																																									  /* Atmosphere and density */
			{
				{RHENIUM, MediumUseful},
				{CADMIUM, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			PurpleColorTabRes,
			PurpleXlatTabRes,
			230,
			2,
			200,
			150,
		 },
		{
			/* SUPER_DENSE_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + TopoAlgo,
		  PURPLE_BODY),														/* Color and type/size of planet */
			HighTectonics,										 /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(SUPER_DENSITY,				 HEAVY),			 /* Atmosphere and density */
			{
				{LEAD, MediumUseful},
				{OSMIUM, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			SuperDenseColorTabRes,
			SuperDenseXlatTabRes,
			500,
			1,
			0,
			160,
		 },
		{
			/* PELLUCID_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  PURPLE_BODY),																		   /* Color and type/size of planet */
			NoTectonics,						   /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LOW_DENSITY,																			 LIGHT),																																				/* Atmosphere and density */
			{
				{TZO_CRYSTALS, TraceUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			PellucidColorTabRes,
			PellucidXlatTabRes,
			400,
			1,
			100,
			190,
		 },
		{
			/* DUST_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  RED_BODY),															  /* Color and type/size of planet */
			LowTectonics,																		  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LOW_DENSITY,																								  MEDIUM),																																	 /* Atmosphere and density */
			{
				{BISMUTH, LightUseful},
				{ALUMINUM, LightUseful},
				{POTASSIUM, LightUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			DustColorTabRes,
			DustXlatTabRes,
			250,
			2,
			80,
			200,
		 },
		{
			/* CRIMSON_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  RED_BODY),																 /* Color and type/size of planet */
			NoTectonics,																		/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,																					 HEAVY),																																													 /* Atmosphere and density */
			{
				{BARIUM, LightUseful},
				{BORON, LightUseful},
				{BERYLLIUM, LightUseful},
				{BISMUTH, LightUseful},
				{BROMINE, LightUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			CrimsonColorTabRes,
			CrimsonXlatTabRes,
			230,
			2,
			200,
			150,
		 },
		{
			/* CIMMERIAN_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + TopoAlgo,
		  RED_BODY),											  /* Color and type/size of planet */
			LowTectonics,																		/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(HIGH_DENSITY,																						Nothing),																																													 /* Atmosphere and density */
			{
				{METHANE, MediumUseful},
				{AMMONIA, MediumUseful},
				{METHANOL, MediumUseful},
				{LITHIUM, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			CimmerianColorTabRes,
			CimmerianXlatTabRes,
			500,
			1,
			0,
			160,
		 },
		{
			/* INFRARED_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  RED_BODY),											  /* Color and type/size of planet */
			LowTectonics,														/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,																	   HEAVY),																																												   /* Atmosphere and density */
			{
				{MERCURY, HeavyUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			InfraredColorTabRes,
			InfraredXlatTabRes,
			400,
			1,
			100,
			190,
		 },
		{
			/* SELENIC_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + CrateredAlgo,
		  WHITE_BODY),	   /* Color and type/size of planet */
			NoTectonics,	   /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LOW_DENSITY,LIGHT),																																									   /* Atmosphere and density */
			{
				{IRON, LightUseful},
				{ALUMINUM, LightUseful},
				{CALCIUM, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			SelenicColorTabRes,
			SelenicXlatTabRes,
			230,
			2,
			200,
			150,
		 },
		{
			/* AURIC_WORLD */
			uqm::makeByte(SMALL_ROCKY_WORLD + TopoAlgo,
		  YELLOW_BODY),														 /* Color and type/size of planet */
			NoTectonics,											 /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(HIGH_DENSITY,				MEDIUM),				 /* Atmosphere and density */
			{
				{GOLD, HugeUseful},
				{GOLD, HugeUseful},
				{GOLD, HugeUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			AuricColorTabRes,
			AuricXlatTabRes,
			500,
			1,
			0,
			160,
		 },


		{
			/* FLUORESCENT_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + CrateredAlgo,
		  VIOLET_BODY),																		   /* Color and type/size of planet */
			LowTectonics,								/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(HIGH_DENSITY,																				LIGHT),																																					/* Atmosphere and density */
			{
				{TECHNETIUM, HugeUseful},
				{NEON, HugeUseful},
				{RADON, LightUseful},
				{POTASSIUM, LightUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			FluorescentColorTabRes,
			FluorescentXlatTabRes,
			400,
			1,
			100,
			190,
		 },
		{
			/* ULTRAVIOLET_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + CrateredAlgo,
		  VIOLET_BODY),															  /* Color and type/size of planet */
			MedTectonics,																		  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(HIGH_DENSITY,																								  HEAVY),																																	 /* Atmosphere and density */
			{
				{POLONIUM, HugeUseful},
				{GOLD, HugeUseful},
				{PHOSPHORUS, HugeUseful},
				{SCANDIUM, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			UltravioletColorTabRes,
			UltravioletXlatTabRes,
			250,
			2,
			80,
			200,
		 },
		{
			/* PLUTONIC_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + CrateredAlgo,
		  YELLOW_BODY),																 /* Color and type/size of planet */
			LowTectonics,																		/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(HIGH_DENSITY,																					 HEAVY),																																													 /* Atmosphere and density */
			{
				{BERYLLIUM, HugeUseful},
				{BORON, HugeUseful},
				{LANTHANUM, MediumUseful},
				{ASTATINE, MediumUseful},
				{FRANCIUM, MediumUseful},
				{TITANIUM, LightUseful},
				{CERIUM, MediumUseful},
				{Nothing, NoDeposit},
			},
			PlutonicColorTabRes,
			PlutonicXlatTabRes,
			250,
			2,
			80,
			200,
		 },
		{
			/* RAINBOW_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + TopoAlgo,
		  YELLOW_BODY),												 /* Color and type/size of planet */
			NoTectonics,																		/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LOW_DENSITY,																						LIGHT),																																												   /* Atmosphere and density */
			{
				{ACTINIUM, HeavyUseful},
				{THORIUM, HeavyUseful},
				{PROTACTINIUM, HeavyUseful},
				{NEPTUNIUM, HeavyUseful},
				{PLUTONIUM, HeavyUseful},
				{OZONE, HugeUseful},
				{OZONE, HugeUseful},
				{Nothing, NoDeposit},
			},
			RainbowColorTabRes,
			RainbowXlatTabRes,
			500,
			1,
			20,
			100,
		 },
		{
			/* SHATTERED_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + CrateredAlgo,
		  ORANGE_BODY),											/* Color and type/size of planet */
			SuperTectonics,														   /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,																		  Nothing),																																												   /* Atmosphere and density */
			{
				{PALLADIUM, HugeUseful},
				{IRIDIUM, HugeUseful},
				{TECHNETIUM, HugeUseful},
				{POLONIUM, HugeUseful},
				{SODIUM, HugeUseful},
				{MANGANESE, HugeUseful},
				{CHROMIUM, HugeUseful},
				{Nothing, NoDeposit},
			},
			ShatteredColorTabRes,
			ShatteredXlatTabRes,
			500,
			1,
			0,
			185,
		 },
		{
			/* SAPPHIRE_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + TopoAlgo,
		  CYAN_BODY),	 /* Color and type/size of planet */
			NoTectonics,	 /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LOW_DENSITY,Nothing),																																								/* Atmosphere and density */
			{
				{REISBURG_HELICES, HugeUseful},
				{RT_SUPER_FLUID, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			SapphireColorTabRes,
			SapphireXlatTabRes,
			80,
			1,
			0,
			128,
		 },
		{
			/* ORGANIC_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + TopoAlgo,
		  CYAN_BODY),													  /* Color and type/size of planet */
			MedTectonics,									  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,			  LIGHT), /* Atmosphere and density */
			{
				{FREE_RADICALS, HeavyUseful},
				{FORMALDEHYDE, HeavyUseful},
				{CARBON, HeavyUseful},
				{CARBON_DIOXIDE, HeavyUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			OrganicColorTabRes,
			OrganicXlatTabRes,
			500,
			1,
			0,
			160,
		 },
		{
			/* XENOLITHIC_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + CrateredAlgo,
		  CYAN_BODY),																		   /* Color and type/size of planet */
			LowTectonics,						   /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,																			 MEDIUM),																																				 /* Atmosphere and density */
			{
				{ALUMINUM, HugeUseful},
				{PLATINUM, LightUseful},
				{GERMANIUM, TraceUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			XenolithicColorTabRes,
			XenolithicXlatTabRes,
			400,
			1,
			100,
			190,
		 },
		{
			/* REDUX_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + CrateredAlgo,
		  GREEN_BODY),															  /* Color and type/size of planet */
			LowTectonics,																		  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,																								  LIGHT),																																		 /* Atmosphere and density */
			{
				{BROMINE, MediumUseful},
				{OXYGEN, MediumUseful},
				{FLUORINE, MediumUseful},
				{SULFUR, MediumUseful},
				{CHLORINE, MediumUseful},
				{IODINE, MediumUseful},
				{ZIRCONIUM, LightUseful},
				{Nothing, NoDeposit},
			},
			ReduxColorTabRes,
			ReduxXlatTabRes,
			500,
			1,
			0,
			190,
		 },
		{
			/* PRIMORDIAL_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + CrateredAlgo,
		  GREEN_BODY),																 /* Color and type/size of planet */
			SuperTectonics,																		 /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,																					 HEAVY),																																													 /* Atmosphere and density */
			{
				{CESIUM, MediumUseful},
				{BARIUM, MediumUseful},
				{RUBIDIUM, MediumUseful},
				{METHANE, HugeUseful},
				{AMMONIA, HugeUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			PrimordialColorTabRes,
			PrimordialXlatTabRes,
			250,
			2,
			10,
			200,
		 },
		{
			/* EMERALD_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + TopoAlgo,
		  GREEN_BODY),												 /* Color and type/size of planet */
			NoTectonics,																		/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LOW_DENSITY,																						Nothing),																																													 /* Atmosphere and density */
			{
				{AGUUTI_NODULES, HugeUseful},
				{ANTIMATTER, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			EmeraldColorTabRes,
			EmeraldXlatTabRes,
			80,
			1,
			0,
			128,
		 },
		{
			/* CHLORINE_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + CrateredAlgo,
		  GREEN_BODY),											  /* Color and type/size of planet */
			HighTectonics,														/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,																	   LIGHT),																																													/* Atmosphere and density */
			{
				{CHLORINE, HeavyUseful},
				{CHLORINE, HeavyUseful},
				{HYDROCHLORIC_ACID, HeavyUseful},
				{HYDROCHLORIC_ACID, HeavyUseful},
				{ZINC, LightUseful},
				{GALLIUM, TraceUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			ChlorineColorTabRes,
			ChlorineXlatTabRes,
			500,
			1,
			0,
			190,
		 },
		{
			/* MAGNETIC_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + CrateredAlgo,
		  GREEN_BODY), /* Color and type/size of planet */
			HighTectonics,  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(HIGH_DENSITY,Nothing),																																									/* Atmosphere and density */
			{
				{ZINC, HeavyUseful},
				{NICKEL, MediumUseful},
				{MAGNETIC_MONOPOLES, TraceUseful},
				{NIOBIUM, LightUseful},
				{IRON, HeavyUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			MagneticColorTabRes,
			MagneticXlatTabRes,
			400,
			1,
			100,
			190,
		 },
		{
			/* WATER_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + CrateredAlgo,
		  BLUE_BODY),														/* Color and type/size of planet */
			LowTectonics,										   /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,			  MEDIUM),					   /* Atmosphere and density */
			{
				{IRON, LightUseful},
				{ALUMINUM, LightUseful},
				{TIN, LightUseful},
				{LEAD, LightUseful},
				{URANIUM, TraceUseful},
				{MOLYBDENUM, TraceUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			WaterColorTabRes,
			WaterXlatTabRes,
			500,
			1,
			0,
			190,
		 },
		{
			/* TELLURIC_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + CrateredAlgo,
		  BLUE_BODY),																		   /* Color and type/size of planet */
			LowTectonics,						  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,																			  HEAVY),																																				  /* Atmosphere and density */
			{
				{IRIDIUM, MediumUseful},
				{RUTHENIUM, MediumUseful},
				{THALLIUM, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			TelluricColorTabRes,
			TelluricXlatTabRes,
			250,
			2,
			80,
			200,
		 },
		{
			/* HYDROCARBON_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + TopoAlgo,
		  BLUE_BODY),															  /* Color and type/size of planet */
			LowTectonics,																		  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,																								  LIGHT),																																	 /* Atmosphere and density */
			{
				{HYDROCARBONS, HugeUseful},
				{BISMUTH, LightUseful},
				{TANTALUM, TraceUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			HydrocarbonColorTabRes,
			HydrocarbonXlatTabRes,
			500,
			1,
			0,
			160,
		 },
		{
			/* IODINE_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + CrateredAlgo,
		  GREEN_BODY),																 /* Color and type/size of planet */
			MedTectonics,																		 /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LOW_DENSITY,																					 MEDIUM),																																													 /* Atmosphere and density */
			{
				{IODINE, HeavyUseful},
				{MAGNESIUM, LightUseful},
				{TUNGSTEN, TraceUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			IodineColorTabRes,
			IodineXlatTabRes,
			230,
			2,
			200,
			150,
		 },
		{
			/* VINYLOGOUS_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + CrateredAlgo,
		  PURPLE_BODY),													/* Color and type/size of planet */
			LowTectonics,																		 /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(NORMAL_DENSITY,																						Nothing),																																													 /* Atmosphere and density */
			{
				{TITANIUM, LightUseful},
				{ARSENIC, LightUseful},
				{POTASSIUM, LightUseful},
				{RHENIUM, LightUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			VinylogousColorTabRes,
			VinylogousXlatTabRes,
			400,
			1,
			100,
			190,
		 },
		{
			/* RUBY_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + TopoAlgo,
		  RED_BODY),											   /* Color and type/size of planet */
			NoTectonics,														/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LOW_DENSITY,																	   LIGHT),																																												   /* Atmosphere and density */
			{
				{TZO_CRYSTALS, HugeUseful},
				{NEUTRONIUM, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			RubyColorTabRes,
			RubyXlatTabRes,
			80,
			1,
			0,
			128,
		 },
		{
			/* MAGMA_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + TopoAlgo,
		  RED_BODY),	/* Color and type/size of planet */
			SuperTectonics, /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(HIGH_DENSITY,			LIGHT),																																								   /* Atmosphere and density */
			{
				{LEAD, LightUseful},
				{NICKEL, LightUseful},
				{IRON, LightUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			MagmaColorTabRes,
			MagmaXlatTabRes,
			500,
			1,
			0,
			160,
		 },
		{
			/* MAROON_WORLD */
			uqm::makeByte(LARGE_ROCKY_WORLD + CrateredAlgo,
		  RED_BODY),														/* Color and type/size of planet */
			NoTectonics,										/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(LOW_DENSITY,			   MEDIUM),																					 /* Atmosphere and density */
			{
				{CESIUM, MediumUseful},
				{SILICON, MediumUseful},
				{PHOSPHORUS, MediumUseful},
				{RHODIUM, MediumUseful},
				{CADMIUM, MediumUseful},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			MaroonColorTabRes,
			MaroonXlatTabRes,
			230,
			2,
			200,
			150,
		 },
		//{ /* DIAMOND_WORLD */
		//	MAKE_BYTE (LARGE_ROCKY_WORLD + TopoAlgo,
		//			WHITE_BODY), /* Color and type/size of planet */
		//	NoTectonics, /* Tectonics - Scaled with Earth at 82    */
		//	MAKE_BYTE (LOW_DENSITY, LIGHT), /* Atmosphere and density */
		//	{
		//		{TZO_CRYSTALS, HugeUseful},
		//		{NEUTRONIUM, MediumUseful},
		//		{Nothing, NoDeposit},
		//		{Nothing, NoDeposit},
		//		{Nothing, NoDeposit},
		//		{Nothing, NoDeposit},
		//		{Nothing, NoDeposit},
		//		{Nothing, NoDeposit},
		//	},
		//	GryGasColorTabRes,
		//	RubyXlatTabRes,
		//	80, 1, 0, 114,
		//},

		{
			uqm::makeByte(GAS_GIANT + GasGiantAlgo,
		  BLUE_BODY),																		   /* Color and type/size of planet */
			NoTectonics,						   /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(GAS_DENSITY,																			   HEAVY),																																				   /* Atmosphere and density */
			{
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			BluGasColorTabRes,
			BluGasXlatTabRes,
			10,
			2,
			8,
			29,
		 },
		{
			uqm::makeByte(GAS_GIANT + GasGiantAlgo,
		  CYAN_BODY),															  /* Color and type/size of planet */
			NoTectonics,																		  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(GAS_DENSITY,																								  HEAVY),																																		 /* Atmosphere and density */
			{
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			CyaGasColorTabRes,
			CyaGasXlatTabRes,
			10,
			2,
			8,
			29,
		 },
		{
			uqm::makeByte(GAS_GIANT + GasGiantAlgo,
		  GREEN_BODY),																 /* Color and type/size of planet */
			NoTectonics,																		  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(GAS_DENSITY,																					 HEAVY),																																													 /* Atmosphere and density */
			{
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			GrnGasColorTabRes,
			GrnGasXlatTabRes,
			10,
			2,
			8,
			29,
		 },
		{
			uqm::makeByte(GAS_GIANT + GasGiantAlgo,
		  GRAY_BODY),												  /* Color and type/size of planet */
			NoTectonics,																		/* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(GAS_DENSITY,																						HEAVY),																																												   /* Atmosphere and density */
			{
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			GryGasColorTabRes,
			GryGasXlatTabRes,
			10,
			2,
			8,
			29,
		 },
		{
			uqm::makeByte(GAS_GIANT + GasGiantAlgo,
		  ORANGE_BODY),										   /* Color and type/size of planet */
			NoTectonics,													  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(GAS_DENSITY,																	 HEAVY),																																												   /* Atmosphere and density */
			{
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			OraGasColorTabRes,
			OraGasXlatTabRes,
			10,
			2,
			8,
			29,
		 },
		{
			uqm::makeByte(GAS_GIANT + GasGiantAlgo,
		  PURPLE_BODY),	   /* Color and type/size of planet */
			NoTectonics,	   /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(GAS_DENSITY,HEAVY),																																									   /* Atmosphere and density */
			{
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			PurGasColorTabRes,
			PurGasXlatTabRes,
			10,
			2,
			8,
			29,
		 },
		{
			uqm::makeByte(GAS_GIANT + GasGiantAlgo,
		  RED_BODY),														 /* Color and type/size of planet */
			NoTectonics,									   /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(GAS_DENSITY,			   HEAVY),																						/* Atmosphere and density */
			{
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			RedGasColorTabRes,
			RedGasXlatTabRes,
			10,
			2,
			8,
			29,
		 },
		{
			uqm::makeByte(GAS_GIANT + GasGiantAlgo,
		  VIOLET_BODY),																		   /* Color and type/size of planet */
			NoTectonics,						   /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(GAS_DENSITY,																				HEAVY),																																					 /* Atmosphere and density */
			{
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			VioGasColorTabRes,
			VioGasXlatTabRes,
			10,
			2,
			8,
			29,
		 },
		{
			/* A Jupiter-like World */
			uqm::makeByte(GAS_GIANT + GasGiantAlgo,
		  YELLOW_BODY),															  /* Color and type/size of planet */
			NoTectonics,																		  /* Tectonics - Scaled with Earth at 82    */
			uqm::makeByte(GAS_DENSITY,																								  HEAVY),																																				/* Atmosphere and density */
			{
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
				{Nothing, NoDeposit},
			},
			YelGasColorTabRes,
			YelGasXlatTabRes,
			10,
			2,
			8,
			29,
		 },
};
