#include <LedControl.h> // Biblioteca LedControl este utilizată pentru controlul unei matrice LED.
#include <LiquidCrystal.h> //Biblioteca LiquidCrystal este utilizată pentru controlul LCD.

LiquidCrystal lcd (2, 3, 4, 5, 6, 7);
LedControl lc = LedControl (12, 11, 10, 1);

//definim pinii
#define JOYSTICK_X  A0
#define JOYSTICK_Y  A1

const short lungimeinitialasarpe = 3; //lungimea inițială a șarpelui (1-63, recomandată 3)

// parametrii sarpelui
int sarpeLungime = lungimeinitialasarpe;
int sarpeViteza = 1; // nu poate fi 0
int sarpeDirectie = 0; // dacă este 0, șarpele nu se mișcă

// directia constantelor
const short sus     = 1;
const short dreapta  = 2;
const short jos   = 3;
const short stanga   = 4;

bool castigator = false;
bool pierdere = false;

const int joystickprag = 160; // prag în cazul în care mișcarea joystick-ului va fi acceptată

struct Point {
  int row = 0;
  int col = 0;
  Point(int row = 0, int col = 0): row(row), col(col) {}
};

struct Coordinate {
  int x = 0;
  int y = 0;
  Coordinate(int x = 0, int y = 0): x(x), y(y) {}
};

Point sarpe; // coordonatele de la care pornește șarpele vor fi generate aleator
Point mancare (-1, -1); // punctul de hrană nu este nicăieri încă
Coordinate initialJoystick (500, 500);  //construiți cu valorile implicite în cazul în care utilizatorul oprește calibrarea
Coordinate values;

int matrice[8][8]; // matricea "matrice": deține o "vârstă" a fiecărui pixel din matrice. Dacă numărul> 0, acesta luminează.
// când vârsta unui anumit pixel depășește lungimea șarpelui, acesta se stinge.
// 1 se adaugă în direcția actuală de șarpe de lângă ultima poziție a capului de șarpe.

void setup() {
  Serial.begin(9600); // setează rata de date în biți pe secundă pentru transmiterea de date seriale.
  lc.shutdown(0, false); // opriți economisirea energiei, permite afișarea
  lc.setIntensity(0, 9);
  lc.clearDisplay(0); // clear screen

  randomSeed(analogRead(0));
  sarpe.row = random(8);
  sarpe.col = random(8);

  calibrateJoystick();

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SNAKE GAME<Start");
  lcd.setCursor(0, 1);
  lcd.print("  Move Joystick");
}

void loop() {
  genereazamancarea(); // dacă nu există alimente, generați unul
  joystickMiscare(); // priveste miscarile joystick-ului
  sarpeMiscare(); // calculează parametrii de șarpelui
  gameStates();
}

//dacă nu există hrană, generați una, verificați și pentru victorie
void genereazamancarea() {
  if (mancare.row == -1 || mancare.col == -1) {
    if (sarpeLungime >= 64) {
      castigator = true;
      return; //împiedicați funcționarea generatorului de alimente, în acest caz se va desfășura pentru totdeauna
    }
    // genereaza mancarea
    do {
      mancare.col = random(8);
      mancare.row = random(8);
    } while (matrice[mancare.row][mancare.col] > 0);
  }
}

// priveste miscarile joystick-ului
void joystickMiscare() {
  int anterioaraDirectie = sarpeDirectie; // salveaza ultima directie
  long timestamp = millis() + sarpeViteza; // atunci când următorul cadru va fi redat

  while (millis() < timestamp) {
    sarpeViteza = 1000 - map(sarpeLungime, 0, 16, 10, 1000);
    if (sarpeViteza == 0) sarpeViteza = 1; // siguranță: viteza nu poate fi 0

    // determina direcția șarpelui

    analogRead(JOYSTICK_Y) < initialJoystick.y - joystickprag ? sarpeDirectie = sus     : 0;
    analogRead(JOYSTICK_Y) > initialJoystick.y + joystickprag ? sarpeDirectie = jos     : 0;
    analogRead(JOYSTICK_X) < initialJoystick.x - joystickprag ? sarpeDirectie = stanga  : 0;
    analogRead(JOYSTICK_X) > initialJoystick.x + joystickprag ? sarpeDirectie = dreapta : 0;

    // ignorați schimbarea direcțională cu 180 de grade (fără efect pentru șarpele care nu se mișcă)
    sarpeDirectie + 2 == anterioaraDirectie && anterioaraDirectie != 0 ? sarpeDirectie = anterioaraDirectie : 0;
    sarpeDirectie - 2 == anterioaraDirectie && anterioaraDirectie != 0 ? sarpeDirectie = anterioaraDirectie : 0;

    //clipirea manacarii
    lc.setLed(0, mancare.row, mancare.col, millis() % 100 < 50 ? 1 : 0);
  }
}

