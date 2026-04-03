import React, { useState, useCallback, createContext, useContext } from 'react';

type Lang = 'en' | 'zh';

const t: Record<string, Record<Lang, string>> = {
  // Navbar
  'nav.products':     { en: 'Products',         zh: '产品' },
  'nav.dashboard':    { en: 'My Dashboard',     zh: '我的面板' },
  'nav.licenses':     { en: 'My Licenses',      zh: '我的许可证' },
  'nav.logout':       { en: 'Log Out',          zh: '退出登录' },
  'nav.login':        { en: 'Login',            zh: '登录' },
  'nav.signup':       { en: 'Sign Up',          zh: '注册' },

  // Home
  'home.series':      { en: 'Series',           zh: '系列' },
  'home.subtitle':    { en: 'Professional audio tools crafted by Micro-Grav Studio', zh: '由 Micro-Grav Studio 精心打造的专业音频工具' },
  'home.products':    { en: 'Products',         zh: '产品' },
  'home.tag':         { en: 'Adaptive Vocal-Aware Backing Rider', zh: '自适应人声感知伴奏增益骑行器' },
  'home.f1':          { en: 'Real-time vocal detection with intelligent gain riding', zh: '实时人声检测 + 智能增益骑行' },
  'home.f2':          { en: 'Interactive LUFS scatter plot with direct curve manipulation', zh: '交互式 LUFS 散点图，支持直接曲线操控' },
  'home.f3':          { en: 'Configurable dynamics, speed, and range controls', zh: '可配置的动态范围、速度和区间控制' },
  'home.f4':          { en: 'Expert mode with advanced parameters', zh: '专家模式与高级参数' },
  'home.getLicense':  { en: 'Get Free License!',  zh: '免费获取许可证！' },
  'home.regLicense':  { en: 'Register & Get Free License', zh: '注册并获取免费许可证' },
  'home.claiming':    { en: 'Claiming...', zh: '领取中...' },
  'home.claimed':     { en: '✅ License Claimed! Check Dashboard', zh: '✅ 已领取！请查看面板' },
  'home.more':        { en: 'More plugins coming soon.', zh: '更多插件即将推出。' },
  
  // Testimonials
  'home.reviews':     { en: 'What Producers Say', zh: '制作人评价' },
  'rev1.text':        { en: '"Finally, a sidechain alternative that sounds completely natural. It has saved me hours of manual volume automation on my vocal tracks."', zh: '“终于有了一个听起来完全自然的侧链替代方案。它为我节省了数小时的人声轨道全手动音量自动化的时间。”' },
  'rev1.author':      { en: 'Alex M. - Mixing Engineer', zh: 'Alex M. - 混音工程师' },
  'rev2.text':        { en: '"The interactive LUFS scatter plot is a game-changer. I can visually set the exact dynamic range I want. Stunning UI!"', zh: '“交互式的 LUFS 散点图绝对是个颠覆性的设计，能够让我可视化地精准设置动态范围，界面惊艳！”' },
  'rev2.author':      { en: 'Sarah K. - Electronic Producer', zh: 'Sarah K. - 电子音乐制作人' },
  'rev3.text':        { en: '"It effortlessly tucks the backing track under the vocals without that obvious pumping effect. Essential in my template."', zh: '“它能毫不费力地把伴奏完美垫在人声下面，且没有明显的抽吸感。现在是我工程模板里的必备插件。”' },
  'rev3.author':      { en: 'David R. - Pop Composer', zh: 'David R. - 流行作曲家' },

  // Team
  'home.team':        { en: 'Meet Our Team',    zh: '关于我们团队' },
  'team.desc':        { en: 'We are a small team of passionate audio developers and creatives dedicated to building next-generation workflow tools for modern music production.', zh: '我们是一支由充满热情的音频开发者和创作者组成的小团队，致力于为现代音乐制作打造下一代工作流工具。' },
  'team1.name':       { en: 'Zilin',            zh: 'Zilin' },
  'team1.role':       { en: 'Founder & Lead DSP Developer', zh: '创始人、主音频算法开发' },
  'team2.name':       { en: 'Partner A',        zh: '小伙伴 A' },
  'team2.role':       { en: 'UX/UI Design & Testing', zh: 'UX/UI 设计与功能测试' },
  'team3.name':       { en: 'Partner B',        zh: '小伙伴 B' },
  'team3.role':       { en: 'Audio Engineering & Presets', zh: '音频工程与预设设计' },

  // Login
  'login.title':      { en: 'Login',            zh: '登录' },
  'login.email':      { en: 'Email',            zh: '邮箱' },
  'login.password':   { en: 'Password',         zh: '密码' },
  'login.submit':     { en: 'Login',            zh: '登录' },
  'login.loading':    { en: 'Logging in...',    zh: '登录中...' },
  'login.noAccount':  { en: "Don't have an account?", zh: '还没有账号？' },
  'login.signup':     { en: 'Sign up',          zh: '注册' },
  'login.invalidCred':{ en: 'Invalid email or password', zh: '邮箱或密码错误' },
  'login.failed':     { en: 'Login failed',     zh: '登录失败' },
  'login.networkErr': { en: 'Network error',    zh: '网络错误' },

  // Register
  'reg.title':        { en: 'Create Account',   zh: '创建账号' },
  'reg.name':         { en: 'Name (optional)',   zh: '姓名（选填）' },
  'reg.email':        { en: 'Email',            zh: '邮箱' },
  'reg.password':     { en: 'Password (min 8 chars)', zh: '密码（至少8位）' },
  'reg.submit':       { en: 'Sign Up',          zh: '注册' },
  'reg.loading':      { en: 'Creating account...', zh: '正在创建...' },
  'reg.hasAccount':   { en: 'Already have an account?', zh: '已有账号？' },
  'reg.login':        { en: 'Login',            zh: '登录' },
  'reg.emailExists':  { en: 'Email already registered', zh: '该邮箱已注册' },
  'reg.failed':       { en: 'Registration failed', zh: '注册失败' },
  'reg.networkErr':   { en: 'Network error',    zh: '网络错误' },
  'reg.success':      { en: '🎉 Registration successful! Redirecting...', zh: '🎉 注册成功！正在跳转...' },

  // Dashboard
  'dash.title':       { en: 'My Licenses',      zh: '我的许可证' },
  'dash.redeem':      { en: 'Redeem License',   zh: '兑换许可证' },
  'dash.redeemPlc':   { en: 'Enter license key...', zh: '输入许可证密钥...' },
  'dash.redeemBtn':   { en: 'Redeem',           zh: '兑换' },
  'dash.redeeming':   { en: 'Redeeming...',     zh: '兑换中...' },
  'dash.redeemOk':    { en: '✅ License redeemed!', zh: '✅ 兑换成功！' },
  'dash.redeemErr':   { en: 'Invalid or already taken key', zh: '无效或已被使用的密钥' },
  'dash.loading':     { en: 'Loading...',       zh: '加载中...' },
  'dash.empty1':      { en: "You don't have any licenses yet.", zh: '你还没有任何许可证。' },
  'dash.empty2':      { en: 'Use the "Redeem License" button to add a license key.', zh: '使用「兑换许可证」按钮来添加许可证密钥。' },
  'dash.product':     { en: 'Product',          zh: '产品' },
  'dash.key':         { en: 'License Key',      zh: '许可证密钥' },
  'dash.status':      { en: 'Status',           zh: '状态' },
  'dash.machine':     { en: 'Machine',          zh: '机器' },
  'dash.actions':     { en: 'Actions',          zh: '操作' },
  'dash.active':      { en: '✅ Active',         zh: '✅ 已激活' },
  'dash.available':   { en: '🔓 Available',     zh: '🔓 可用' },
  'dash.revoked':     { en: '❌ Revoked',        zh: '❌ 已撤销' },
  'dash.unbind':      { en: 'Unbind',           zh: '解绑' },
  'dash.unbinding':   { en: 'Unbinding...',     zh: '解绑中...' },

  // Footer
  'footer.rights':    { en: 'All rights reserved.', zh: '保留所有权利。' },
};

export type { Lang };

function detectLang(): Lang {
  const saved = localStorage.getItem('liveflow_lang');
  if (saved === 'en' || saved === 'zh') return saved;
  const nav = navigator.language || '';
  return nav.startsWith('zh') ? 'zh' : 'en';
}

interface LangCtx {
  lang: Lang;
  setLang: (l: Lang) => void;
  i: (key: string) => string;
}

const LangContext = createContext<LangCtx>({ lang: 'en', setLang: () => {}, i: (k) => k });
export const useLang = () => useContext(LangContext);

export function LangProvider({ children }: { children: React.ReactNode }) {
  const [lang, setLangState] = useState<Lang>(detectLang);
  const setLang = useCallback((l: Lang) => {
    localStorage.setItem('liveflow_lang', l);
    setLangState(l);
  }, []);
  const i = useCallback((key: string) => t[key]?.[lang] ?? key, [lang]);
  return <LangContext.Provider value={{ lang, setLang, i }}>{children}</LangContext.Provider>;
}
