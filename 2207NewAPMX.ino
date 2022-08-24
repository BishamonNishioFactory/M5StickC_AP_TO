//2022/7/14(木曜日)完成！
//2022/8/24 GitHubに登録完了。今後は、大地GMと大島に期待してチーム開発に以降する。

//昼ミーティングにてお披露目。大地GMに設置を任せる。標準作業管理票は酒井GMが作成する。
//G36 がPULLUP。3.3V を入力すれば、HIGHとなる。5Vでは反応しない、あるいは飛んでしまうという現象。

//G0,G26 は、それぞれOUTPUT。G0スイッチセンシングあるいは、あんどん作動状態においてパトライトを点灯させる。ここは5Vが流れている。

//NewHPT仕上ハンドラインをベースとして、仕上混流ラインを作成する。
//2022/7/7
//
//コンセプト
//仕上混流工程では自動センシングは困難。
//工場の最終工程として、検査・梱包が最も大切。
//最終工程の着完を把握することで、台数をカウントする。
//同時に、黄色パトライトで廻りに検査中であることを知らしめる。
//検査中は、検査担当者に絶対に声がけしてはならない。

//SW操作により、
// ONでRUN1を送信＋黄色パトライト起動
//
// OFFでRUN2を送信＋黄色パトライト停止
//
//デュアルボタンにより、従来通りのあんどん発動（仕上混流）

//ポンプの数字が今ひとつおかしい。実施より少なく感じる。
//もしかしたら、実在しない変数でのif の下りが悪さしているのかもしれない。
//そのあたりを修正した。2022/6/30


//仕上げハンドラインのシステムを再構築
//2022/6/1 Start

//ESP8266を改め、M5StickCでつくる


//G0とG26を共にインプットプルアップとする。G0をポンプ、G26をフレームのcur_value1入力とする。
//ただし、十進数変換をわかりやすくするために、プルアアップをコードで逆転させる

//画面設計は、それぞれのSWの反応回数のみを、シンプルに表示させるだけにととめる。


//本プログラムは、M5StickC に最適化されている。(Plus でも動作はするが、画面表示が乱れる)
#include <M5StickC.h>
//#include <M5StickCPlus.h>

#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include <WiFiMulti.h>
#include <HTTPClient.h>

//IOXhop_FirebaseESP32のヘッダーファイルがいつの間にかなくなっていた。
//ライブラリ https://github.com/ioxhop/IOXhop_FirebaseESP32　からzipを再入手して、IDEからインクルードし直したら治った。　2021/10/25
#include <IOXhop_FirebaseESP32.h>  

//ArduinoJson のバージョンは、5.13.5であること。
//バージョンが６の場合、エラーとなる。2021/10/25
#include "ArduinoJson.h"


#include <time.h>
#define JST 3600*9
//#define JST 0

WiFiClientSecure httpsClient;

int input1=0;
int input2=26;
int input3=36;

int sw=0;
int before_sw;
int beforeinput=99;

boolean swChange = false;
long swStartMills=0; //前回実行の時間を格納する。

long longBeforeconnect = 0 ; //６０秒ごとにWifiのコネクト状態を確認する

bool boolStatus1 = false;
bool boolStatus2 = false;

long longMillis1 = 0;
long longMillis2 = 0;

int intNowMillis1 = 0;
int intNowMillis2 = 0;

long NowMillis1 = 0;
long NowMillis2 = 0;

long nextFirebaseTry = 0;
//===機械の設定====================================================  

//String MachineNo1 = "GT999";  //
String MachineNo1 = "AP_MX";  //
//String MachineNo1 = "AP_TO";  //
//String MachineNo2 = "HPT_F";  //

//===WiFi設定===================================================================
//
//#define WIFI_SSID "GlocalMe_88440" // ①
//#define WIFI_PASSWORD "85533446"

const char* WIFI_SSID = "B_IoT";
const char* WIFI_PASSWORD = "wF7y82Az";

//===Firebase==================================================================


#define FIREBASE_DB_URL "https://ay-vue.firebaseio.com/" // 

//#define FIREBASE_DB_URL "https://akilog-default-rtdb.firebaseio.com/" // 

String user_path2 = "NishioMachineCT";

const String JustNowAndonStatus = "JustNowStatus";

//===プログラムで使う変数==================================================================

time_t nowTime;
String startTime1 = "999";
//String startTime2 = "999";

long beforeMiriSec = 0 ;

//WiFiMulti WiFiMulti;
int count = 1;  // ③

boolean ErrBool = false ;  //ERR1でtrurとする　○秒以下では処理を行わない、の為の実装
boolean RunBool = false ;  //RUN1でtrueとする　○秒以下では処理を行わない、の為の実装

