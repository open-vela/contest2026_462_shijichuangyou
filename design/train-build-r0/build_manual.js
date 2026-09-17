/* Generates a new R0 planning manual and dimensioned SVG layout sheets. */
const fs = require('fs');
const path = require('path');
const root = __dirname;
const out = path.join(root, 'drawings');
const esc = s => String(s).replaceAll('&','&amp;').replaceAll('<','&lt;').replaceAll('>','&gt;').replaceAll('"','&quot;');
const ink='#283c33', green='#dce8d6', pale='#f6f3eb', orange='#a34f32', grey='#718177';
const line=(x1,y1,x2,y2,stroke=ink,dash='')=>`<line x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" stroke="${stroke}" stroke-width="2" ${dash?`stroke-dasharray="${dash}"`:''}/>`;
const rect=(x,y,w,h,fill='none',stroke=ink,r=0)=>`<rect x="${x}" y="${y}" width="${w}" height="${h}" rx="${r}" fill="${fill}" stroke="${stroke}" stroke-width="2"/>`;
const circle=(x,y,r,fill='none',stroke=ink)=>`<circle cx="${x}" cy="${y}" r="${r}" fill="${fill}" stroke="${stroke}" stroke-width="2"/>`;
const text=(x,y,s,size=20,color=ink,anchor='start')=>`<text x="${x}" y="${y}" font-size="${size}" fill="${color}" text-anchor="${anchor}">${esc(s)}</text>`;
const arrow=(x1,y1,x2,y2)=>`<line x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" stroke="${orange}" stroke-width="3" marker-end="url(#arrow)"/>`;
function hd(x1,x2,y,s){return line(x1,y,x2,y,grey)+line(x1,y-9,x1,y+9,grey)+line(x2,y-9,x2,y+9,grey)+text((x1+x2)/2,y-12,s,20,ink,'middle');}
function vd(x,y1,y2,s){return line(x,y1,x,y2,grey)+line(x-9,y1,x+9,y1,grey)+line(x-9,y2,x+9,y2,grey)+`<text transform="translate(${x+26},${(y1+y2)/2}) rotate(-90)" text-anchor="middle" font-size="20" fill="${ink}">${esc(s)}</text>`;}
function notes(x,y,rows){return rows.map((s,i)=>text(x,y+i*31,s,19)).join('');}
const sheets=[];
function sheet(id,title,body){
  const file=id+'.svg';
  const svg=`<svg xmlns="http://www.w3.org/2000/svg" width="420mm" height="297mm" viewBox="0 0 1400 990"><defs><marker id="arrow" markerWidth="9" markerHeight="9" refX="8" refY="4" orient="auto"><path d="M0 0L8 4L0 8" fill="none" stroke="${orange}" stroke-width="1.5"/></marker></defs><rect width="1400" height="990" fill="white"/><g font-family="Noto Sans CJK SC,Source Han Sans SC,sans-serif">${text(48,57,id+'  '+title,29)}${text(48,92,'单位 mm｜R0 工程布局稿｜数字为设计值，不是实测板尺寸｜禁止直接量产',19,orange)}${line(45,112,1355,112)}${body}${line(45,930,1355,930)}${text(48,958,'拾迹小火车  |  2026-09-13  |  按标注数字读图，不按显示或打印比例量取',17)}${text(1350,958,'HOLD / 实测与试片后放行',17,orange,'end')}</g></svg>`;
  fs.writeFileSync(path.join(out,file),svg);
  sheets.push({id,title,file:'drawings/'+file});
}

// D00: exploded assembly, deliberately diagrammatic rather than AI imagery.
let s='';
s+=text(70,155,'装配层次与核心取出方向',24);
s+=rect(135,205,455,27,green,ink,10)+text(620,226,'P03 可拆车顶 + 2 颗 M3 手拧螺钉',22);
s+=rect(210,292,210,234,pale,ink,10)+rect(233,327,164,168,green,grey,4)+text(445,357,'P00 完整独立核心',23)+text(445,391,'拟定包络 70 × 78 × 28',20)+text(445,424,'不是裸屏；内胆需要实物补图',20,orange);
s+=arrow(315,282,315,244);
s+=rect(130,575,470,105,pale,ink,8)+rect(200,570,225,100,'white',grey)+text(640,614,'P02 车体与顶部开放式核心卡座',22)+text(640,647,'侧面显示；从顶部垂直抽取',20);
s+=arrow(315,540,315,568);
s+=rect(130,743,470,24,green)+text(640,764,'P01 底盘 + 4 个轴支撑',22);
for(const x of [205,520]){s+=circle(x,829,44,pale)+circle(x,829,8);}
s+=line(180,829,550,829,grey)+text(640,831,'P04 车轮 ×4 / Φ3 ×84 光轴 ×2',22);
s+=notes(845,235,['拆卸：','1. 先拔掉外接 USB 线。','2. 松开两颗手拧螺钉。','3. 提起车顶，不拉电线。','4. 握住核心壳体向上抽出。','5. 核心独立运行。','','首版不使用磁吸供电；','车顶是机械防脱件，','不是电池压板。']);
sheet('D00','总装与拆卸示意',s);

