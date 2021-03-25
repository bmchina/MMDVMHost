/*
 *   Copyright (C) 2020 by Jonathan Naylor G4KLX
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "M17Convolution.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <cstdlib>

const unsigned int PUNCTURE_LIST_LINK_SETUP[] = {
	3U, 6U, 9U, 12U, 19U, 22U, 25U, 28U, 35U, 38U, 41U, 44U, 51U, 54U, 57U, 64U, 67U, 70U, 73U, 80U, 83U, 86U, 89U, 96U, 99U, 102U,
	105U, 112U, 115U, 118U, 125U, 128U, 131U, 134U, 141U, 144U, 147U, 150U, 157U, 160U, 163U, 166U, 173U, 176U, 179U, 186U, 189U,
	192U, 195U, 202U, 205U, 208U, 211U, 218U, 221U, 224U, 227U, 234U, 237U, 240U, 247U, 250U, 253U, 256U, 263U, 266U, 269U, 272U,
	279U, 282U, 285U, 288U, 295U, 298U, 301U, 308U, 311U, 314U, 317U, 324U, 327U, 330U, 333U, 340U, 343U, 346U, 349U, 356U, 359U,
	362U, 369U, 372U, 375U, 378U, 385U, 388U, 391U, 394U, 401U, 404U, 407U, 410U, 417U, 420U, 423U, 430U, 433U, 436U, 439U, 446U,
	449U, 452U, 455U, 462U, 465U, 468U, 471U, 478U, 481U, 484U};

const unsigned int PUNCTURE_LIST_DATA[] = {
	5U, 11U, 17U, 20U, 23U, 29U, 35U, 46U, 52U, 58U, 61U, 64U, 70U, 76U, 87U, 93U, 99U, 102U, 105U, 111U, 117U, 128U, 134U, 140U,
	143U, 146U, 152U, 158U, 169U, 175U, 181U, 184U, 187U, 193U, 199U, 210U, 216U, 222U, 225U, 228U, 234U, 240U, 251U, 257U, 263U,
	266U, 269U, 275U, 281U, 292U, 298U, 304U, 307U, 310U, 316U, 322U};

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT1(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT1(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

const uint8_t BRANCH_TABLE1[] = {0U, 0U, 0U, 0U, 2U, 2U, 2U, 2U};
const uint8_t BRANCH_TABLE2[] = {0U, 2U, 2U, 0U, 0U, 2U, 2U, 0U};

const unsigned int NUM_OF_STATES_D2 = 8U;
const unsigned int NUM_OF_STATES = 16U;
const uint32_t     M = 4U;
const unsigned int K = 5U;

CM17Convolution::CM17Convolution() :
m_metrics1(NULL),
m_metrics2(NULL),
m_oldMetrics(NULL),
m_newMetrics(NULL),
m_decisions(NULL),
m_dp(NULL)
{
	m_metrics1  = new uint16_t[16U];
	m_metrics2  = new uint16_t[16U];
	m_decisions = new uint64_t[300U];
}

CM17Convolution::~CM17Convolution()
{
	delete[] m_metrics1;
	delete[] m_metrics2;
	delete[] m_decisions;
}

void CM17Convolution::encodeLinkSetup(const unsigned char* in, unsigned char* out) const
{
	assert(in != NULL);
	assert(out != NULL);

	unsigned char temp1[31U];
	::memset(temp1, 0x00U, 31U);
	::memcpy(temp1, in, 30U);

	unsigned char temp2[61U];
	encode(temp1, temp2, 244U);

	unsigned int n = 0U;
	unsigned int index = 0U;
	for (unsigned int i = 0U; i < 488U; i++) {
		if (i != PUNCTURE_LIST_LINK_SETUP[index]) {
			bool b = READ_BIT1(temp2, i);
			WRITE_BIT1(out, n, b);
			n++;
		} else {
			index++;
		}
	}
}

void CM17Convolution::encodeData(const unsigned char* in, unsigned char* out) const
{
	assert(in != NULL);
	assert(out != NULL);

	unsigned char temp1[21U];
	::memset(temp1, 0x00U, 21U);
	::memcpy(temp1, in, 20U);

	unsigned char temp2[41U];
	encode(temp1, temp2, 164U);

	unsigned int n = 0U;
	unsigned int index = 0U;
	for (unsigned int i = 0U; i < 328U; i++) {
		if (i != PUNCTURE_LIST_DATA[index]) {
			bool b = READ_BIT1(temp2, i);
			WRITE_BIT1(out, n, b);
			n++;
		} else {
			index++;
		}
	}
}

void CM17Convolution::decodeLinkSetup(const unsigned char* in, unsigned char* out)
{
	assert(in != NULL);
	assert(out != NULL);

	uint8_t temp[500U];

	unsigned int n = 0U;
	unsigned int index = 0U;
	for (unsigned int i = 0U; i < 368U; i++) {
		if (n == PUNCTURE_LIST_LINK_SETUP[index]) {
			temp[n++] = 1U;
			index++;
		}

		bool b = READ_BIT1(in, i);
		temp[n++] = b ? 2U : 0U;
	}

	for (unsigned int i = 0U; i < 8U; i++)
		temp[n++] = 0U;

	start();

	n = 0U;
	for (unsigned int i = 0U; i < 244U; i++) {
		uint8_t s0 = temp[n++];
		uint8_t s1 = temp[n++];

		decode(s0, s1);
	}

	chainback(out, 240U);
}

void CM17Convolution::decodeData(const unsigned char* in, unsigned char* out)
{
	assert(in != NULL);
	assert(out != NULL);

	uint8_t temp[350U];

	unsigned int n = 0U;
	unsigned int index = 0U;
	for (unsigned int i = 0U; i < 272U; i++) {
		if (n == PUNCTURE_LIST_DATA[index]) {
			temp[n++] = 1U;
			index++;
		}

		bool b = READ_BIT1(in, i);
		temp[n++] = b ? 2U : 0U;
	}

	for (unsigned int i = 0U; i < 8U; i++)
		temp[n++] = 0U;

	start();

	n = 0U;
	for (unsigned int i = 0U; i < 164U; i++) {
		uint8_t s0 = temp[n++];
		uint8_t s1 = temp[n++];

		decode(s0, s1);
	}

	chainback(out, 160U);
}

void CM17Convolution::start()
{
	::memset(m_metrics1, 0x00U, NUM_OF_STATES * sizeof(uint16_t));
	::memset(m_metrics2, 0x00U, NUM_OF_STATES * sizeof(uint16_t));

	m_oldMetrics = m_metrics1;
	m_newMetrics = m_metrics2;
	m_dp = m_decisions;
}

void CM17Convolution::decode(uint8_t s0, uint8_t s1)
{
  *m_dp = 0U;

  for (uint8_t i = 0U; i < NUM_OF_STATES_D2; i++) {
    uint8_t j = i * 2U;

    uint16_t metric = std::abs(BRANCH_TABLE1[i] - s0) + std::abs(BRANCH_TABLE2[i] - s1);

    uint16_t m0 = m_oldMetrics[i] + metric;
    uint16_t m1 = m_oldMetrics[i + NUM_OF_STATES_D2] + (M - metric);
    uint8_t decision0 = (m0 >= m1) ? 1U : 0U;
    m_newMetrics[j + 0U] = decision0 != 0U ? m1 : m0;

    m0 = m_oldMetrics[i] + (M - metric);
    m1 = m_oldMetrics[i + NUM_OF_STATES_D2] + metric;
    uint8_t decision1 = (m0 >= m1) ? 1U : 0U;
    m_newMetrics[j + 1U] = decision1 != 0U ? m1 : m0;

    *m_dp |= (uint64_t(decision1) << (j + 1U)) | (uint64_t(decision0) << (j + 0U));
  }

  ++m_dp;

  assert((m_dp - m_decisions) <= 300);

  uint16_t* tmp = m_oldMetrics;
  m_oldMetrics = m_newMetrics;
  m_newMetrics = tmp;
}

void CM17Convolution::chainback(unsigned char* out, unsigned int nBits)
{
	assert(out != NULL);

	uint32_t state = 0U;

	while (nBits-- > 0) {
		--m_dp;

		uint32_t  i = state >> (9 - K);
		uint8_t bit = uint8_t(*m_dp >> i) & 1;
		state = (bit << 7) | (state >> 1);

		WRITE_BIT1(out, nBits, bit != 0U);
	}
}

void CM17Convolution::encode(const unsigned char* in, unsigned char* out, unsigned int nBits) const
{
	assert(in != NULL);
	assert(out != NULL);
	assert(nBits > 0U);

	uint8_t d1 = 0U, d2 = 0U, d3 = 0U, d4 = 0U;
	uint32_t k = 0U;
	for (unsigned int i = 0U; i < nBits; i++) {
		uint8_t d = READ_BIT1(in, i) ? 1U : 0U;

		uint8_t g1 = (d + d3 + d4) & 1;
		uint8_t g2 = (d + d1 + d2 + d4) & 1;

		d4 = d3;
		d3 = d2;
		d2 = d1;
		d1 = d;

		WRITE_BIT1(out, k, g1 != 0U);
		k++;

		WRITE_BIT1(out, k, g2 != 0U);
		k++;
	}
}