boolean SetKKT = false ;  //BtnA １秒以上の長押しで!(否定演算子)で反転させる。


int intNowMillis = 0;
int incrementSeconds = 0;

boolean RunBool1 = true ;  //RUN1でtrueとする　○秒以下では処理を行わない、の為の実装
boolean RunBool2 = false ;  //RUN1でtrueとする　○秒以下では処理を行わない、の為の実装

int last_value1 = 0,last_value2 = 0; //DualBotton によるあんどん機能で使用
int cur_value1 = 0,cur_value2 = 0;

//====================================================

// 時計の設定
//https://qiita.com/tranquility/items/5d0b1a259a0570be35ec
const char* ntpServer = "ntp.jst.mfeed.ad.jp"; // NTPサーバー
const long  gmtOffset_sec = 9 * 3600;          // 時差９時間
//#define JST 0
const int   daylightOffset_sec = 0;            // サマータイム設定なし

RTC_TimeTypeDef RTC_TimeStruct; // 時刻
RTC_DateTypeDef RTC_DateStruct; // 日付

unsigned long setuptime; // スリープ開始判定用（ミリ秒）
//unsigned long setuptime2nd; // wifi再接続不調時のリブート用（ミリ秒）

int sMin = 0; // 画面書き換え判定用（分）
int sHor = 0; // 画面書き換え判定用（時）
int sDat = 0; //

//===Firebase==============================================
//=======================================================================================
#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFDA0      /* 255, 180,   0 */
#define TFT_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define TFT_PINK        0xFC9F
//=======================================================================================

//Slack○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○

//テスト用
//const String webHook_URL = "/services/THP92F74L/B03C4SJPR8S/Eav0RRsIn5N7u4S6GKAo5Rkr";

//HPライン
//const String webHook_URL = "/services/THP92F74L/B03FN9NM56K/XCIuJbt9zXp3rlkWwXdRdxn2"; 

//HFライン
//const String webHook_URL = "/services/THP92F74L/B03DBEKB3NY/W4N1EmcYWsPwY83Kz3guxKkF";

//仕上ハンドライン
//const String webHook_URL = "/services/THP92F74L/B03FNCS80VD/dNt13ENC60LnaJzzpiGG1YuD";

//仕上混流ライン
const String webHook_URL = "/services/THP92F74L/B03FNAP0J9M/Wd98uBQwFmI1UlvbOxihe8qR";


//const String ThisAndon = "HP";
const String ThisAndon = "SHF";

int R_switch=32;
int B_switch=33;

const char *server = "hooks.slack.com";

const String message = "slack emoji test";

//String NowAreaName = "仕上ハンドポンプ〜";

//const char *json4 ="{\"text\": \"" + message + "\", \"icon_emoji\": \":space_invader:\"}";
//const char *json4 ="{\"text\": \"テストです\", \"icon_emoji\": \":space_invader:\"}";
//const char *json4 ="{\"text\": \"テストです:space_invader:\"}"; //一応は成功か？文章内の絵文字としてスペースインベーダーが現れた！
const char *json ="{\"text\": \":pig:あんどん作動！TLは現場へ\"}"; //やっぱり成功といえる。本当に実装したいのは、アイコンの方！

//const char *json4 ="{\"icon_emoji\":\":tiger:\",\"text\": \"HPライン送信テストです:snail:\"}"; //やっぱり成功といえる。本当に実装したいのは、アイコンの方！

//const char *json = "{\"text\":\"あんどん作動:ghost！TLは現場へ\",\"icon_emoji\":\":ghost:\",\"username\":\"m5stackpost\"}";
const char *json2 = "{\"text\":\":frog:【３分超！】GMは至急現場へ\",\"icon_emoji\": \":snail:\"}";
const char *json3 = "{\"text\":\":panda_face:【６分超！】あら！？生技TL頼みます！\",\"icon_emoji\":\":tiger:\",\"username\":\"m5stackpost\"}";



bool json2Status = false;
bool json3Status = false;

//以下はsllackの証明で、全て同じ。
const char* slack_root_ca= \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n" \
"-----END CERTIFICATE-----\n" ;

HTTPClient http;

