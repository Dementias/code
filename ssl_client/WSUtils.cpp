#include <iostream>
#include <string>
#include <time.h>

void ReverseByteOrder(uint8_t *val, uint64_t size)
{
	uint8_t saved;
	for (uint64_t i = 0; i < size / 2; ++i)
	{
		saved = val[size - 1 - i];
		val[size - 1 - i] = val[i];
		val[i] = saved;
	}

}

bool IsPrefix(const unsigned char *s, const unsigned char *prefix, const uint64_t prefix_size)
{
	for (int i = 0; i < prefix_size; ++i)
	{
		if (s[i] != prefix[i])
		{
			return false;
		}
	}
	return true;
}

void FillWithRandomBytes(unsigned char *a, size_t N)
{
	for (int i = 0; i < N; ++i)
	{
		a[i] = rand() % 256;
	}
}

uint8_t DecryptHeader(uint8_t *header, uint64_t *len)
{
	uint8_t opc;
	uint8_t pay_len;
	bool mask_present;

	opc = (header[0]) & 15;
	pay_len = header[1] & 127;

	if (header[1] & 128)
	{
		mask_present = true;
	}
	else
	{
		mask_present = false;
	}

	uint64_t actual_length;
	uint8_t mask_index;

	if (pay_len <= 125)
	{
		actual_length = pay_len;
		mask_index = 2;
	}

	else if (pay_len == 126)
	{
		actual_length = header[3] + (header[2] << 8);
		mask_index = 4;
	}

	else
	{
		actual_length = 0;
		for (int i = 0; i < 8; ++i)
		{
			actual_length = actual_length << 8;
			actual_length += header[2 + i];
		}
	}

	std::cout << "actual_length = " << actual_length << std::endl;

	if (mask_present)
	{
		return mask_index + 4;
	}

	*len = actual_length;
	return mask_index;
}

void GenMaskingKey(unsigned char *key)
{
	FillWithRandomBytes(key, 4);
}

int GenHeader(unsigned char *buffer, uint64_t len)
{

	uint8_t header_size;
	uint8_t starting_index = 0;

	if (len <= 125)
	{
		header_size = 6;
		buffer[0] = (1 << 7) + 1;
		buffer[1] = len + (1 << 7);
		GenMaskingKey(buffer + 2);
	}

	else if (len <= ((1 << 15) - 1))
	{

		header_size = 8;
		buffer[0] = (1 << 7) + 1;
		buffer[1] = 126 + (1 << 7);
		uint8_t *t = (uint8_t *)&len;
		for (int i = 0; i < 2; ++i)
		{
			buffer[2 + i] = t[i];
		}
		GenMaskingKey(buffer + 4);
	}

	else if (len <= 1 << 63 - 1)
	{
		header_size = 14;
		buffer[0] = (1 << 7) + 1;
		buffer[1] = 127 + (1 << 7);
		uint8_t *t = (uint8_t *)&len;
		for (int i = 0; i < 8; ++i)
		{
			buffer[2 + i] = t[i];
		}
		GenMaskingKey(buffer + 10);
	}

	else
	{
		/* trading_data fragmentation is needed */
		header_size = 14;
	}

	return header_size;
}

void PutMaskKeyOnBuffer(unsigned char *buffer, unsigned char *masking_key, int len)
{
	for (int i = 0; i < len; ++i)
	{
		buffer[i] ^= masking_key[i % 4];
	}
}