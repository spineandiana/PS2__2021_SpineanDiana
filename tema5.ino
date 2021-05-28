#include <LiquidCrystal.h>
#include <EEPROM.h>

#define BUTTON_OK 6
#define BUTTON_CANCEL 7
#define BUTTON_PREV 8
#define BUTTON_NEXT 9
#define BEC_pin 10

enum Buttons {
  EV_OK,
  EV_CANCEL,
  EV_NEXT,
  EV_PREV,
  EV_NONE,
  EV_MAX_NUM
};

enum Menus {
  MENU_MAIN = 0,
  MENU_START,
  MENU_KP,
  MENU_KI,
  MENU_KD,
  MENU_TEMP,
  MENU_TINCAL,
  MENU_TMEN,
  MENU_TRAC,
  MENU_MAX_NUM
};

LiquidCrystal lcd(12, 11, 4, 3, 8, 5);

double temp=25;
float temperatura_citita_senzor;
float output;
double kp = 15, ki = 12, kd = 10;
int tIncal = 25, tMen = 14, tRac = 16;
double eroare= 0, suma_erori= 0, eroare_anterioara = 0, derivativa = 0, moving_sp;
double dt= 0.25; // timp esantionare
int ora = 0, min = 0, sec = 0;
int change = 0;
unsigned long uptime = -1;
int timp_inc , timp_men, timp_rac;
Menus scroll_menu = MENU_MAIN;
Menus current_menu =  MENU_MAIN;


void state_machine(enum Menus menu, enum Buttons button);
Buttons GetButtons(void);
void print_menu(enum Menus menu);

typedef void (state_machine_handler_t)(void);

void print_menu(enum Menus menu)
{
  lcd.clear();
  switch(menu)
  {
    case MENU_KP:
      lcd.setCursor(0,1);
      lcd.print("1. Mesaje ");
     // lcd.print(kp);
      break;
    
    case MENU_KI:
      lcd.setCursor(0,1);
      lcd.print("2. Control");
     // lcd.print(ki);
      break;
    
    case MENU_KD:
      lcd.setCursor(0,1);
      lcd.print("3. Temperatura");
      //lcd.print(kd);
      break;
    
    case MENU_TEMP:
      lcd.setCursor(0,1);
      lcd.print("4. Inundatii");
     // lcd.print(temp);
      break;
    /*
    case MENU_TINCAL:
      lcd.setCursor(0, 1);
     // lcd.print("TINCAL = ");
      // lcd.print(tIncal);
      break;
    
    case MENU_TMEN:
      lcd.setCursor(0, 1);
      //lcd.print("TMEN = ");
      // lcd.print(tMen);
      break;
    */
    /*
    case MENU_TRAC:
      lcd.setCursor(0, 1);
     // lcd.print("TRAC = ");
     // lcd.print(tRac);
      break;
    */
    case MENU_START:
      lcd.setCursor(0, 1);
      lcd.print("START!");
      break;
    
    case MENU_MAIN:
    default:
      lcd.setCursor(0,0);
      lcd.print("PR. Sincretic");
      lcd.setCursor(0,1);
      lcd.print("Spinean Diana");
  }
  
  if (current_menu == MENU_START)
  {
    lcd.clear();
    afisare_timp();
  }
  else if(current_menu != MENU_MAIN)
  {
    lcd.setCursor(0,0);
    lcd.print("Modifica - P:");
    lcd.print(menu);
  }
}

void enter_menu(void)
{
  current_menu = scroll_menu;
}

void go_home(void)
{
  scroll_menu = MENU_MAIN;
  current_menu = scroll_menu;
  change = 0;
}

void go_next(void)
{
  scroll_menu = (Menus) ((int)scroll_menu + 1);
  scroll_menu = (Menus) ((int)scroll_menu % MENU_MAX_NUM);
}

void go_prev(void)
{
  scroll_menu = (Menus) ((int)scroll_menu - 1);
  scroll_menu = (Menus) ((int)scroll_menu % MENU_MAX_NUM);
}

void save_kp(void)
{
  EEPROM.put(0,kp);
  go_home();
}

void save_ki(void)
{
  EEPROM.put(0,ki);
  go_home();
}

void save_kd(void)
{
  EEPROM.put(0,kd);
  go_home();
}

void save_temp(void)
{
  EEPROM.put(0,temp);
  go_home();
}

void save_tIncal(void)
{
  EEPROM.put(0,tIncal);
  go_home();
}

void save_tMen(void)
{
  EEPROM.put(0,tMen);
  go_home();
}

void save_tRac(void)
{
  EEPROM.put(0,tRac);
  go_home();
}

void inc_kp(void)
{
  kp++;
  change++;
}

void inc_ki(void)
{
  ki++;
  change++;
}

void inc_kd(void)
{
  kd++;
  change++;
}

void dec_kp(void)
{
  kp--;
  change--;
}

void dec_ki(void)
{
  ki--;
  change--;
}

void dec_kd(void)
{
  kd--;
  change--;
}

void inc_temp(void)
{
    temp++;
    change++;
}

void dec_temp(void)
{
  temp--;
  change--;
}

void cancel_Temp(void)
{
  temp -= change;
  go_home();
}

void cancel_KP(void)
{
  kp -= change;
  go_home();
}

void cancel_KI(void)
{
  ki -= change;
  go_home();
}

void cancel_KD(void)
{
  kd -= change;
  go_home();
}

void cancel_tIncal(void)
{
  tIncal -= change;
  go_home();
}

void inc_tIncal(void)
{
  tIncal++;
  change++;
}

void dec_tIncal(void)
{
  tIncal--;
  change--;
}

