#define FCPU 16000000
#define FOSC 16000000 // Clock Speed
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD-1

#define MSG_MAX_LEN 32
#define FLOOD_COUNT_DETECTION_CYCLES 100

#include <avr/io.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

int volatile temp_q = 0;
int volatile time_count = 0;
const int rs = 12, en = 11, d4 = 4, d5 = 3, d6 = 8, d7 = 5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int pwm_val[3][2];
bool pwm_parsing = false;
int pwm_pos = 0;

char msg_val[MSG_MAX_LEN + 1];
bool msg_parsing = false;
int msg_pos = 0;
volatile bool msg_rec_completed = false;
volatile bool refresh_needed = false;

static char row_up[17] = "Spinean PS 2";
static char row_down[17] = "Semestrul II";

volatile bool flood_detected = false;
volatile int flood_count = 0;


typedef struct mesaje_nvm {
  char mesaj[MSG_MAX_LEN + 1];
  bool stare_citire;
  bool valid;  
} mesaje_nvm_t;

mesaje_nvm_t mesaje_ram[10];

void Read_EEPROM_Mesaje ()
{
  EEPROM.get(0, mesaje_ram);
}

void Write_EEPROM_Mesaje()
{
  EEPROM.put(0, mesaje_ram);
}

void Clear_EEPROM_Mesaje()
{
   for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
}

void Salveaza_Mesaj(char* msg, int len)
{
 int i=0;
 for(i = 0; i<10; i++)
 {
  if(mesaje_ram[i].valid != true)
  {
    break; 
  }
 }

 if (i==10)
 {
  i=0;
 }

 //Am gasit un loc pentru salvarea mesajului.
 memcpy(mesaje_ram[i].mesaj, msg, MSG_MAX_LEN + 1);
 mesaje_ram[i].valid = true;
 mesaje_ram[i].stare_citire = false;
 Write_EEPROM_Mesaje();
 Afiseaza_Mesaje();
}

void Afiseaza_Mesaje()
{
  char buf[MSG_MAX_LEN + 1];
  for ( int j=0; j < 10; j++)
  {
    memset(buf, 0, sizeof(buf));
    if (mesaje_ram[j].valid == true)
    {
      snprintf(buf, MSG_MAX_LEN + 1, "%d - %s\n", j, mesaje_ram[j].mesaj);
    }
    else
    {
      snprintf(buf, MSG_MAX_LEN + 1, "%d - gol \n", j);
    }
    for (int i = 0; i < strlen(buf); i++)
    {
      USART_Transmit(buf[i]);
    }
  }
}

void setup()
{

}
void loop()
{}

int main()
{
  DDRB |= 1 << PB5;
  DDRB |= 1 << PB1;
  DDRB |= 1 << PB3;

  OCR2A = 65535; // 100 ms
  TCCR2A |= 1 << WGM21;
  TCCR2B |= (1 << CS22) | (1 << CS20) | (1 << CS21);
  TIMSK2 = (1 << OCIE2A);

  Disp_Init();
  PWM_Init();
  USART_Init(MYUBRR);
  ADC_Init();
  EINT_Init();

  //Clear_EEPROM_Mesaje();
  Read_EEPROM_Mesaje();
  Afiseaza_Mesaje();
  sei();
  while (1)
  {

    if (true == flood_detected)
    {
      char buf[] = "!INUNDATIE!\n";
      for (int i = 0; i < strlen(buf); i++)
      {
        USART_Transmit(buf[i]);
      }
      flood_detected = false;
    }

    else if (true == msg_rec_completed)
    {
      memset(row_up, 0, sizeof(row_up));
      memset(row_down, 0, sizeof(row_down));
      memcpy(row_up, &msg_val[0], 16);
      memcpy(row_down, &msg_val[16], 16);

      msg_rec_completed = false;
      Salveaza_Mesaj(msg_val, sizeof(msg_val));
      memset(msg_val, 0, sizeof(msg_val));

      Debug_Display_Rows();
    }
    if (true == refresh_needed)
    {
      Refresh_Display();
    }

  }
}

void EINT_Init(void)
{
  PORTD |= (1 << PORTD2); // Rezistor de tip pull-up activat
  EICRA = 0;
  EIMSK |= 1 << INT0;
}

void Disp_Init(void)
{
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Sincretic 2021");
}

void Disp_Char(char c)
{
  PORTD |= 1 << PD3 | 1 << PD2 | 1 << PD4 | 1 << PD5  | 1 << PD7;
  PORTB |= 1 << PB0 | 1 << PB2;

  if (c == 'P')
  {
    PORTD &= ~(1 << PD3 | 1 << PD4 | 1 << PD2 | 1 << PD7);
    PORTB &= ~(1 << PB0 | 1 << PB2);
  }
  else if (c == 'N')
  {
    PORTD &= ~(1 << PD4 | 1 << PD7);
    PORTB &= ~(1 << PB2);
  }
}


