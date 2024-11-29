// Librerias importadas
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Configuracion de red WiFi
const char* netWifi = "LIB-8846994";       // Nombre de red WiFi
const char* passwordWifi = "3t5YqpxshgSh"; // Contraseña de red WiFi

// Prototipo de la funcion para conectar WeMos D1 a WiFi
void connectToWiFi();

// Configuracion de la API CallMeBot
const char* phoneNumber = "50765039529";         // Numero de telefono con codigo de pais sin "+"
const char* apiKey = "4371090";                  // API Key de CallMeBot
const char* connectionMessage = "Sistema encendido"; // Mensaje de inicio
HTTPClient http;
WiFiClient client;

// Prototipo de funciones para la API CallMeBot
String urlencode(String str);
void startUpMessage();

// PIN y configuracion del sensor PIR
const int pirSensor = D2;                          // PIN donde estara conectado el sensor PIR (GPIO4)
const int thresholdActivate = 3;                   // Umbral de activacion de movimiento
const int thresholdDeactivate = 2;                 // Umbral de desactivacion de movimiento
const int debounceTime = 500;                      /* Tiempo de debounce en milisegundos, cada 500 ms se hara una lectura
                                                   Ideal de para el temporizador                                     */
const unsigned long reportInterval = 60000;        // 60 segundos entre reportes de movimiento

// Prototipo de funciones para el sensor PIR
bool PIR();
void sendPirMessage(const char* message);

// PIN y configuracion del sensor magnetico
const int magneticSensor = D1;
bool currentState = false;
bool previousState  = false;

// Prototipo de funciones para el sensor magnetico
void MAGNETIC();
void sendMagneticMessage(const char* message);

void setup() 
{
  // Configurando el pin del PIR como entrada
  pinMode(pirSensor, INPUT);

  // Configurando el pin del sensor magnetico como entrada
  pinMode(magneticSensor,INPUT_PULLUP);

  // Iniciando comunicacion serial y conexion WiFi para la placa WeMos D1 Mini
  Serial.begin(115200);
  
  // Iniciando conexion Wi-Fi
  connectToWiFi();
  
  // Envio de mensaje de inicio a traves de la API CallMeBot
  startUpMessage();
}

void loop() 
{
  // Llamando la funcion del PIR
  if (PIR())
  {
    
  }

  // Llamando la funcion del sensor magnetico
  MAGNETIC();
  
  delay(100);   // Pequeño retraso para no saturar la depuraciones serie
}

// Funcion para conectar la placa WeMos a WiFi
void connectToWiFi()
{
  WiFi.begin(netWifi, passwordWifi); // Iniciando conexion WiFi
  Serial.print("Conectando a Wi-Fi...");
    
  // Imprimiendo puntos hasta la conexion
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }

  // Impresion de la IP asignada
  Serial.println("\nConexion establecida"); 
  Serial.print("Direccion IP: ");
  Serial.println(WiFi.localIP());
}

// Funcion para mandar un mensaje de inicio
void startUpMessage()
{
  // Aseguramos que la conexion Wi-Fi este establecida
  if (WiFi.status() == WL_CONNECTED) 
  {
    
    // Construye la URL con la codificación correcta
    String url = "http://api.callmebot.com/whatsapp.php?phone=" + 
                 String(phoneNumber) + 
                 "&apikey=" + String(apiKey) + 
                 "&text=" + urlencode(connectionMessage);
    
    http.begin(client, url);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode = 200) 
    {
      String response = http.getString();
      Serial.println("Código de respuesta HTTP: " + String(httpResponseCode));
      Serial.println("Respuesta: " + response);
    } 
    else 
    {
      Serial.println("Error al enviar mensaje");
      Serial.println("Código de error: " + String(httpResponseCode));
    }

    // Cerrando comunicacion
    http.end();
  }
}

// Funcion para codificar el URL
String urlencode(String str) 
{
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') 
    {
      encodedString += '+';
    } 
    else if (isalnum(c)) 
    {
      encodedString += c;
    } 
    else 
    {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) 
      {
        code1 = (c & 0xf) - 10 + 'A';
      }
      
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      
      if (c > 9) 
      {
        code0 = c - 10 + 'A';
      }
      
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }
  return encodedString;
}

