#include <ESP32Servo.h>
#include <Wire.h>
#include <RTClib.h>
#include <TimeAlarms.h>
#include <BluetoothSerial.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#define RELAY_PIN 25      // Replace with the actual pin connected to the relay
#define SERVO_PIN 33      // Replace with the actual pin connected to the servo
#define MAX_FEED_TIMES 4  // Maximum number of feed times
#define INC_BUTTON_PIN 14 // Increment button pin
#define DEC_BUTTON_PIN 12 // Decrement button pin
#define SET_BUTTON_PIN 26 // Set button pin
int settings_now = 0;
// int buttonPin
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address, 16 columns, 2 rows

RTC_DS3231 rtc;
BluetoothSerial SerialBT;
Servo dispenserServo;
int old = 9;
bool incButtonPressed = false;
bool decButtonPressed = false;
bool setButtonPressed = false;
bool settingMode = false;
int settingIndex = -1; // Index of the current setting being modified (0: hour, 1: minute, 2: duration)

struct FeedTime
{
    int hour;
    int minute;
    int duration;
};

FeedTime feedTimes[MAX_FEED_TIMES]; // Array to store feed times and durations
int feedCount = 3;
void loadFeedTimesFromEEPROM()
{
    int address = 0;
    for (int i = 0; i < MAX_FEED_TIMES; i++)
    {
        EEPROM.get(address, feedTimes[i]);
        address += sizeof(FeedTime);
    }
}

void saveFeedTimesToEEPROM()
{
    int address = 0;
    for (int i = 0; i < MAX_FEED_TIMES; i++)
    {
        EEPROM.put(address, feedTimes[i]);
        address += sizeof(FeedTime);
    }
    EEPROM.commit(); // Commit changes to EEPROM
}

void displaySettingMode()
{

    switch (settingIndex)
    {
    case 0:
        if (old != settingIndex)
        {
            old = settingIndex;
            Serial.println("clearing  lcd");
            lcd.clear();
        }
        lcd.setCursor(0, 0);
        lcd.println("Set Hour Feed 1:");
        lcd.setCursor(0, 1);
        lcd.print(feedTimes[0].hour);
        break;
    case 1:
        if (old != settingIndex)
        {
            old = settingIndex;
            Serial.println("clearing  lcd");
            lcd.clear();
        }
        lcd.setCursor(0, 0);
        lcd.println("Set Min Feed 1:");
        lcd.setCursor(0, 1);
        lcd.print(feedTimes[0].minute);
        break;
    case 2:
        lcd.setCursor(0, 0);
        lcd.println("Set Duration1(min):");
        lcd.setCursor(0, 1);
        lcd.print(feedTimes[0].duration);
        break;
    case 3:
        if (old != settingIndex)
        {
            old = settingIndex;
            Serial.println("clearing  lcd");
            lcd.clear();
        }
        lcd.setCursor(0, 0);
        lcd.println("Set Hour Feed 2: ");
        lcd.setCursor(0, 1);
        lcd.print(feedTimes[1].hour);
        break;
    case 4:
        if (old != settingIndex)
        {
            old = settingIndex;
            Serial.println("clearing  lcd");
            lcd.clear();
        }
        lcd.setCursor(0, 0);
        lcd.println("Set Min Feed 2: ");
        lcd.setCursor(0, 1);
        lcd.print(feedTimes[1].minute);
        break;
    case 5:
        if (old != settingIndex)
        {
            old = settingIndex;
            Serial.println("clearing  lcd");
            lcd.clear();
        }
        lcd.setCursor(0, 0);
        lcd.println("Set Duration2(min):");
        lcd.setCursor(0, 1);
        lcd.print(feedTimes[1].duration);
        break;
    case 6:
        if (old != settingIndex)
        {
            old = settingIndex;
            Serial.println("clearing  lcd");
            lcd.clear();
        }
        lcd.setCursor(0, 0);
        lcd.println("fish feeder time:");
        lcd.setCursor(0, 1);
        DateTime now = rtc.now();
        // lcd.print(now.year());
        // lcd.print(" ");
        lcd.print(now.month());
        lcd.print("/");
        lcd.print(now.day());
        lcd.print(" ");
        lcd.print(now.hour());
        lcd.print(":");
        lcd.print(now.minute());
        lcd.print(":");
        lcd.print(now.second());
        saveFeedTimesToEEPROM();
        break;
        // Repeat for additional feed times if needed
    }
}

