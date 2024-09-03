#include <Arduino.h>
#include <config.h>   //Fitxero donde poner tus credenciales, si las vas a poner directamente en el programa borrar  esta línea.

#include <Ticker.h>
#include "DHTesp.h" // Click here to get the library: http://librarymanager/All#DHTesp
#include <WiFi.h>  // Incluye la biblioteca WiFi para el ESP32
#include <HTTPClient.h>  //Include de la librería HTTPClient para hacer las llamadas a GOOGLE

#ifndef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP32 ONLY!)
#error Select ESP32 board.
#endif

/** Initialize DHT sensor 1 */
DHTesp dhtSensor1;

/** Task handle for the light value read task */
TaskHandle_t tempTaskHandle = NULL;
/** Pin number for DHT11 1 data pin */
int dhtPin1 = 17;
/** Ticker for temperature reading */
Ticker tempTicker;
/** Flags for temperature readings finished */
bool gotNewTemperature = false;
/** Data from sensor 1 */
TempAndHumidity sensor1Data;


/* Flag if main loop is running */
bool tasksEnabled = false;

// Configura los parámetros de la red Wi-Fi
const char* ssid = SSID_WIFI;         // Reemplaza con el nombre de tu red Wi-Fi
const char* password = PASSWORD_WIFI; // Reemplaza con la contraseña de tu red Wi-Fi

// Google script ID and required credentials
String GOOGLE_SCRIPT_ID = googleScriptID;


/**
 * Task to reads temperature from DHT11 sensor
 * @param pvParameters
 *		pointer to task parameters
 */
void tempTask(void *pvParameters) {
	Serial.println("tempTask loop started");
	while (1) // tempTask loop
	{
		if (tasksEnabled && !gotNewTemperature) { // Read temperature only if old data was processed already
			// Reading temperature for humidity takes about 250 milliseconds!
			// Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
			sensor1Data = dhtSensor1.getTempAndHumidity();	// Read values from sensor 1
			gotNewTemperature = true;
		}
		vTaskSuspend(NULL);
	}
}

/**
 * triggerGetTemp
 * Sets flag dhtUpdated to true for handling in loop()
 * called by Ticker tempTicker
 */
void triggerGetTemp() {
	if (tempTaskHandle != NULL) {
		 xTaskResumeFromISR(tempTaskHandle);
	}
}

/*
Función para mandar los datos al excell, en este caso 
mandamos la temperatura y la humedad de un sensor DHT11
*/
void mandar_datos(float temper, float hum){

	String tempString = String(temper, 2);  //Convertimos las variable float a String
	String humString = String(hum, 2);	//Convertimos las variable float a String
	/*Generamos la url que necesitamos para mandar los datos*/
	String urlFinal = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?" + "temp=" + tempString + "&hum=" + humString;
	/*Imprimimos esa url para ver si es la que necesitamos*/
	Serial.println(urlFinal);
	/*Creamos el objeto HTTPClient que será donde metamos la url para mandar*/
	HTTPClient http;
	http.begin(urlFinal.c_str());
	http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
	int httpCode = http.GET(); 
	Serial.print("HTTP Status Code: ");
    Serial.println(httpCode);
    //---------------------------------------------------------------------
    //getting response from google sheet
    String payload;
    if (httpCode > 0) {
        payload = http.getString();
        //Serial.println("Payload: "+payload);
	}
	/*Acabamos el mensaje*/
	http.end(); 
}

/**
 * Arduino setup function (called once after boot/reboot)
 */
void setup() {
	Serial.begin(115200);
	Serial.println("Example for 1 DHT11/22 sensors");

	// Initialize temperature sensor 1
	dhtSensor1.setup(dhtPin1, DHTesp::DHT11);
	
	// Start task to get temperature
	xTaskCreatePinnedToCore(
			tempTask,											 /* Function to implement the task */
			"tempTask ",										/* Name of the task */
			4000,													 /* Stack size in words */
			NULL,													 /* Task input parameter */
			5,															/* Priority of the task */
			&tempTaskHandle,								/* Task handle. */
			1);														 /* Core where the task should run */

	if (tempTaskHandle == NULL) {
		Serial.println("[ERROR] Failed to start task for temperature update");
	} else {
		// Start update of environment data every 30 seconds
		tempTicker.attach(600, triggerGetTemp);
	}

	// Signal end of setup() to tasks
	tasksEnabled = true;
	//Inicialización de la conexion de la red wifi
	WiFi.begin(ssid, password); // Inicia la conexión a la red Wi-Fi

  	// Espera a que se realice la conexión
  	while (WiFi.status() != WL_CONNECTED) {
    	delay(500);
    	Serial.print(".");
	}
} // End of setup.


/**
 * loop
 * Arduino loop function, called once 'setup' is complete (your own code
 * should go here)
 */
void loop() {
	if (gotNewTemperature) {
		Serial.println("Sensor 1 data:");
		Serial.println("Temp: " + String(sensor1Data.temperature,2) + "'C Humidity: " + String(sensor1Data.humidity,1) + "%");
    gotNewTemperature = false;
	Serial.println(WiFi.localIP());
	/*Función donde se mandan los datos, tiene dos parámetros que son la temperatura y la humedad*/
	mandar_datos(sensor1Data.temperature, sensor1Data.humidity);
	}

} // End of loop