// Funcion del sensor PIR 
bool PIR() 
{
  // Variables para manejar la logica del sensor
  static unsigned long lastChangeTime = 0;         // El tiempo en que tuvo el ultimo cambio
  static unsigned long lastMotionReportTime = 0;   // El tiempo en que se reporto por ultima vez movimiento
  static unsigned long lastSerialPrintTime = 0;    // El tiempo en que se imprimio en puerto serie
  static unsigned long lastMessageSentTime = 0;    // El tiempo en que se envio el ultimo mensaje PIR
  static int motionCount = 0;                      // Conteo de lecturas de movimiento
  static bool previousMotionState = false;         // Estado previo de lectura de movimiento
  static bool messageSent = false;                 // Variable de control de mensajes

  unsigned long currentTime = millis();            // La variable currentTime guardara los valores de la funcion millis
  int pirState = digitalRead(pirSensor);           // Leyendo estado del sensor

  if (currentTime - lastChangeTime >= debounceTime)  // Eliminacion de rebotes (evitar lecturas erraticas)
  {
    if (pirState == HIGH)
    {
      motionCount++;    // Contador de lecutras incrementando
      motionCount = min(motionCount, thresholdActivate * 2);
    }
    else 
    {
      motionCount = max(0, motionCount - 1);
    }

    bool currentMotionState = (motionCount >= thresholdActivate);   // Histeresis, evitando que el estado cambie debido a fluctuaciones

    if (currentMotionState != previousMotionState) // Cambio de estado
    {
      previousMotionState = currentMotionState;

      // Si se detecta movimiento y han pasado 5 segundos desde la última impresión en el puerto serie
      if (currentMotionState && (currentTime - lastSerialPrintTime >= 5000)) 
      {
        Serial.println("Movimiento detectado cerca de la puerta");
        lastSerialPrintTime = currentTime;  // Actualizamos el tiempo de la última impresión serie
      }

      // Si se detecta movimiento y no se ha enviado mensaje en el último minuto
      if (currentMotionState && (currentTime - lastMessageSentTime >= 60000) && !messageSent) 
      {
        // sendPirMessage("Movimiento detectado cerca de la puerta");      // Llamada para enviar el mensaje
        messageSent = true;                                             // Ya se ha enviado el mensaje
        lastMessageSentTime = currentTime;                              // Actualizamos el tiempo de envío del mensaje
      }
    }

    // Si ha pasado 1 minuto desde el último mensaje, se puede enviar otro mensaje
    if (currentTime - lastMessageSentTime >= reportInterval) 
    {
      messageSent = false; // Reseteamos el estado para poder enviar otro mensaje en el siguiente ciclo
      // La idea es no sobrecargar la API ya que podria generar muchos errores
    }

    // Si no se detecta movimiento, decrementamos el contador de movimiento
    // Si el contador de movimiento llega a un valor bajo, lo consideramos como sin movimiento
    if (motionCount < thresholdDeactivate) 
    {
      previousMotionState = false;  // Restableciendo el estado a "no hay movimiento"
    }

    lastChangeTime = currentTime;   // Se actualiza el tiempo de la ultima lectura
  }

  return previousMotionState;
}

void sendPirMessage(const char* message)
{
  
  if (WiFi.status() != WL_CONNECTED) 
  {
  Serial.println("Error: No hay conexión WiFi.");
  return; // No enviar el mensaje si no hay conexión
  }

  String url = "http://api.callmebot.com/whatsapp.php?phone=" + 
               String(phoneNumber) + 
               "&apikey=" + String(apiKey) + 
               "&text=" + urlencode(message);
  
  http.begin(client, url);
  int httpResponseCode = http.GET();

  if (httpResponseCode = 200)
  {
    String response = http.getString();
    Serial.println("Código de respuesta HTTP: " + String(httpResponseCode));
    Serial.println("Respuesta de la API: " + response);
    Serial.println("Mensaje PIR enviado");
  }
  else
  {
    Serial.println("Error al enviar mensaje PIR");
    Serial.println("Código de error HTTP: " + String(httpResponseCode)); // Imprime el código de error
  }

  http.end();
}

void MAGNETIC()
{
 currentState = digitalRead(magneticSensor) == LOW; // Leyendo el estado del sensor

 if (currentState != previousState)
 {
  if (currentState)
  {
    Serial.println("Puerta cerrada");
  }
  else 
  {
    // Este mensaje debe ser especifico porque el Arduino lo leera
    Serial.println("La puerta ha sido abierta");
    // sendMagneticMessage("La puerta ha sido abierta"); // Mensaje de CallMeBot
  }

  // Actualizando estado previo
  previousState = currentState;
 }
}

void sendMagneticMessage(const char* message)
{
  if (WiFi.status() != WL_CONNECTED) 
  {
  Serial.println("Error: No hay conexión WiFi.");
  return; // No enviar el mensaje si no hay conexión
  }

  String url = "http://api.callmebot.com/whatsapp.php?phone=" + 
               String(phoneNumber) + 
               "&apikey=" + String(apiKey) + 
               "&text=" + urlencode(message);
  
  http.begin(client, url);
  int httpResponseCode = http.GET();

  if (httpResponseCode = 200)
  {
    String response = http.getString();
    Serial.println("Código de respuesta HTTP: " + String(httpResponseCode));
    Serial.println("Respuesta de la API: " + response);
    Serial.println("Mensaje Magnetico enviado");
  }
  else
  {
    Serial.println("Error al enviar mensaje PIR");
    Serial.println("Código de error HTTP: " + String(httpResponseCode)); // Imprime el código de error
  }

  http.end();
}
