/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/log.h>
#include <swamp-runtime/print.h>
#include <swamp-runtime/types.h>
#include <swamp-typeinfo/deserialize.h>
#include <swamp-typeinfo/typeinfo.h>

#include <clog/clog.h>
#include <flood/out_stream.h>
#include <raff/raff.h>
#include <raff/tag.h>

#include <string.h> // strcmp
#include <swamp-runtime/swamp_unpack.h>

void swampOctetStreamInit(SwampOctetStream* self, const uint8_t* octets, size_t octet_count)
{
    self->octets = octets;
    self->octetCount = octet_count;
    self->position = 0;
}

static inline uint8_t read_uint8(SwampOctetStream* s)
{
    if (s->position >= s->octetCount) {
        char* p = 0;
        *p = -1;
    }

    return s->octets[s->position++];
}

static inline int32_t read_int32(SwampOctetStream* s)
{
    if (s->position + 4 >= s->octetCount) {
        char* p = 0;
        *p = -1;
    }
    uint32_t h0 = read_uint8(s);
    uint32_t h1 = read_uint8(s);
    uint32_t h2 = read_uint8(s);
    uint32_t h3 = read_uint8(s);
    uint32_t t = (h0 << 24) | (h1 << 16) | (h2 << 8) | h3;
    return t;
}

static inline int32_t read_uint32(SwampOctetStream* s)
{
    if (s->position + 4 >= s->octetCount) {
        char* p = 0;
        *p = -1;
    }
    uint32_t h0 = read_uint8(s);
    uint32_t h1 = read_uint8(s);
    uint32_t h2 = read_uint8(s);
    uint32_t h3 = read_uint8(s);
    uint32_t t = (h0 << 24) | (h1 << 16) | (h2 << 8) | h3;
    return t;
}

static inline int32_t read_uint16(SwampOctetStream* s)
{
    if (s->position + 4 >= s->octetCount) {
        char* p = 0;
        *p = -1;
    }
    uint8_t h2 = read_uint8(s);
    uint8_t h3 = read_uint8(s);
    uint32_t t = (h2 << 8) | h3;
    return t;
}

static inline void read_string(SwampOctetStream* s, char* buf)
{
    uint8_t len = read_uint8(s);
    if (s->position + len >= s->octetCount) {
        char* p = 0;
        *p = -1;
    }

    const char* raw = (const char*) &s->octets[s->position];
    for (int i = 0; i < len; i++) {
        buf[i] = raw[i];
    }
    s->position += len;
    buf[len] = 0;
}

static inline uint8_t read_count(SwampOctetStream* s)
{
    return read_uint8(s);
}

static inline uint32_t read_dword_count(SwampOctetStream* s)
{
    return read_uint32(s);
}

static void read_type_ref(SwampOctetStream* s, uint16_t* typeRef)
{
    *typeRef = read_uint16(s);
}


int readAndVerifyRaffHeader(SwampOctetStream* s)
{
    const uint8_t* p = &s->octets[s->position];

    int count = raffReadAndVerifyHeader(p, s->octetCount - s->position);
    if (count < 9) {
        return -1;
    }

    s->position += count;

    return 0;
}

int readAndVerifyRaffChunkHeader(SwampOctetStream* s, RaffTag icon, RaffTag name)
{
    const uint8_t* p = &s->octets[s->position];

    RaffTag foundIcon, foundName;

    uint32_t chunkSize;

    int count = raffReadChunkHeader(p, s->octetCount - s->position, foundIcon, foundName, &chunkSize);
    if (count < 0) {
        return count;
    }

    if (!raffTagEqual(icon, foundIcon)) {
        SWAMP_LOG_SOFT_ERROR("icon is wrong");
        return -2;
    }

    if (!raffTagEqual(name, foundName)) {
        SWAMP_LOG_SOFT_ERROR("tag is wrong");
        return -3;
    }

    s->position += count;

    return chunkSize;
}

int readRaffMarker(SwampOctetStream* s, RaffTag tag, int verboseLevel)
{
    int count = raffReadMarker(&s->octets[s->position], s->octetCount - s->position, tag);
    if (count < 0) {
        return count;
    }

    if (SWAMP_LOG_SHOULD_LOG(verboseLevel)) {
        SWAMP_LOG_DEBUG("");
        char temp[64];
        SWAMP_LOG_DEBUG("tag: %s", raffTagToString(temp, 64, tag));
    }

    s->position += count;

    return count;
}

int verifyMarker(SwampOctetStream* s, RaffTag expectedMarker, int verboseFlag)
{
    RaffTag marker;

    readRaffMarker(s, marker, verboseFlag);

    if (!raffTagEqual(marker, expectedMarker)) {
        return -1;
    }

    return 0;
}

int readTypeInformation(SwampUnpack* self, SwampOctetStream* s, int verboseFlag)
{
    RaffTag expectedPacketName = {'s', 't', 'i', '0'};
    RaffTag expectedPacketIcon = {0xF0, 0x9F, 0x93, 0x9C};

    int upcomingOctetsInChunk = readAndVerifyRaffChunkHeader(s, expectedPacketIcon, expectedPacketName);
    if (upcomingOctetsInChunk <= 0) {
        return upcomingOctetsInChunk;
    }

    int errorCode = swtiDeserialize(&s->octets[s->position], upcomingOctetsInChunk, &self->typeInfoChunk);
    if (errorCode < 0) {
        CLOG_SOFT_ERROR("swtiDeserialize: error %d", errorCode);
        return errorCode;
    }

    if (verboseFlag) {
        swtiChunkDebugOutput(&self->typeInfoChunk, 0, "readTypeInformation");
    }

    s->position += upcomingOctetsInChunk;

    return 0;
}

