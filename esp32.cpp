// CONFIGURACIÓN DE BLYNK
// Credenciales únicas del proyecto para conectar con la nube de Blynk
#define BLYNK_TEMPLATE_ID "TMPL2ZnfA-IVm"
#define BLYNK_TEMPLATE_NAME "ProyectoMICS"
#define BLYNK_AUTH_TOKEN "GO3XL7kK4IjBK-Ke5qkRsSSpwq1wr35j"

#define BLYNK_PRINT Serial // Habilita la depuración de Blynk en el monitor serial

// LIBRERÍAS
#include <DHTesp.h>           // Librería para el sensor DHT11/DHT22
#include <WiFi.h>             // Para conectar el ESP32 a internet
#include <WiFiClient.h>       // Cliente WiFi base
#include <BlynkSimpleEsp32.h> // Librería principal de Blynk para ESP32
#include <HTTPClient.h>       // Para realizar peticiones HTTP POST al servidor web

// VARIABLES GLOBALES Y DEFINICIONES
const char* ssid = "DAVID";             // Nombre de la red WiFi
const char* password = "0987019816";    // Contraseña WiFi

// URL del script PHP donde se guardarán los datos
const char* serverName = "http://monitoreogasesdvepc.com/insertar.php"; 

// Definición de Pines (Hardware Serial 2 y GPIOs)
#define RXD2 16             // Pin de recepción serial
#define TXD2 17             // Pin de transmisión serial
#define DHT_PIN 15          // Pin de datos del sensor DHT
#define LED_VERDE 5         // Indicador de nivel seguro
#define LED_AMARILLO 18     // Indicador de nivel precaución
#define LED_ROJO 19         // Indicador de nivel peligro

DHTesp dhtSensor;           // Instancia del objeto sensor DHT
bool notificacionEnviada = false; // Bandera para controlar el envío de notificaciones

