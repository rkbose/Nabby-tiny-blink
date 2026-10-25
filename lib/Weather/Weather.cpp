// This file is part of Nabby-tiny.
// Nabby-tiny is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// Nabby-tiny is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// You should have received a copy of the GNU General Public License along with Nabby-tiny. If not, see <https://www.gnu.org/licenses/>.

#include "WString.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

String latitude = "52.437";
String longitude = "4.639";
String location = "Santpoort-noord, NL"; // Example: Santpoort-noord, NL

// Type the timezone you want to get the time for
String timezone = "Europe/Amsterdam";

// Store date and time
String current_date_time;
String current_date;
String last_weather_update;
String temperature;
String humidity;
int is_day;
int weather_code = 0;
String weather_description;

// SET VARIABLE TO 0 FOR TEMPERATURE IN FAHRENHEIT DEGREES
#define TEMP_CELSIUS 1

#if TEMP_CELSIUS
String temperature_unit = "";
const char degree_symbol[] = "\u00B0C";
#else
String temperature_unit = "&temperature_unit=fahrenheit";
const char degree_symbol[] = "\u00B0F";
#endif

/*
  WMO Weather interpretation codes (WW)- Code	Description
  0	Clear sky
  1, 2, 3	Mainly clear, partly cloudy, and overcast
  45, 48	Fog and depositing rime fog
  51, 53, 55	Drizzle: Light, moderate, and dense intensity
  56, 57	Freezing Drizzle: Light and dense intensity
  61, 63, 65	Rain: Slight, moderate and heavy intensity
  66, 67	Freezing Rain: Light and heavy intensity
  71, 73, 75	Snow fall: Slight, moderate, and heavy intensity
  77	Snow grains
  80, 81, 82	Rain showers: Slight, moderate, and violent
  85, 86	Snow showers slight and heavy
  95 *	Thunderstorm: Slight or moderate
  96, 99 *	Thunderstorm with slight and heavy hail
*/

void get_weather_data()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        // Construct the API endpoint
        String url = String("http://api.open-meteo.com/v1/forecast?latitude=" + latitude +
                            "&longitude=" + longitude +
                            "&current=temperature_2m,is_day,rain" +
                            "&hourly=temperature_2m,is_day,weather_code" + 
                           "&minutely_15=rain,temperature_2m" +
                            temperature_unit +
                            "&timezone=" + timezone +
                            "&forecast_days=1");

        // https://api.open-meteo.com/v1/forecast?latitude=52.437&longitude=4.639&hourly=temperature_2m,weather_code&current=temperature_2m,is_day,rain&timezone=Europe%2FBerlin&forecast_days=1

        http.begin(url);
        int httpCode = http.GET(); // Make the GET request

        if (httpCode > 0)
        {
            // Check for the response
            if (httpCode == HTTP_CODE_OK)
            {
                String payload = http.getString();
                Serial.println("Request information :");
                // Serial.println(payload);
                // Parse the JSON to extract the time
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, payload);
                if (!error)
                {
                    current_date_time = String(doc["current"]["time"]);
                    Serial.printf("Current_Datetime: %s\n", current_date_time.c_str());
                    // Split the datetime into date and time
                    int splitIndex = current_date_time.indexOf('T');
                    current_date = current_date_time.substring(0, splitIndex);
                    last_weather_update = current_date_time.substring(splitIndex + 1, splitIndex + 9); // Extract time portion
                    Serial.printf("Last Weather Update: %s %s\n", current_date.c_str(), last_weather_update.c_str());

                    temperature = String(doc["hourly"]["temperature_2m"]);
                    Serial.printf("Temperature: %s %s\n", temperature.c_str(), degree_symbol);
                    // humidity = String(doc["current"]["relative_humidity_2m"]);
                    // is_day = String(doc["current"]["is_day"]).toInt();
                    // weather_code = String(doc["current"]["weather_code"]).toInt();

                    // Serial.printf("Humidity: %s%%\n", humidity.c_str());
                    // Serial.printf("Is Day: %s\n", is_day ? "Yes" : "No");
                    // Serial.printf("Weather Code: %d\n", weather_code);
                    //  Serial.printf("Timezone: %s\n", String(timezone).c_str());


                }
                else
                {
                    Serial.print("deserializeJson() failed: ");
                    Serial.println(error.c_str());
                }
            }
            else
            {
                Serial.println("Failed");
            }
        }
        else
        {
            Serial.printf("GET request failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end(); // Close connection
    }
    else
    {
        Serial.println("Not connected to Wi-Fi");
    }
}
