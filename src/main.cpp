#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "printf.h"
#include "RF24.h"
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define CE_PIN 34
#define CSN_PIN 21 // 21*****************************
#define IRQ_PIN 16
#define moteur_joystick_pin 6
#define direction_joystick_pin 5 // 5*****************************
#define camera_joystick_pin 10   // 10*****************************

float dutycylce_moteur = 0.0;
float dutycylce_direction = 0.0;
float dutycylce_camera = 0.0;

float new_data[5]; // 6 donnees, 3 ID, 3 dutycycle.
uint8_t i = 0;

RF24 radio(CE_PIN, CSN_PIN);
Adafruit_INA219 ina219;
Adafruit_ST7735 tft = Adafruit_ST7735(11, 7, 35, 36, 12); // TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST

// variable pour le INA219
float busvoltage = 0;
int etat_batterie_manette = 0;
int etat_batterie_manette_previous = 0;
//-----------------------------------------------------------------------

// vairable pour le NRF24L01
union payload
{
  float number[8];
  uint8_t *data[32];
};
payload myPayload;

uint8_t address[][6] = {"00001", "00002"};

float lecture_Joystick_moteur = 0.0;
float lecture_Joystick_direction = 0.0;
float lecture_Joystick_camera = 0.0;
int etat_batterie_vehicule = 0;
int etat_batterie_vehicule_previous = 0;
float voltage_batterie_vehicule = 0.0;
uint8_t bytes = 4;
bool report = true;
float data_receive = 0.0;
int current_time = 0;
int previous_time = 0;
bool premier_init_done = false;
//-----------------------------------------------------------------------

// variable pour l'ecran

// color definitions
const uint16_t Color_Black = 0x0000;
const uint16_t Color_Yellow = 0xFFE0;

//-----------------------------------------------------------------------

void check_send_status(bool status, float data)
{

  if (status == 1)
  { // ce IF peut etre enlever
    Serial.println(F("Sent"));
    Serial.println(data);
  }
  else
  {
    Serial.println(F("Failed")); // payload was not delivered
  }
}

void clean_screen_data_posX_100(int postionY)
{

  tft.setTextColor(Color_Yellow);
  tft.setCursor(100, postionY);
  tft.print("888");
  tft.setCursor(100, postionY);
  tft.print("WWW");
  tft.setCursor(100, postionY);
  tft.print("ttt");
  tft.setCursor(100, postionY);
  tft.print("ggg");
  tft.setCursor(100, postionY);
  tft.print("BBB");
  tft.setCursor(100, postionY);
  tft.print("www");
  tft.setCursor(100, postionY);
  tft.print("kkk");
  tft.setCursor(100, postionY);
  tft.print("sss");
  tft.setCursor(100, postionY);
  tft.print("MMM");
  tft.setTextColor(Color_Black);
}

void clean_screen_data_posX_105(int position_y)
{

  tft.setTextColor(Color_Yellow);
  tft.setCursor(105, position_y);
  tft.print("888");
  tft.setCursor(105, position_y);
  tft.print("WWW");
  tft.setCursor(105, position_y);
  tft.print("ttt");
  tft.setCursor(105, position_y);
  tft.print("ggg");
  tft.setCursor(105, position_y);
  tft.print("BBB");
  tft.setCursor(105, position_y);
  tft.print("www");
  tft.setCursor(105, position_y);
  tft.print("kkk");
  tft.setCursor(105, position_y);
  tft.print("sss");
  tft.setCursor(105, position_y);
  tft.print("MMM");
  tft.setTextColor(Color_Black);
}

void data_ecran()
{

  clean_screen_data_posX_100(2);
  tft.setCursor(100, 2);
  tft.print(etat_batterie_manette);
  tft.setCursor(120, 2);
  tft.print("%");

  // afficher etat de la batterie vehicule
  clean_screen_data_posX_100(22);
  tft.setCursor(100, 22);
  tft.print(etat_batterie_vehicule);
  tft.setCursor(120, 22);
  tft.print("%");

  if (etat_batterie_manette <= 5)
  {

    tft.setCursor(3, 62);
    tft.print("Danger");
    tft.setCursor(3, 82);
    tft.print("Recharger Bat. Man");
  }
  else
  {
    tft.fillRect(3, 62, 120, 40, Color_Yellow);
  }

  if (etat_batterie_vehicule <= 5)
  {

    tft.setCursor(3, 102);
    tft.print("Danger");
    tft.setCursor(3, 122);
    tft.print("Recharger Bat. Veh");
  }
  else
  {
    tft.fillRect(3, 102, 120, 40, Color_Yellow);
  }
}

void demarrage_ecran()
{

  if (premier_init_done == false)
  {
    tft.initR(INITR_BLACKTAB);
    tft.enableDisplay(true);
    tft.setFont();
    tft.fillScreen(Color_Yellow);
    tft.setTextColor(Color_Black);
    tft.setTextSize(1, 2);

    tft.setCursor(3, 2);
    tft.print("Batterie Man. = ");
    tft.setCursor(100, 2);
    tft.print("NA");
    tft.setCursor(120, 2);
    tft.print("%");

    tft.setCursor(3, 22);
    tft.print("Batterie Veh. = ");
    tft.setCursor(100, 22);
    tft.print("NA");
    tft.setCursor(120, 22);
    tft.print("%");

    premier_init_done = true;
  }
  else
  {
    tft.initR(INITR_BLACKTAB);
  }
}

float Receiver()
{

  radio.read(&myPayload, bytes);
  data_receive = myPayload.number[0];
  return data_receive;
}

void Transmiter(float data_to_send)
{

  report = radio.write(&data_to_send, sizeof(float));
  check_send_status(report, data_to_send);
}