void slack_connect(int SendCount){
//  M5.Beep.tone(7000);
  delay(50);
//  M5.Beep.mute();
  // post slack
//  M5.Lcd.println("connect slack.com");




//  webhook送信
  http.begin( server, 443,webHook_URL, slack_root_ca );
//  http.begin( server, 443, "/services/THP92F74L/B03C4SJPR8S/Eav0RRsIn5N7u4S6GKAo5Rkr", slack_root_ca );

//  HFライン
//  http.begin( server, 443, "/services/THP92F74L/B03DBEKB3NY/W4N1EmcYWsPwY83Kz3guxKkF", slack_root_ca );



  String ReturnMSG = "";

  while(ReturnMSG !="200") { //リターン値が、200（正常終了）ではない場合...初期値には""が代入されているので、必ず１回はループする

    //WiFi接続状況を確認する
    if( WiFi.status() != WL_CONNECTED) {
      M5.Lcd.fillRect(5,85,150,60,YELLOW);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(6, 86, 1);
      M5.Lcd.setTextColor(BLACK); 
      M5.Lcd.printf("Connecting to %s\n", WIFI_SSID);
      int cnt2 = 0;
      while(WiFi.status() != WL_CONNECTED) {
        cnt2++;
        delay(500);
        M5.Lcd.print(".");
        if(cnt2%10==0) {
          WiFi.disconnect();
          WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
          M5.Lcd.println("");
        }
        if(cnt2>=30) {
          ESP.restart();
        }
      }
    }



    //slackへのpost実行
    http.addHeader("Content-Type", "application/json" );
    switch(SendCount){
      case 0:
            ReturnMSG = http.POST((uint8_t*)json, strlen(json));  
            break;
      case 1:
            ReturnMSG = http.POST((uint8_t*)json2, strlen(json2));  
            break;
      case 2:
            ReturnMSG = http.POST((uint8_t*)json3, strlen(json3));  
            break;
    }
    
  }
  
  
  Serial.println(ReturnMSG);

  if(ReturnMSG=="200"){
    Serial.println("slack送信成功！");
  }
  
//  M5.Lcd.println("post hooks.slack.com");

}
//○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○○




void printLocalTime() {
  static const char *wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};
  M5.Rtc.GetTime(&RTC_TimeStruct); // 時刻の取り出し
  M5.Rtc.GetData(&RTC_DateStruct); // 日付の取り出し

  M5.Lcd.setRotation(3);
  
  if (sMin == RTC_TimeStruct.Minutes) {
    M5.Lcd.fillRect(140,10,20,10,BLACK); // 秒の表示エリアだけ書き換え
  } else {
//    M5.Lcd.fillScreen(BLACK); // 「分」が変わったら画面全体を書き換え
    M5.Lcd.fillRect(0, 10, 139, 100, BLACK);
//     Adafruit();
  }

  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setCursor(0, 10, 7);  //x,y,font 7:48ピクセル7セグ風フォント

  M5.Lcd.printf("%02d:%02d", RTC_TimeStruct.Hours, RTC_TimeStruct.Minutes); // 時分を表示

  M5.Lcd.setTextFont(1); // 1:Adafruit 8ピクセルASCIIフォント
  M5.Lcd.printf(":%02d\n",RTC_TimeStruct.Seconds); // 秒を表示

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(5, 65, 1);  //x,y,font 1:Adafruit 8ピクセルASCIIフォント
//  M5.Lcd.printf("Date:%04d.%02d.%02d %s\n", RTC_DateStruct.Year, RTC_DateStruct.Month, RTC_DateStruct.Date, wd[RTC_DateStruct.WeekDay]);
  M5.Lcd.printf("%04d.%02d.%02d %s\n", RTC_DateStruct.Year, RTC_DateStruct.Month, RTC_DateStruct.Date, wd[RTC_DateStruct.WeekDay]);

  M5.Lcd.drawRect(95, 63, 55, 15, BLUE);
  M5.Lcd.setCursor(105, 66, 1);
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.println("AP_MX");

  sMin = RTC_TimeStruct.Minutes; // 「分」を保存
  sHor = RTC_TimeStruct.Hours; // 「時」を保存

//  long NowMillis1 = longMillis1

  NowMillis1 = millis() - longMillis1;
  NowMillis2 = millis() - longMillis2;


//  
//  Serial.println("★★★★★★★★★");
//  Serial.println(longMillis1);
//  Serial.println(millis());
//  Serial.println(NowMillis1);
//  Serial.println(boolStatus1);
//
//  Serial.println("★★★★★★★★★★★★★★★★★★");
//  Serial.println(longMillis2);
//  Serial.println(millis());
//  Serial.println(NowMillis2);
//  Serial.println(boolStatus2);

  
//センサー反応後、６０秒経過したら、終了satusを送信する。
//ライン停滞でセンサー入りっぱなしの場合も多く、マシンアワーとして把握する意味も無いため。
//  if(NowMillis1 >= 60000 && boolStatus1 == true){
//    sendToFirebase(MachineNo1,"RUN2");
//    boolStatus1 == false;
//  }
//
//  if(NowMillis2 >= 60000 && boolStatus2 == true){
//    sendToFirebase(MachineNo2,"RUN2");
//    boolStatus2 == false;
//  }
//
}

