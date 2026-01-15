// LIBRERÍAS
#include "DFRobot_MICS.h"   // Librería oficial para manejar el sensor de gases MEMS MICS
#include <SoftwareSerial.h> // Librería para crear un puerto serial virtual

// DEFINICIONES Y CONFIGURACIÓN
#define CALIBRATION_TIME   2      // Tiempo de calibración
#define ADC_PIN            A0     // Pin analógico donde se lee el valor general del gas
#define POWER_PIN          2      // Pin digital que controla el encendido del sensor

// Instancia del objeto para controlar el sensor mediante ADC y Pin de Poder
DFRobot_MICS_ADC mics(ADC_PIN, POWER_PIN);

// Configuración del puerto serial por software
SoftwareSerial espSerial(8, 3); 

// SETUP: Configuración inicial
void setup() {
  Serial.begin(115200);   // Puerto serial hardware para depuración en PC
  espSerial.begin(9600);  // Puerto serial software para enviar datos al ESP32 (Debe coincidir con el del ESP32)
  
  while (!Serial); // Espera a que el puerto serial de PC esté listo
  
  // Intenta iniciar la comunicación con el sensor
  while (!mics.begin()) {
    Serial.println("No devices found! Please check the connections.");
    delay(1000);
  }
  Serial.println("Device connected successfully!");

  // Verifica si el sensor está en modo suspensión y lo despierta
  uint8_t mode = mics.getPowerState();
  if (mode == SLEEP_MODE) {
    mics.wakeUpMode(); // Envía señal de despertar
    Serial.println("Sensor woken up successfully!");
  } else {
    Serial.println("Sensor is already in wake-up mode.");
  }

  // Espera el tiempo definido en CALIBRATION_TIME
  while (!mics.warmUpTime(CALIBRATION_TIME)) {
    Serial.println("Please wait until the warm-up time is over!");
    delay(1000);
  }
  Serial.println("Sensor is ready for use.");
}

// LOOP: Bucle principal
void loop() {

  // LECTURA DE DATOS DEL SENSOR 
  // Obtiene la concentración de cada gas específico usando la librería
  float co = mics.getGasData(CO);       // Monóxido de Carbono
  float ch4 = mics.getGasData(CH4);     // Metano
  float etan = mics.getGasData(C2H5OH); // Etanol
  float h2 = mics.getGasData(H2);       // Hidrógeno
  float nh3 = mics.getGasData(NH3);     // Amoniaco
  
  // Lee el valor del pin analógico
  int gasGeneral = analogRead(A0);

  // IMPRESIÓN EN MONITOR SERIAL 
  // Solo para ver qué pasa mientras tienes el Arduino conectado a la compu
  Serial.print("CO: "); Serial.print(co, 1);
  Serial.print(" | Metano: "); Serial.println(ch4, 1);
  Serial.print(" | Etanol: "); Serial.println(etan, 1);
  Serial.print(" | Hidrogeno: "); Serial.println(h2, 1);
  Serial.print(" | Amoniaco: "); Serial.println(nh3, 1);
  Serial.print(" | Gas General: "); Serial.println(gasGeneral);

  // ENVÍO DE DATOS AL ESP32
  // Se envía en formato CSV 
  // El orden DEBE SER EL MISMO que el ESP32 espera recibir
  espSerial.print(co, 1);
  espSerial.print(",");      // Separador
  espSerial.print(ch4, 1);
  espSerial.print(",");
  espSerial.print(etan, 1);
  espSerial.print(",");
  espSerial.print(h2, 1);
  espSerial.print(",");
  espSerial.print(nh3, 1);
  espSerial.print(",");
  espSerial.print(gasGeneral); 
  espSerial.println();       // Salto de línea indica fin del mensaje

  // RETARDO
  delay(60000); 
}