// calculeaza datele despre mișcarea șarpelui
void sarpeMiscare() {
  switch (sarpeDirectie) {
    case sus:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Try not to lose!");
      sarpe.row--;
      edge();
      lc.setLed(0, sarpe.row, sarpe.col, 1);
      break;

    case dreapta:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Try not to lose!");
      sarpe.col++;
      edge();
      lc.setLed(0, sarpe.row, sarpe.col, 1);
      break;

    case jos:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Try not to lose!");
      sarpe.row++;
      edge();
      lc.setLed(0, sarpe.row, sarpe.col, 1);
      break;

    case stanga:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Try not to lose!");
      sarpe.col--;
      edge();
      lc.setLed(0, sarpe.row, sarpe.col, 1);
      break;

    default:
      return;
  }

  if (matrice[sarpe.row][sarpe.col] != 0 && sarpeDirectie != 0) {
    pierdere = true;
    return;
  }

  // verificați dacă mâncarea a fost consumată
  if (sarpe.row == mancare.row && sarpe.col == mancare.col) {
    sarpeLungime++;
    mancare.row = -1; // reseteaza mancarea
    mancare.col = -1;
  }

  // vârstele de creștere dacă toate led-urile aprinse
  updateMatrice();

  // schimbați vârsta capului de șarpe de la 0 la 1
  matrice[sarpe.row][sarpe.col]++;
}

// determină ca șarpele să apară pe cealaltă parte a ecranului dacă se iese din margine
void edge() {

  if (sarpe.col < 0) {
    sarpe.col = 7;
  }
  else if (sarpe.col > 7) {
    sarpe.col = 0;
  }

  if (sarpe.row  < 0) {
    sarpe.row = 7;
  }
  else if (sarpe.row > 7) {
    sarpe.row = 0;
  }
}


// vârstele de creștere dacă toate led-urile aprinse, opriți la cele vechi în funcție de lungimea șarpelui
void updateMatrice() {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      if (matrice[row][col] > 0 ) {
        matrice[row][col]++;
      }

      //dacă vârsta depășește lungimea șarpei, opriți-o
      if (matrice[row][col] > sarpeLungime) {
        lc.setLed(0, row, col, 0);
        matrice[row][col] = 0;
      }
    }
  }
}

void gameStates() {
  if (pierdere || castigator) {
    unrollSnake();
    scoreMessage(sarpeLungime);
    if (pierdere) {
      lcd.setCursor(2, 1);
      lcd.print("Start Again!");
    }
    castigator = false;
    pierdere = false;
    sarpeLungime = lungimeinitialasarpe;
    sarpeDirectie = 0;
    memset(matrice, 0, sizeof(matrice[0][0]) * 8 * 8);
    lc.clearDisplay(0);
  }
}

void unrollSnake() {
  // opriți LED-ul pentru mancare
  lc.setLed(0, mancare.row, mancare.col, 0);
  delay(600);
  for (int i = 1; i <= sarpeLungime; i++) {
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        if (matrice[row][col] == i) {
          lc.setLed(0, row, col, 0);
          delay(100);
        }
      }
    }
  }
}

// calibrați joystick-ul de 10 ori
void calibrateJoystick() {
  for (int i = 0; i < 10; i++) {
    values.x += analogRead(JOYSTICK_X);
    values.y += analogRead(JOYSTICK_Y);
  }

  initialJoystick.x = values.x / 10;
  initialJoystick.y = values.y / 10;
}

void scoreMessage(int score) {
  if (score < 0 || score > 99) return;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GameOverScore:");
  lcd.setCursor(14, 0);
  lcd.print(score);
}
