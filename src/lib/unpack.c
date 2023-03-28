/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Peter Bjorklund. All rights reserved.
 *  Licensed under the MIT License. See LICENSE in the project root for license information.
 *--------------------------------------------------------------------------------------------*/
#include <swamp-runtime/log.h>
#include <swamp-runtime/types.h>
#include <swamp-typeinfo-serialize/deserialize.h>
#include <swamp-typeinfo/typeinfo.h>

#include <raff/raff.h>
#include <raff/tag.h>

#include <string.h> // strcmp
#include <swamp-runtime/fixup.h>

static int readAndVerifyRaffHeader(SwampOctetStream* s)
{
    const uint8_t* p = &s->octets[s->position];

    int count = raffReadAndVerifyHeader(p, s->octetCount - s->position);
    if (count < 9) {
        return -1;
    }

    s->position += count;

    return 0;
}

static int readAndVerifyRaffChunkHeader(SwampOctetStream* s, RaffTag icon, RaffTag name)
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

static int readRaffMarker(SwampOctetStream* s, RaffTag tag, int verboseLevel)
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

static int verifyMarker(SwampOctetStream* s, RaffTag expectedMarker, int verboseFlag)
{
    RaffTag marker;

    readRaffMarker(s, marker, verboseFlag);

    if (!raffTagEqual(marker, expectedMarker)) {
        return -1;
    }

    return 0;
}

static int readTypeInformation(SwampUnpack* self, SwampOctetStream* s, int verboseFlag, struct ImprintAllocator* allocator)
{
    RaffTag expectedPacketName = {'s', 't', 'i', '0'};
    RaffTag expectedPacketIcon = {0xF0, 0x9F, 0x93, 0x9C};

    int upcomingOctetsInChunk = readAndVerifyRaffChunkHeader(s, expectedPacketIcon, expectedPacketName);
    if (upcomingOctetsInChunk <= 0) {
        return upcomingOctetsInChunk;
    }

    int errorCode = swtisDeserialize(&s->octets[s->position], upcomingOctetsInChunk, &self->typeInfoChunk, allocator);
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

static int readConstantStaticMemory(SwampUnpack* self, SwampOctetStream* s, int verboseFlag)
{
    RaffTag expectedPacketName = {'d', 'm', 'e', '1'};
    RaffTag expectedPacketIcon = {0xF0, 0x9F, 0x92, 0xBB};

    int upcomingOctetsInChunk = readAndVerifyRaffChunkHeader(s, expectedPacketIcon, expectedPacketName);
    if (upcomingOctetsInChunk <= 0) {
        return upcomingOctetsInChunk;
    }

    if (self->verboseFlag) {
        SWAMP_LOG_INFO("read dynamic memory %d", upcomingOctetsInChunk);
        SWAMP_LOG_INFO("done!\n");
    }

    self->constantStaticMemoryMaxSize = 256 * 1024;
    if (upcomingOctetsInChunk > self->constantStaticMemoryMaxSize) {
        SWAMP_ERROR("too much static memory")
    }
    self->constantStaticMemoryOctets = tc_malloc(self->constantStaticMemoryMaxSize);
    tc_memcpy_octets((void*)self->constantStaticMemoryOctets, &s->octets[s->position], upcomingOctetsInChunk);
    self->constantStaticMemorySize = upcomingOctetsInChunk;

    s->position += upcomingOctetsInChunk;

    return 0;
}

static int readLedger(SwampUnpack* self, SwampOctetStream* s, SwampResolveExternalFunction bindFn, int verboseFlag)
{
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

    self->ledger.ledgerOctets = tc_malloc(upcomingOctetsInChunk);
    tc_memcpy_octets((void*)self->ledger.ledgerOctets, &s->octets[s->position], upcomingOctetsInChunk);
    self->ledger.ledgerSize = upcomingOctetsInChunk;
    self->ledger.constantStaticMemory = self->constantStaticMemoryOctets;

    s->position += upcomingOctetsInChunk;

    return 0;
}

int swampUnpackSwampOctetStream(SwampUnpack* self, SwampOctetStream* s, SwampResolveExternalFunction bindFn, int verboseFlag, struct ImprintAllocator* allocator)
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

    if ((errorCode = readTypeInformation(self, s, verboseFlag, allocator)) < 0) {
        SWAMP_LOG_SOFT_ERROR("problem with type information chunk");
        return errorCode;
    }

    if ((errorCode = readConstantStaticMemory(self, s, verboseFlag)) != 0) {
        SWAMP_LOG_SOFT_ERROR("problem with code chunk");
        return errorCode;
    }

    if ((errorCode = readLedger(self, s, bindFn, verboseFlag)) != 0) {
        SWAMP_LOG_SOFT_ERROR("problem with code chunk");
        return errorCode;
    }

    self->entry = swampFixupLedger(self->constantStaticMemoryOctets, bindFn, (SwampConstantLedgerEntry*) self->ledger.ledgerOctets);
    if (self->entry == 0) {
        return -1;
    }
    return 0;
}

static void readWholeFile(const char* filename, SwampOctetStream* stream)
{
    FILE* fp = fopen(filename, "rb");
    if (fp == 0) {
        SWAMP_LOG_INFO("swampUnpack readWholeFile error:%s", filename);
        return;
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        SWAMP_LOG_INFO("swampUnpack seek err:");
        return;
    }

    long bufsize = ftell(fp);
    if (bufsize == -1) {
        fclose(fp);
        SWAMP_LOG_INFO("swampUnpack bufsize error");
        return;
    }

    uint8_t* source = tc_malloc(sizeof(uint8_t) * (bufsize));

    if (fseek(fp, 0L, SEEK_SET) != 0) {
        fclose(fp);
        SWAMP_LOG_INFO("swampUnpack seek error");
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
    tc_free((void*)self->constantStaticMemoryOctets);
    tc_free((void*)self->ledger.ledgerOctets);
    swtiChunkDestroy(&self->typeInfoChunk);
}

int swampUnpackFilename(SwampUnpack* self, const char* pack_filename, SwampResolveExternalFunction bindFn, int verboseFlag, struct ImprintAllocator* allocator)
{
    SwampOctetStream stream;
    SwampOctetStream* s = &stream;
    readWholeFile(pack_filename, s);
    int result = swampUnpackSwampOctetStream(self, s, bindFn, verboseFlag, allocator);
    tc_free((void*) s->octets);
    return result;
}

const SwampFunc* swampUnpackEntryPoint(SwampUnpack* self)
{
    return self->entry;
}
