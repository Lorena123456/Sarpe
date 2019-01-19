#include <LedControl.h>
#include <LiquidCrystal.h>

LedControl lc = LedControl (12, 11, 10, 1);
LiquidCrystal lcd (2, 3, 4, 5, 6, 7);

//definim pinii
#define JOYSTICK_X  A0
#define JOYSTICK_Y  A1
#define sus       1
#define dreapta   2
#define jos       3
#define stanga    4
#define lungimeinitialasarpe  3
#define joystickprag  160

int sarpeDirectie = 0;
int sarpeViteza = 1;
int sarpeLungime = lungimeinitialasarpe;
bool castigator = false;
bool pierdere = false;
struct Coordonate {
  int x = 0;
  int y = 0;
  Coordonate(int x = 0, int y = 0): x(x), y(y) {}
};
struct Punct {
  int linie = 0;
  int coloana = 0;
  Punct(int linie = 0, int coloana = 0): linie(linie), coloana(coloana) {}
};

Punct sarpe; // coordonatele de la care pornește șarpele vor fi generate aleator
Punct mancare (-1, -1); // punctul de hrană nu este nicăieri încă
Coordonate initialJoystick (500, 500);
Coordonate valuare;

int matrice[8][8];

void setup() {
  Serial.begin(9600);
  lc.shutdown(0, false);
  lc.setIntensity(0, 9);
  lc.clearDisplay(0);

  randomSeed(analogRead(0));
  sarpe.linie = random(8);
  sarpe.coloana = random(8);

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   Press start");
  lcd.setCursor(0, 1);
}

void loop() {
  genereazaMancarea();
  joystickMiscare();
  sarpeMiscare();
  stadiileJocului();
}

//dacă nu există hrană, generați una, verificați și pentru victorie
void genereazaMancarea() {
  if (mancare.linie == -1 || mancare.coloana == -1) {
    if (sarpeLungime >= 64) {
      castigator = true;
      return; //împiedicați funcționarea generatorului de mancare, în acest caz se va desfășura pentru totdeauna
    }
    // genereaza mancarea
    do {
      mancare.coloana = random(8);
      mancare.linie = random(8);
    } while (matrice[mancare.linie][mancare.coloana] > 0);
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

    //clipirea manacarii
    lc.setLed(0, mancare.linie, mancare.coloana, millis() % 100 < 50 ? 1 : 0);
  }
}


void sarpeMiscare() {
  switch (sarpeDirectie) {
    case sus:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Have fun!");
      sarpe.linie--;
      margine();
      lc.setLed(0, sarpe.linie, sarpe.coloana, 1);
      break;

    case dreapta:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Have fun!");
      sarpe.coloana++;
      margine();
      lc.setLed(0, sarpe.linie, sarpe.coloana, 1);
      break;

    case jos:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Have fun!");
      sarpe.linie++;
      margine();
      lc.setLed(0, sarpe.linie, sarpe.coloana, 1);
      break;

    case stanga:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Have fun!");
      sarpe.coloana--;
      margine();
      lc.setLed(0, sarpe.linie, sarpe.coloana, 1);
      break;

    default:
      return;
  }

  if (matrice[sarpe.linie][sarpe.coloana] != 0 && sarpeDirectie != 0) {
    pierdere = true;
    return;
  }

  // verificați dacă mâncarea a fost consumată
  if (sarpe.linie == mancare.linie && sarpe.coloana == mancare.coloana) {
    sarpeLungime++;
    mancare.linie = -1; // reseteaza mancarea
    mancare.coloana = -1;
  }

  Matrice();

  matrice[sarpe.linie][sarpe.coloana]++;
}

void margine() {
  if (sarpe.coloana < 0) {
    sarpe.coloana = 7;
  }
  else if (sarpe.coloana > 7) {
    sarpe.coloana = 0;
  }

  if (sarpe.linie  < 0) {
    sarpe.linie = 7;
  }
  else if (sarpe.linie > 7) {
    sarpe.linie = 0;
  }
}

void Matrice() {
  for (int linie = 0; linie < 8; linie++) {
    for (int coloana = 0; coloana < 8; coloana++) {
      if (matrice[linie][coloana] > 0 ) {
        matrice[linie][coloana]++;
      }

      if (matrice[linie][coloana] > sarpeLungime) {
        lc.setLed(0, linie, coloana, 0);
        matrice[linie][coloana] = 0;
      }
    }
  }
}

void stadiileJocului() {
  if (pierdere || castigator) {
    revenireSarpe();
    mesajScor(sarpeLungime);
    if (pierdere) {
      lcd.setCursor(2, 1);
      lcd.print("Press Start!");
    }
    castigator = false;
    pierdere = false;
    sarpeLungime = lungimeinitialasarpe;
    sarpeDirectie = 0;
    memset(matrice, 0, sizeof(matrice[0][0]) * 8 * 8);
    lc.clearDisplay(0);
  }
}

void revenireSarpe() {
  lc.setLed(0, mancare.linie, mancare.coloana, 0);
  delay(600);
  for (int i = 1; i <= sarpeLungime; i++) {
    for (int linie = 0; linie < 8; linie++) {
      for (int coloana = 0; coloana < 8; coloana++) {
        if (matrice[linie][coloana] == i) {
          lc.setLed(0, linie, coloana, 0);
          delay(100);
        }
      }
    }
  }
}

void mesajScor(int scor) {
  if (scor < 0 || scor > 99) return;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("      Score:");
  lcd.setCursor(14, 0);
  lcd.print(scor);
}