// D01: three orthogonal views with nominal overall dimensions.
s=text(70,150,'侧视图  X-Z',23)+text(810,150,'端视图  Y-Z',23);
s+=rect(70,180,450,261,pale,ink,12)+rect(64,171,462,9,green,ink,4)+rect(70,441,450,12,green);
s+=rect(190,192,210,234,'white',grey,8)+text(295,293,'核心包络',21,grey,'middle')+text(295,328,'70 × 78',22,grey,'middle');
for(const x of [160,430]) s+=circle(x,513,48,pale)+circle(x,513,42)+circle(x,513,5);
s+=line(60,513,535,513,grey,'7 5')+hd(64,526,601,'总长 154')+vd(565,171,561,'总高 130（离轨，轮缘着地）')+hd(160,430,574,'轴距 90');
s+=rect(815,180,270,261,pale,ink,10)+rect(809,171,282,9,green,ink,4)+rect(815,441,270,12,green);
for(const x of [854,1046])s+=rect(x-15,465,30,96,pale);
s+=line(824,513,1076,513,grey)+hd(809,1091,601,'总宽 94')+hd(854,1046,574,'踏面中心距 64');
s+=text(780,653,'俯视图（车顶移除）',23)+rect(780,685,450,216,pale,ink,10);
s+=rect(900,693,210,67.2,green,grey,4)+text(1005,738,'核心厚 28',18,ink,'middle');
for(const x of [807,1203]){s+=circle(x,793,5);for(const y of [709,877])s+=circle(x,y,5);}
s+=hd(780,1230,673,'车体长 150')+vd(1275,685,901,'车体宽 90（图示比例不同）');
s+=notes(75,694,['设计参数：车体 150 × 90 × 87；壁厚 2.4。','底盘厚 4；车顶厚 3；四周外挑 2。','轴中心为 Z=0；底盘下表面 Z=20。','车体底面 Z=24；核心底面 Z=29。','核心顶面 Z=107；车顶下表面 Z=111。','车顶下限位块与核心壳顶部名义间隙 0.5。','轨道基座上使用时，总高为 135。']);
sheet('D01','整车三视与总体尺寸',s);

// D02: measured interface gate, no invented electronics mounting holes.
s=text(70,154,'外部标准包络（拟定）',23)+rect(100,210,350,390,pale,ink,16)+hd(100,450,181,'Cw = 70（拟定）')+vd(70,210,600,'Ch = 78（拟定）');
s+=rect(155,275,240,225,'none',grey,4)+text(275,352,'实际玻璃与显示区域',20,grey,'middle')+text(275,388,'位置、宽高待实测',20,orange,'middle');
s+=text(104,639,'正面：不能按效果图开玻璃孔',20,orange);
s+=rect(545,210,140,390,pale,ink,8)+hd(545,685,181,'Ct = 28（拟定）')+text(615,420,'侧视',22,grey,'middle');
s+=notes(785,176,['必须回填 MEASUREMENTS.csv：','M01–03 主板、屏幕与排线包络','M04–06 玻璃、触摸区与偏移','M07–09 USB、按键与安装孔坐标','M10 成品保护电池的实际长宽厚','M11 板端与电池插头的真实极性','M12–13 封装后总尺寸与重量','M14 充电上限、电流与保护条件','','电池电芯参考（不是成品包尺寸）：','EEMB LP503450 / 3.7V / 950mAh','电芯上限 52 × 34.5 × 5.3。','保护板、胶带与线根会改变包络。']);
s+=rect(80,710,620,128,green,grey,5)+text(105,747,'内胆分层原则',23)+notes(105,784,['玻璃边框承托 → 屏幕/主板固定 → 绝缘隔离 → 电池保护腔','不压元件、不折死排线、不用螺钉顶住电池。']);
s+=notes(790,727,['R0 只交付 core_proxy 假核心量规。','它是实心块，不能装电子件。','内胆前后壳和接口开孔尚未出加工模型。','实测补图是整套打印的前置停止点。']);
sheet('D02','独立核心包络与实测要求',s);

// D03: horizontal cradle section and roof retention.
s=text(70,156,'核心卡座水平剖面（向下看）',23);
s+=rect(160,240,805,24,pale)+rect(185,264,24,285,green)+rect(917,264,24,285,green)+rect(185,525,756,24,green);
s+=rect(213,268,700,280,pale,grey,6);
s+=text(563,390,'完整核心 70 × 28',26,ink,'middle');
s+=hd(209,917,213,'卡座内宽 Cw + 2g = 70.8')+vd(1010,264,552,'有效深度 Ct + 2g = 28.8');
s+=text(70,611,'g = 0.4 单边设计间隙；必须由同工艺试片确认。',22,orange);
s+=text(70,656,'前侧窗口：宽 Cw−6，高 Ch−6；每边遮住核心外壳 3。',21);
s+=text(70,691,'此处是车体窗口，不是玻璃孔；如遮挡触控区，必须改图。',21);
s+=rect(1130,245,150,18,green)+rect(1135,263,30,35,green)+rect(1135,303,95,230,pale,grey);
s+=text(1112,197,'车顶限位局部',21)+vd(1303,298,303,'0.5')+arrow(1110,283,1133,283);
s+=notes(1090,590,['只挡住核心壳体上沿。','不得压玻璃或裸电池。','2颗M3手拧螺钉锁车顶。','松开后，整颗核心上抽。']);
s+=notes(70,773,['车顶螺钉：M3×8 作为首轮候选；实装核对啮合长度。','螺母槽：对边 5.8，深 3；先拿实际螺母做试片。','卡座面、轴孔和螺母槽禁喷漆；不得靠强压解决不合。','本剖面是尺寸关系示意，不按图上线长直接切料。']);
sheet('D03','滑入卡座与车顶防脱结构',s);