void PWM_Init(void)
{
  // 1. LED ca iesire.
  DDRD |= 1 << PD6;
  DDRB |= 1 << PB1;
  DDRB |= 1 << PB2;

  // 2. Alegem Timer PWM CS
  TCCR0B = (1 << CS02);
  TCCR1B = (1 << CS12);

  // 3. Mod FAST PWM
  TCCR0A |= (1 << WGM00) | (1 << WGM01);
  TCCR1A |= 1 << WGM10;
  TCCR1B |= 1 << WGM12;

  // 4. Setam OCR output mode
  TCCR0A |= (1 << COM0A1);
  TCCR1A |= (1 << COM1A1);
  TCCR1A |= (1 << COM1B1);


  // 5. Test valori OCR   // P 22 44 98 W
  OCR0A = 0;    // Green
  OCR1A = 0;    // Blue
  OCR1B = 0;    // Red
}

void ADC_Init(void)
{
  ADCSRA = 1 << ADEN | 1 << ADIE | 1 << ADSC | 1 << ADPS1 | 1 << ADPS2 | 1 << ADPS0;
  ADMUX = 1 << REFS0;
}

ISR(ADC_vect)
{
  while (ADCSRA & (1 << ADSC));
  temp_q = ADC;

  // Logica procesare temperaturii citite. 35C
  if (temp_q > 173)
  {
    PORTB |= 1 << PB3;
  }
  else
  {
    PORTB &= ~(1 << PB3);
  }
  ADMUX = 1 << REFS0;

  ADCSRA |= 1 << ADSC;
}

ISR(USART_RX_vect)
{
  char c = USART_Receive();
  if ((c == 'A') && !msg_parsing)
  {
    PORTB |= 1 << PB5;
  }
  else if ((c == 'S') && !msg_parsing)
  {
    PORTB &= ~(1 << PB5);
  }
  else if ((c == 'P') && !msg_parsing)
  {
    // incepem parsare PWM
    pwm_parsing = true;
    pwm_pos = 0;
  }
  else if ((c == 'W') && !msg_parsing)
  {
    // incheiem parsarea PWM
    pwm_parsing = false;

    OCR1B = pwm_val[0][0] * 10 + pwm_val[0][1];    // Red
    OCR0A = pwm_val[1][0] * 10 + pwm_val[1][1];    // Green
    OCR1A = pwm_val[2][0] * 10 + pwm_val[2][1];    // Blue
  }
  else if (c == '#')
  {
    msg_parsing = true;
    msg_pos = 0;
  }
  else if (c == '^')
  {
    msg_parsing = false;
    msg_rec_completed = true;
  }

  else
  {
    if (pwm_parsing == true)
    {
      // salvam valorile pwm...
      pwm_val[pwm_pos / 2][pwm_pos % 2] = c - '0';
      pwm_pos++;
    }
    if (msg_parsing == true)
    {
      if (('\n' != c) && (msg_pos < MSG_MAX_LEN))
      {
        msg_val[msg_pos++] = c;
      }
    }
  }
}

ISR(TIMER2_COMPA_vect)
{
  time_count++;
  if (!(time_count % 100))
  {
    refresh_needed = true;
  }

  if (!(time_count % 50))
  {
    Transmitere_Temperatura();
  }
}

ISR(INT0_vect)
{
  flood_count++;
  if (flood_count > FLOOD_COUNT_DETECTION_CYCLES)
  {
    flood_detected = true;
    flood_count = 0;
    // Avem cotinuintate intre PD2 si GND, posibil eveniment inundatie
  }
}

void USART_Init(unsigned int ubrr)
{
  /* Set baud rate */
  UBRR0H = (unsigned char)(ubrr >> 8);
  UBRR0L = (unsigned char)ubrr;
  /* Enable receiver and transmitter */
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
  /* Set frame format: 8data, 2stop bit */
  UCSR0C = (1 << USBS0) | (3 << UCSZ00);
}

unsigned char USART_Receive(void)
{
  /* Wait for data to be received */
  while (!(UCSR0A & (1 << RXC0)))
    ;
  /* Get and return received data from buffer */
  return UDR0;
}

void USART_Transmit(unsigned char data)
{
  /* Wait for empty transmit buffer */
  while (!(UCSR0A & (1 << UDRE0)))
    ;
  /* Put data into buffer, sends the data */
  UDR0 = data;
}

void Transmitere_Temperatura()
{
  float q = 5000.0 / 1023;
  float temp_u = temp_q * q;
  int temp_final = (temp_u / 10);
  int temp_zecimale = 0;
  temp_zecimale = temp_u - temp_final * 10;

  char buf[50];
  memset(buf, 0, sizeof(buf));
  sprintf(buf, "T=%d.%d\n", temp_final, temp_zecimale);
  

  for (int i = 0; i < strlen(buf); i++)
  {
    USART_Transmit(buf[i]);
  }
}

void Debug_Display_Rows()
{
  for (int i = 0; i < 16; i++)
  {
    USART_Transmit(row_up[i]);
  }
  USART_Transmit('\n');

  for (int i = 0; i < 16; i++)
  {
    USART_Transmit(row_down[i]);
  }

  USART_Transmit('\n');
  USART_Transmit('\n');
}

void Refresh_Display()
{
  cli();
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(row_up);
  lcd.setCursor(0, 1);
  lcd.print(row_down);
  refresh_needed = false;
  sei();
}