void cancel_tMen(void)
{
  tMen -= change;
  go_home();
}

void inc_tMen(void)
{
  tMen++;
  change++;
}

void dec_tMen(void)
{
  tMen--;
  change--;
}

void cancel_tRac(void)
{
  tRac -= change;
  go_home();
}

void inc_tRac(void)
{
  tRac++;
  change++;
}

void dec_tRac(void)
{
  tRac--;
  change--;
}

void continua(void)
{
  lcd.setCursor(0, 1);
  lcd.print("Asteapta...");
}

state_machine_handler_t* sm[MENU_MAX_NUM][EV_MAX_NUM] = 
{ //events: OK , CANCEL , NEXT, PREV
  {enter_menu, go_home, go_next, go_prev},    // MENU_MAIN
  {continua, go_home, continua, continua},      // MENU_START
  {save_kp, cancel_KP, inc_kp, dec_kp},         // MENU_KP
  {save_ki, cancel_KI, inc_ki, dec_ki},         // MENU_Ki
  {save_kd, cancel_KD, inc_kd, dec_kd},         // MENU_Kd
  {save_temp, cancel_Temp, inc_temp, dec_temp}, // MENU_TEMP
  {save_tIncal, cancel_tIncal, inc_tIncal, dec_tIncal},// MENU_TINCAL
  {save_tMen, cancel_tMen, inc_tMen, dec_tMen},        // MENU_TMEN
  {save_tRac, cancel_tRac, inc_tRac, dec_tRac}         // MENU_TRAC
};

void state_machine(enum Menus menu, enum Buttons button)
{
  sm[menu][button]();
}

Buttons GetButtons(void)
{
  enum Buttons ret_val = EV_NONE;
  if (digitalRead(BUTTON_OK))
  {
    ret_val = EV_OK;
  }
  else if (digitalRead(BUTTON_CANCEL))
  {
    ret_val = EV_CANCEL;
  }
  else if (digitalRead(BUTTON_NEXT))
  {
    ret_val = EV_NEXT;
  }
  else if (digitalRead(BUTTON_PREV))
  {
    ret_val = EV_PREV;
  }
  Serial.print(ret_val);
  return ret_val;
}

void setup()
{
  Serial.begin(9600);
  lcd.begin(16,2);
  pinMode(7, INPUT);
  digitalWrite(7, LOW); // down
    pinMode(13, INPUT);
  digitalWrite(13, LOW); // down
    pinMode(2, INPUT);
  digitalWrite(2, LOW); // down
   pinMode(9, INPUT);
  digitalWrite(9, LOW); // down
  
  pinMode(10,OUTPUT);
  
  TCCR1A = 0; // Timer/Counter1 Control Register A
  TCCR1B = 0; // Timer/Counter1 Control Register B
  OCR1A = 65535/4; // Output compare register
  TIMSK1 |= (1 << OCIE1A); //Timer/Counter1 Interrupt Mask Register
  TCCR1B |= (1 << WGM12) | (1 << CS10)|(1 << CS12);//prescaler 1024 
}

ISR(TIMER1_COMPA_vect) //numaratorul pentru ceas si secunde
{
  sec++;
  if( sec == 59 )
    {
      sec = 0;
      min++;
    }
  
    if( min == 59 )
    {
      min = 0;
      ora++;
    }
}

//Citeste Temperatura
int get_temperature(int pin) 
{
  int temperature = analogRead(A0);
  float voltage = temperature * 5.0;
  voltage = voltage / 1024.0;
  return ((voltage - 0.5) * 100);
}

void PID()
{   
  eroare = moving_sp - temperatura_citita_senzor;
    
  suma_erori= suma_erori + eroare *dt;
    
  derivativa = (eroare - eroare_anterioara) / dt;
  
  output = (kp * eroare) + (ki * suma_erori ) + (kd * derivativa);
    
  eroare_anterioara = eroare;
  analogWrite(BEC_pin, int(output));
}
//Afisare timp ramas si calcul set point temporar
void afisare_timp()
{
  int min1 =0; int sec1=0;
    int remaining = 0;
    timp_inc=tIncal;
    timp_men=tMen;
    timp_rac=tRac;
    lcd.setCursor(0,0);
    lcd.print("P:");
    lcd.print(moving_sp);
    uptime++;
  
    if(uptime<= timp_inc)
    {
      lcd.setCursor(0,1);
      lcd.print("TInc:");
      remaining = timp_inc - uptime;
      moving_sp = temperatura_citita_senzor + (temp - temperatura_citita_senzor) * (timp_inc - remaining)/timp_inc;
    }
    else if(uptime <= (timp_inc + timp_men))
    {
      lcd.setCursor(0,1);
      lcd.print("Tmen:");
      remaining = (timp_inc +timp_men) - uptime;
    }
    else if( uptime <= (timp_inc + timp_men + timp_rac))
    {
      lcd.setCursor(0,1);
      lcd.print("TRac: ");
      remaining = (timp_inc + timp_men + timp_rac) - uptime;
      moving_sp = temp - (temp - temperatura_citita_senzor) * (timp_rac - remaining)/timp_rac;
    }
    else
    {
      lcd.setCursor(0,1);
      lcd.print("Oprit: ");
    }
  
    min1 = remaining / 60;
    sec1 = remaining % 60;
    lcd.print(min1);lcd.print(":");lcd.print(sec1);
    PID();
}

void loop()
{
  temperatura_citita_senzor = get_temperature(A0);
  
  volatile Buttons event = GetButtons();
  if (event != EV_NONE)
  {
    state_machine(current_menu, event);
  }
    print_menu(scroll_menu);
    delay(1000);
}