// D04: axle stack and wheel section.
s=text(65,156,'单根轴的横向装配  Y方向（整车共2根）',23);
s+=line(230,385,1070,385,grey)+rect(230,377,840,16,'#b8c1c5',grey);
for(const [x,w,h,fill]of [[237,50,75,green],[290,80,280,pale],[370,20,320,green],[390,20,58,'white'],[410,80,240,pale],[810,80,240,pale],[890,20,58,'white'],[910,20,320,green],[930,80,280,pale],[1013,50,75,green]])s+=rect(x,385-h/2,w,h,fill,ink);
s+=hd(230,1070,624,'光轴 Φ3 ×84；两端去毛刺')+hd(330,970,661,'踏面中心距 64')+text(220,213,'轮缘朝车体中心',20,orange)+arrow(344,225,379,239)+arrow(965,225,920,239);
s+=notes(70,729,['从左到右：限位环 / 左轮 / 2片尼龙垫片 / 左轴座 / 右轴座 / 2片垫片 / 右轮 / 限位环。','轮：踏面 Φ28×8；内侧轮缘 Φ32×2；总厚10；孔 Φ3.2（以实配为准）。','轴座：孔 Φ3.3；中心位置 Y=±20；每座宽8，外侧端面 Y=±24。','垫片：每轮内侧2片，内径3.2、厚1；限位环内径3、厚5，锁紧前留小量转动间隙。','四轮可各自绕轴转动；不要胶死车轮。孔修整必须在安装电子件之前完成。','本轮为直轨手推结构，不宣称能通过效果图中的小半径弯轨。']);
s+=notes(1112,258,['首轮五金：','光轴 ×2','车轮 ×4','尼龙垫片 ×8','轴限位环 ×4','','轴向总窜动目标','0.2–0.5，','以装配实测为准。']);
sheet('D04','轮轴剖面与五金装配',s);

// D05: custom straight track, no guessed commercial standard compatibility.
s=text(70,155,'直轨俯视图',23)+rect(80,200,600,336,pale);
s+=rect(80,230,600,20,green)+rect(80,486,600,20,green)+hd(80,680,175,'单段长 150')+vd(721,200,536,'基板宽 84')+vd(765,250,486,'两轨内侧净距 59');
s+=text(845,174,'轨道横断面',23)+rect(860,423,336,16,pale)+rect(890,411,20,12,green)+rect(1146,411,20,12,green);
s+=hd(900,1156,378,'轨中心距 64')+text(850,480,'底板厚4；轨宽5；轨高3。',20)+text(850,516,'总厚7；不导电，不接电池。',20)+text(850,552,'这是自定义轨道，不兼容现成轨距。',20,orange);
s+=text(75,608,'展示底板布局：建议 600 × 200 ×6 木板（不属于打印CAD）',22);
s+=rect(80,637,720,240,pale,grey);
for(let i=0;i<4;i++){s+=rect(80+i*180,707,180,100.8,'white',grey);s+=line(80+i*180,719,260+i*180,719)+line(80+i*180,796,260+i*180,796);}
s+=rect(87,707,12,100.8,green)+rect(781,707,12,100.8,green);
s+=rect(240,660,72,42,green)+rect(590,812,72,42,green)+text(276,688,'青叶站',16,ink,'middle')+text(626,840,'拾迹站',16,ink,'middle');
s+=notes(850,663,['轨道4段平口对接，共600长。','先在底板上拉中心线，再固定。','接缝错台目标≤0.2，手推不绊轮。','两端固定P07挡块，内侧贴软垫。','站台仅作布景，首版不自动识别。','可在站牌贴NFC标签供手机读，','不能因此让黄山派自动读到站。']);
sheet('D05','直轨与站台布置',s);

// E01: functional wiring only, gates intentionally prevent unsafe plug-in.
s+= ''; // New sheet below deliberately starts from an empty string.
s=text(65,156,'电源连接逻辑（非针脚焊接图）',24);
s+=rect(75,228,235,140,pale,ink,8)+text(193,277,'合规 USB 5V 电源',22,ink,'middle')+text(193,319,'现有 Type-C 口',20,ink,'middle');
s+=rect(440,210,355,220,green,ink,8)+text(618,258,'黄山派原有电源系统',24,ink,'middle')+text(618,310,'充电芯片 / 电池座 / 系统负载',20,ink,'middle')+text(618,360,'屏幕、主板由核心内部供电',19,ink,'middle');
s+=rect(935,210,360,220,pale,ink,8)+text(1115,260,'1S 带保护成品电池包',23,ink,'middle')+text(1115,305,'候选 EEMB LP503450 电芯',19,ink,'middle')+text(1115,348,'3.7V / 950mAh / 4.2V充电体系',18,ink,'middle')+text(1115,390,'保护板 + 引线 + 匹配插头',19,ink,'middle');
s+=arrow(310,291,437,291)+line(795,272,935,272,orange)+line(795,361,935,361,ink)+text(810,250,'BAT+ 对 BAT+',18,orange)+text(808,392,'BAT− 对 BAT−',18);
s+=rect(80,485,1220,111,'#fff0e8',orange,6)+text(105,526,'停止点 E：在充电上限、电流和真实极性确认前，不接入电池，也不让USB给它充电。',23,orange)+text(105,565,'不根据“正向线序”文字猜左右针位；按本块板丝印、原理图和成品插头测量确认。',21,orange);
s+=notes(85,655,['供应商交付：带PCM保护板与成品插头，不买裸电芯自行焊接；需要确认完整包装尺寸。','建议工程首充目标：4.20V体系、约200mA，须先确认充电芯片档位与实际配置；本轮未改固件。','LP503450规格有温度与电流限制；保护板不是正常充电控制器，不能用来补救错误充电电压。','禁止USB 5V直连BAT；禁止增加第二块充电板与板载充电器并联；禁止带电轨道和裸露触点。','首充在开放、不可燃的稳定表面有人看护；异常温升、异味、鼓包立即停止使用，不做破坏性测试。','车体首版不增加电气接线。USB充电或调试时取出核心，避免线缆拖车和封闭壳体积热。']);
sheet('E01','电池与供电停止点',s);