void incrementSettingValue()
{
    switch (settingIndex)
    {
    case 0: // Hour for Feed 1
        feedTimes[0].hour = (feedTimes[0].hour + 1) % 24;
        break;
    case 1: // Minute for Feed 1
        feedTimes[0].minute = (feedTimes[0].minute + 1) % 60;
        break;
    case 2: // Hour for Feed 2
        feedTimes[0].duration = (feedTimes[0].duration + 1) % 60;
        break;
    case 3: // Hour for Feed 2
        feedTimes[1].hour = (feedTimes[1].hour + 1) % 24;
        break;
    case 4: // Minute for Feed 2
        feedTimes[1].minute = (feedTimes[1].minute + 1) % 60;
        break;
    case 5: // Hour for Feed 2
        feedTimes[1].hour = (feedTimes[1].duration + 1) % 60;
        break;
        // Repeat for additional feed times if needed
    }
    displaySettingMode();
}

void decrementSettingValue()
{
    switch (settingIndex)
    {
    case 0: // Hour for Feed 1
        feedTimes[0].hour = (feedTimes[0].hour - 1 + 24) % 24;
        break;
    case 1: // Minute for Feed 1
        feedTimes[0].minute = (feedTimes[0].minute - 1 + 60) % 60;
        break;
    case 2: // Hour for Feed 2
        feedTimes[0].duration = (feedTimes[0].duration - 1) % 60;
        break;
    case 3: // Hour for Feed 2
        feedTimes[1].hour = (feedTimes[1].hour - 1 + 24) % 24;
        break;
    case 4: // Minute for Feed 2
        feedTimes[1].minute = (feedTimes[1].minute - 1 + 60) % 60;
        break;
    case 5: // Hour for Feed 2
        feedTimes[1].hour = (feedTimes[1].duration - 1) % 60;
        break;
        // Repeat for additional feed times if needed
    }
    displaySettingMode();
}

void saveSettings()
{
    // Save settings to memory or send over Bluetooth
}

void handleButtons()
{
    if (digitalRead(INC_BUTTON_PIN) == LOW)
    {
        incrementSettingValue();
    }
    else if (digitalRead(DEC_BUTTON_PIN) == LOW)
    {
        decrementSettingValue();
    }
    else if (digitalRead(SET_BUTTON_PIN) == LOW)
    {
        settingMode = true;
        settingIndex++;
        if (settingIndex > 6)
        {
            settingIndex = 0;
        }
        displaySettingMode();
        saveSettings();
    }
    else
    {
        displaySettingMode();
    }
}

time_t dailyAlarm(int hour, int minute, int second)
{
    DateTime now = rtc.now();
    DateTime alarmTime = DateTime(now.year(), now.month(), now.day(), hour, minute, second);
    if (now.unixtime() < alarmTime.unixtime())
    {
        alarmTime = alarmTime + TimeSpan(1, 0, 0, 0); // If the alarm time has already passed today, set it for tomorrow
    }
    return now.unixtime() + (alarmTime.unixtime() - now.unixtime());
}
void feed(int feedIndex)
{
    int feedDuration = feedTimes[feedIndex].duration;
    Serial.println("feeding");
    digitalWrite(RELAY_PIN, HIGH); // Engage throwing motor
    dispenserServo.write(0);       // Start the dispenser servo
    delay(feedDuration * 1000);    // Wait for the specified feed duration
    dispenserServo.write(90);      // Stop the dispenser servo
    delay(3000);                   // Wait for 3 seconds before turning off the throwing motor
    digitalWrite(RELAY_PIN, LOW);  // Turn off the throwing motor
}
/**
   Adds a feed time to the array if there is space.

   @param hour the hour of the feed time
   @param minute the minute of the feed time
   @param duration the duration of the feed time

   @return void

   @throws ErrorType none
*/

