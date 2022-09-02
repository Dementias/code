#ifndef SSL_CLIENT_WEBSOCKETPROTOCOL_H
#define SSL_CLIENT_WEBSOCKETPROTOCOL_H

#include <cstdint>

const uint8_t MAX_HEADER_SIZE = 14;

bool IsPrefix(const unsigned char *, const unsigned char *, const uint64_t);

void PrnBuffer(const unsigned char *, size_t);

void ReverseByteOrder(uint8_t *, uint64_t);

void FillWithRandomBytes(unsigned char *a, std::size_t N);

uint8_t DecryptHeader(uint8_t *, uint64_t *);

void GenMaskingKey(unsigned char *);

int GenHeader(unsigned char *, uint64_t);

void PutMaskKeyOnBuffer(unsigned char *, unsigned char *, int);

#endif //SSL_CLIENT_WEBSOCKETPROTOCOL_H