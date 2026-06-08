#include <Wire.h>
#include <Keypad.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define RELAY_PIN 18
#define BUZZER_PIN 5

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','4','7','*'},
  {'2','5','8','0'},
  {'3','6','9','#'},
  {'A','B','C','D'}
};
byte rowPins[ROWS] = {27, 14, 12, 13};
byte colPins[COLS] = {32, 33, 25, 26};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

HardwareSerial SerialRasp(2);

String password = "1234";
String input = "";
int intentos = 0;
bool bloqueado = false;

void mostrarMensaje(String linea1, String linea2 = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 20);
  display.println(linea1);
  if (linea2 != "") {
    display.setCursor(0, 40);
    display.println(linea2);
  }
  display.display();
}

void abrirCaja() {
  digitalWrite(RELAY_PIN, LOW);
  tone(BUZZER_PIN, 1000, 500);
  SerialRasp.println("ABIERTO");
  mostrarMensaje("** ABIERTO **", "Bienvenido!");
  delay(5000);
  digitalWrite(RELAY_PIN, HIGH);
  SerialRasp.println("CERRADO");
  mostrarMensaje("Caja Fuerte", "Ingrese clave:");
  input = "";
  intentos = 0;
}

void setup() {
  Serial.begin(115200);
  SerialRasp.begin(9600, SERIAL_8N1, 16, 17);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  Wire.begin(21, 22);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED no encontrado!");
    while(1);
  }

  SerialRasp.println("CERRADO");
  Serial.println("Sistema listo!");
  mostrarMensaje("Caja Fuerte", "Ingrese clave:");
}

void loop() {
  if (SerialRasp.available()) {
    String cmd = SerialRasp.readStringUntil('\n');
    cmd.trim();
    if (cmd == "OPEN") {
      abrirCaja();
    } else if (cmd.startsWith("PASS:")) {
      password = cmd.substring(5);
      mostrarMensaje("Clave cambiada!", "Nueva: " + password);
      SerialRasp.println("PASS_OK");
      delay(2000);
      mostrarMensaje("Caja Fuerte", "Ingrese clave:");
    }
  }

  if (bloqueado) {
    mostrarMensaje("BLOQUEADO", "Espere...");
    delay(10000);
    bloqueado = false;
    intentos = 0;
    SerialRasp.println("DESBLOQUEADO");
    mostrarMensaje("Caja Fuerte", "Ingrese clave:");
    return;
  }

  char key = keypad.getKey();
  if (key) {
    Serial.println(key);
    if (key == '#') {
      if (input == password) {
        abrirCaja();
      } else {
        intentos++;
        tone(BUZZER_PIN, 200, 1000);
        if (intentos >= 3) {
          bloqueado = true;
          SerialRasp.println("BLOQUEADO");
          mostrarMensaje("BLOQUEADO!", "3 intentos");
        } else {
          mostrarMensaje("Clave incorrecta", "Intento: " + String(intentos));
          delay(2000);
          mostrarMensaje("Caja Fuerte", "Ingrese clave:");
        }
        input = "";
      }
    } else if (key == '*') {
      input = "";
      mostrarMensaje("Caja Fuerte", "Ingrese clave:");
    } else {
      input += key;
      String asteriscos = "";
      for (int i = 0; i < input.length(); i++) asteriscos += "*";
      mostrarMensaje("Clave:", asteriscos);
    }
  }
}