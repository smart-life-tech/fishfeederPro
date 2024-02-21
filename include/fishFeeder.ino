#include <ESP32Servo.h>
#include <Wire.h>
#include <RTClib.h>
#include <TimeAlarms.h>
#include <BluetoothSerial.h>
#define RELAY_PIN 2      // Replace with the actual pin connected to the relay
#define SERVO_PIN 3      // Replace with the actual pin connected to the servo
#define MAX_FEED_TIMES 4 // Maximum number of feed times

RTC_DS3231 rtc;
BluetoothSerial SerialBT;
Servo dispenserServo;

struct FeedTime
{
    int hour;
    int minute;
    int duration;
};

FeedTime feedTimes[MAX_FEED_TIMES]; // Array to store feed times and durations
int feedCount = 0;                  // Variable to keep track of the number of feed times

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
 * Adds a feed time to the array if there is space.
 *
 * @param hour the hour of the feed time
 * @param minute the minute of the feed time
 * @param duration the duration of the feed time
 *
 * @return void
 *
 * @throws ErrorType none
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
 * Function to set up the system, initialize pins, set default feed times,
 * and set up alarms for automatic feeding.
 */
void setup()
{
    Serial.begin(9600);
    Serial.begin(9600);
    SerialBT.begin("ESP32_BT"); // Bluetooth device name

    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        while (1)
            ;
    }

    if (rtc.lostPower())
    {
        Serial.println("RTC lost power, let's set the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    dispenserServo.attach(SERVO_PIN);

    // Initialize other pins
    pinMode(RELAY_PIN, OUTPUT);

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
}

void loop()
{
    Alarm.delay(1000); // Allow Alarms to trigger
    if (SerialBT.available())
    {
        processBluetoothData();
    }
}

