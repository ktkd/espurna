/*

RTMEM MODULE

Copyright (C) 2019 by Maxim Prokhorov <prokhorov dot max at outlook dot com>

*/

#include "espurna.h"
#include "rtcmem.h"

static constexpr uint32_t RtcmemMagic { RTCMEM_MAGIC };

static constexpr uintptr_t RtcmemBlocks { RTCMEM_BLOCKS };
static constexpr uintptr_t RtcmemBegin { RTCMEM_ADDR };
static constexpr uintptr_t RtcmemEnd { RtcmemBegin + RtcmemBlocks };

volatile RtcmemData* Rtcmem = reinterpret_cast<volatile RtcmemData*>(RtcmemBegin);

namespace espurna {
namespace peripherals {
namespace {

namespace rtc {
namespace internal {

bool status = false;

} // namespace internal

void erase() {
    DEBUG_MSG_P(PSTR("[RTCMEM] Erasing start=0x%08x end=0x%08x\n"),
        RtcmemBegin, RtcmemEnd);

    auto begin = reinterpret_cast<volatile uint32_t*>(RtcmemBegin);
    auto end = reinterpret_cast<volatile uint32_t*>(RtcmemEnd);
    for (auto it = begin; it != end; ++it) {
        *it = 0;
    }
}

void init() {
    erase();
    Rtcmem->magic = RtcmemMagic;
}

// Treat memory as dirty on cold boot, hardware wdt reset and rst pin
bool status() {
    bool readable;

    switch (systemResetReason()) {
        case REASON_EXT_SYS_RST:
        case REASON_DEFAULT_RST:
            readable = false;
            break;
        default:
            readable = true;
    }

    readable = readable
        && (RtcmemMagic == Rtcmem->magic);

    return readable;
}

#if TERMINAL_SUPPORT
namespace terminal {

PROGMEM_STRING(Init, "RTCMEM.INIT");

void init(::terminal::CommandContext&& ctx) {
    rtc::init();
    terminalOK(ctx);
}

PROGMEM_STRING(Dump, "RTCMEM.DUMP");

void dump(::terminal::CommandContext&& ctx) {
    ctx.output.printf_P(PSTR("boot_status=%s status=%s capacity=%u\n"),
        internal::status ? "OK" : "INIT",
        status() ? "OK" : "INIT",
        RtcmemSize);

    constexpr size_t Blocks = 8;
    constexpr auto Increment = alignof(uint32_t) * Blocks;

    alignas(4) uint8_t buffer[Blocks];
    
    String line;
    for (auto addr = RtcmemBegin; addr < RtcmemEnd; addr += Increment) {
        std::memcpy(buffer, reinterpret_cast<uint32_t*>(addr), 1);

        line += PSTR("0x");
        line += String(addr, 16);
        line += ':';

        for (auto offset = &buffer[0]; offset < &buffer[Blocks]; offset += 4) {
            line += PSTR(" ");
            line += hexEncode(offset, offset + 4);
        }

        line += '\n';

        ctx.output.print(line);
        line = "";
    }
}

static constexpr ::terminal::Command Commands[] PROGMEM {
    {Init, init},
    {Dump, dump},
};

void setup() {
    espurna::terminal::add(Commands);
}

} // namespace terminal
#endif

bool current_status() {
    return internal::status;
}

void setup() {
#if TERMINAL_SUPPORT
    terminal::setup();
#endif

    internal::status = status();
    if (!internal::status) {
        init();
    }

}

} // namespace rtc

} // namespace
} // namespace peripherals
} // namespace espurna

bool rtcmemStatus() {
    return espurna::peripherals::rtc::current_status();
}

void rtcmemSetup() {
    espurna::peripherals::rtc::setup();
}
