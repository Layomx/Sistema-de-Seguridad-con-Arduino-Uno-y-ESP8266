#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD con dirección 0x27 (Es necesario hacer un escaneo de la direccion)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variables para el temporizador
unsigned long startUpTime = 0;
bool printMessage = false;

void setup() {
  Serial.begin(115200);  // Inicia la comunicación serial, debe ser la misma que la WeMos D1 Mini
  lcd.init();            // Iniciando el LCD
  lcd.backlight();       // Encendiendo la luz del fondo
}

void loop() {
  // Verificando si hay datos disponibles en el puerto serial
  if (Serial.available() > 0) {
    // Con esto leeremos los mensajes del puerto serial hasta el salto de linea
    String message = Serial.readStringUntil('\n'); 

    // Prioridad 1: Que hayan abierto la puerta
    if (message.indexOf("La puerta ha sido abierta") >= 0) {
      lcd.clear(); // Borra la pantalla
      lcd.setCursor(0, 0);
      lcd.print("Puerta");
      lcd.setCursor(0, 1);
      lcd.print("Abierta!");

      // Guardando el tiempo en que se mostro el mensaje
      startUpTime = millis();
      printMessage = true;  // El mensaje ha sido mostrado
    }
    // Prioridad 2: Movimiento detectado cerca de la puerta
    if (message.indexOf("Movimiento detectado cerca de la puerta") >= 0) {
      lcd.clear(); // Borra la pantall
      lcd.setCursor(0, 0);
      lcd.print("Movimiento");
      lcd.setCursor(0, 1);
      lcd.print("detectado!");

      // Guardando el tiempo en que se mostro el mensaje
      startUpTime = millis();
      printMessage = true;  // El mensaje ha sido mostrado
    }
  }

  // Si el mensaje fue mostrado y han pasado 10 segundos entonces se reinicia su estado
  if (printMessage && (millis() - startUpTime >= 10000)) {
    lcd.clear();  // Borra la pantalla después de 10 segundos
    printMessage = false;  // Reiniciando la variable
  }
}
