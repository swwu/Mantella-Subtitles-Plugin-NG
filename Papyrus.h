#pragma once

#include <RE/Skyrim.h>
#include <SKSE/Trampoline.h>

namespace MantellaSubtitles{
    bool RegisterFns(RE::BSScript::IVirtualMachine* vm);
    bool AddSubtitleForSpeaker(RE::Actor* speaker, RE::BSString subtitle, int32_t ms_to_show);
}