//=======================================================================================
//Firebaseへの送信
//◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆
void FirebaseAndonSend(bool NowStatus){
  M5.Lcd.setCursor(0,0);
//  M5.Lcd.setTextSize(2);
//  M5.Lcd.printf("Send to Firebase");
  int FirebaseAndonCount =0;
  Firebase.set("/AndonStatus/"+JustNowAndonStatus,NowStatus);
  while(Firebase.failed()){
    FirebaseAndonCount ++;
    Serial.print("★★★Firebase 送信失敗★★★リトライ");
    Serial.print(FirebaseAndonCount);
    Serial.println(Firebase.error());  
//      Serial.println(String(t) + "000");
    M5.Lcd.fillScreen(BLACK);
    delay(500);
    M5.Lcd.print(".");
    if(FirebaseAndonCount%10==0){       //500msecX１０回で５秒経過
      M5.Lcd.println("");
      M5.Lcd.println("Send to Firebase"+FirebaseAndonCount);
      Serial.println("Firebase 再送信実行！");
      //WiFiを検査し、切断されている場合は再接続を試みる””””””””””””””””””””””””””””””””””””””
      while(WiFi.status() != WL_CONNECTED) {
        Serial.println("wifi再接続実行！"); 
        wifiConnect();
      }
      Firebase.set("/AndonStatus/"+JustNowAndonStatus,NowStatus);
    }

    

    if(FirebaseAndonCount >100){
      Serial.print("リトライが100回を超過しました。再起動します");
      M5.Lcd.fillScreen(RED);
      M5.Lcd.printf("Restart later 5sec");
      delay(5000);
      ESP.restart();
      break;
    }
  }


}

//◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆
void sendToFirebase(String NowMachine,String NowStatus){
//  int LED_pin2=36;
  digitalWrite(GPIO_NUM_10, LOW);
  delay(10);
  M5.Lcd.setRotation(3);
//  if(NowMachine == MachineNo1){     //センサーを２つ使っているので、１つ目か否か（２つ目）を判定している
    if(NowStatus=="RUN1"){
      Serial.println("ん？入ってる？");
      digitalWrite(input1, HIGH); 
      boolStatus1 = true;
//      longMillis1 = millis();
     
      M5.Lcd.fillCircle(150, 30, 8, RED);
      M5.Lcd.drawCircle(150, 30, 8, WHITE);
      
      M5.Lcd.setCursor(148,26);
      M5.Lcd.print("0");

//      digitalWrite(input3,HIGH);//input3は、検査実施中の黄色パトライト

    }else{
      digitalWrite(input1, LOW); 
      boolStatus1 = false;
      
      M5.Lcd.fillCircle(150, 30, 8, BLACK);
//      digitalWrite(input3,LOW);
    }
     longMillis1 = millis();
    
            
//  }else{
//    if(NowStatus=="RUN1"){
//
//      boolStatus2 = true;
//      longMillis2 = millis();
//      
//      M5.Lcd.fillCircle(150, 50, 8, BLUE);
//      M5.Lcd.drawCircle(150, 50, 8, WHITE);
//      
//      M5.Lcd.setCursor(146,47);
//      M5.Lcd.print("26");
//
//    }else{
//      boolStatus2 = false;
//      
//      M5.Lcd.fillCircle(150, 50, 8, BLACK);
//    }
//    longMillis2 = millis();
//    
//  }
  

  time_t t;
  t = time(NULL);

  Serial.println("■■■■■■■■■■■■■■■■■■■■■■■");
//  Serial.println(String(t) + "000");
  
  //Buffer para a criação dos jsons
  StaticJsonBuffer<150> jsonBufferSensor; 
  JsonObject& sensorData = jsonBufferSensor.createObject(); 


//  if(NowMachine == MachineNo1){     //センサーを２つ使っているので、１つ目か否か（２つ目）を判定している
    sensorData["startTime"] = startTime1 ;           //前回の停止時間と同じ（370行目あたりで格納している）
//  }else{
//    sensorData["startTime"] = startTime2 ;
//  }
  
  sensorData["endTime"] = String(t) + "000" ; //送信時点の時間
//  sensorData["endTime"] = String(nowTime) + "000" ; //センサー反応時点にさかのぼった時間 あかんよーわからんわー
                                                      //特にエラー赤色灯の時間が若干短くなってしまうが、堪忍してちょ

//  センサー反応時点の時刻を表示させたいが、ダメだ。不完全。
//  struct tm *tm;
//  static const char *wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};


//  t = time(NULL);
//  t = (String(nowTime) + "000").toInt();
//  tm = localtime(&t);
//  Serial.printf(" %04d/%02d/%02d(%s) %02d:%02d:%02d\n",
//        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
//        wd[tm->tm_wday],
//        tm->tm_hour, tm->tm_min, tm->tm_sec);
//   delay(3000);
//  
  sensorData["machine"] = NowMachine ;
  sensorData["status"] = NowStatus ;
  
//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
//  Firebase.set("/PresentData/"+MachineNo, sensorData); //2021/7/23 実験成功！！
                                                             //Cloud Function のデプロイがうまくいかないが、
                                                             //ここで、pushとsetを送信することで、
                                                             //web上の読み込み量を劇的に低下させることが出来るはず！
                                                             //ただし、CloudFunction がデプロイ出来るようになったら、
                                                             //WiFi等の通信負荷を低減するために、この方法はやめるべきである。
//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
  delay(10);
  Firebase.push("/NishioMachineCT", sensorData);

  if(Firebase.failed()){
    Serial.println("FirebasePushに失敗しました！！！！");
  }else{
//    Serial.println("FirebasePush大成功！！！！");
  }

 //WiFi接続状態を確認して、不調であれば、nextFirebaseTry　で設定した時間でリトライを行う　+++++++++++ 
  nextFirebaseTry= millis() + (1000 * 200);//200秒間///送信リトライを継続するための基準時刻
  int FirebaseFailedCount = 0;
  while(Firebase.failed()){
//    WiFi.disconnect();
//    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    FirebaseFailedCount ++;

              //WiFiを検査し、切断されている場合は再接続を試みる””””””””””””””””””””””””””””””””””””””
                
                M5.Lcd.setRotation(3);
                M5.Lcd.setTextSize(1);
                M5.Lcd.setTextColor(WHITE);
                M5.Lcd.setCursor(0, 0, 1);
                
                M5.Lcd.fillScreen(BLUE);
                M5.Lcd.printf("ReTryWiFi %s\n", WIFI_SSID);
                M5.Lcd.setTextSize(1);
              int cnt3 = 0;
              while(WiFi.status() != WL_CONNECTED) { 
                
                cnt3++;
                delay(500);
                M5.Lcd.print(".");
                if(cnt3%20==0) {
                  WiFi.disconnect();
                  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
                  M5.Lcd.println("");
                }
                if(cnt3>=100) {   //1500sec x 50 = 7500sec(75秒)　経過で再起動。通常のsetup となる。
                  ESP.restart();
                }
              }
              //””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””””

    Firebase.push("/NishioMachineCT", sensorData);
    if(Firebase.failed()){
      Serial.print("★★★★★★★★★★Firebase 送信失敗★★★★★★★★★");
      Serial.println(Firebase.error());  
      Serial.println(String(t) + "000");
      
    }else{
      int MyRand = 16581265 - random(23);
//      lineNotify("FB.failed()から復活",stickerPackage3,MyRand);
//      lineNotify("FB.failed()から復活(" + FirebaseFailedCount + ")回目" + MachineNo,stickerPackage3,MyRand);
//      lineNotify("Firebase.failed()発動" + FirebaseFailedCount + "回目",stickerPackage3,MyRand);
      Serial.println("■■■■■■■■Firebase 送信成功■■■■■■■■■");
      Serial.println(String(t) + "000");
    }
   
    delay(1000);
    
    if(nextFirebaseTry<millis()){
      Serial.print("100秒を超過しました。リトライを終了します");
      break;
    }
  }
  //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//  startTime = String(t) +"000";
//  if(NowMachine == MachineNo1){     //センサーを２つ使っているので、１つ目か否か（２つ目）を判定している

    startTime1 = String(t) +"000";          
//  }else{
//    startTime2 = String(t) +"000";     
//  }
  

  
  jsonBufferSensor.clear();

  digitalWrite(10,HIGH); //内蔵LEDを消灯
//  digitalWrite(input2, LOW); 
}


WiFiClientSecure client;

void wifiConnect(){
  M5.Lcd.setRotation(3);
  M5.Lcd.setCursor(0, 0, 1);
  M5.Lcd.fillScreen(BLACK);
  
  int cnt=0;
  M5.Lcd.printf("Connecting to %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  while(WiFi.status() != WL_CONNECTED) {
    cnt++;
    delay(500);
    M5.Lcd.print(".");
    if(cnt%10==0) {
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      M5.Lcd.println("");
    }
    if(cnt>=60) {
      ESP.restart();
    }
  }
//  
//  int MyRand = 16581265 - random(23);
//  lineNotify("WiFiに再接続しました。",stickerPackage3,MyRand);
  M5.Lcd.printf("\nWiFi connected\n");
  delay(500);
  M5.Lcd.fillScreen(BLACK);
}
//◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆◆

//void EndProcess(){
////    digitalWrite(LED_pin2,HIGH);  
////    cur_value1 = digitalRead(B_switch);
////    delay(300);
////    cur_value2 = digitalRead(B_switch);
////    
////    if(cur_value1==cur_value1){
//      BoolNowStatus = false;
//    
//      RunBool1=false;
//      sendToFirebase(MachineNo,"RUN2"); //稼働中（cdsセンサー緑を検知）
////    }
//    
//}

