/*
 * Sux: Succinct data structures
 *
 * Copyright (C) 2009-2013 Sebastiano Vigna
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published by the Free
 *  Software Foundation; either version 3 of the License, or (at your option)
 *  any later version.
 *
 *  This library is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */


using namespace std;

#ifdef VERBOSE
#include <cstdio>
#endif
#include <cassert>
#include <cstring>
#include <vector>
#include <algorithm>
#include "bal_paren.h"

bal_paren::bal_paren() {}

bal_paren::bal_paren( const uint64_t * const bits, const uint64_t num_bits ) {
	this->bits = bits;
	num_words = ( num_bits + 63 ) / 64;

	uint8_t* count = new uint8_t[ num_words ]();
	uint8_t* residual = new uint8_t[ num_words ]();

	vector<uint64_t> opening_pioneers(0);
	vector<uint64_t> opening_pioneers_matches(0);

	int first_nonzero_block = -1;
	for( int block = num_words; block-- != 0; ) {
		const int l = min( 64ULL, num_bits - block * 64ULL );
#ifdef VERBOSE
		if ( block % 100000 == 0 ) {
			fprintf( stderr, "." );
			fflush( stderr );
		}
#endif

		if ( block != num_words -1 ) {
			int excess = 0;
			int countFarOpening = count_far_open( bits[ block ], l );
			bool somethingAdded = false;

			for( int j = l; j-- != 0; ) {
				if ( ( bits[ block ] & 1ULL << j ) == 0 ) {
					if ( excess > 0 ) excess = -1;
					else --excess;
				}
				else {
					if ( ++excess > 0 ) {
						assert( first_nonzero_block >= 0 );
						// Find block containing matching far close parenthesis
						countFarOpening--;
						if ( --count[ first_nonzero_block ] == 0 || countFarOpening == 0 ) {
							// This is an opening pioneer
							opening_pioneers.push_back( block * 64ULL + j );
							opening_pioneers_matches.push_back( - ( block * 64ULL + j ) + ( first_nonzero_block * 64ULL + find_far_close( bits[ first_nonzero_block ], residual[ first_nonzero_block ]) ) );
						}
						residual[ first_nonzero_block ]++;

						if ( block != 0 || j != 0 ) {
							while( count[ first_nonzero_block ] == 0 ) first_nonzero_block++;
							assert( first_nonzero_block < num_words );
						}
					}
				}
			}
		}
		count[ block ] = count_far_close( bits[ block ], l );
		if ( count[ block ] != 0 ) first_nonzero_block = block;
	}
#ifdef VERBOSE
	fprintf( stderr, "\n" );
#endif
	for( int i = num_words; i-- != 0; ) assert( count[ i ] == 0 );

	opening_pioneers_bits = new uint64_t[ num_words ]();


	for( uint64_t i = 0; i < opening_pioneers.size(); i++ ) set( opening_pioneers_bits, opening_pioneers[ i ] );
	opening_pioneers_rank = new rank9( opening_pioneers_bits, num_bits );

	reverse(opening_pioneers.begin(), opening_pioneers.end());
	this->opening_pioneers = new uint64_t[ opening_pioneers.size() ];
	copy(opening_pioneers.begin(), opening_pioneers.end(), this->opening_pioneers );

	reverse(opening_pioneers_matches.begin(), opening_pioneers_matches.end());
	this->opening_pioneers_matches = new uint64_t[ opening_pioneers_matches.size() ];
	copy(opening_pioneers_matches.begin(), opening_pioneers_matches.end(), this->opening_pioneers_matches );

#ifndef NDEBUG
	for( uint64_t i = 0; i < num_bits; i++ ) {
		if (bits[ i / 64 ] & 1ULL << (i & 63)) {
			int64_t c = 1;
			uint64_t j;
			for( j = i + 1; j < num_bits; j++ ) {
				if ( bits[ j / 64 ] & 1ULL << (j & 63) ) c++;
				else c--;

				if ( c == 0 ) break;
			}
#ifdef VERBOSE
			if ( find_close(i) != j ) printf( "find_close(%lld) = %lld != %lld\n", i, find_close(i), j);
#endif
			assert(c == 0);
			assert(find_close(i) == j);
		}
	}
#endif

}

bal_paren::~bal_paren() {
	delete [] opening_pioneers;
	delete [] opening_pioneers_matches;
}

long long far_find_close;

uint64_t bal_paren::find_close( const uint64_t pos ) {
		const int word = (int)( pos / 64 );
		const int bit = (int)( pos & 63 );
		assert( ( bits[ word ] & 1ULL << bit ) != 0 );

		int result = find_near_close( bits[ word ] >> bit );

		if ( result < 64 - bit ) {
			return word * 64ULL + bit + result;
		}

		far_find_close++;

		const long pioneerIndex = opening_pioneers_rank->rank( pos + 1 ) - 1;
		const long pioneer = opening_pioneers[ pioneerIndex ];
		const long match = pioneer + opening_pioneers_matches[ pioneerIndex ];

		if ( pos == pioneer ) {
			return match;
		}

		int dist = (int)( pos - pioneer );

		int e = 2 * __builtin_popcountll( ( bits[ word ] >> ( pioneer % 64 ) ) & ( 1ULL << dist ) - 1 ) - dist;

		const int matchWord = (int)( match / 64 );
		const int matchBit = (int)( match % 64 );

		const int numFarClose = matchBit - 2 * __builtin_popcountll( bits[ matchWord ] & ( 1ULL << matchBit ) - 1 );
		return matchWord * 64ULL + find_far_close( bits[ matchWord ], numFarClose - e );

}

uint64_t bal_paren::bit_count() {
	return -1;
}

void bal_paren::print_counts() {}
