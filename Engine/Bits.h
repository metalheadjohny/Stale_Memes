#pragma once
#include <stdint.h>



namespace Bits
{
	// Absolute magic to find the most significant set bit, I barely understand it
	uint32_t msbDeBruijn32(uint32_t v)
	{
		constexpr int MultiplyDeBruijnBitPosition[32] =
		{
			0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
			8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
		};

		v |= v >> 1; // first round down to one less than a power of 2
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;

		return MultiplyDeBruijnBitPosition[(uint32_t)(v * 0x07C4ACDDU) >> 27];
	}


	// Used for Morton order, @todo make smarter so it works on 8 and 32 bit as well
	inline uint32_t intertwine(uint16_t a, uint16_t b)
	{
		constexpr uint32_t B[]			{ 0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF };
		constexpr uint32_t bitOffset[]	{ 1, 2, 4, 8 };

		// Gives wiggle room to bits, but we enforce they are at most 16 bits by parameter type
		uint32_t x = a;
		uint32_t y = b;

		/* Example: 8-bit values intertwined into 16 bits, first line (same for x and y)
			0000 0000 0101 1101	-> SHIFT 4 LEFT
			0000 0101 1101 0000 -> OR WITH ORIGINAL VALUE
			0000 0101 1101 1101	-> MASK EVERY OTHER BYTE USING AND WITH MASK 0000 1111 0000 1111
			0000 0101 0000 1101	-> NOW THE BITS ARE SPLIT APART! REPEAT IN FINER STEPS UNTIL DONE
		*/

		x = (x | (x << bitOffset[3])) & B[3];
		x = (x | (x << bitOffset[2])) & B[2];
		x = (x | (x << bitOffset[1])) & B[1];
		x = (x | (x << bitOffset[0])) & B[0];

		y = (y | (y << bitOffset[3])) & B[3];
		y = (y | (y << bitOffset[2])) & B[2];
		y = (y | (y << bitOffset[1])) & B[1];
		y = (y | (y << bitOffset[0])) & B[0];

		return x | (y << 1);	// Y is shifted once to the left, y msb is result msb
	}
}