// D06: process-matched coupon; actual part geometry comes from the SCAD file.
s=text(65,156,'只先打印这一组配合试片，再决定整套间隙',24);
s+=rect(100,215,560,340,pale);
for(let i=0;i<3;i++){const g=[0.3,0.4,0.5][i];s+=rect(120+i*180,295,(10+2*g)*10,260,green)+circle(170+i*180,255,[3,3.2,3.4][i]*5);s+=text(173+i*180,598,`g=${g}`,23,ink,'middle');}
s+=hd(100,660,187,'试片宽56')+vd(710,215,555,'长34 / 厚8')+rect(890,255,100,200,green)+hd(890,990,232,'插片宽10')+vd(1040,255,455,'长20 / 厚4');
s+=notes(825,557,['左至右：单边间隙0.3 / 0.4 / 0.5。','孔径左至右：3.0 / 3.2 / 3.4。','试片和插片用同批材料、同工艺打印。','先试清理后尺寸，再试最终表面处理。']);
s+=notes(80,728,['选合：不掰、不敲即可滑入，反复插拔后不明显晃动；不同供应商结果可能不同。','孔试配：使用同一根Φ3光轴，必要时远离电子件手工修孔，不以标称孔径直接判合格。','这是工艺选择试片，不替代实际长导轨验证；完整卡座必须再用假核心装配测试。','只提供R0 CAD源码，本轮未运行CAD渲染或导出STL。请供应商先做模型检查后再打印试片。']);
sheet('D06','滑槽与轴孔配合试片',s);

const bom=[
['P00','完整独立核心内胆','1套','前后壳及固定架','待M01–M14实测补图，CAD不含内胆','暂停加工'],
['P01','底盘','1','尼龙打印；含4个轴座','CAD part=chassis','整套放行后'],
['P02','车体与卡座','1','尼龙打印；壁厚2.4','CAD part=body','整套放行后'],
['P03','可拆车顶','1','尼龙打印；厚3','CAD part=roof','整套放行后'],
['P04','车轮','4','踏面Φ28；内轮缘Φ32；轴孔Φ3.2','CAD part=wheel；孔需实配','整套放行后'],
['P05','直轨','4','150×84×7；自定义轨道','CAD part=track','整套放行后'],
['P06','站牌','2','60×35底座；总高48','CAD part=station；贴纸字牌','整套放行后'],
['P07','端部挡块','2','10×84×12；内侧贴软垫','CAD part=end_stop','整套放行后'],
['T01','配合试片与插片','1组','尼龙；单边0.3/0.4/0.5','CAD part=fit_coupon','先问商家模型检查及试片报价'],
['T02','假核心量规','1','70×78×28只是当前拟定包络','CAD part=core_proxy；实心块不得装电子件','尺寸方案确认后试装'],
['H01','圆光轴','2','Φ3×84；端部去毛刺','请商家切长；普通钢或不锈钢','通用五金可询价'],
['H02','轴限位环','4','内径3；外径约8；厚5；带紧定螺钉','实际外径不得与轮或车体干涉','按到货尺寸实配'],
['H03','尼龙平垫片','8','内径3.2；厚1；外径约8','每轮内侧2片','通用五金可询价'],
['H04','M3机器螺钉','4','M3×14候选；配薄垫片','底盘到车体；装配复核长度','通用五金可询价'],
['H05','M3手拧螺钉','2','M3×8候选；手拧头不碰外壳','车顶锁止；装配复核长度','通用五金可询价'],
['H06','M3六角螺母','6','常规约5.5对边；实际尺寸先配槽','车体4颗，车顶2颗','通用五金可询价'],
['H07','展示底板','1','建议600×200×6木板','另购或加工；CAD未包含','可先按摆放空间调整'],
['H08','轨道固定与挡块软垫','1组','薄双面胶与EVA防撞垫','只用于无源轨道及挡块；不包紧电池','按现场选'],
['E01','已有黄山派与屏幕','1套','保持原板与排线','不拆焊、不打孔','已有'],
['E02','带保护成品锂聚合物电池','1','EEMB LP503450电芯方案；3.7V 950mAh','必须由供应商配PCM和匹配插头；裸电芯页不是成品采购链接','电气及尺寸确认后采购'],
['E03','电池成品线束','1','与板端匹配的2芯GH1.25；极性待核','由供应商制作；不按颜色或文字猜针序','电气停止点'],
['O01','无源NFC标签','0–2可选','用于手机打开站点或作品链接','不是车端读卡器','V1非必需']
];
const csvrow=r=>r.map(x=>'"'+String(x).replaceAll('"','""')+'"').join(',');
fs.writeFileSync(path.join(root,'BOM.csv'),'\ufeff'+[['编号','名称','数量','规格','说明','采购状态'],...bom].map(csvrow).join('\r\n')+'\r\n');

