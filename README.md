# ArduinoLCM
LC meter based on Franclin oscillator

[フランクリン発振回路を使用したLCメーター]
[The LC meter composed by Franclin oscillator.]

Arduinoを使ってLCメーターを作ります。
I will introduce the LC meter controlled by Arduino.

1.初めに　[Preface]

フランクリン発振回路はLC共振回路を使い、位相反転増幅器と組み合わせ、逆相信号を負帰還し発振させる方式です。
このフランクリン発振回路とマイコンの周波数カウンター機能を組み合わせ、
基準Cを利用し発振周波数から連立方程式を解くことで被測定LあるいはCの値を求めるという方式が今回紹介するLCメーターです。
1990年ごろに東欧の技術者の方がアイデアを発案され、1995年ごろのオーストラリアのアマチュア無線誌に掲載されたのが普及した始まりのようです。
その前にアメリカのAADE（ボーイング社のOBらしい）が製品化して販売してました。日本ではストロベリーリナックス社も10年以上前から製品化されています。

The Franclin oscillator is composed by the LC resonator and the phase inverting amplifier. 
The digital capability could lead LC meter which was calculating L or C value from the basic capacitor's capacitance by solving correlated equation from the oscillation frequency number.

[To compose the Franclin oscillator and MPU ]
A certain Eastern Europian electronics engineer found this idea in along 1990’s. 
After then the Australian Ham magazine had picked it up as a technical article. 
At that time AADE( On his WEB he wrote that he had been a Boeing engineer.) had already introduced this circuit, produced, and sold as a product for HAMs. 
In Japan the Strawberry-Linux company had developed a clone and sold those for HAMs.


2. 測定原理　　[The principal]

前置きは以上にして、ArduinoでLC測定のやり方ですが、LC値の導出計算方法は次になります。
// a.	基本回路のLCで発振させ、周波数値を取得
// b.	基準C（計算時に数値の根拠となるので精度の良いものを使う）を並列に入れて周波数値を取得
// c.	F1＝1/2*π*sqrt(C1*L1),F2=1/2*π*sqrt((C1+C2)*L1)
// d.	F1,F2,C2が既知なのでこの二式から連立方程式の解としてC1とL1を求める。
// e.	被測定L2をL1へ直列につなぎF31=1/2*π*sqrt(C1*(L1+L2))からL2を算出する。
被測定C3の場合は並列につないでF32=1/2*π*sqrt((C1+C3)*L1)から算出する。

Below is explanation how to calculate the capacitance or the inductance.
// a. To oscillate and measure the frequency of F1.
// b. To add the standard capacitor paralleled and get the frequency of F2.
// c. There are two equation for the circuit. F1＝1/2*π*sqrt(C1*L1), F2=1/2*π*sqrt((C1+C2)*L1).
// d. F1, F2, C2 are the valid value. We could draw C1 and L1 by mathematical process.
// e. When you put L2 serially and get F31, you shall draw L2 value out of equation. Also C3 by F32.
F31=1/2*π*sqrt(C1*(L1+L2)), F32=1/2*π*sqrt((C1+C3)*L1)

以上からArduinoのスケッチで必要なことは「周波数のカウント」「測定時のLCつなぎ替え」「周波数からインダクタンスやキャパシタンスへの変換」「SWリレー制御とLCD表示」となります。
スケッチはGITHUBで公開しております。上記の処理を淡々と描いたものなのでコメントが不十分ですが、ご理解できると思います。
Arduino circuit is expected to count the frequency and to change wiring LCs.
Also sketch shall calculate to convert the frequency into the inductance or the capacitance and to display such data.

Please refer the sketch itself.

3. 回路の説明　　The application circuit

回路ですが、まずフランクリン発振回路ですが、74HCU04を使います。74HCU04はロジック回路インバーターですが、実は周波数特性の良い高利得FETとして使えます。
基準Cの切り替えにはリレーを使います。半導体で切り替えできないことはありませんが測定範囲が狭まってしまいます。
またL測定、C測定の切り替えに二極双投のスイッチを使います。

表示は繋ぎの楽なi2c方式LCD（16文字X2行）にしました。

回路図は次になります。

I use 74HCU04 inverter IC for the Franclin oscillator. For the sake of switching the standard capacitor, I had adopted the relay. 
If you use semiconductor switching, accuracy may decline. The two pole two circuit switch is necessary for LC change.
I use i2c interfacing LCD for display.

There are the circuit diagrams on below site.
https://cdn-ak.f.st-hatena.com/images/fotolife/n/nobcha23/20200714/20200714202214.jpg
https://cdn-ak.f.st-hatena.com/images/fotolife/n/nobcha23/20200726/20200726214857.jpg

4. 実験基板について  [How to assemble the circuit.]

バニラシールドなどのブレッドボードに組めば実験は簡単にできます。
手持ち自作測定器用として、Arduino　Nanoで制御する基板を作製中です。

You may use VANILA shield for this circuit as trial.

　　nobcha48　AT　gmail.com