void addFeedTime(int hour, int minute, int duration)
{
    if (feedCount < MAX_FEED_TIMES)
    {
        feedTimes[feedCount].hour = hour;
        feedTimes[feedCount].minute = minute;
        feedTimes[feedCount].duration = duration;
        feedCount++;
    }
}
void testFeedTime(int hour, int minute, int duration)
{
    if (feedCount < MAX_FEED_TIMES)
    {
        feedTimes[feedCount].hour = hour;
        feedTimes[feedCount].minute = minute;
        feedTimes[feedCount].duration = duration;
        feedCount++;
    }
}
void feedCallback()
{
    int feedIndex = Alarm.getTriggeredAlarmId(); // Get the index of the triggered feed time
    feed(feedIndex);
}

void processBluetoothData()
{
    String receivedData = SerialBT.readStringUntil('\n');

    if (receivedData.startsWith("FEED"))
    {
        // Format: FEED HH:MM DURATION (e.g., FEED 06:00 10)
        int spaceIndex1 = receivedData.indexOf(' ');
        int spaceIndex2 = receivedData.indexOf(' ', spaceIndex1 + 1);
        String timeStr = receivedData.substring(spaceIndex1 + 1, spaceIndex2);
        String durationStr = receivedData.substring(spaceIndex2 + 1);

        int hour = timeStr.substring(0, 2).toInt();
        int minute = timeStr.substring(3).toInt();
        int duration = durationStr.toInt();

        if (feedCount < MAX_FEED_TIMES)
        {
            addFeedTime(hour, minute, duration);
        }
    }
}

/**
   Function to set up the system, initialize pins, set default feed times,
   and set up alarms for automatic feeding.
*/
void setup()
{
    Serial.begin(9600);
    Serial.println("Ready to receive data over Bluetooth");
    // SerialBT.begin("ESP32_FISH_FEEDER"); // Bluetooth device name

    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        // while (1) ;
    }
    Serial.println("found RTC");
    if (rtc.lostPower())
    {
        Serial.println("RTC lost power, let's set the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    dispenserServo.attach(SERVO_PIN);

    // Initialize other pins
    pinMode(RELAY_PIN, OUTPUT);
    Serial.println("found RTC11");
    // Add default feed times (you can customize this based on your needs)
    addFeedTime(9, 33, 10); // Example: Feed 1 at 6:00 AM for 10 seconds
    addFeedTime(12, 0, 15); // Example: Feed 2 at 12:00 PM for 15 seconds
    // Add more default feed times if needed

    // Set up alarms for added feed times
    for (int i = 0; i < feedCount; i++)
    {
        Alarm.timerRepeat(dailyAlarm(feedTimes[i].hour, feedTimes[i].minute, 0), feedCallback);
    }
    // Set up alarms
    // Alarm.timerRepeat(dailyAlarm(6, 0, 0), feed1);  // 6am for feed 1
    // Alarm.timerRepeat(dailyAlarm(12, 0, 0), feed2); // 12pm for feed 2
    // Add more alarms for additional feed times if needed
    lcd.init();
    lcd.backlight();
    lcd.println("fish feed timer");
    lcd.setCursor(0, 1);
    DateTime now = rtc.now();
    // lcd.print(now.year());
    // lcd.print(" ");
    lcd.print(now.month());
    lcd.print(" ");
    lcd.print(now.day());
    lcd.print(" ");
    lcd.print(now.hour());
    lcd.print(" ");
    lcd.print(now.minute());
    lcd.print(" ");
    lcd.print(now.second());
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(INC_BUTTON_PIN, INPUT_PULLUP);
    pinMode(DEC_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SET_BUTTON_PIN, INPUT_PULLUP);
    testFeedTime(9, 40, 0);
    settingIndex = 6;

    loadFeedTimesFromEEPROM();
    Serial.println("Ready to receive data over Bluetooth");
}

void loop()
{
    Alarm.delay(1000); // Allow Alarms to trigger
    if (SerialBT.available())
    {
        processBluetoothData();
    }
    handleButtons();
}