// Configuración inicial
void setup() {
  Serial.begin(115200); // Inicia el puerto serial para depuración
  
  // Se usa Serial2 en los pines 16 y 17 a 9600 baudios
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  // Configuración de pines de salida para los LEDs
  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARILLO, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  
  // Inicialización del sensor DHT22
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  // Conexión a la red WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");
  // Bucle que espera hasta que la conexión sea exitosa
  while(WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi Conectado!");

  // Inicia la conexión con los servidores de Blynk usando el Token
  Blynk.config(BLYNK_AUTH_TOKEN);
}

// LOOP: Bucle principal de ejecución
void loop() {

  Blynk.run(); // Mantiene la conexión con Blynk activa y procesa comandos entrantes

  // Verifica si hay datos llegando por el puerto Serial2 
  if (Serial2.available()) {
    String linea = Serial2.readStringUntil('\n');
    linea.trim(); // Elimina espacios en blanco al inicio y final
    
    // PARSEO DE DATOS 
    // Se asume que el formato recibido es: "CO,CH4,Etanol,H2,NH3,General"
    int c1 = linea.indexOf(',');
    int c2 = linea.indexOf(',', c1 + 1);
    int c3 = linea.indexOf(',', c2 + 1);
    int c4 = linea.indexOf(',', c3 + 1);
    int c5 = linea.indexOf(',', c4 + 1);

    // Si se encontraron todas las comas, procedemos a separar los strings
    if (c5 > 0) {
      String strCO = linea.substring(0, c1);
      String strCH4 = linea.substring(c1 + 1, c2);
      String strEtanol = linea.substring(c2 + 1, c3);
      String strH2 = linea.substring(c3 + 1, c4);
      String strNH3 = linea.substring(c4 + 1, c5);
      String strGeneral = linea.substring(c5 + 1);

      // Calculo de porcentaje basado en lectura analógica
      float valorRaw = strGeneral.toFloat(); 
      float porcentajeGeneral = (valorRaw * 100.0) / 1023.0;

      // Lectura del sensor local (Temperatura y Humedad)
      float temp = dhtSensor.getTemperature();
      float hum = dhtSensor.getHumidity();
      // Si la lectura falla (NaN), asignamos 0 para evitar errores
      if (isnan(temp)) { temp=0; hum=0; }

      // ENVÍO A BLYNK (Pines Virtuales) 
      Blynk.virtualWrite(V0, hum);                 // Humedad
      Blynk.virtualWrite(V1, temp);                // Temperatura
      Blynk.virtualWrite(V2, strCO.toFloat());     // Monóxido de Carbono
      Blynk.virtualWrite(V3, strEtanol.toFloat()); // Etanol
      Blynk.virtualWrite(V4, strCH4.toFloat());    // Metano
      Blynk.virtualWrite(V5, strH2.toFloat());     // Hidrógeno
      Blynk.virtualWrite(V6, strNH3.toFloat());    // Amoniaco
      Blynk.virtualWrite(V7, porcentajeGeneral);   // Gas general en %
      // Enviamos el estado de los LEDs a Blynk (0 o 255) para visualizarlos en la app
      Blynk.virtualWrite(V8, digitalRead(LED_VERDE) * 255);   
      Blynk.virtualWrite(V9, digitalRead(LED_AMARILLO) * 255); 
      Blynk.virtualWrite(V10, digitalRead(LED_ROJO) * 255);


      // ENVÍO AL SERVIDOR WEB 
      if(WiFi.status() == WL_CONNECTED){
        HTTPClient http;
        http.begin(serverName); // Inicia conexión con la URL
        // Especificamos que enviaremos datos tipo formulario
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        // Construcción del string de datos para el POST
        String postData = "co=" + strCO + 
                          "&metano=" + strCH4 + 
                          "&etanol=" + strEtanol + 
                          "&h2=" + strH2 + 
                          "&nh3=" + strNH3 + 
                          "&temp=" + String(temp) + 
                          "&hum=" + String(hum) +
                          "&gas_general=" + strGeneral;
        
        Serial.println("Enviando: " + postData);
        
        // Ejecuta la petición POST y guarda el código de respuesta
        int codigo = http.POST(postData);
        
        if (codigo > 0) {
          String respuesta = http.getString();
          Serial.println("Codigo HTTP: " + String(codigo));
          Serial.println("--- EL SERVIDOR DICE: ---");
          Serial.println(respuesta); 
          Serial.println("-------------------------");
        } else {
          Serial.print("Error en la petición: ");
          Serial.println(codigo);
        }
        http.end(); // Cierra la conexión HTTP
      }
      
      // LÓGICA DE ALARMAS Y LEDS
      float gasecitoGeneral = strGeneral.toFloat();

      // ESTADO: SEGURO (0 - 200)
      if (gasecitoGeneral >= 0 && gasecitoGeneral <= 200 ) {
        digitalWrite(LED_VERDE, HIGH); 
        Blynk.virtualWrite(V8, 255); // Enciende LED virtual en App
      } else {
        digitalWrite(LED_VERDE, LOW); 
        Blynk.virtualWrite(V8, 0);
      }

      // ESTADO: PRECAUCIÓN (201 - 600)
      if (gasecitoGeneral > 200 && gasecitoGeneral <= 600 ) {
        digitalWrite(LED_AMARILLO, HIGH); 
        Blynk.virtualWrite(V9, 255);
      } else {
        digitalWrite(LED_AMARILLO, LOW); 
        Blynk.virtualWrite(V9, 0);
      }

      // ESTADO: PELIGRO (> 600)
      if (gasecitoGeneral > 600){
        digitalWrite(LED_ROJO, HIGH);
        Blynk.virtualWrite(V10, 255);

         // Lógica de Notificación Push 
         if (notificacionEnviada == false) {
          Serial.println("¡CRITICO! Enviando notificación al celular...");
          String mensajeAlerta = "¡PELIGRO! Fuga detectada. Nivel General: " + String(gasecitoGeneral);
          
          // 'alerta_fuga' debe coincidir con el nombre del evento creado en la consola de Blynk
          Blynk.logEvent("alerta_fuga", mensajeAlerta); 
          
          notificacionEnviada = true; // Marca que ya se envió para no saturar
          }
      } else {
        digitalWrite(LED_ROJO, LOW); 
        Blynk.virtualWrite(V10, 0);

        // Si el nivel bajó de 600 y la notificación ya se había enviado, reseteamos la bandera
        if (notificacionEnviada == true) {
           notificacionEnviada = false;
           Serial.println("Niveles normalizados. Alarma reseteada.");
        }
      }
    } // Fin if(c5 > 0)
  } // Fin if(Serial2.available)
}