void setup()
{
  Serial.begin(9600);
  Wire.begin();

  if (!ina219.begin())
  {
    Serial.println("Failed to find INA219 chip");
    /* while (1)
     {
       delay(10);
     }*/
  }

  SPI.begin();
  if (!radio.begin())
  {
    Serial.println(F("radio hardware is not responding!!"));
    while (1)
    {
    } // hold in infinite loop
  }

  Serial.println("The radio is responding.");
  radio.printDetails();
  radio.setAddressWidth(3);
  radio.setChannel(10);
  radio.setDataRate(RF24_250KBPS);
  radio.enableAckPayload();
  radio.enableDynamicPayloads();
  radio.setCRCLength(RF24_CRC_8);
  radio.setPALevel(RF24_PA_MAX); // si la modification ne fonctionne pas, remettre >>> radio.setPALevel(RF24_PA_MIN);

  radio.openWritingPipe(address[0]);
  radio.openReadingPipe(1, address[1]);

  radio.setAutoAck(true); // a mettre a la toute fin du set up

  radio.stopListening();
  SPI.end();

  ina219.setCalibration_16V_400mA();

  SPI.begin();
  demarrage_ecran();
  SPI.end();

  SPI.begin();
}

void loop()
{
  current_time = millis();

  if ((current_time - previous_time) > 400)
  {

    radio.stopListening();

    lecture_Joystick_moteur = analogRead(moteur_joystick_pin);
    Serial.println("lecture_Joystick_moteur");
    Serial.println(lecture_Joystick_moteur);
    if ((lecture_Joystick_moteur >= 4200) || (lecture_Joystick_moteur <= 3900))
    { // Servo 20K

      dutycylce_moteur = (-25.0 / 4095) * lecture_Joystick_moteur + 60; // equation servo 20Kg
      if (dutycylce_moteur > 46)
      { // 60
        dutycylce_moteur = 46.0;
      }
      if (dutycylce_moteur < 29)
      { // 10
        dutycylce_moteur = 29.0;
      }
    }
    else
    {
      dutycylce_moteur = 35.0; // valeur milieu servo 20Kg
    }
    new_data[0] = 9500.0;
    new_data[1] = dutycylce_moteur;
    Serial.println(new_data[1]);

    lecture_Joystick_direction = analogRead(direction_joystick_pin); //------------------------------------
    // Serial.println(lecture_Joystick_direction);

    if ((lecture_Joystick_direction >= 4200) || (lecture_Joystick_direction <= 3900))
    { // Servo direction
      if (lecture_Joystick_direction >= 4200)
      { // tourner a droite

        dutycylce_direction = (-20.0 / 4095) * lecture_Joystick_direction + 50; // equation servo cam et dir
        if (dutycylce_direction < 25.0)
        {
          dutycylce_direction = 25.0;
        }
      }
      else if (lecture_Joystick_direction <= 3900)
      { // tourner a gauche

        dutycylce_direction = (-25.0 / 4096) * lecture_Joystick_direction + 55; // equation servo cam et dir
        if (dutycylce_direction > 38.0)
        {
          dutycylce_direction = 38.0;
        }
      }
    }
    else
    {
      dutycylce_direction = 30.0; // 30.0
    }
    new_data[2] = 9400.0;
    new_data[3] = dutycylce_direction;

    lecture_Joystick_camera = analogRead(camera_joystick_pin); //-------------------------------------
    // Serial.println("lecture_Joystick camera");

    if ((lecture_Joystick_camera >= 4200) || (lecture_Joystick_camera <= 3900))
    { // Servo direction
      if (lecture_Joystick_camera >= 4200)
      { // tourner a droite

        dutycylce_camera = (-20.0 / 4095) * lecture_Joystick_camera + 50; // equation servo cam et dir
        if (dutycylce_camera < 10.0)
        {
          dutycylce_camera = 10.0;
        }
      }
      else if (lecture_Joystick_camera <= 3900)
      { // tourner a gauche

        dutycylce_camera = (-25.0 / 4096) * lecture_Joystick_camera + 55; // equation servo cam et dir
      }
    }
    else
    {
      dutycylce_camera = 30.0;
    }
    new_data[4] = 9300.0;
    new_data[5] = dutycylce_camera;
    // Serial.println("Camera");

    for (i = 0; i < 6; i++)
    {

      Transmiter(new_data[i]);
      Serial.print("i = ");
      Serial.println(i);
      Serial.println(new_data[i]);
    }
    radio.startListening();
    i = 0;
    previous_time = current_time;
  }

  if (radio.available())
  {

    // le reste du temps la manette attend des donnees Receiver
    voltage_batterie_vehicule = Receiver();
    SPI.end();

    // Calculer l'etat de decharge de la batterie
    busvoltage = ina219.getBusVoltage_V();

    etat_batterie_manette = ((busvoltage - 3.4) / 0.8) * 100;
    if (etat_batterie_manette < 0)
    {
      etat_batterie_manette = 0;
    }
    else if (etat_batterie_manette > 100)
    {
      etat_batterie_manette = 100;
    }

    etat_batterie_vehicule = ((voltage_batterie_vehicule - 7.5) / 0.9) * 100;

    if (etat_batterie_vehicule < 0)
    {
      etat_batterie_vehicule = 0;
    }
    else if (etat_batterie_vehicule > 100)
    {
      etat_batterie_vehicule = 100;
    }

    if ((etat_batterie_manette != etat_batterie_manette_previous) || (etat_batterie_vehicule != etat_batterie_vehicule_previous))
    {

      etat_batterie_manette_previous = etat_batterie_manette;
      etat_batterie_vehicule_previous = etat_batterie_vehicule;

      SPI.begin();
      demarrage_ecran();
      data_ecran();
      SPI.end();
    }

    SPI.begin();
  }
}