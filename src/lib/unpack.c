/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/log.h>
#include <swamp-runtime/types.h>
#include <swamp-typeinfo/deserialize.h>
#include <swamp-typeinfo/typeinfo.h>

#include <clog/clog.h>
#include <raff/raff.h>
#include <raff/tag.h>

#include <string.h> // strcmp
#include <swamp-runtime/swamp_unpack.h>


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