void setup() {

  setCpuFrequencyMhz(80);
  M5.begin(); 
  M5.Lcd.setRotation(3);
  M5.Lcd.setCursor(0, 0, 1);
  M5.Lcd.fillScreen(BLACK);


  wifiConnect();
//INPUT_PULLUPは、接続されていないときに、HIGH(1)となる。
//https://mag.switch-science.com/2013/05/23/input_pullup/
//  pinMode(input1,INPUT_PULLUP);    //0　　
  
//  pinMode(input3,INPUT);    //36　　
//  pinMode(input3,INPUT_PULLUP);    //36　

//  G36はインプット専用で、入力は3.3Vのみ認識する（5Vは認識しない、あるいは飛んでしまう）
//  サイトによっては、PULLUPが無効との記述もあるが、下記は、HAT(PIR)のサンプルから流用している。

  
  pinMode(36,INPUT_PULLUP);
  
  pinMode(input2,OUTPUT);    //26   青色パトライト（あんどん）
  pinMode(input1,OUTPUT);    //0 　　　黄色パトライト（検査実施中）
//  pinMode(input1,INPUT_PULLUP);//36



  pinMode(R_switch,INPUT);
  pinMode(B_switch,INPUT);

  pinMode(GPIO_NUM_10, OUTPUT);       
  digitalWrite(GPIO_NUM_10, HIGH);    //なぜか、起動時からLEDが光ってしまうので、強制的に消灯させる。
  

  Firebase.begin(FIREBASE_DB_URL);   // ④


//======================================================================
//なぜか、時刻が時差を考慮されないケースがある。
//何度かやり直して、2204_Watch_ENV_LS_Patoからsetup対象部分を移植したら成功した

  configTime( JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");


// Get local time
    struct tm timeInfo;
    if (getLocalTime(&timeInfo)) {
      M5.Lcd.print("NTP : ");
      M5.Lcd.println(ntpServer);

      // Set RTC time
      RTC_TimeTypeDef TimeStruct;
      TimeStruct.Hours   = timeInfo.tm_hour;
      TimeStruct.Minutes = timeInfo.tm_min;
      TimeStruct.Seconds = timeInfo.tm_sec;
      M5.Rtc.SetTime(&TimeStruct);

      RTC_DateTypeDef DateStruct;
      DateStruct.WeekDay = timeInfo.tm_wday;
      DateStruct.Month = timeInfo.tm_mon + 1;
      DateStruct.Date = timeInfo.tm_mday;
      DateStruct.Year = timeInfo.tm_year + 1900;
      M5.Rtc.SetData(&DateStruct);
    }

  time_t t;
  t = time(NULL);
  client.setInsecure();



  longBeforeconnect = millis();
  

  printLocalTime();
  if(digitalRead(input1)==LOW){ //再起動時にRUN状態であれば、強制的にRunBoolｗを真にする
    RunBool=true;
  };

  delay(100);
  beforeMiriSec = millis();
//  M5.Lcd.fillScreen(BLACK);
  
}

void loop() {
   M5.update();
   Serial.print("センサーの値は");
   Serial.println(digitalRead(36));
//   Serial.println(digitalRead(input3));
    delay(10);
   //WiFiを検査し、切断されている場合は再接続を試みる””””””””””””””””””””””””””””””””””””””
    while(WiFi.status() != WL_CONNECTED) { 
      wifiConnect();
    }
    
    boolean chkflag=false;
    int before_sw1_gyaku;
    int before_sw2_gyaku;
    int sw1_gyaku;
    int sw2_gyaku;
    long nowMillis;

//    int before_sw1=digitalRead(input3);     //G0
    int before_sw1=digitalRead(36);     //G0

    

    if(SetKKT){//KKT計画停止モード の場合おは、before_swを８にセットする
    //  Serial.print(SetKKT);
      before_sw=8;
    }else{
    //  before_sw=0;  //KKT計画停止モードでは無い場合、before_swを０にセットする
//      before_sw1_gyaku = before_sw1;
//        if(before_sw1==1){
//          before_sw1=0;
//        }else{
//          before_sw1=1;
//        }
    }
    
   delay(300); //チャタリング対策300ミリsecをdelayさせることで、反応し続けていると判断出来る。
                //これ以上長いと間延びした感じになる。短いとフラッシュ点滅に反応してしまう。

   M5.Lcd.setRotation(3);
   nowMillis = millis()-swStartMills;

   delay(10);
//   int sw1=digitalRead(input3); //G0
   int sw1=digitalRead(36); //G0

    if(SetKKT){
      sw=8;
    }else{
//      sw=0;
//    
//        sw1_gyaku = sw1;
//        sw=sw1_gyaku;
//        if(sw1==1){
//          sw1=0;
//        }else{
//          sw1=1;
//        }
    }
   //チャタリング対策
  if (before_sw1==sw1){     
      chkflag=true;             //３００ミリsecの差を読み取り、センサー値を確定(チャタリングでは無いと判断)
  
  }else{
      chkflag=false;
  }

  if(chkflag==true){          //チャタリングでは無い場合 =>ほとんどの場合は真となる。
    Serial.print("センサー起動　sw.....=、");
    Serial.println(sw1);
    delay(10);


    if(beforeinput!=sw1){  //値に変化がある場合
      Serial.print("あははは");
      swChange=true;            //センサー値変化=>値変化変数swChangeを真とする
      swStartMills=millis();    //センサー値変化のmillis()時刻を記録
      time_t nowTime;
      nowTime = time(NULL);           //センサー値変化時点のRTC時刻を記録
      
    }
    

    //センサー起動時ににFirebase(Run1)送信を行い、６０秒後に自動的にfirebase(Run2）送信を行う    
    if(swChange==true){       //値変化変数swChangeが真の場合
          Serial.println("swChangeに入りました");
            delay(10);
            switch (sw1){
              case 0:
                    if(nowMillis>500){       //0.5秒以上点灯している状態（寸動を除去する）    
                        switch(beforeinput){
                          case 0:

                                break;
                          case 1:
                                if(RunBool){
                                  sendToFirebase(MachineNo1,"RUN2"); 
                                  RunBool = false;
                                };
//                                sendToFirebase(MachineNo1,"RUN2"); //稼働中（cdsセンサー緑を検知）
//                                RunBool = false;
//                                break;
                          
                                break;

                          case 99:
                                //初期値（９９）の場合は何も処理をしない。beforeinputが最新ｓｗの値に更新される
                                break;
                          };
                          beforeinput=sw1;   //次回サイクルに備えて、前回分として値を格納しておく。
                          swChange=false;   //値変化の処理が完了したので、値変化のフラッグを初期値（false）に戻す。
                    };
                     break;                   
              case 1://SW起動
//                     Serial.println(nowMillis);
//                     Serial.println("うふふふふ");
//                     delay(5000);
                     if(nowMillis>500){       //0.5秒以上点灯している状態（寸動を除去する）         
                       switch(beforeinput){
                          case 0:
                                delay(10);
                                
                                sendToFirebase(MachineNo1,"RUN1"); //稼働中（cdsセンサー緑を検知）
                                RunBool = true;
                                break;
                          case 1:
                                break;

                          case 99:
//                                //初期値（９９）の場合は何も処理をしない。beforeinputが最新ｓｗの値に更新される
                                break;
                          };
                          beforeinput=sw1;   //次回サイクルに備えて、前回分として値を格納しておく。
                          swChange=false;   //値変化の処理が完了したので、値変化のフラッグを初期値（false）に戻す。
                       };

                      break;


                
            }               

        }
    }


int LED_pin2=26;

  cur_value1 = 1;
  cur_value2 = 1;
//  cur_value1 = digitalRead(B_switch); // read the value of BUTTON. 
//  cur_value2 = digitalRead(R_switch);
//
  delay(10);

    M5.update();

 if(digitalRead(B_switch)==LOW){
    cur_value1 = digitalRead(B_switch);
    delay(300);
    cur_value2 = digitalRead(B_switch);
    if(cur_value1==cur_value1){
      if(RunBool1){

        M5.Lcd.fillScreen(BLUE);
        digitalWrite(LED_pin2,HIGH);      //外付けパトライトは、HIGH で通電
//        Firebase.set("/AndonStatus/"+ThisAndon,true);
        FirebaseAndonSend(true);

        Serial.println("❏❏❏❏❏❏❏点灯！");
         
        delay(100);
        incrementSeconds = 0;
        RunBool2=true;  //起動後の秒数を、 incrementSecondsに足し込むためのトリガー
        json2Status=false; //３分超メッセージのステータス
        json3Status=false; //６分超メッセージのステータス
        slack_connect(0); //スラック送信        
        
//        Firebase.setBool(firebaseData, is_power_on_key,true);
//        M5.Lcd.fillCircle(200, 110, 20, CYAN);
//        M5.Lcd.drawCircle(225, 45, 14, BLUE);
        
//        M5.Lcd.fillRect(30,90,200,40,BLUE);
        delay(10);
        if(digitalRead(R_switch)==HIGH){
          RunBool1=true;
        }
      }else{
        RunBool1=true;
      }
    }


    
  }else if(!RunBool1){
    M5.Lcd.fillScreen(RED);
    digitalWrite(LED_pin2,LOW);
    delay(100);
//    Firebase.set("/AndonStatus/"+ThisAndon,false);
    FirebaseAndonSend(false);

    Serial.println("RunBoolの値その２");
    Serial.print(RunBool1);

  
    Serial.print("消灯ーーーーーーー");
    
    delay(500);
    M5.Lcd.fillScreen(BLACK);
    
//    Firebase.setBool(firebaseData, is_power_on_key,false);  //Firebaseに、falseを代入
    RunBool1 = true;                    //これを入れることで、繰り返しが無くなる
  }

  delay(10);

  if(digitalRead(R_switch)==LOW){
    cur_value1 = digitalRead(B_switch);
    delay(300);
    cur_value2 = digitalRead(B_switch);

    RunBool2=false;
    incrementSeconds = 0;
    
    M5.Lcd.fillRect(20,90,200,40,RED);
    delay(300);
    M5.Lcd.fillRect(20,90,200,40,BLACK);
    if(cur_value1==cur_value1){
      RunBool1=false;
    }
  }
  delay(10);
 

//
//  if(digitalRead(B_switch)==LOW){
//    cur_value1 = digitalRead(B_switch);
//    delay(300);
//    cur_value2 = digitalRead(B_switch);
//    if(cur_value1==cur_value1){
//      if(RunBool1){
//        Serial.println("いいんじゃない？");
////        M5.Lcd.fillScreen(BLUE);
//        digitalWrite(input2,HIGH);      //外付けパトライトは、HIGH で通電
////        M5.Lcd.setRotation(0);
//        M5.Lcd.setCursor(1,1);
////        M5.Lcd.setCursor(3,145);
//        M5.Lcd.fillRect(0, 0, 140, 10, RED);
//        M5.Lcd.setCursor(40,1);
//        M5.Lcd.setTextColor(WHITE);
//        M5.Lcd.println("Andon sadou");
////        Firebase.set("/AndonStatus/"+ThisAndon,true);
//        FirebaseAndonSend(true);
//
//        Serial.println("❏❏❏❏❏❏❏点灯！");
//         
//        delay(100);
//        incrementSeconds = 0;
//        RunBool2=true;  //起動後の秒数を、 incrementSecondsに足し込むためのトリガー
//        json2Status=false; //３分超メッセージのステータス
//        json3Status=false; //６分超メッセージのステータス
////        slack_connect(0); //スラック送信        
//        
////        Firebase.setBool(firebaseData, is_power_on_key,true);
////        M5.Lcd.fillCircle(200, 110, 20, CYAN);
////        M5.Lcd.drawCircle(225, 45, 14, BLUE);
//        
////        M5.Lcd.fillRect(30,90,200,40,BLUE);
//        delay(10);
//      if(digitalRead(R_switch)==HIGH){
//          RunBool1=true;
//      }else{
//        RunBool1=true;
//      }
//    }
//
//
//    
//  }else if(!RunBool1){
////    M5.Lcd.fillScreen(RED);  
//    digitalWrite(input2,LOW);
////    M5.Lcd.setRotation(0);
//    M5.Lcd.fillRect(0, 0, 140, 10, BLACK);
////    M5.Lcd.fillRect(0, 145, 80, 10, BLACK);
//    delay(100);
////    Firebase.set("/AndonStatus/"+ThisAndon,false);
//    FirebaseAndonSend(false);
//
//    Serial.println("RunBoolの値その２");
//    Serial.print(RunBool1);
//
//  
//    Serial.print("消灯ーーーーーーー");
//    
//    delay(500);
//    M5.Lcd.fillScreen(BLACK);
//    
////    Firebase.setBool(firebaseData, is_power_on_key,false);  //Firebaseに、falseを代入
//    RunBool1 = true;                    //これを入れることで、繰り返しが無くなる
//  }
//
//  delay(10);
//
//  if(digitalRead(R_switch)==LOW){
//    Serial.println("そりゃそうだよね〜");
//    cur_value1 = digitalRead(R_switch);
//    delay(300);
//    cur_value2 = digitalRead(R_switch);
//
//    RunBool2=false;
//    incrementSeconds = 0;
//    
//    M5.Lcd.fillRect(20,90,200,40,RED);
//    delay(300);
//    M5.Lcd.fillRect(20,90,200,40,BLACK);
//    if(cur_value1==cur_value1){
//      RunBool1=false;
//      FirebaseAndonSend(false);
//      digitalWrite(input2,LOW);      //外付けパトライトは、HIGH で通電
//
//    }
//  }
//  delay(10);
 


//  }
    if(millis()>beforeMiriSec+1000){
      //一秒ごとに実行される、時刻更新処理
      printLocalTime();
      beforeMiriSec = millis();
  }
}
