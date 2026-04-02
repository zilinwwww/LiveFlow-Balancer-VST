#pragma once

#include <juce_core/juce_core.h>
#include <map>

// Force MSVC to interpret strings as UTF-8 to prevent Mojibake
#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

// Safe UTF-8 decoding macro for Chinese strings under MSVC /std:c++20
// Bypasses the const char8_t[] type check by casting directly.
#define ZH(x) juce::String::fromUTF8(reinterpret_cast<const char*>(u8##x))

namespace liveflow::i18n
{

enum class Language
{
    English,
    Chinese
};

struct HelpSectionDef
{
    juce::String title;
    juce::String description;
};

inline juce::String getText (const juce::String& key, Language lang)
{
    static const std::map<juce::String, juce::String> dictEN = {
        { "Btn_ToggleLang", ZH("EN / 中文") },
        { "Btn_Expert", "Expert" },
        { "Btn_Help", "Help" },
        { "Btn_About", "About" },
        { "About_Title", "About LiveFlow Balancer" },
        { "About_Desc", "LiveFlow Balancer v1.0\n\nAdvanced vocal-aware backing rider." },

        { "Label_Balance", "Balance" },
        { "Label_Dynamics", "Dynamics" },
        { "Label_Speed", "Speed" },
        { "Label_Range", "Range" },
        { "Label_Anchor", "Anchor" },

        { "Toggle_Auto", "Auto" },
        { "Toggle_Expand", "Expand" },
        { "Toggle_Compress", "Compress" },

        { "Label_BoostCeiling", "Boost Ceiling" },
        { "Label_DuckFloor", "Duck Floor" },
        { "Label_NoiseGate", "Noise Gate" },
        { "Label_LookAhead", "Look Ahead" },
        { "Label_ReleaseHyst", "Release Hyst" },

        // Smart Track Profiler
        { "Btn_Listen", "Listen" },
        { "Btn_StopListen", "Stop" },
        { "Btn_Cancel", "Cancel" },
        { "Btn_Export", "Export" },
        { "Btn_Import", "Import" },
        { "Btn_Delete", "Delete" },
        { "Label_Profile", "Profile" },
        { "Label_NumZones", "Zones" },
        { "Label_NumBands", "EQ Bands" },
        { "Label_MaxRecord", "Max Record" },
        { "Profile_None", "No Profile" },
        { "Profile_Recording", "Recording..." },
        { "Profile_Analyzing", "Analyzing..." },
        { "Profile_Ready", "Ready" },
        { "Profile_Error", "Error" },
        { "Panel_Profiler", "Smart Track Profiler" },
        { "Zone_Active", "Active Zone" },

        { "Header_Title", "LIVEFLOW USER MANUAL" },
        { "Header_Sub", "Adaptive vocal-aware backing rider" }
    };

    static const std::map<juce::String, juce::String> dictZH = {
        { "Btn_ToggleLang", ZH("EN / 中文") },
        { "Btn_Expert", ZH("专家") },
        { "Btn_Help", ZH("帮助") },
        { "Btn_About", ZH("关于") },
        { "About_Title", ZH("关于 LiveFlow Balancer") },
        { "About_Desc", ZH("LiveFlow Balancer v1.0\n\n高级人声动态伴奏避让与骑推子效果器。") },

        { "Label_Balance", ZH("平衡") },
        { "Label_Dynamics", ZH("动态") },
        { "Label_Speed", ZH("速度") },
        { "Label_Range", ZH("范围") },
        { "Label_Anchor", ZH("基准点") },

        { "Toggle_Auto", "Auto" },
        { "Toggle_Expand", ZH("扩展") },
        { "Toggle_Compress", ZH("常规") },

        { "Label_BoostCeiling", ZH("增益上限") },
        { "Label_DuckFloor", ZH("减益上限") },
        { "Label_NoiseGate", ZH("噪声门") },
        { "Label_LookAhead", ZH("预读取") },
        { "Label_ReleaseHyst", ZH("脱扣延迟") },

        // Smart Track Profiler
        { "Btn_Listen", ZH("智能听诊") },
        { "Btn_StopListen", ZH("停止") },
        { "Btn_Cancel", ZH("取消") },
        { "Btn_Export", ZH("导出") },
        { "Btn_Import", ZH("导入") },
        { "Btn_Delete", ZH("删除") },
        { "Label_Profile", ZH("声学配方") },
        { "Label_NumZones", ZH("区间数") },
        { "Label_NumBands", ZH("EQ段数") },
        { "Label_MaxRecord", ZH("最大录制") },
        { "Profile_None", ZH("无配方") },
        { "Profile_Recording", ZH("录制中...") },
        { "Profile_Analyzing", ZH("分析中...") },
        { "Profile_Ready", ZH("已就绪") },
        { "Profile_Error", ZH("分析出错") },
        { "Panel_Profiler", ZH("智能音轨分析") },
        { "Zone_Active", ZH("活跃区间") },

        { "Header_Title", ZH("LIVEFLOW 帮助手册") },
        { "Header_Sub", ZH("智能人声避让骑推子器") }
    };

    const auto& activeDict = (lang == Language::Chinese) ? dictZH : dictEN;
    auto it = activeDict.find (key);
    return it != activeDict.end() ? it->second : key;
}

inline std::vector<HelpSectionDef> getHelpSections (Language lang)
{
    if (lang == Language::Chinese)
    {
        return {
            { ZH("BALANCE (人声平衡)"), ZH("决定人声要比伴奏高多少。默认建议设置在 +3dB。当你把它调到负数时，也就是伴奏比人声响，适用于极其特殊的混合场景。") },
            { ZH("DYNAMICS (动态)"), ZH("控制推子的响应力度与方向。\n【正值 (Compress)】：标准模式。当人声太响，会向下鸭滑压低伴奏；当人声太弱，会自动抬升伴奏。\n【负值 (Expand)】：扩展模式（反向）。当人声太响，会提拉伴奏；当人声太弱，则会压低伴奏。") },
            { ZH("SPEED (速度)"), ZH("反应时间，类似于压缩器的 Attack/Release 集合。\n较快的值会让伴奏闪避得非常迅速，就像在 DJ 喊麦。\n较慢的值会让伴奏以平滑推子的方式跟进，顺滑不易察觉。") },
            { ZH("RANGE (范围)"), ZH("主界面的全局安全锁！无论算法算出要增加或减少多少音量，都不会超过这个限制范围。\n把它转到最大 (20dB) 来解除封印，使用 Expert 面板里的高阶限制器来分别控制。") },
            { ZH("ANCHOR (基准点设置)"), ZH("算法必须知道什么水平的音量算是“标准”，这个基准就是描点！\n【Auto】模式下自动监测人声平稳区间。\n【Manual】模式下手对准你这首歌人声音量分布最密集的主线。") },
            { ZH("BOOST CEILING (增益上限) [EXPERT]"), ZH("向上拉升的极端上限！不管歌手声音多弱都不会超过这个顶界。这让你对“伴奏推高的风险”有独立绝对控制。") },
            { ZH("DUCK FLOOR (减益上限) [EXPERT]"), ZH("向下压暗的深坑底线！不管给伴奏多少让步，都不会低于这个限制。通常代表允许伴奏随时大幅避让的极限。") },
            { ZH("NOISE GATE (噪声门) [EXPERT]"), ZH("防止环境底噪、呼吸声触发伴奏避让的绝对下限。") },
            { ZH("LOOK AHEAD (预判) [EXPERT]"), ZH("提前感知人声峰值避开爆破元音。适用于录音室。直播环境建议关掉以避免引入延迟。") },
            { ZH("RELEASE HYST (脱扣延迟) [EXPERT]"), ZH("防止歌手在每个词之间几百毫秒的空隙导致伴奏猛弹回来。它会让避让状态保持得更加坚决。") },
            { ZH("SMART TRACK PROFILER (智能音轨分析) [v1.1]"), ZH("录制一段实时的演出，自动计算伴奏和人声的频谱碰撞并生成最优的多段动态 EQ 曲线。避免了全频段避让带来的沉闷感。") },
            { ZH("LISTEN (智能听诊) [PROFILER]"), ZH("点击开始录音采样。同时播放伴奏并演唱，Profiler 会在后台分析不同音量下的频谱碰撞点。再次点击停止并生成配方。") },
            { ZH("ZONES & BANDS (区间与段数) [PROFILER]"), ZH("Zones 决定算法根据伴奏响度分几个能量区间（静音/主歌/副歌），Bands 决定每个区间能最多避让几个频段。") }
        };
    }
    else
    {
        return {
            { "BALANCE", "Target relationship between vocal and backing track. Default is +3dB. Negative values mean backing track will be louder than vocals." },
            { "DYNAMICS", "Controls fader action strength and direction.\nPositive (Compress): Standard ducker. Backing track goes down when vocal is present, and boosts when missing.\nNegative (Expand): Reverse ducker. Backing track swells UP during vocal performance, and attenuates during silence." },
            { "SPEED", "Reaction ballistics encompassing both attack and release mechanisms.\nFast (30ms): Aggressive and hyper-responsive pumping.\nSlow (300ms): Undetectable, natural fader riding action." },
            { "RANGE", "Global absolute constraint on algorithmically suggested gain changes.\nMax out at 20dB to disable global range, and switch to using Expert limits (Boost Ceiling / Duck Floor) independently." },
            { "ANCHOR", "The standard LUFS reference point that the plugin considers a 'baseline volume'.\nAuto: Algorithm continuously monitors the densest average LUFS.\nManual: Align the white anchor manually to the thickest part of the vocal spectrum." },
            { "BOOST CEILING [EXPERT]", "Absolute threshold for lifting the backing track. Bypasses the global Range knob to give independent control over positive upward gain." },
            { "DUCK FLOOR [EXPERT]", "Absolute threshold for pushing the backing track down. Prevents the backing from dying completely during extreme shouting." },
            { "NOISE GATE [EXPERT]", "Vocal detection cutoff threshold. Prevents ambient room noise and mic bleed from triggering fader actions." },
            { "LOOK AHEAD [EXPERT]", "Pre-computes fader movements to catch transients perfectly. Great for post-production but adds latency in live streaming. Set to 0ms for live." },
            { "RELEASE HYST [EXPERT]", "Hysteresis prevents pumping by holding the ducked state explicitly over tiny gaps (like breathing) between words." },
            { "SMART TRACK PROFILER [v1.1]", "Record a live performance segment to automatically compute spectral collisions and generate an optimal multi-band dynamic EQ curve. Avoids the muffled sound of full-band ducking." },
            { "LISTEN [PROFILER]", "Click to start recording. Play the backing track and sing at the same time. The profiler analyzes collisions across different volume levels. Click again to generate a Profile." },
            { "ZONES & BANDS [PROFILER]", "Zones: How many energy levels (quiet/verse/chorus) the algorithm uses. Bands: Maximum number of frequency notches per zone." }
        };
    }
}

#undef ZH

} // namespace liveflow::i18n
