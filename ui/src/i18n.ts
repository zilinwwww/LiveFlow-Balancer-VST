// 1:1 port of src_old/ui/Translations.h
export type Language = 'en' | 'zh';

const dictEN: Record<string, string> = {
  'Btn_ToggleLang': 'EN / 中文',
  'Btn_Expert': 'Expert',
  'Btn_Help': 'Help',
  'Btn_About': 'About',
  'About_Title': 'About LiveFlow Balancer',
  'About_Desc': 'LiveFlow Balancer v1.0\n\nAdvanced vocal-aware backing rider.',
  'Label_Balance': 'Balance',
  'Label_Dynamics': 'Dynamics',
  'Label_Speed': 'Speed',
  'Label_Range': 'Range',
  'Label_Anchor': 'Anchor',
  'Toggle_Auto': 'Auto',
  'Toggle_Expand': 'Expand',
  'Toggle_Compress': 'Compress',
  'Label_BoostCeiling': 'Boost Ceiling',
  'Label_DuckFloor': 'Duck Floor',
  'Label_NoiseGate': 'Noise Gate',
  'Label_LookAhead': 'Look Ahead',
  'Label_ReleaseHyst': 'Release Hyst',
  'Header_Title': 'LIVEFLOW USER MANUAL',
  'Header_Sub': 'Adaptive vocal-aware backing rider',
};

const dictZH: Record<string, string> = {
  'Btn_ToggleLang': 'EN / 中文',
  'Btn_Expert': '专家',
  'Btn_Help': '帮助',
  'Btn_About': '关于',
  'About_Title': '关于 LiveFlow Balancer',
  'About_Desc': 'LiveFlow Balancer v1.0\n\n高级人声动态伴奏避让与骑推子效果器。',
  'Label_Balance': '平衡',
  'Label_Dynamics': '动态',
  'Label_Speed': '速度',
  'Label_Range': '范围',
  'Label_Anchor': '基准点',
  'Toggle_Auto': 'Auto',
  'Toggle_Expand': '扩展',
  'Toggle_Compress': '常规',
  'Label_BoostCeiling': '增益上限',
  'Label_DuckFloor': '减益上限',
  'Label_NoiseGate': '噪声门',
  'Label_LookAhead': '预读取',
  'Label_ReleaseHyst': '脱扣延迟',
  'Header_Title': 'LIVEFLOW 帮助手册',
  'Header_Sub': '智能人声避让骑推子器',
};

export function getText(key: string, lang: Language): string {
  const dict = lang === 'zh' ? dictZH : dictEN;
  return dict[key] ?? key;
}

export interface HelpSection {
  title: string;
  description: string;
}

export function getHelpSections(lang: Language): HelpSection[] {
  if (lang === 'zh') {
    return [
      { title: 'BALANCE (人声平衡)', description: '决定人声要比伴奏高多少。默认建议设置在 +3dB。当你把它调到负数时，也就是伴奏比人声响，适用于极其特殊的混合场景。' },
      { title: 'DYNAMICS (动态)', description: '控制推子的响应力度与方向。\n【正值 (Compress)】：标准模式。当人声太响，会向下鸭滑压低伴奏；当人声太弱，会自动抬升伴奏。\n【负值 (Expand)】：扩展模式（反向）。当人声太响，会提拉伴奏；当人声太弱，则会压低伴奏。' },
      { title: 'SPEED (速度)', description: '反应时间，类似于压缩器的 Attack/Release 集合。\n较快的值会让伴奏闪避得非常迅速，就像在 DJ 喊麦。\n较慢的值会让伴奏以平滑推子的方式跟进，顺滑不易察觉。' },
      { title: 'RANGE (范围)', description: '主界面的全局安全锁！无论算法算出要增加或减少多少音量，都不会超过这个限制范围。\n把它转到最大 (20dB) 来解除封印，使用 Expert 面板里的高阶限制器来分别控制。' },
      { title: 'ANCHOR (基准点设置)', description: '算法必须知道什么水平的音量算是"标准"，这个基准就是描点！\n【Auto】模式下自动监测人声平稳区间。\n【Manual】模式下手对准你这首歌人声音量分布最密集的主线。' },
      { title: 'BOOST CEILING [EXPERT]', description: '向上拉升的极端上限！不管歌手声音多弱都不会超过这个顶界。这让你对"伴奏推高的风险"有独立绝对控制。' },
      { title: 'DUCK FLOOR [EXPERT]', description: '向下压暗的深坑底线！不管给伴奏多少让步，都不会低于这个限制。通常代表允许伴奏随时大幅避让的极限。' },
      { title: 'NOISE GATE [EXPERT]', description: '防止环境底噪、呼吸声触发伴奏避让的绝对下限。' },
      { title: 'LOOK AHEAD [EXPERT]', description: '提前感知人声峰值避开爆破元音。适用于录音室。直播环境建议关掉以避免引入延迟。' },
      { title: 'RELEASE HYST [EXPERT]', description: '防止歌手在每个词之间几百毫秒的空隙导致伴奏猛弹回来。它会让避让状态保持得更加坚决。' },
    ];
  }
  return [
    { title: 'BALANCE', description: 'Target relationship between vocal and backing track. Default is +3dB. Negative values mean backing track will be louder than vocals.' },
    { title: 'DYNAMICS', description: 'Controls fader action strength and direction.\nPositive (Compress): Standard ducker. Backing goes down when vocal is present.\nNegative (Expand): Reverse ducker. Backing swells UP during vocal performance.' },
    { title: 'SPEED', description: 'Reaction ballistics encompassing both attack and release.\nFast (30ms): Aggressive hyper-responsive pumping.\nSlow (300ms): Undetectable, natural fader riding.' },
    { title: 'RANGE', description: 'Global absolute constraint on gain changes. Max out at 20dB to disable, and use Expert limits independently.' },
    { title: 'ANCHOR', description: 'The standard LUFS reference. Auto: Algorithm monitors densest average. Manual: Align to the thickest part of the vocal spectrum.' },
    { title: 'BOOST CEILING [EXPERT]', description: 'Absolute threshold for lifting the backing track. Gives independent control over positive upward gain.' },
    { title: 'DUCK FLOOR [EXPERT]', description: 'Absolute threshold for pushing backing down. Prevents backing from dying completely during extreme shouting.' },
    { title: 'NOISE GATE [EXPERT]', description: 'Vocal detection cutoff. Prevents ambient noise and mic bleed from triggering fader actions.' },
    { title: 'LOOK AHEAD [EXPERT]', description: 'Pre-computes fader movements for perfect transient catching. Adds latency. Set to 0ms for live.' },
    { title: 'RELEASE HYST [EXPERT]', description: 'Hysteresis prevents pumping by holding ducked state over tiny gaps between words.' },
  ];
}
