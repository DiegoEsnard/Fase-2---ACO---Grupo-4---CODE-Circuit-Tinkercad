/****************************************************************************************
  PROYECTO: “Habitación inteligente – Confort + Seguridad”       (Arquitectura de Computadoras)

  OBJETIVO
  --------
  • Automatizar la iluminación de una habitación según la luz ambiente.
  • Implementar un sistema de alarma: detección de movimiento (PIR) + alerta sonora/luminosa.
  • Mostrar temperatura, luz y estado de la alarma en una pantalla LCD 16×2 mediante I²C.

  COMPONENTES
  -----------
  * Arduino UNO R3
  * Sensor de movimiento PIR  (PIR_PIN  = D2)
  * Fotoresistor + R10 kΩ     (LDR_PIN  = A0)
  * Sensor TMP36 (temperatura)(TMP_PIN  = A2)
  * LED blanco  (lámpara)      (LAMP_PIN = D4)
  * LED rojo    (testigo)      (ALARM_LED= D5)
  * Buzzer piezo                (BUZZER_PIN = D6)
  * Pulsador (armar/desarmar)   (BTN_PIN  = D7 · INPUT_PULLUP)
  * LCD 16×2 I²C  (dir 0x27)    (SDA → A4, SCL → A5)

  NOTAS
  -----
  - UMBRAL_LUZ = 900    valor analógico (0-1023) por debajo del cual se considera “poca luz”.
  - TMP36:  0.5 V = 0 °C, 10 mV/°C.  Cálculo:  Temp(°C) = (Vout − 0.5) × 100.
  - Debounce por software: 50 ms.
****************************************************************************************/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);                   // dirección I²C 0x27, 16 columnas, 2 filas

/*─────────────────────────── DEFINICIÓN DE PINES ───────────────────────────*/
#define PIR_PIN     2   // señal OUT del sensor PIR
#define LDR_PIN     A0  // divisor fotoresistencia + R10k
#define TMP_PIN     A2  // salida analógica TMP36
#define LAMP_PIN    4   // LED blanco (PWM opcional)
#define ALARM_LED   5   // LED rojo
#define BUZZER_PIN  6   // Buzzer piezo
#define BTN_PIN     7   // Pulsador (INPUT_PULLUP)

/*─────────────────────────── PARÁMETROS GLOBALES ──────────────────────────*/
const int            UMBRAL_LUZ   = 900;       // 0-1023  (luz baja < 900)
const unsigned long  DEBOUNCE_MS  = 50;        // rebote pulsador

bool  alarmaActiva   = false;                  // estado ON/OFF de la alarma
bool  estadoEstable  = HIGH;                   // nivel estable del botón
unsigned long lastDebounce = 0;                // última transición válida
String mensajePrevio = "";                     // último mensaje mostrado en LCD

/*────────────────────────────────── SETUP ─────────────────────────────────*/
void setup() {
  pinMode(PIR_PIN , INPUT);
  pinMode(LDR_PIN , INPUT);
  pinMode(TMP_PIN , INPUT);
  pinMode(LAMP_PIN , OUTPUT);
  pinMode(ALARM_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BTN_PIN   , INPUT_PULLUP);           // botón a GND → LOW al presionar

  lcd.init();                                  // inicializa LCD
  lcd.backlight();
  Serial.begin(9600);                          // monitor serie (depuración)
}

/*────────────────────────────────── LOOP ──────────────────────────────────*/
void loop() {

  /* 1) ── GESTIÓN DEL BOTÓN (DEBOUNCE + TOGGLE) ─────────────────────────*/
  gestionarBoton();

  /* 2) ── LECTURA DE SENSORES ───────────────────────────────────────────*/
  bool pir  = digitalRead(PIR_PIN);                             // 1 = movimiento
  int  luz  = analogRead(LDR_PIN);                              // 0–1023
  int  tmp  = analogRead(TMP_PIN);                              // TMP36 (0–1023)

  float volt   = tmp * 5.0 / 1023.0;                            // Vout (V)
  float tempC  = (volt - 0.5) * 100.0;                          // fórmula TMP36

  /* 3) ── LÁMPARA AUTOMÁTICA (CONFORT) ─────────────────────────────────*/
  digitalWrite(LAMP_PIN, (luz < UMBRAL_LUZ));                   // ON si poca luz

  /* 4) ── ALARMA (SEGURIDAD) ───────────────────────────────────────────*/
  digitalWrite(ALARM_LED, alarmaActiva);                        // testigo rojo
  if (alarmaActiva && pir) {                                    // intruso
      tone(BUZZER_PIN, 1200);                                   // alarma sonora
      digitalWrite(ALARM_LED, !digitalRead(ALARM_LED));         // parpadeo
  } else {
      noTone(BUZZER_PIN);
  }

  /* 5) ── LCD 16×2: LÍNEA SUPERIOR (siempre actualiza) ─────────────────*/
  lcd.setCursor(0,0);
  lcd.print("T:"); lcd.print(tempC,1); lcd.print("C ");          // ej. 25.8C
  lcd.print("L:"); lcd.print(luz/10); lcd.print("   ");          // escalar 0-102

  /* 6) ── LCD 16×2: LÍNEA INFERIOR (solo si cambia) ────────────────────*/
  String mensaje = alarmaActiva ? (pir ? "INTRUSO" : "Alarma ON")
                                : "Alarma OFF";
  if (mensaje != mensajePrevio) {                                // evita parpadeo
      lcd.setCursor(0,1);
      lcd.print("                ");                             // limpia fila
      lcd.setCursor(0,1);
      lcd.print(mensaje);
      mensajePrevio = mensaje;
  }

  /* 7) ── DEPURACIÓN POR MONITOR SERIE (opcional) ─────────────────────*/
  Serial.print("Temp:"); Serial.print(tempC);
  Serial.print("  Luz:"); Serial.print(luz);
  Serial.print("  PIR:"); Serial.println(pir);

  delay(120);                                                   // ~8 Hz de refresco
}

/*──────────────────── FUNCIÓN: GESTIONAR BOTÓN CON DEBOUNCE ─────────────*/
void gestionarBoton() {
  bool lectura = digitalRead(BTN_PIN);

  /* ¿cambio de nivel? → espera DEBOUNCE_MS para confirmar */
  if (lectura != estadoEstable) {
      if (millis() - lastDebounce > DEBOUNCE_MS) {
          estadoEstable = lectura;               // nuevo estado confirmado
          if (estadoEstable == LOW)              // flanco HIGH→LOW
              alarmaActiva = !alarmaActiva;      // invertir bandera
      }
  } else {
      lastDebounce = millis();                   // reinicia temporizador
  }
}


