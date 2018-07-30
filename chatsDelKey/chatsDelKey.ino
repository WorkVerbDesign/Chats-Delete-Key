/*
 Written kinda by Jesse
 twitch.tv/oh_bother
 rewritten to make more sense, I lost the URL for the original. It was a mess.
 Feel free to do the same with my code. 

  THE GOAL:
    An internet-of-things (buzzoword) Kitschy like hot topic-ey 
    LED Delete key that twitch chat can operate by sending commands
    
  MY CODE:
    I need to talk about how it works here.
    
  THE CIRCUIT:
    button wired to D16 (d0 silkscreen) and ground
    data out to LEDs on pin D14 (d5 silkscreen)
    3904 from ESP 5v pin to wall wort
    
  NOTES:
    chatroom location of #del_key":
    #chatrooms:32178044:98fc68b5-9421-4073-9905-bdb1fc553913

    when login:
    after NICK response with " 376 " (space before and after important to filter)
    after PASS response with " 366 "
    messages might stack and get ip ban on user

    should bot print a pattern description?
    !describe [pattern]
    
    For coding patterns, use the 0-255 counter variable to animate
 */

#include "config.h" //config file
#include "FastLED.h"
#include "ESP8266WiFi.h" //lowercase i's thanks
#include "Bounce2.h"

/*
  moved to config.h:
  
  static struct namingtion[ const char *werds, uint32_t val ]
  werds = html color name "inQuotes"
  val = hexidecimal value of RGB
  
  const char* ssid     = "MySSID";
  const char* password = "routerPw";
  const String OAuth ="oauth:numbers";
  const char* host = "irc.chat.twitch.tv";

*/

FASTLED_USING_NAMESPACE

//==============Pins===========================
#define NUM_LEDS 34
#define DATA_PIN 14
#define BUTTON_PIN 2

//==============Constants======================
const int bounceTime = 20; //milisecs

const String chunnel = "#chatrooms:32178044:98fc68b5-9421-4073-9905-bdb1fc553913";
const String userName = "oh_bother";

const int fps = 120;

//==============Globals========================
int state = 0;
int messageNo = 0;
char message[1024];
String msg = "";
String command = "";

uint8_t gHue = 0;
int commandNo = 0;

//==============Library stuff==================
//fastLED
CRGB leds[NUM_LEDS];

//bounce2
Bounce debouncer = Bounce();

//espWIFI
WiFiClient client;

//==============Setup==========================
void setup() {
  //fastLED
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  //wifi
  WiFi.begin(ssid, password);

  //button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  //bounce
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(bounceTime);

  //zero out our char array
  memset(message, 0, sizeof(message));

  //serial
  Serial.begin(9600);
}

//some reason this has to go here. after setup. idk.
// CueXXIII: remove line 108 and change line 111 to void (*func)();
// WE ARE HERE FUCKFACE!

const int listSize = 10;

typedef void (*SimplePatternList)();
const static struct{
  const char *pattern;
  SimplePatternList func;
  const char *desc;
}patternList[]={
  {"rainbow", rainbow, "fades the led strip colors slowly"},
  {"rainbowWithGlitter", rainbowWithGlitter, "just like rainbow but with sparkling"}, 
  {"confetti", confetti, "random LEDs fadde through a rainbow pallette"}, 
  {"sinelon", sinelon, "rainbow LED goes back and forth on the LED strip."}, 
  {"juggle", juggle, "kinda like rainbow snake?"}, 
  {"bpm", bpm, "phasing color spectrum whizzes across the delete key?"},
  {"allRed", allRed, "makes the key red"},
  {"allBlue", allBlue, "makes the key blue"},
  {"allGreen", allGreen, "makes the key green"},
  {"cops", cops, "oh shit it's the fuzz"},
  {"caution", caution, "construction beacon"}
};


//==============Loooooop========================
void loop() {
  //bounce
  debouncer.update();
  ircMachine();
  animRun();
}

//==============Main Functions==================
//uses global state to run IRC bot
void ircMachine(){
  switch(state){
    case 0:
      wifiConnect();
      break;
      
    case 1:
      hostConnect();
      break;
      
    case 2:
      ircConnect();
      break;

    case 3:
      ircStore();
      break;

    case 4:
      parseMessage();
      break;
  }
  checkConnect();
}

//run the animation based on non delay timers
//also should 
void animRun(){
  patternList[commandNo].func();
  
  FastLED.show();
  FastLED.delay(1000/fps);

  EVERY_N_MILLISECONDS(20){ gHue++; }
}

//==============Secondary Functions=============
void checkConnect(){
  if(WiFi.status() != WL_CONNECTED){
    state = 0;
    messageNo = 0;
  }else if(!client.connected()){
    state = 1;
    messageNo = 0;
  }
}

void wifiConnect(){
  if(WiFi.status() == WL_CONNECTED){
    state = 1;
  }else{
    blinkenLight(200);
  }
  digitalWrite(LED_BUILTIN, LOW);
}

void hostConnect(){
  if(client.connect(host, 6667)) {
    state = 2;
  }else{
    //you fail
    blinkenLight(1000);
  }
}

void ircConnect(){
  blinkenLight(500);
  client.print("PASS ");
  client.print(OAuth);
  client.println("\r");
  
  blinkenLight(250);
  //client.println("NICK oh_bother\r");
  client.print("NICK ");
  client.print(userName);
  client.println("\r");
  
  blinkenLight(250);
  //client.println("JOIN #oh_bother\r");
  client.print("JOIN ");
  client.print(chunnel);
  client.println("\r");
  
  blinkenLight(250);
  state = 3;
}