int readDynamicMemory(SwampUnpack* self, SwampOctetStream* s, int verboseFlag)
{
    int errorCode;

    RaffTag expectedPacketName = {'d', 'm', 'e', '0'};
    RaffTag expectedPacketIcon = {0xF0, 0x9F, 0x92, 0xBB};

    int upcomingOctetsInChunk = readAndVerifyRaffChunkHeader(s, expectedPacketIcon, expectedPacketName);
    if (upcomingOctetsInChunk <= 0) {
        return upcomingOctetsInChunk;
    }

    if (self->verboseFlag) {
        SWAMP_LOG_INFO("read dynamic memory %d", upcomingOctetsInChunk);
        SWAMP_LOG_INFO("done!\n");
    }

    self->dynamicMemoryMaxSize = 128 * 1024;
    self->dynamicMemoryOctets = malloc(self->dynamicMemoryMaxSize );
    tc_memcpy_octets(self->dynamicMemoryOctets, &s->octets[s->position], upcomingOctetsInChunk);
    self->dynamicMemorySize = upcomingOctetsInChunk;

    s->position += upcomingOctetsInChunk;

    return 0;
}

int readLedger(SwampUnpack* self, SwampOctetStream* s, int verboseFlag)
{
    int errorCode;

    RaffTag expectedPacketName = {'l', 'd', 'g', '0'};
    RaffTag expectedPacketIcon = {0xF0, 0x9F, 0x97, 0x92};

    int upcomingOctetsInChunk = readAndVerifyRaffChunkHeader(s, expectedPacketIcon, expectedPacketName);
    if (upcomingOctetsInChunk <= 0) {
        return upcomingOctetsInChunk;
    }

    if (self->verboseFlag) {
        SWAMP_LOG_INFO("read ledger memory %d", upcomingOctetsInChunk);
        SWAMP_LOG_INFO("done!\n");
    }

    self->ledgerOctets = malloc(upcomingOctetsInChunk);
    tc_memcpy_octets(self->ledgerOctets, &s->octets[s->position], upcomingOctetsInChunk);
    self->ledgerSize = upcomingOctetsInChunk;

    s->position += upcomingOctetsInChunk;

    return 0;
}

int swampUnpackSwampOctetStream(SwampUnpack* self, SwampOctetStream* s, int verboseFlag)
{
    int errorCode = readAndVerifyRaffHeader(s);
    if (errorCode != 0) {
        return errorCode;
    }

    RaffTag expectedPacketName = {'s', 'p', 'k', '5'};
    RaffTag expectedPacketIcon = {0xF0, 0x9F, 0x93, 0xA6};

    int upcomingOctetsInChunk = readAndVerifyRaffChunkHeader(s, expectedPacketIcon, expectedPacketName);
    if (upcomingOctetsInChunk < 0) {
        return upcomingOctetsInChunk;
    }

    if ((errorCode = readTypeInformation(self, s, verboseFlag)) < 0) {
        SWAMP_LOG_SOFT_ERROR("problem with type information chunk");
        return errorCode;
    }

    if ((errorCode = readDynamicMemory(self, s, verboseFlag)) != 0) {
        SWAMP_LOG_SOFT_ERROR("problem with code chunk");
        return errorCode;
    }

    if ((errorCode = readLedger(self, s, verboseFlag)) != 0) {
        SWAMP_LOG_SOFT_ERROR("problem with code chunk");
        return errorCode;
    }

    return 0;
}

static void readWholeFile(const char* filename, SwampOctetStream* stream)
{
    uint8_t* source = 0;
    FILE* fp = fopen(filename, "rb");
    if (fp == 0) {
        SWAMP_LOG_INFO("errror:%s\n", filename);
        return;
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        SWAMP_LOG_INFO("seek err:\n");
        return;
    }

    long bufsize = ftell(fp);
    if (bufsize == -1) {
        SWAMP_LOG_INFO("bufsize error\n");
        return;
    }

    source = malloc(sizeof(uint8_t) * (bufsize));

    if (fseek(fp, 0L, SEEK_SET) != 0) {
        SWAMP_LOG_INFO("seek error\n");
        return;
    }

    size_t new_len = fread(source, sizeof(uint8_t), bufsize, fp);
    stream->octets = source;
    stream->octetCount = new_len;
    stream->position = 0;

    fclose(fp);
}

void swampUnpackInit(SwampUnpack* self, int verbose_flag)
{
    self->entry = 0;
    self->verboseFlag = verbose_flag;
}

void swampUnpackFree(SwampUnpack* self)
{
    free(self->dynamicMemoryOctets);
    free(self->ledgerOctets);
    swtiChunkDestroy(&self->typeInfoChunk);
}

int swampUnpackFilename(SwampUnpack* self, const char* pack_filename, int verboseFlag)
{
    SwampOctetStream stream;
    SwampOctetStream* s = &stream;
    readWholeFile(pack_filename, s);
    int result = swampUnpackSwampOctetStream(self, s, verboseFlag);
    free((void*) s->octets);
    return result;
}

SwampFunc* swampUnpackEntryPoint(SwampUnpack* self)
{
    return self->entry;
}
