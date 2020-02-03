// String consts

const char timePrompt[] = "\r\nPlease provide initial time in format HH:mm:ss \r\n";
const char alarmPrompt[] = "\r\nSet time for alarm in format HH:mm:ss\r\n";

const char repeatPrompt[] = "\r\nSet number of repetition (0-30, default is 0), "
                            "confirm with non-digit character\r\n";
const char delayPrompt[] = "\r\nSet delay between repetitions in seconds(15-3600, default is 60),"
                            "confirm with non-digit character\r\n";
                            
const char defaultValue[] = "\r\nInvalid value! Default value will be used.\r\n";

const char melodyPrompt[] = "\r\nPlease choose ringtone:\r\n"
                          "1 - Queen - Bohemian rhapsody\r\n"
                          "2 - Deep Purple - Smoke on the water\r\n"
                          "3 - Black Sabbath - Ironman\r\n";

const char lightPrompt[] = "\r\nPlease choose light notification:\r\n"
                          "1 - All diodes blinking\r\n"
                          "2 - Left to right and back - cradle\r\n"
                          "3 - Jumping in order 1., 3., 2., 4.\r\n";

const char success[] = "\r\nSuccessfully configured! Alarm is turned on.\r\n"
                       "Alarm will be repeated every day\r\n"
                       "SW2 - turn alarm on/off\r\n"
                       "SW3 - show current time\r\n"
                       "SW4 - new initialization proccess\r\n";

const char currentTimeStr[] = "\r\nCurrent time: ";
const char alarmTimeStr[] = "\r\nAlarm time: ";

const char alarmOnStr[] = "\r\nAlarm on\r\n";
const char alarmOffStr[] = "\r\nAlarm off\r\n";
const char alarmStr[] = "\r\nAlarm!!!\r\n";

#define MAX_DAY_TIME 86400  // in seconds - midnight

// LEDs
#define LED_D9  0x20
#define LED_D10 0x10
#define LED_D11 0x8
#define LED_D12 0x4

// Buttons
#define BTN_SW2 0x400
#define BTN_SW3 0x1000
#define BTN_SW4 0x8000000

// Wave lengths for playing melodies
// * 10 for longer tone
#define A_3   157 * 10

#define C_4   132 * 10
#define D_4   118 * 10
#define E_4   105 * 10
#define F_4   99 * 10
#define G_4   88 * 10
#define Gis_4 83 * 10
#define Ais_4 74 * 10

#define D_5   59 * 10

#define F_6   25 * 10
#define G_6   22 * 10

// Methods declarations
int main();
void AppInit();

void MCUInit();
void PortsInit();
void UART5Init();
void RTCInit();

void SendCh(char ch);
char ReceiveCh();
void SendStr(const char s[]);
unsigned long GetTime(const char prompt[]);
int GetNumber(const char prompt[]);
void GetRingTone();
void GetLight();
void GetTimeString(char* buffer, unsigned long time);

void EnableTimer(int daySeconds);

void PlayMelody();
void PlayBohemian();
void PlaySmoke();
void PlayIronman();
void Beep(int note, int duration, int delay);

void TurnOnLights();
void LightAll();
void LightCradle();
void LightJump();

void RTC_IRQHandler();
void PORTE_IRQHandler();