void ircStore(){
  if(client.available()) {
    char c = client.read();
    
    if(c == '\r'){
      message[messageNo] = '\0'; //check if this matters
      messageNo = 0;
      state = 4;
    }else{
      message[messageNo] = c;
      messageNo++;
    }
  }
}

//look for commands in message
void parseMessage(){
  msg = String(message);
  
  if(msg.startsWith("PING")){
    pong();
  }else if(msg.indexOf(" :!bother ") >= 0){
    bother();
  }else if(msg.indexOf(" :!list") >= 0){
    list();
  }else if(msg.indexOf(" :!about ") >= 0){
    about();
  }else if(msg.indexOf(" :!help") >= 0){
    help();
  }
  
  state = 3;
}

//==============BOT FUNCTIONS================
void pong(){ 
  //"PONG :tmi.twitch.tv"
  client.println("PONG\r");
  Serial.println("PING'D");
}

void bother(){
  int blep = msg.indexOf(" :!bother ") + 10;
  int blop = msg.indexOf(' ', blep);
  
  if(blop == -1){
    command = msg.substring(blep);
  }else{
    command = msg.substring(blep, blop);
  }
  Serial.println(command);
  //do something to animation variable or array

  //FIGURE OUT HOW TO CALC NUMBER OF ENTRIES IN STRUCT
  for(int i = 0; i <= listSize; i++){   
    if(command == patternList[i].pattern){
      commandNo = i;
      Serial.println(commandNo);
      return;
    }
  }
}

void list(){
  client.print("PRIVMSG ");
  client.print(chunnel);
  client.print(" :Commands are: ");
  
  client.print(patternList[0].pattern);
  
  for(int i = 1; i <= listSize; i++){
    client.print(", ");
    client.print(patternList[i].pattern);
  }
  
  client.println(".\r");
}

void about(){
  String question;
  int answer;
  
  int blep = msg.indexOf(" :!about ") + 9;
  int blop = msg.indexOf(' ', blep);
  
  if(blop == -1){
    question = msg.substring(blep);
  }else{
    question = msg.substring(blep, blop);
  }

  //FIGURE OUT HOW TO CALC NUMBER OF ENTRIES IN STRUCT
  for(int i = 0; i <= listSize; i++){   
    if(question == patternList[i].pattern){
      client.print("PRIVMSG ");
      client.print(chunnel);
      
      client.print(" :");
      client.print(patternList[i].pattern); 
      client.print(" ");    
      client.print(patternList[i].desc);
      client.println("\r");
      return;
    }
  }
}

void help(){
  client.print("PRIVMSG ");
  client.print(chunnel);
  client.println(" :Welcome to the Delete key bot. Light up the delete key with the !bother <pattern> command. !list will give a list of patterns. !about <pattern> describeas patterns on the list.\r");
}

//==============HELPER FUNCTIONS================
//this just blinks the built in LED at interval
void blinkenLight(int miller_time){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(miller_time);
    digitalWrite(LED_BUILTIN, LOW);
    delay(miller_time);
}


//=============LED SHIZ=========================
void rainbow(){
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter(){
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter){
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti(){
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon(){
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm(){
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle(){
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

void allRed(){
  //turn the whole mofo red
  for(int i = 0; i <= (NUM_LEDS - 1); i++){
    leds[i] = CRGB::Red;
  }
}

void allGreen(){
  //turn the whole mofo red
  for(int i = 0; i <= (NUM_LEDS - 1); i++){
    leds[i] = CRGB::Green;
  }
}

void allBlue(){
  //turn the whole mofo red
  for(int i = 0; i <= (NUM_LEDS - 1); i++){
    leds[i] = CRGB::Blue;
  }
}

void cops(){
  //from half red half blue
  const int left[17] = {0, 6, 7, 8, 9, 10, 14, 15, 16, 17, 23, 24, 25, 26, 30, 31, 32};
  const int right[17] = {1, 2, 3, 4, 5, 11, 12, 13, 18, 19, 20, 21, 22, 27, 28, 29, 33};
  
  for(int i = 0; i < 17; i++){
    if((gHue % 64) >= 32 ){
      leds[left[i]] = CRGB::Red;
      leds[right[i]] = CRGB::Blue;
    }else{
      leds[left[i]] = CRGB::Blue;
      leds[right[i]] = CRGB::Red;
    }
    if((gHue % 32) >= 16){
      leds[left[i]] = CRGB::Black;
      leds[right[i]] =  CRGB::Black;
    }
  }
}

DEFINE_GRADIENT_PALETTE( lightGradnt ){
  0,     0,   0,   0,   //black
  128, 255, 255,   0,   //yeller
  255,   0,   0,   0    //black
};

void caution(){
  //0 to 40, location map of leds
  //0-40 number scale around delte key, not top
  const uint8_t sirenMap[26] = {1, 7, 12, 17, 21, 25, 28, 31, 35, 39, 4, 10, 19, 27, 29, 37, 3, 7, 11, 18, 20, 26, 30, 36, 38};

  CRGBPalette16 lightPal = lightGradnt;

  for( int i = 0; i < 26; i++){
    uint8_t lightIndex = (sirenMap[i] * 6) + (gHue*4);
    leds[i] = ColorFromPalette(lightPal, lightIndex); 
  }
  for(int i = 26; i < 34; i++){
    leds[i] = CRGB::Black;
  }
}