const drawingsHtml=sheets.map(d=>`<article class="drawing" id="${d.id}"><h3>${d.id} ${d.title}</h3><a href="${d.file}" target="_blank" rel="noopener"><img src="${d.file}" alt="${d.id} ${d.title}" loading="lazy"></a><p><a href="${d.file}" download>下载独立SVG矢量图</a>。可放大阅读；标注为设计值，未获得实物适配放行。</p></article>`).join('');
const bomHtml=bom.map(r=>'<tr>'+r.map(v=>'<td>'+esc(v)+'</td>').join('')+'</tr>').join('');
const html=`<!doctype html><html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>拾迹小火车制作手册 R0</title><style>
:root{--ink:#283c33;--paper:#f6f3eb;--green:#dce8d6;--line:#ccd3c8;--warn:#a34f32}*{box-sizing:border-box}body{margin:0;background:var(--paper);color:var(--ink);font-family:"Noto Sans CJK SC",sans-serif;line-height:1.85}main{max-width:1240px;padding:40px 30px 70px;margin:auto}h1,h2,h3{font-family:"Noto Serif CJK SC",serif;font-weight:500}h1{font-size:48px;line-height:1.3;margin:18px 0}h2{font-size:29px;margin:0 0 20px}h3{font-size:23px}.eyebrow{font-size:13px;color:#67776b;letter-spacing:.1em}header{border-bottom:2px solid var(--ink);padding-bottom:25px}.lead{font-size:19px;max-width:950px}.hold{border-left:4px solid var(--warn);background:#fff0e8;padding:14px 20px}.ok{border-left:4px solid #66815e;background:var(--green);padding:14px 20px}section{margin-top:45px;border-top:1px solid var(--line);padding-top:24px;scroll-margin-top:20px}a{color:inherit;text-underline-offset:4px}nav,.downloads{display:flex;gap:10px;flex-wrap:wrap;margin:22px 0}nav a,.downloads a{padding:7px 12px;border:1px solid var(--line);font-size:14px;text-decoration:none}p{margin:12px 0}li{margin:9px 0}ul,ol{padding-left:25px}table{width:100%;border-collapse:collapse;font-size:14px}th,td{padding:10px 12px;border-bottom:1px solid var(--line);text-align:left;vertical-align:top}th{background:var(--green)}.table{overflow-x:auto}.table table{min-width:720px}.grid{display:grid;grid-template-columns:1fr 1fr;gap:28px}.drawing{margin:35px 0}.drawing img{display:block;width:100%;height:auto;background:#fff;border:1px solid var(--line)}.drawing p{font-size:13px;color:#67776b}pre{white-space:pre-wrap;word-break:break-word;background:#e9eee3;padding:16px;font-size:13px}code{font-family:monospace}footer{margin-top:45px;font-size:13px;color:#67776b}.check label{display:block;padding:10px 0;border-bottom:1px solid var(--line)}.check input{margin-right:9px}button{font:inherit;padding:8px 14px;color:var(--ink);border:1px solid var(--line);background:white;cursor:pointer}.small{font-size:13px;color:#66776a}
@media(max-width:720px){main{padding:24px 16px}h1{font-size:34px}.grid{grid-template-columns:1fr}.lead{font-size:17px}.drawing{margin-left:-8px;margin-right:-8px}}@media print{@page{size:A4;margin:14mm}body{background:#fff}main{padding:0;max-width:none}h1{font-size:30px}h2{font-size:23px}nav,.downloads,button{display:none}section{break-before:page;margin-top:0}.drawing{break-before:page}.drawing img{max-height:175mm;object-fit:contain}.grid{display:block}.table{overflow:visible}table{font-size:10px}.table table{min-width:0}th,td{padding:5px}h3{break-after:avoid}tr,.hold,.ok{break-inside:avoid}}
</style></head><body><main>
<header><div class="eyebrow">拾迹创游 / 制作与供应商打样手册 / 2026-09-13</div><h1>可拆式角色小火车制作方案</h1><p class="lead">适用方式：商家3D打印，你自行装配。目标是做出一辆能手推、能展示角色、能把完整黄山派核心取出独立使用的小火车，而不是重新设计电动机车。</p><p><strong>版本 R0：工程布局及采购准备包。</strong>已经给出结构关系、拟定尺寸、七张矢量图、参数化车体源码、物料清单和执行顺序。<strong>没有完成实物测量、核心内胆加工设计、STL导出或实物装配验证。</strong></p></header>
<div class="hold"><strong>不能跳过的两个停止点</strong><p>尺寸停止点 M：先得到主板/屏幕/排线和成品电池的真实尺寸，再定核心内胆、车体卡座和接口开孔。电气停止点 E：先确认充电芯片、充电上限、电流和插头极性，再接电池。当前不能把本包直接当作整套生产放行文件。</p></div>
<nav><a href="#scope">做什么</a><a href="#battery">电池选择</a><a href="#measure">测量</a><a href="#drawing-set">图纸</a><a href="#bom">采购清单</a><a href="#print">打印要求</a><a href="#assembly">装配步骤</a><a href="#accept">验收</a><a href="#supplier">发给商家</a></nav>
<div class="downloads"><a href="train-build-r0.zip" download>下载整套R0制作包</a><a href="BOM.csv" download>采购BOM</a><a href="MEASUREMENTS.csv" download>实测记录表</a><a href="cad/train_r0.scad" download>OpenSCAD车体源码</a><a href="SUPPLIER_REQUEST.txt" download>供应商询价说明</a></div>
<button onclick="window.print()">打印手册或另存为PDF</button><p class="small">PDF由浏览器按当前版式打印生成，本包没有预先生成或校验PDF。图纸必须按标注数字读图，不得直接从打印图上量取加工尺寸。</p>
<section id="scope"><h2>1 首版范围与产品使用方式</h2><div class="grid"><div><h3>这一版包含</h3><ul><li>手推四轮车、直轨、两个站牌和机械挡块。</li><li>位于车厢侧面的角色核心，车顶取出，无须拆排线。</li><li>核心内自带电池，离车后继续使用；前提是电池及内胆通过后续确认。</li><li>现有角色切换、点击反馈和功能页。首版站台布景配合手动触屏操作。</li></ul></div><div><h3>这一版不包含</h3><ul><li>电机、自主行驶、弯轨转向、轨道供电、磁吸充电。</li><li>自动识别车站、NFC读卡、音频播报、GPS或蓝牙控制。</li><li>儿童玩具安全认证、防水、跌落保证或量产寿命承诺。</li><li>未测量的“黄山派标准外壳”，以及可直接下单的最终内胆STL。</li></ul></div></div><p>取出流程：拔掉USB线 → 松开两颗车顶手拧螺钉 → 提起车顶 → 握住完整核心壳体向上抽出。不是只拿走显示屏，也不需要拔FPC。</p><p>造型建议：奶油白车体、苔绿车顶、灰绿车轮、青叶站与拾迹站的纸质字牌。侧面角色窗口最大化；贴花和花叶仅放车头、车顶边和站牌，不遮触控区。</p><p>先直轨是有意取舍：固定轴距的首版底盘不能直接照此前概念图配小半径弯轨。弯轨要重新设计转向架、轮轨间隙和最小转弯半径。</p></section>
<section id="battery"><h2>2 电池选择与采购停止点</h2><div class="ok"><strong>推荐候选：基于 EEMB LP503450 电芯的成品保护电池包。</strong><p>官方当前规格是3.7V、950mAh，电芯外形上限52 ×34.5 ×5.3mm，约19g。不能把“503450”自行解读为包含保护板和线材后的最终尺寸，也不要照其他厂家的同名1000mAh产品替换。</p></div><p>选它的理由：比小容量电池更适合作为显示原型的初始候选；板状结构方便与屏幕/主板分层。这个判断不构成续航保证。额定能量约3.515Wh；实际续航必须用整机工作状态测量，不套用开发板宣传待机时间。</p><h3>直接发给电池供应商的要求</h3><pre>请报价一只基于EEMB LP503450电芯的1S成品锂聚合物电池包。
额定3.7V，官方当前电芯容量950mAh，4.2V充电体系。
必须由供应商配好过充/过放/过流/短路保护PCM及绝缘包装，不要裸电芯。
线材与2芯GH1.25配套插头按黄山派实际电池座确认；针序以板端丝印及测量为准。
请提供成品包长宽厚上限（含PCM、胶带、线根）、引线长度、极性照片、充电条件与规格书。
请说明是否可提供单件样品。尺寸与极性确认前不要制作不可退的专用线束。</pre>
<p>官方产品页是<strong>裸电芯页</strong>，不是带正确插头的即买即用成品。采购对象必须是满足上述条件的完成品；若供应商只卖裸电芯或保护资料不清，换供应商，不自行焊电芯。</p>
<p>工程首充建议采用4.20V体系、约200mA的保守目标，并确认芯片实际可配置档位及温度限制。<strong>这只是拟定目标，本轮没有写入任何充电寄存器。</strong>电芯规格允许的最大电流不是推荐日常充电值，保护板也不能替代充电器。</p>
<p>现有本地代码提到了AW32001，检查到的片段主要涉及充电使能和看门狗，不能据此证明电压、电流已正确配置。公开硬件说明在不同段落也出现充电芯片名称差异，因此必须以你的实板版本、原理图及读回配置为准。</p>
<div class="hold">在停止点E解除之前，可继续用现有USB方式开发，但<strong>不要把新电池和USB一起接上尝试充电</strong>。也不要直接把USB 5V接BAT、并联第二个充电模块或让导电轨道承担供电。</div>
<p class="small">电芯参数来源：<a href="https://www.eemb.com/product-138">EEMB产品页</a>与<a href="https://eemb.oss-accelerate.aliyuncs.com//uploads/20230311/9b4528f45ac3cb1d13483e978793975b.pdf">厂商规格书，第3–4页</a>。成品包仍须供应商确认。</p></section>
<section id="measure"><h2>3 先测量，再冻结核心内胆</h2><p>准备数显卡尺、直尺、纸和手机。关机、拔USB，先不装新电池。不拆焊、不改变原屏幕排线的自然弯曲。以主板左下角为坐标基准，拍清正面、背面和侧面。</p><ol><li>填M01–M03：主板外形、突出接口、原板屏组件厚度。跳线帽与插座必须算进去。</li><li>填M04–M06：玻璃外形、有效显示/触摸区和偏移；不要从390×450像素反算开孔。</li><li>填M07–M09：USB、两个按键、固定孔的位置与操作空间。拍摄尺子必须与被测边尽量同平面。</li><li>拿到电池供应商成品尺寸，填M10；电子确认后再填M11和M14。</li><li>据此补完整独立内胆的支撑、绝缘、接口开口、背盖与螺钉，再得到最终M12宽高厚。</li><li>用最终M12替换CAD中的core_w/core_h/core_t。70×78×28只是本次工程布局的假设，不是已量出的黄山派尺寸。</li></ol><p>本包提供<strong>实心假核心量规</strong>，用于验证卡座空间与抽取通道；它不能作为装主板、电池的外壳。内胆加工模型尚未发布是有意的安全与适配停止点，而不是让你把板子随意塞进空腔。</p><p>内胆设计原则：玻璃非显示边缘承托；PCB按实际固定点安装；电池与尖锐焊点、金属螺钉之间有绝缘隔离和非挤压空间；USB插头要能完全插入。先做空壳和等尺寸假件，不拿电池当配合量规。</p></section>
<section id="drawing-set"><h2>4 必要图纸</h2><p>七张SVG均为确定性矢量图，不是AI生成的尺寸图。标注的数值是R0设计意图；机械、内胆和电气停止点尚未解除。每张可单独下载给商家评审。</p>${drawingsHtml}</section>
<section id="bom"><h2>5 采购与零件清单</h2><p>没有替你购买或支付。先向商家要工艺与报价，不给未经报价的价格承诺。按状态分批采购，避免先买完整套件却发现核心尺寸要调整。</p><div class="table"><table><thead><tr><th>编号</th><th>名称</th><th>数量</th><th>规格</th><th>说明</th><th>状态</th></tr></thead><tbody>${bomHtml}</tbody></table></div><h3>你需要的工具</h3><p>卡尺、万用表、与五金匹配的六角扳手/螺丝刀、手钻或手动铰孔工具、细砂纸、去毛刺工具、标记笔。轴孔修整与涂装必须在电子件入壳之前进行；光轴尽量请商家切长，不在主板旁锯切金属。</p></section>
<section id="print"><h2>6 给打印商家的制造要求</h2><p>承力件优先询价MJF PA12或适用SLS尼龙，明确材料牌号，不把不同“尼龙”当成同一种工艺。普通展示树脂不作为轴座、滑轨和长期锁止件的默认材料。若改FDM PETG，需要重新协商层向、支撑和配合间隙，不能照搬尼龙公差。</p><ul><li>先用同工艺打印D06试片；0.3、0.4、0.5是单边间隙，不是总间隙。</li><li>壁厚2.4、底盘4、车顶3是设计起点。供应商需确认该材料和零件尺寸下的DFM，不自动声明合格。</li><li>清粉、去支撑、倒钝边、修孔。保持卡座内壁、轴孔、螺母槽和接缝基准面不喷漆。</li><li>四轮与两轴分别装配，不打印成不可拆的一体转动机构。孔径按实轴试配。</li><li>所有M3孔是通孔和螺母连接，不是自攻螺纹。固定螺母可在不接触电子件的位置采用适用胶固定，充分固化后装电子件；不得往螺纹灌胶。</li><li>零件分别标P编号。首套不先做昂贵上色；先验收装配，最后再处理奶油白、苔绿表面。</li></ul><p>打印供应商公开说明中的制造公差可能接近滑动间隙的量级，所以试片是必要步骤，不是多余手续。参考：<a href="https://jlc3dp.com/help/article/3d-printing-design-guideline">嘉立创3D打印设计指南</a>。</p>
<h3>CAD文件怎么使用</h3><p>提供OpenSCAD参数化源码。默认展示装配布局；此环境没有OpenSCAD，本轮<strong>未渲染CAD、未做网格或碰撞检查、未导出STL</strong>。商家若只接STL/STEP，不能把本源码改后缀当作STL；要先由能使用OpenSCAD的人员转档并检查。</p><pre># 仅输出工艺试片；先由商家检查模型
openscad -o fit_coupon.stl -D 'part="fit_coupon"' cad/train_r0.scad

# 输出假核心；参数必须改为确认后的完整封装尺寸
openscad -o core_proxy.stl -D 'part="core_proxy"' cad/train_r0.scad

# 只有尺寸、内胆与试片获确认后才解除 fit_confirmed 停止点。
# 下面是操作模板，不代表已经放行或执行。
openscad -o body.stl -D 'part="body"' -D 'fit_confirmed=true' cad/train_r0.scad</pre><p>其他part名称：chassis、roof、wheel、track、station、end_stop。改变核心参数后，静态SVG中的总体数值可能需要同步更新；不要只改模型却继续使用旧图纸下单。命令用法参考<a href="https://files.openscad.org/documentation/manual/Using_OpenSCAD_in_a_command_line_environment.html">OpenSCAD官方命令行说明</a>。</p></section>
<section id="assembly"><h2>7 按顺序装配</h2><ol>
<li><strong>先做试片。</strong>拿同一根Φ3光轴试孔，拿10mm插片试三种槽。选出无需敲打、滑入顺畅且晃动可接受的一档，记录材料、批次和后处理。</li>
<li><strong>完成核心内胆。</strong>用实测尺寸补内胆模型、打印空壳、用假件试装。确认USB、开关键和触控都能操作。电池与尖锐部件隔离，电池不承受固定力。</li>
<li><strong>用假核心试车体。</strong>暂不装电子件，插入假核心并盖车顶。两侧不夹紧，顶部限位名义间隙0.5；松开车顶后可直抽，不碰车体装饰。</li>
<li><strong>安装轮轴。</strong>将光轴穿过两个轴座。每个外侧轴座外放两片1mm尼龙垫片，再装车轮，轮缘朝内；最后装限位环。四轮应各自转动，轴向窜动目标0.2–0.5，不能为消间隙而把轮夹死。</li>
<li><strong>连接底盘与车体。</strong>预放四颗M3螺母，M3×14候选螺钉从底部锁入；逐颗交替拧到贴合即可。先确认螺钉末端不会进入核心或电池空间。</li>
<li><strong>检查车顶锁止。</strong>预放两颗车顶螺母，试M3×8手拧螺钉。要求可靠啮合且不顶到底；车顶限位只挡核心壳体，不压显示玻璃。</li>
<li><strong>铺轨。</strong>在600×200底板上画中心线，4段直轨平口对接。对齐轨面、再固定到板上。不要强行扭曲轨道迁就底板翘曲。</li>
<li><strong>安装挡块与站牌。</strong>轨道两端固定挡块并贴软垫，站牌不侵入车体通行和取核心的空间。先不用磁铁或电子检测。</li>
<li><strong>先空车手推。</strong>确认四轮落轨、不绊缝、不碰车体。再放假核心重复检查。如果某处卡顿，先查轨道对齐、轮夹紧和孔毛刺，不靠大力推动。</li>
<li><strong>通过电气停止点。</strong>按E01由有能力确认的人核对充电系统和极性，再进行电池供电与有人看护的首次充电；本手册不提供未经核对的具体针位连接。</li>
<li><strong>装入电子核心。</strong>确认所有切削、打磨和涂装完成且内部干净。安装完整核心和车顶，在轨道上低速手推，同时操作屏幕。</li>
<li><strong>最后做表面装饰。</strong>只在非配合面上色和贴图；螺钉、释放位置与USB口保持可见可达。涂装后再重复滑入与滚动检查。</li>
</ol></section>
<section id="accept"><h2>8 首件验收与小批量放行</h2><p>以下是拟定工程检查，不是产品认证。不要自行做电池短路、过充、针刺、跌落等破坏性试验。</p><div class="check">
${['尺寸表完整；电池为有资料的成品保护包；实际核心尺寸已冻结。','电子件不承担卡座夹紧力；玻璃与排线无挤压；没有螺钉朝向电池。','假核心与真实封装各能正常插拔；建议完成30次人工拆装观察，不出现明显裂纹或卡涩。','车顶可靠锁止，手拧螺钉可正常释放；连接不依赖临时胶带或裸磁铁。','四轮都能转动；轮缘朝内；直轨接缝目标错台不超过0.2；低速往返10次无卡轮。','使用者不需接触裸板即可开关机、触控、取出核心与接入USB。','拔USB后核心独立运行至少完成一轮30分钟工程观察；记录亮度与功能状态，不把它称为续航结果。','取出核心和在车内时，角色切换、点击反馈、功能页都能用；首版不声称自动到站。','确认充电参数、极性、异常处理；充电在开放环境有人看护，记录实际结果。','所有零件、CAD、静态图纸、BOM、供应商材料和最终实测尺寸版本一致。'].map(t=>`<label><input type="checkbox">${t}</label>`).join('')}</div>
<p>工程样通过后，才按同一套定版图做小批量；保留一件首件样和一套同批试片。若更换材料、打印商、涂层或电池，重新确认相关配合，不把旧结果直接套用。</p><p>若计划出售或作为儿童玩具发放，还需针对销售地、年龄定位、电池、机械小件等另行做合规评估。本包仅支持个人DIY与桌面展示原型，不能作为认证声明。</p></section>
<section id="supplier"><h2>9 现在可以做的事</h2><div class="grid"><div class="ok"><strong>现在可以执行</strong><ul><li>按测量表拍摄带尺正反面与侧面照片。</li><li>向电池供应商询问完成品规格，不先买裸电芯。</li><li>把本包与SUPPLIER_REQUEST.txt交给打印商，确认其能否做CAD检查和试片。</li><li>询价通用五金与配合试片，不急着下单整套壳。</li></ul></div><div class="hold"><strong>现在不能直接执行</strong><ul><li>按70×78×28假设尺寸把整套当最终件生产。</li><li>把core_proxy实心量规当作真实电子外壳。</li><li>根据效果图造孔，或把未转档SCAD改后缀上传。</li><li>接未确认极性的新电池，或凭官方“最大电压”说明开始充电。</li></ul></div></div><p>下一版R1需要：板屏实物尺寸、选定保护电池的成品尺寸、充电确认、试片结果。随后才能补核心内胆、修改必要开口、同步尺寸图并导出可制造的分件模型。</p></section>
<footer><p>来源：<a href="https://wiki.lckfb.com/zh-hans/hspi-sf32lb52/hardware/board.html">黄山派官方硬件文档</a>；<a href="https://www.eemb.com/product-138">EEMB LP503450产品页</a>及其官方规格书；<a href="https://jlc3dp.com/help/article/3d-printing-design-guideline">嘉立创3D打印设计指南</a>；<a href="https://files.openscad.org/documentation/manual/Using_OpenSCAD_in_a_command_line_environment.html">OpenSCAD文档</a>。查阅日期2026-09-13。</p><p>状态：R0工程准备包；非加工放行。无采购、接线、充电配置修改、固件修改或烧录操作。本轮不声称已生成STL、完成实物验证或得到官方比赛验收。</p></footer></main></body></html>`;
fs.writeFileSync(path.join(root,'index.html'),html);
fs.writeFileSync(path.join(root,'README.txt'),`拾迹可拆式角色小火车 R0 制作准备包\n\n先打开 index.html 阅读手册。drawings/ 含7张SVG矢量图；BOM.csv是采购清单；MEASUREMENTS.csv是必须回填的实测项；SUPPLIER_REQUEST.txt可发打印商询价；cad/train_r0.scad是参数化车体源码。\n\n本版不是生产放行：无实测板尺寸，无完整内胆模型，无STL，无CAD网格/干涉检查和实物装配验证。core_proxy是实心假核心，不是电子外壳。电池候选是LP503450电芯的成品保护包，不是裸电芯。先解除尺寸与电气停止点，再做整套打印和电池接入。\n`);
console.log(JSON.stringify({manual:path.join(root,'index.html'),drawings:sheets.length,bom_rows:bom.length,status:'R0 HOLD; not manufacturing release'}));
