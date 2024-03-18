#include <ESP32Servo.h>
#include <Wire.h>
#include <RTClib.h>
#include <TimeAlarms.h>
#include <BluetoothSerial.h>
#include <LiquidCrystal_I2C.h>

#define RELAY_PIN 25      // Replace with the actual pin connected to the relay
#define SERVO_PIN 33      // Replace with the actual pin connected to the servo
#define MAX_FEED_TIMES 4  // Maximum number of feed times
#define INC_BUTTON_PIN 14 // Increment button pin
#define DEC_BUTTON_PIN 12 // Decrement button pin
#define SET_BUTTON_PIN 26 // Set button pin

// int buttonPin
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address, 16 columns, 2 rows

RTC_DS3231 rtc;
BluetoothSerial SerialBT;
Servo dispenserServo;

bool incButtonPressed = false;
bool decButtonPressed = false;
bool setButtonPressed = false;
bool settingMode = false;
int settingIndex = 0; // Index of the current setting being modified (0: hour, 1: minute, 2: duration)

struct FeedTime
{
    int hour;
    int minute;
    int duration;
};

FeedTime feedTimes[MAX_FEED_TIMES]; // Array to store feed times and durations
int feedCount = 0;

void displaySettingMode()
{
    lcd.clear();
    switch (settingIndex)
    {
    case 0:
        lcd.print("Set Hour: ");
        lcd.print(feedTimes[settingIndex].hour);
        break;
    case 1:
        lcd.print("Set Minute: ");
        lcd.print(feedTimes[settingIndex].minute);
        break;
    case 2:
        lcd.print("Set Duration: ");
        lcd.print(feedTimes[settingIndex].duration);
        lcd.print(" sec");
        break;
    }
}

void incrementSettingValue()
{
    switch (settingIndex)
    {
    case 0: // Hour
        feedTimes[settingIndex].hour = (feedTimes[settingIndex].hour + 1) % 24;
        break;
    case 1: // Minute
        feedTimes[settingIndex].minute = (feedTimes[settingIndex].minute + 1) % 60;
        break;
    case 2:                                                                             // Duration
        feedTimes[settingIndex].duration = (feedTimes[settingIndex].duration + 1) % 61; // Cap duration at 60 seconds
        break;
    }
    displaySettingMode();
}

void decrementSettingValue()
{
    switch (settingIndex)
    {
    case 0: // Hour
        feedTimes[settingIndex].hour = (feedTimes[settingIndex].hour - 1 + 24) % 24;
        break;
    case 1: // Minute
        feedTimes[settingIndex].minute = (feedTimes[settingIndex].minute - 1 + 60) % 60;
        break;
    case 2:                                                                                  // Duration
        feedTimes[settingIndex].duration = (feedTimes[settingIndex].duration - 1 + 61) % 61; // Cap duration at 60 seconds
        break;
    }
    displaySettingMode();
}

void saveSettings()
{
    // Save settings to memory or send over Bluetooth
}
// Variable to keep track of the number of feed times
void handleButtons()
{
    if (digitalRead(INC_BUTTON_PIN) == LOW && !incButtonPressed)
    {
        incButtonPressed = true;
        if (settingMode)
        {
            incrementSettingValue();
        }
    }
    else if (digitalRead(DEC_BUTTON_PIN) == LOW && !decButtonPressed)
    {
        decButtonPressed = true;
        if (settingMode)
        {
            decrementSettingValue();
        }
    }
    else if (digitalRead(SET_BUTTON_PIN) == LOW && !setButtonPressed)
    {
        setButtonPressed = true;
        if (!settingMode)
        {
            settingMode = true;
            displaySettingMode();
        }
        else
        {
            // Exit setting mode and save settings
            settingMode = false;
            saveSettings();
        }
    }

    if (digitalRead(INC_BUTTON_PIN) == HIGH)
    {
        incButtonPressed = false;
    }
    if (digitalRead(DEC_BUTTON_PIN) == HIGH)
    {
        decButtonPressed = false;
    }
    if (digitalRead(SET_BUTTON_PIN) == HIGH)
    {
        setButtonPressed = false;
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
    Serial.println("Ready .....");
    // SerialBT.begin("ESP32_FISH_FEEDER"); // Bluetooth device name

    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC.. moving up");
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
    addFeedTime(6, 0, 10);  // Example: Feed 1 at 6:00 AM for 10 seconds
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
    lcd.println("fish feeder timer");
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
