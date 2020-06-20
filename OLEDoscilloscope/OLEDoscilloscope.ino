/*
    簡易オシロ (_20190212_OLEDoscilloscope.ino)
    設定スイッチ機能立ち上げ中 1285byte ram free
    2019/02/12版 ラジオペンチ

    Blog Entry:
    http://radiopench.blog96.fc2.com/blog-entry-893.html
    Schematic:
    https://blog-imgs-124-origin.fc2.com/r/a/d/radiopench/20190213SchemOscllo.png
    Arduino Sketch:
    https://blog-imgs-124-origin.fc2.com/r/a/d/radiopench/20190212OLEDoscloScope.txt
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/pgmspace.h>               // PROGMEMを使うために使用（たぶんインクルード不要）
#include <EEPROM.h>

#define SCREEN_WIDTH 128                // OLED display width
#define SCREEN_HEIGHT 64                // OLED display height
#define REC_LENGTH 200                  // 波形データのバッファサイズ

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1      // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//　レンジの表示名をフラッシュメモリに保存
const char vRangeName[10][5] PROGMEM = {"A50V", "A 5V", " 50V", " 20V", " 10V", "  5V", "  2V", "  1V", "0.5V", "0.2V"}; // 縦軸表示文字（\0含んだ文字数が必要）
const char * const vstring_table[] PROGMEM = {vRangeName[0], vRangeName[1], vRangeName[2], vRangeName[3], vRangeName[4], vRangeName[5], vRangeName[6], vRangeName[7], vRangeName[8], vRangeName[9]};
const char hRangeName[8][6] PROGMEM = {" 50ms", " 20ms", " 10ms", "  5ms", "  2ms", "  1ms", "500us", "200us"};          // 横軸(48バイト）
const char * const hstring_table[] PROGMEM = {hRangeName[0], hRangeName[1], hRangeName[2], hRangeName[3], hRangeName[4], hRangeName[5], hRangeName[6], hRangeName[7]};

int waveBuff[REC_LENGTH];      // 波形データー記録メモリ (RAMがギリギリ)
char chrBuff[10];              // 表示フォーマットバッファ
String hScale = "xxxAs";
String vScale = "xxxx";

float lsb5V = 0.0055549;       // 5Vレンジの感度係数　標準値：0.005371 V/1LSBで定義
float lsb50V = 0.051513;       // 50Vレンジの感度係数 0.05371

volatile int vRange;           // 垂直レンジ  0:A50V, 1:A 5V, 2:50V, 3:20V, 4:10V, 5:5V, 6:2V, 7:1V, 8:0.5V
volatile int hRange;           // 水平レンジ　0:50m, 1:20m, 2:10m, 3:5m, 4;2m, 5:1m, 6:500u, 7;200u
volatile int trigD;            // トリガ方向（極性）0:ポジ、1:ネガ
volatile int scopeP;           // 操作スコープ位置 0:垂直レンジ, 1:水平レンジ, 2:トリガ方向
volatile boolean hold = false; // ホールド
volatile boolean paraChanged = false; // 割込みでパラメーターが変更された時に true
volatile int saveTimer;        // EEPROM保存までの残り時間
int timeExec;                  // 現在のレンジの概算実行時間(ms)

int dataMin;                   // バッファの最小値(min:0)
int dataMax;                   // バッファの最大値(max:1023)
int dataAve;                   // バッファの平均値（精度確保のため10倍で保存 max:10230)
int rangeMax;                  // グラフをフルスケールにするバッファの値
int rangeMin;                  // グラフを下限にするバッファの値
int rangeMaxDisp;              // max表示の値（100倍で指定）
int rangeMinDisp;              // min表示の値
int trigP;                     // データバッファ上のトリガ位置
boolean trigSync;              // トリガ検出フラグ
int att10x;                    // 入力アッテネーター（1で有効）

void setup() {

  pinMode(2, INPUT_PULLUP);    // ボタン入力割込み(int0割込み）
  pinMode(3, INPUT_PULLUP);    // Selectボタン
  pinMode(4, INPUT_PULLUP);    // Upボタン
  pinMode(5, INPUT_PULLUP);   // Downボタン
  pinMode(6, INPUT_PULLUP);   // Hold スイッチ
  pinMode(12, INPUT);          // アッテネーター1/10
  pinMode(13, OUTPUT);         // 状態表示


  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    //Serial.println(F("SSD1306 failed"));
    for (;;);                               // Don't proceed, loop forever
  }
  loadEEPROM();                             // EEPROMから前回の設定内容を読み出し
  analogReference(INTERNAL);                // ADCのフルスケール1.1Vに設定（内部vrefを使用)
  attachInterrupt(0, pin2IRQ, FALLING);     // スイッチ操作検出時に割込み発生
  startScreen();                            // 共通部分を描画
}

void loop() {
  digitalWrite(13, HIGH);
  setConditions();                          // 測定パラメーターのセット←これ使うとRAMが40バイト減る
  readWave();                               // 波形読み取り (最小1.6ms )
  digitalWrite(13, LOW);                    //
  dataAnalize();                            // データーの各種情報を収集(0.4-0.7ms)
  writeCommonImage();                       // 固定イメージの描画(4.6ms)
  plotData();                               // グラフプロット(5.4ms+α)
  dispInf();                                // 各種情報表示(6.2ms)
  display.display();                        // バッファを転送して表示(37ms)
  saveEEPROM();                             // 必要があればEEPROMに設定内容を保存
  while (hold == true) {                    // Holdボタンが押されていたら待つ
    dispHold();
    delay(10);
  }
}

void setConditions() {   // 測定条件設定
  // 横軸のレンジをPROGMEMから持ってくる
  strcpy_P(chrBuff, (char*)pgm_read_word(&(hstring_table[hRange])));  // 横軸レンジ名を
  hScale = chrBuff;                                                   // hScaleに設定

  // 縦軸レンジ設定
  strcpy_P(chrBuff, (char*)pgm_read_word(&(vstring_table[vRange])));  // 縦軸レンジ名を
  vScale = chrBuff;                                                   // vScaleに設定

  switch (vRange) {              // 縦軸のレンジごとの設定
    case 0: {                    // Auto50Vレンジ
//        rangeMax = 1023;
//        rangeMin = 0;
        att10x = 1;              // 入力アッテネーター使用
        break;
      }
    case 1: {                    // Auto 5Vレンジ
//        rangeMax = 1023;
//        rangeMin = 0;
        att10x = 0;              // 入力アッテネーターを使わない
        break;
      }
    case 2: {                    // 50Vレンジ
        rangeMax = 50 / lsb50V;  // フルスケール画素数の設定
        rangeMaxDisp = 5000;     // 縦軸目盛り。100倍の値で設定
        rangeMin = 0;
        rangeMinDisp =0;
        att10x = 1;              // 入力アッテネーター使用
        break;
      }
    case 3: {                    // 20Vレンジ
        rangeMax = 20 / lsb50V;  // フルスケール画素数の設定
        rangeMaxDisp = 2000;
        rangeMin = 0;
        rangeMinDisp =0;
        att10x = 1;              // 入力アッテネーター使用
        break;
      }
    case 4: {                    // 10Vレンジ
        rangeMax = 10 / lsb50V;  // フルスケール画素数の設定
        rangeMaxDisp = 1000;
        rangeMin = 0;
        rangeMinDisp =0;
        att10x = 1;              // 入力アッテネーター使用
        break;
      }
    case 5: {                    // 5Vレンジ
        rangeMax = 5 / lsb5V;    // フルスケール画素数の設定
        rangeMaxDisp = 500;
        rangeMin = 0;
        rangeMinDisp =0;
        att10x = 0;              // 入力アッテネーターを使わない
        break;
      }
    case 6: {                    // 2Vレンジ
        rangeMax = 2 / lsb5V;    // フルスケール画素数の設定
        rangeMaxDisp = 200;
        rangeMin = 0;
        rangeMinDisp =0;
        att10x = 0;              // 入力アッテネーターを使わない
        break;
      }
    case 7: {                    // 1Vレンジ
        rangeMax = 1 / lsb5V;    // フルスケール画素数の設定
        rangeMaxDisp = 100;
        rangeMin = 0;
        rangeMinDisp =0;
        att10x = 0;              // 入力アッテネーターを使わない
        break;
      }
    case 8: {                    // 0.5Vレンジ
        rangeMax = 0.5 / lsb5V;  // フルスケール画素数の設定
        rangeMaxDisp = 50;
        rangeMin = 0;
        rangeMinDisp =0;
        att10x = 0;              // 入力アッテネーターを使わない
        break;
      }
    case 9: {                    // 0.5Vレンジ
        rangeMax = 0.2 / lsb5V;  // フルスケール画素数の設定
        rangeMaxDisp = 20;
        rangeMin = 0;
        rangeMinDisp =0;
        att10x = 0;              // 入力アッテネーターを使わない
        break;
      }
  }
}

void writeCommonImage() {     // 共通画面の作画
  display.clearDisplay();                   // 画面全消去(0.4ms)
  display.setTextColor(WHITE);              // 白文字で描く
  display.setCursor(86, 0);                 // Start at top-left corner
  display.println(F("av    V"));            // 1行目固定文字
  display.drawFastVLine(26, 9, 55, WHITE);  // 左縦線
  display.drawFastVLine(127, 9, 55, WHITE); // 左縦線

  display.drawFastHLine(24, 9, 7, WHITE);   // Max値の補助マーク
  display.drawFastHLine(24, 36, 2, WHITE);  //
  display.drawFastHLine(24, 63, 7, WHITE);  //

  display.drawFastHLine(51, 9, 3, WHITE);   // Max値の補助マーク
  display.drawFastHLine(51, 63, 3, WHITE);  //

  display.drawFastHLine(76, 9, 3, WHITE);   // Max値の補助マーク
  display.drawFastHLine(76, 63, 3, WHITE);  //

  display.drawFastHLine(101, 9, 3, WHITE);  // Max値の補助マーク
  display.drawFastHLine(101, 63, 3, WHITE); //

  display.drawFastHLine(123, 9, 5, WHITE);  // 右端Max値の補助マーク
  display.drawFastHLine(123, 63, 5, WHITE); // 　

  for (int x = 26; x <= 128; x += 5) {
    display.drawFastHLine(x, 36, 2, WHITE); // 中心線(水平線)を点線で描く
  }
  for (int x = (127 - 25); x > 30; x -= 25) {
    for (int y = 10; y < 63; y += 5) {
      display.drawFastVLine(x, y, 2, WHITE); // 縦線を点線で3本描く
    }
  }
}

void readWave() {                            // 波形をメモリーに記録
  if (att10x == 1) {                         // もし1/10アッテネーターが必要なら
    pinMode(12, OUTPUT);                     // アッテネーター制御ピンを出力にして
    digitalWrite(12, LOW);                                // LOWを出力
  } else {                                   // アッテネーター不使用なら
    pinMode(12, INPUT);                      // アッテネーター制御ピンをHi-zにする（入力にする）
  }

  switch (hRange) {                          // レンジに応じ記録速度を変更

    case 0: {                                // 50msレンジ
        timeExec = 400 + 50;                 // 概算実行時間(ms) EEPROM保存までのカウントダウンに使用
        ADCSRA = ADCSRA & 0xf8;              // 下3ビットをクリア
        ADCSRA = ADCSRA | 0x07;              // 分周比128 (arduinoのオリジナル）
        for (int i = 0; i < REC_LENGTH; i++) {     // 200データー
          waveBuff[i] = analogRead(0);       // 約112μs
          delayMicroseconds(1888);           // サンプリング周期調整
        }
        break;
      }

    case 1: {                                // 20msレンジ
        timeExec = 160 + 50;                 // 概算実行時間(ms) EEPROM保存までのカウントダウンに使用
        ADCSRA = ADCSRA & 0xf8;              // 下3ビットをクリア
        ADCSRA = ADCSRA | 0x07;              // 分周比128 (arduinoのオリジナル）
        for (int i = 0; i < REC_LENGTH; i++) {     // 200データー
          waveBuff[i] = analogRead(0);       // 約112μs
          delayMicroseconds(688);            // サンプリング周期調整
        }
        break;
      }

    case 2: {                                // 10 msレンジ
        timeExec = 80 + 50;                  // 概算実行時間(ms) EEPROM保存までのカウントダウンに使用
        ADCSRA = ADCSRA & 0xf8;              // 下3ビットをクリア
        ADCSRA = ADCSRA | 0x07;              // 分周比128 (arduinoのオリジナル）
        for (int i = 0; i < REC_LENGTH; i++) {     // 200データー
          waveBuff[i] = analogRead(0);       // 約112μs
          delayMicroseconds(288);            // サンプリング周期調整
        }
        break;
      }

    case 3: {                                // 5 msレンジ
        timeExec = 40 + 50;                  // 概算実行時間(ms) EEPROM保存までのカウントダウンに使用
        ADCSRA = ADCSRA & 0xf8;              // 下3ビットをクリア
        ADCSRA = ADCSRA | 0x07;              // 分周比128 (arduinoのオリジナル）
        for (int i = 0; i < REC_LENGTH; i++) {     // 200データー
          waveBuff[i] = analogRead(0);       // 約112μs
          delayMicroseconds(88);             // サンプリング周期調整
        }
        break;
      }

    case 4: {                                // 2 msレンジ
        timeExec = 16 + 50;                  // 概算実行時間(ms) EEPROM保存までのカウントダウンに使用
        ADCSRA = ADCSRA & 0xf8;              // 下3ビットをクリア
        ADCSRA = ADCSRA | 0x06;              // 分周比64 (0x1=2, 0x2=4, 0x3=8, 0x4=16, 0x5=32, 0x6=64, 0x7=128)
        for (int i = 0; i < REC_LENGTH; i++) {     // 200データー
          waveBuff[i] = analogRead(0);       // 約56μs
          delayMicroseconds(24);             // サンプリング周期調整
        }
        break;
      }

    case 5: {                                // 1 msレンジ
        timeExec = 8 + 50;                   // 概算実行時間(ms) EEPROM保存までのカウントダウンに使用
        ADCSRA = ADCSRA & 0xf8;              // 下3ビットをクリア
        ADCSRA = ADCSRA | 0x05;              // 分周比16 (0x1=2, 0x2=4, 0x3=8, 0x4=16, 0x5=32, 0x6=64, 0x7=128)
        for (int i = 0; i < REC_LENGTH; i++) {     // 200データー
          waveBuff[i] = analogRead(0);       // 約28μs
          delayMicroseconds(12);             // サンプリング周期調整
        }
        break;
      }

    case 6: {                                // 500usレンジ
        timeExec = 4 + 50;                   // 概算実行時間(ms) EEPROM保存までのカウントダウンに使用
        ADCSRA = ADCSRA & 0xf8;              // 下3ビットをクリア
        ADCSRA = ADCSRA | 0x04;              // 分周比16(0x1=2, 0x2=4, 0x3=8, 0x4=16, 0x5=32, 0x6=64, 0x7=128)
        for (int i = 0; i < REC_LENGTH; i++) {     // 200データー
          waveBuff[i] = analogRead(0);       // 約16μs
          delayMicroseconds(4);              // サンプリング周期調整
          // 時間微調整　1.875μs（nop 1つで1クロック、0.0625μs @16MHz)
          asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
          asm("nop"); asm("nop"); asm("nop");
        }
        break;
      }

    case 7: {                                // 200usレンジ
        timeExec = 2 + 50;                   // 概算実行時間(ms) EEPROM保存までのカウントダウンに使用
        ADCSRA = ADCSRA & 0xf8;              // 下3ビットをクリア
        ADCSRA = ADCSRA | 0x02;              // 分周比:4(0x1=2, 0x2=4, 0x3=8, 0x4=16, 0x5=32, 0x6=64, 0x7=128)
        for (int i = 0; i < REC_LENGTH; i++) {
          waveBuff[i] = analogRead(0);       // 約6μs
          // 時間微調整　1.875μs（nop 1つで1クロック、0.0625μs @16MHz)
          asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
          asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
          asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop"); asm("nop");
        }
        break;
      }
  }
}

void dataAnalize() {                   // 作図のための各種情報を設定
  int d;
  long sum = 0;

  // 最大最小値を求める
  dataMin = 1023;                         // 最小
  dataMax = 0;                            // 最大値記録変数を初期化
  for (int i = 0; i < REC_LENGTH; i++) {  // 最大と最小値を求める
    d = waveBuff[i];
    sum = sum + d;
    if (d < dataMin) {                    // 最小と
      dataMin = d;
    }
    if (d > dataMax) {                    // 最大値を求める
      dataMax = d;
    }
  }

  // 平均値を求める
  dataAve = (sum + 10) / 20;               // 平均値計算（精度確保のため10倍値で計算）

  // 表示のmax,minを決める
  if (vRange <= 1) {                       // Autoレンジなら（レンジ番号が1かそれ以下）
    rangeMin = dataMin - 20;               // 表示レンジ下限を-20下に設定
    rangeMin = (rangeMin / 10) * 10;       // 10ステップに丸め
    if (rangeMin < 0) {
      rangeMin = 0;                        // 但し下限は0
    }
    rangeMax = dataMax + 20;               // 表示レンジ上限を+20上に設定
    rangeMax = ((rangeMax / 10) + 1) * 10; // 切り上げで10ステップに丸め
    if (rangeMax > 1020) {
      rangeMax = 1023;                     // 但し1020以上なら1023で抑える
    }

    if (att10x == 1) {                            // アッテネータ有
      rangeMaxDisp = 100 * (rangeMax * lsb50V);   // 表示範囲はデーターで決める。つまり上限はADCのフルスケールまで
      rangeMinDisp = 100 * (rangeMin * lsb50V);   // 下限は測定結果によるが、最低でもゼロ
    } else {                                      // アッテネータ無し
      rangeMaxDisp = 100 * (rangeMax * lsb5V); 
      rangeMinDisp = 100 * (rangeMin * lsb5V);
    }
  } else {                                   // 固定レンジなら
                                             // 必要な処理をここに書く（今のところ無し）
  }

  // トリガ位置を探す
  for (trigP = ((REC_LENGTH / 2) - 51); trigP < ((REC_LENGTH / 2) + 50); trigP++) { // データー範囲の中央で、中央値を跨いでいるポイントを探す
    if (trigD == 0) {                        // トリガ方向が0（正トリガ）なら
      if ((waveBuff[trigP - 1] < (dataMax + dataMin) / 2) && (waveBuff[trigP] >= (dataMax + dataMin) / 2)) {
        break;                              // 立ち上がりトリガ検出！
      }
    } else {                                // 0でなかったら（負トリガ）
      if ((waveBuff[trigP - 1] > (dataMax + dataMin) / 2) && (waveBuff[trigP] <= (dataMax + dataMin) / 2)) {
        break;
      }                                    // 立下りトリガ検出！
    }
  }
  trigSync = true;
  if (trigP >= ((REC_LENGTH / 2) + 50)) {  // トリガが見つからなかったら中央にしておく
    trigP = (REC_LENGTH / 2);
    trigSync = false;                      // Unsync表示用フラグ
  }
}

void startScreen() {                 // 開始時の画面表示
  display.clearDisplay();
  display.setTextSize(2);            // 文字を2倍角で、
  display.setTextColor(WHITE);       //
  display.setCursor(10, 15);         //
  display.println(F("DSO start"));   // 開始画面表示
  display.setCursor(10, 35);         //
  display.println(F("    v0.8"));
  display.display();                 // 表示
  delay(1000);
  display.clearDisplay();
  display.setTextSize(1);            // 以降は標準文字サイズ
}

void dispHold() {                            // 画面にHoldを表示
  display.fillRect(32, 12, 24, 8, BLACK);    // 4文字分黒塗り
  display.setCursor(32, 12);
  display.print(F("Hold"));                  // Hold 表示
  display.display();                         //
}

void dispInf() {                             // 各種情報表示
  float voltage;
  // 垂直感度表示
  display.setCursor(2, 0);                   // 画面の左上に
  display.print(vScale);                     // 垂直感度常時
  if (scopeP == 0) {                         // スコープが当たっていたら
    display.drawFastHLine(0, 7, 27, WHITE);  // 下側枠線を表示
    display.drawFastVLine(0, 5, 2, WHITE);
    display.drawFastVLine(26, 5, 2, WHITE);
  }

  // 水平感度表示
  display.setCursor(34, 0);                  //
  display.print(hScale);                     // 横軸スケール(time/div)表示
  if (scopeP == 1) {                         // スコープが当たっていたら
    display.drawFastHLine(32, 7, 33, WHITE); // 下側枠線を表示
    display.drawFastVLine(32, 5, 2, WHITE);
    display.drawFastVLine(64, 5, 2, WHITE);
  }

  // トリガ極性表示
  display.setCursor(75, 0);                  // 波形の中央上に
  if (trigD == 0) {
    display.print(char(0x18));               // トリガ極性表示↑
  } else {
    display.print(char(0x19));               //               ↓
  }
  if (scopeP == 2) {      // スコープが当たっていたら
    display.drawFastHLine(71, 7, 13, WHITE); // 下側枠線を表示
    display.drawFastVLine(71, 5, 2, WHITE);
    display.drawFastVLine(83, 5, 2, WHITE);
  }

  // 平均電圧表示
  if (att10x == 1) {                         // 10倍アッテネーターが入っていたら
    voltage = dataAve * lsb50V / 10.0;       // 50Vレンジの感度係数で電圧計算
  } else {
    voltage = dataAve * lsb5V / 10.0;        // 5Vレンジの感度係数で電圧計算
  }
  dtostrf(voltage, 4, 2, chrBuff);           // x.xx 形式に変換
  display.setCursor(98, 0);                  // 画面の右上に
  display.print(chrBuff);                    // 電圧の平均値を表示
  //  display.print(saveTimer);                  // デバッグ用の表示はここが便利

  // 縦軸目盛り表示
  voltage = rangeMaxDisp / 100.0;            // Max電圧を換算
  if (vRange == 1 || vRange > 4) {           // 感度が5V以下もしくはAuto5Vなら
    dtostrf(voltage, 4, 2, chrBuff);         //  *.** 形式に変換
  } else {                                   //
    dtostrf(voltage, 4, 1, chrBuff);         // **.* 形式に変換
  }
  display.setCursor(0, 9);
  display.print(chrBuff);                    // Max値表示

  voltage = (rangeMaxDisp + rangeMinDisp) / 200.0; // 中央値を計算
  if (vRange == 1 || vRange > 4) {           // 感度が5V以下もしくはAuto5Vなら
    dtostrf(voltage, 4, 2, chrBuff);         // 小数点以下2桁
  } else {                                   //
    dtostrf(voltage, 4, 1, chrBuff);         // 小数点以下1桁
  }
  display.setCursor(0, 33);
  display.print(chrBuff);                    // 中心値表示

  voltage = rangeMinDisp / 100.0;            // Min値を計算
  if (vRange == 1 || vRange > 4) {           // 感度が5V以下もしくはAuto5Vなら
    dtostrf(voltage, 4, 2, chrBuff);         // 数点以下2桁
  } else {
    dtostrf(voltage, 4, 1, chrBuff);         // 小数点以下1桁
  }
  display.setCursor(0, 57);
  display.print(chrBuff);                    // Min値表示

  // トリガ検出ミス表示
  if (trigSync == false) {                   // トリガの検出に失敗していたら
    display.setCursor(60, 55);               // 画面の中央下に
    display.print(F("Unsync"));              // Unsync を表示
  }
}

void plotData() {                    // 配列の値に基づきデーターをプロット
  long y1, y2;
  for (int x = 0; x <= 98; x++) {
    y1 = map(waveBuff[x + trigP - 50], rangeMin, rangeMax, 63, 9); // プロット座標へ変換
    y1 = constrain(y1, 9, 63);                                     // はみ出し部は潰す
    y2 = map(waveBuff[x + trigP - 49], rangeMin, rangeMax, 63, 9); //
    y2 = constrain(y2, 9, 63);                                     //
    display.drawLine(x + 27, y1, x + 28, y2, WHITE);               // 点間を線で結ぶ
  }
}

void saveEEPROM() {                    // ボタン操作が終わって少し待ってから、EEPROMに設定値を保存
  if (paraChanged == true) {           // もしパラメータ変更があった場合は
    saveTimer = saveTimer - timeExec;  // カウントダウンタイマー更新
    if (saveTimer < 0) {               // タイムアップしたら
      paraChanged = false;             // パラメーター変更フラグクリア
      EEPROM.write(0, vRange);        // 現在の設定状態を保存
      EEPROM.write(1, hRange);
      EEPROM.write(2, trigD);
      EEPROM.write(3, scopeP);
    }
  }
}

void loadEEPROM() {                // EEPROMに保存した設定値を読み出す
  int x;
  x = EEPROM.read(0);             // vRange
  if ((x < 0) || (x > 9)) {        // 0-9の範囲外だったら
    x = 5;                         // デフォルト値設定
  }
  vRange = x;

  x = EEPROM.read(1);             // hRange
  if ((x < 0) || (x > 7)) {        // 0-9の範囲外だったら
    x = 3;                         // デフォルト値設定
  }
  hRange = x;
  x = EEPROM.read(2);             // trigD
  if ((x < 0) || (x > 1)) {        // 0-9の範囲外だったら
    x = 1;                         // デフォルト値設定
  }
  trigD = x;
  x = EEPROM.read(3);             // scopeP
  if ((x < 0) || (x > 2)) {        // 0-9の範囲外だったら
    x = 1;                         // デフォルト値設定
  }
  scopeP = x;
}

void pin2IRQ() {                   // Pin2(int0)割込みの処理
//　操作ボタン（タクトスイッチ）のpin8,9,10,11はダイオードで束ねてPin2に
//　接続されている。つまりいずれかのボタンが押されればここのルーチンが発動。

// PINB 8 Select, 9 Up, 10 Down, 11 Hold
// PIND 0 (01), 1 (02), 2 (04), 3 Select (0x08), 4 Up (0x10), 5 Down (0x20), 6 Hold (0x40)
  
  int x;                           // ポート情報保持変数
  x = PIND;                        // ポートBの情報を読む

  if ( (x & 0x07) != 0x07) {       // 下位3ビットが全部Highで無ければ（どれかが押されていたら）
    saveTimer = 5000;              // EEPROM保存タイマーセット(ms単位で設定）
    paraChanged = true;            // パラメータ変化有りフラグON
  }

  if ((x & 0x08) == 0) {
    scopeP++;
    if (scopeP > 2) {
      scopeP = 0;
    }
  }

  if ((x & 0x10) == 0) {           // UPボタンが押されていて、
    if (scopeP == 0) {             // 垂直レンジにスコープが当たっていたら
      vRange++;
      if (vRange > 9) {
        vRange = 9;
      }
    }
    if (scopeP == 1) {             // 水平レンジにスコープが当たっていたら
      hRange++;
      if (hRange > 7) {
        hRange = 7;
      }
    }
    if (scopeP == 2) {             // トリガ方向ボタンにスコープが当たっていたら
      trigD = 0;                   // プラストリガ
    }
  }

  if ((x & 0x20) == 0) {           // DOWNボタンが押されていて、
    if (scopeP == 0) {             // 垂直レンジにスコープが当たっていたら
      vRange--;
      if (vRange < 0) {
        vRange = 0;
      }
    }
    if (scopeP == 1) {             // 水平レンジにスコープが当たっていたら
      hRange--;
      if (hRange < 0) {
        hRange = 0;
      }
    }
    if (scopeP == 2) {             // トリガ方向ボタンにスコープが当たっていたら
      trigD = 1;                   // マイナストリガ
    }
  }

  if ((x & 0x40) == 0) {           // HOLDボタンが押されていたら
    hold = ! hold;                 // フラグ反転
  }
}
