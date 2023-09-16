#include "Papyrus.h"
#include "InjectTopic.h"

//using namespace Sample;
using namespace RE;
using namespace RE::BSScript;
using namespace REL;
using namespace SKSE;

namespace MantellaSubtitles {
    constexpr std::string_view PapyrusClass = "MantellaSubtitles";

    // Start handlers for Papyrus functions.
    //
    // Note that SKSE cannot allow creation of new instanced Papyrus types. Therefore, all Papyrus extensions will be
    // static functions (labeled "global" in Papyrus). For the purpose of binding a function, this is indicated by the
    // first parameter being a <code>RE::StaticFunctionTag*</code>.
    //
    // CommonLibSSE handles conversion of Papyrus types to C++ types. Forms, active magic effects, and aliases can be
    // translated back and forth between Papyrus classes and a poitner to their C++ equivalent, e.g. ObjectReference
    // and RE::TESObjectREFR*. Papyrus arrays and be translated back and forth to std::vector or other array-like
    // classes (e.g. std::list, or even Skyrim classes like RE::BSTArray). Strings can be translated as parameters that
    // are std::string, std::string_view, or Skyrim's RE::BSFixedString (a case-preserving but case-insensitive interned
    // string), and the primitive types are converted to <code>bool</code>, <code>int</code>, and <code>float</code>.

    bool SetInjectTopicAndSubtitleForSpeakerPapyrus(RE::StaticFunctionTag*, RE::Actor* speaker, RE::TESTopic* topic, RE::BSString subtitle) { 
        SKSE::log::debug("Set inject for topic {:x} with subtitle \"{}\"", topic->GetFormID(), subtitle);
        injectSpeaker = speaker;
        injectTopic = topic;
        injectSubtitle = subtitle;

        return true;
    }

    // ms_to_show of <0 means we don't handle cleanup here. This is useful in
    // cases where we're adding "overriding" subtitles for someone who is
    // already speaking.
    bool AddSubtitleForSpeaker(RE::Actor* speaker, RE::BSString subtitle, int32_t ms_to_show) {
        SKSE::log::debug("Injecting subtitle for speaker {:x} with subtitle \"{}\"", speaker->GetFormID(), subtitle);
        auto* subtitleManager = RE::SubtitleManager::GetSingleton();

        // I assume this lock is for contention over the subtitle manager, but
        // who knows
        subtitleManager->lock.Lock();
        auto siSpeakerRef = speaker->CreateRefHandle();
        RE::SubtitleInfo newSubtitleInfo = {
            .speaker=speaker->CreateRefHandle(),
            .pad04=0,
            .subtitle=subtitle,
            .targetDistance=0.0,
            .forceDisplay=true
        };

        // de-prioritize other subtitles so ours shows; additionally, if
        // there's already a subtitle for our speaker, remove it (it's getting
        // replaced by this one)
        RE::SubtitleInfo* to_del = NULL;
        for (auto& s: subtitleManager->subtitles) {
            s.forceDisplay = false;
            if (s.speaker.get()->GetFormID() == speaker->GetFormID()) {
                to_del = &s;
            }
        }
        if (to_del) {
            subtitleManager->subtitles.erase(to_del);
        }

        subtitleManager->subtitles.push_back(newSubtitleInfo);
        subtitleManager->lock.Unlock();

        // in the case where ms_to_show >= 0, it's likely a "manual" call.
        // since, in that case, this isn't actually attached to a person
        // speaking, it won't correctly get expired from the subtitlemanager
        // unless we manually do so ourselves. we spawn a thread to do this.
        if (ms_to_show >= 0) {
            std::thread t([=]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(ms_to_show));

                // it's probably fine to keep the pointer from the outside scope
                // but just in case
                auto* subtitleManager = RE::SubtitleManager::GetSingleton();
                RE::SubtitleInfo* to_del = NULL;

                subtitleManager->lock.Lock();
                for (auto& s : subtitleManager->subtitles) {
                    if (s.speaker == siSpeakerRef) {
                        to_del = &s;
                        break;
                    }
                }
                if (to_del) {
                    subtitleManager->subtitles.erase(to_del);
                }
                subtitleManager->lock.Unlock();
            });
            t.detach();
        }

        return true;
    }

    bool AddSubtitleForSpeakerPapyrus(
        RE::StaticFunctionTag*, RE::Actor* speaker, RE::BSString subtitle, int32_t ms_to_show
    ) { 
        return AddSubtitleForSpeaker(speaker, subtitle, ms_to_show);
    }

    /**
     * This is the function that acts as a registration callback for Papyrus functions. Within you can register functions
     * to bind to native code. The first argument of such bindings is the function name, the second is the class name, and
     * third is the function that will be invoked in C++ to handle it. The callback should return <code>true</code> on
     * success.
     */
    bool RegisterFns(IVirtualMachine* vm) {
        vm->RegisterFunction("SetInjectTopicAndSubtitleForSpeaker", PapyrusClass, SetInjectTopicAndSubtitleForSpeakerPapyrus);
        vm->RegisterFunction("AddTopicAndSubtitleForSpeaker", PapyrusClass, AddSubtitleForSpeakerPapyrus);

        return true;
    }
}

