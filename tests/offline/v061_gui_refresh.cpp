// Exercises the shipped native VST entry/dispatcher, not a direct setProgram shortcut.
// Host cache only refreshes on audioMasterUpdateDisplay (SDK host-contract.md).
#include "EarlyDelay.cpp"
#include <dlfcn.h>
#include <array>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace {
constexpr int N = EarlyDelay::kNumParams;
std::array<float, N> cached{}, expected{};
std::array<std::string, N> labels{}, expectedLabels{};
int updates = 0, automation = 0;
bool completeAtCallback = true;
void require(bool yes, const char* message) { if (!yes) throw std::runtime_error(message); }
VstIntPtr host(AEffect* e, VstInt32 opcode, VstInt32, VstIntPtr, void*, float)
{
    if (opcode == audioMasterVersion) return 2400;
    if (opcode == audioMasterAutomate) { ++automation; return 1; }
    if (opcode == audioMasterUpdateDisplay) {
        ++updates;
        for (int i=0; i<N; ++i) {
            cached[i] = e->getParameter(e, i);
            char text[64]{};
            e->dispatcher(e, effGetParamDisplay, i, 0, text, 0);
            labels[i] = text;
            completeAtCallback &= cached[i] == expected[i] && labels[i] == expectedLabels[i];
        }
        return 1;
    }
    return 0;
}
void target(EarlyDelay& oracle)
{
    for (int i=0; i<N; ++i) {
        expected[i]=oracle.getNormalised(i);
        expectedLabels[i]=oracle.displayText(i);
    }
}
void poison(AEffect* e)
{
    int before=updates;
    for (int i=0; i<N; ++i) {
        float other = expected[i] < .5f ? 1.f : 0.f;
        e->setParameter(e, i, other);
        cached[i]=other;
        labels[i]="stale";
    }
    require(updates == before, "ordinary parameter writes must not rebuild UI");
}
void check(AEffect* e, int before, const std::string& name)
{
    require(updates == before+1, "missing/duplicate display update after complete load");
    require(completeAtCallback, "callback saw partially loaded parameter bank/display text");
    require(automation == 0, "preset loading must not generate automation gestures");
    std::cout << "STATE " << name;
    for (int i=0; i<N; ++i) {
        require(cached[i] == expected[i] && cached[i] == e->getParameter(e,i), "stale host parameter cache");
        require(labels[i] == expectedLabels[i], "stale display text");
        std::cout << ' ' << std::setprecision(9) << cached[i];
    }
    std::cout << '\n';
}
}
int main(int argc,char** argv)
{
    try {
        require(argc==2, "usage: v061_gui_refresh native-plugin.so");
        void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
        if (!lib) throw std::runtime_error(dlerror());
        auto entry=reinterpret_cast<AEffect*(*)(audioMasterCallback)>(dlsym(lib,"VSTPluginMain"));
        require(entry!=nullptr,"missing VSTPluginMain");
        AEffect* e=entry(host);
        require(e && e->numParams==N && e->numPrograms==20,"identity/count changed");
        e->dispatcher(e,effOpen,0,0,nullptr,0);
        EarlyDelay oracle;
        // Initial/manual state is not an extra factory program. Exercise it via chunk.
        auto initial=oracle.internalSaveState();
        for (int program=0; program<e->numPrograms; ++program) {
            oracle.setProgram(program); target(oracle);
            require(!oracle.getBool(EarlyDelay::kHold),"factory HOLD must be off");
            poison(e); int before=updates;
            e->dispatcher(e,effSetProgram,0,program,nullptr,0);
            check(e,before,"program-"+std::to_string(program));
            require(e->dispatcher(e,effGetProgram,0,0,nullptr,0)==program,"wrong selected program");
            // A repeat selection must also discard edits and refresh (even same index).
            poison(e); before=updates;
            e->dispatcher(e,effSetProgram,0,program,nullptr,0);
            check(e,before,"repeat-"+std::to_string(program));
            void* ptr=nullptr;
            auto size=e->dispatcher(e,effGetChunk,0,0,&ptr,0);
            std::vector<uint8_t> chunk(static_cast<uint8_t*>(ptr),static_cast<uint8_t*>(ptr)+size);
            for (int bankOrPreset=0; bankOrPreset<2; ++bankOrPreset) {
                poison(e); before=updates;
                require(e->dispatcher(e,effSetChunk,bankOrPreset,chunk.size(),chunk.data(),0)==1,"chunk rejected");
                check(e,before,"chunk-"+std::to_string(program)+"-"+std::to_string(bankOrPreset));
            }
        }
        oracle.internalLoadState(initial.data(),initial.size()); target(oracle);
        poison(e); int before=updates;
        require(e->dispatcher(e,effSetChunk,0,initial.size(),initial.data(),0)==1,"initial state rejected");
        check(e,before,"initial-manual");
        // Saved user presets may legitimately have HOLD, INVERT and STEREOIZE ON.
        for (int i=0; i<N; ++i) oracle.setNormalised(i,.87f);
        auto custom=oracle.internalSaveState(); target(oracle); poison(e); before=updates;
        require(e->dispatcher(e,effSetChunk,1,custom.size(),custom.data(),0)==1,"custom state rejected");
        check(e,before,"all-switches-on");
        before=updates;
        e->dispatcher(e,effSetProgram,0,-1,nullptr,0);
        e->dispatcher(e,effSetProgram,0,e->numPrograms,nullptr,0);
        char invalid[]="INVALID";
        require(e->dispatcher(e,effSetChunk,0,sizeof invalid,invalid,0)==0,"invalid chunk accepted");
        require(updates==before,"invalid loads must not rebuild UI");
        e->dispatcher(e,effSetSampleRate,0,0,nullptr,44100);
        e->dispatcher(e,effSetBlockSize,0,128,nullptr,0);
        e->dispatcher(e,effMainsChanged,0,1,nullptr,0);
        float l[128]{},r[128]{}; float* io[]={l,r};
        e->processReplacing(e,io,io,128);
        require(updates==before && automation==0,"audio callback emitted GUI/automation notification");
        e->dispatcher(e,effClose,0,0,nullptr,0); dlclose(lib);
        // Hosts without a callback remain supported.
        e=VSTPluginMain(nullptr);e->dispatcher(e,effSetProgram,0,1,nullptr,0);
        e->dispatcher(e,effClose,0,0,nullptr,0);
        std::cout << "PASS 20 factory programs + initial/manual state; repeat loads, bank/preset chunks, custom switches, invalid loads, no audio-thread refresh/automation\n";
    } catch(const std::exception& ex) { std::cerr << "FAIL " << ex.what() << '\n'; return 1; }
}
