#ifdef USES_P134

/*
//#######################################################################################################
//#################################### Plugin 134: PPD42NJ/NS ###########################################
//#################################### by dony71 ########################################################
//#######################################################################################################
//https://diyprojects.io/calculate-air-quality-index-iaq-iqa-dsm501-arduino-esp8266/#.Wwc8j6Qvypp
//https://forum.mysensors.org/topic/147/air-quality-sensor/216
*/

#include <SimpleTimer.h>

#define PLUGIN_134
#define PLUGIN_ID_134 134
#define PLUGIN_NAME_134 "Dust sensor - PPD42NJ/NS [TESTING]"
#define PLUGIN_VALUENAME1_134 "PM10_ug_per_m3" // from the datasheet the detection is from PM1 and up. You could have from PM1 to PM2.5, on subtracting PM2.5 value on PM1 value. This value come from the pin #4
#define PLUGIN_VALUENAME2_134 "PM25_ug_per_m3"
#define PLUGIN_VALUENAME3_134 "PM10_mili_ppmv" // from the datasheet the detection is from PM2.5 and up. This value come from the pin #2. With different resistor topn the pin #1, you could adjust the size threshold detection
#define PLUGIN_VALUENAME4_134 "PM25_mili_ppmv"
#define PLUGIN_VALUENAME5_134 "AQI_PM10" // Air Quality Index Level
#define PLUGIN_VALUENAME6_134 "AQI_PM25" // Air Quality Index Level
#define PLUGIN_VALUENAME7_134 "AQI_PM10_Status" // Air Quality Index Level
#define PLUGIN_VALUENAME8_134 "AQI_PM25_Status" // Air Quality Index Level

//#define DUST_SENSOR_DIGITAL_PIN_PM10  D6        // PPD42 Pin 2 of PPD42 (red)
//#define DUST_SENSOR_DIGITAL_PIN_PM25  D7        // PPD42 Pin 4 (yellow)
//#define COUNTRY                       2         // 0. France, 1. Europe, 2. USA/China
#define FRANCE                        0
#define EUROPE                        1
#define USA_CHINA                     2

#define EXCELLENT                     1 //"Excellent"
#define GOOD                          2 //"Bon"
#define ACCEPTABLE                    3 //"Moyen"
#define MODERATE                      4 //"Mediocre"
#define HEAVY                         5 //"Mauvais"
#define SEVERE                        6 //"Tres mauvais"
#define HAZARDOUS                     7 //"Dangereux"

int             DUST_SENSOR_DIGITAL_PIN_PM10;  // PPD42 Pin 2 of PPD42 (red)
int             DUST_SENSOR_DIGITAL_PIN_PM25;  // PPD42 Pin 4 (yellow)
unsigned long   duration;
unsigned long   starttime;
unsigned long   endtime;
unsigned long   lowpulseoccupancy = 0;
float           concentration = 0;
float           ratio = 0;
unsigned long   SLEEP_TIME    = 2 * 1000;       // Sleep time between reads (in milliseconds)
unsigned long   sampletime_ms = 5 * 60 * 1000;  // Durée de mesure - sample time (ms)

//ppmv = mg/m3 * (0.08205*Temp)/Molecular_mass
//mg/m3          = milligrams of pollutant per cubic meter of air at sea level atmospheric pressure and Temp
//Temp           = external temperature in Kelvin (Kelvin = Celsius + 273.15), read from temperature sensor if available
//0.08205        = Universal gas constant in atm·m3/(kmol·K)
//Molecular_mass = average molecular mass of dry air : 28.97 g/mol

struct structAQI {
  // variable enregistreur - recorder variables
  //unsigned long   durationPM10;
  unsigned long   lowpulseoccupancyPM10 = 0;
  //unsigned long   durationPM25;
  unsigned long   lowpulseoccupancyPM25 = 0;
  unsigned long   starttime;
  unsigned long   endtime;
  // Sensor AQI data
  float         concentrationPM25  = 0;
  float         concentrationPM10  = 0;
  int           AqiPM10            = 0;
  int           AqiPM25            = 0;
  float         ppmvPM10           = 0;
  float         ppmvPM25           = 0;
  // Indicateurs AQI - AQI display
  //String        AqiString          = "";
  int           AqiStringPM10          = 0;
  int           AqiStringPM25          = 0;
  //int           AqiColor           = 0;
  // Country Selection for AQI Level
  byte          COUNTRY            = 2;   // default USA
  int           enableTEMP         = 0;
  float         temperature        = 0;
};
struct structAQI AQI;

SimpleTimer timer;


////////////////////////////////////////////////////////////////////////////
boolean Plugin_134(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_134;
        Device[deviceCount].Type = DEVICE_TYPE_DUAL;
//        Device[deviceCount].VType = SENSOR_TYPE_QUAD;
//        Device[deviceCount].VType = SENSOR_TYPE_HEXA;
        Device[deviceCount].VType = SENSOR_TYPE_OCTA;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 8;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_134);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_134));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_134));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_134));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_134));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[4], PSTR(PLUGIN_VALUENAME5_134));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[5], PSTR(PLUGIN_VALUENAME6_134));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[6], PSTR(PLUGIN_VALUENAME7_134));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[7], PSTR(PLUGIN_VALUENAME8_134));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        addFormSeparator(2);
        // mode
        addFormCheckBox(F("Enable ppmv measurement"), F("plugin_134_enable_compensation"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        addFormNote(F("If this is enabled, the Temperature values below need to be configured."));
        // temperature
        addHtml(F("<TR><TD>Temperature:<TD>"));
        addTaskSelect(F("plugin_134_temperature_task"), Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
        LoadTaskSettings(Settings.TaskDevicePluginConfig[event->TaskIndex][1]); // we need to load the values from another task for selection!
        addHtml(F("<TR><TD>Temperature Value:<TD>"));
        addTaskValueSelect(F("plugin_134_temperature_value"), Settings.TaskDevicePluginConfig[event->TaskIndex][2], Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
        addFormSeparator(2);
        // country
        String options[3];
        options[0] = F("France");
        options[1] = F("Europe");
        options[2] = F("USA/China");
        int optionValues[3] = { FRANCE, EUROPE, USA_CHINA };
        byte countryType = Settings.TaskDevicePluginConfig[event->TaskIndex][3];
        addFormSelector(F("AQI Level Country"), F("plugin_134_country_type"), 3, options, optionValues, countryType);

        LoadTaskSettings(event->TaskIndex); // we need to restore our original taskvalues!
        
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = isFormItemChecked(F("plugin_134_enable_compensation") );
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_134_temperature_task"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = getFormItemInt(F("plugin_134_temperature_value"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][3] = getFormItemInt(F("plugin_134_country_type"));

        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        String log = F("INIT : PPD42NJ/NS ");
        DUST_SENSOR_DIGITAL_PIN_PM10 = Settings.TaskDevicePin1[event->TaskIndex];
        pinMode(DUST_SENSOR_DIGITAL_PIN_PM10, INPUT);
        log = F("PPD42NJ/NS: Controller GPIO PM1.0: ");
        log += DUST_SENSOR_DIGITAL_PIN_PM10;
        addLog(LOG_LEVEL_INFO, log);
        DUST_SENSOR_DIGITAL_PIN_PM25 = Settings.TaskDevicePin2[event->TaskIndex];
        pinMode(DUST_SENSOR_DIGITAL_PIN_PM25, INPUT);
        log = F("PPD42NJ/NS: Controller GPIO PM2.5: ");
        log += DUST_SENSOR_DIGITAL_PIN_PM25;
        addLog(LOG_LEVEL_INFO, log);

        // wait 60s for PPD42 to warm up
        //log = F("(wait 60s for PPD42NJ/NS to warm up)");
        log = F("(wait 10s for PPD42NJ/NS to warm up)");
        addLog(LOG_LEVEL_INFO, log);
        //for (int i = 1; i <= 60; i++)
        for (int i = 1; i <= 10; i++)
          delay(1000); // 1s
        // set measuring times interval
        log = F("Ready!");
        addLog(LOG_LEVEL_INFO, log);

        AQI.enableTEMP = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        AQI.COUNTRY = Settings.TaskDevicePluginConfig[event->TaskIndex][3];

        AQI.starttime = millis();
        timer.setInterval(sampletime_ms, updateAQI);

        success = true;
        break;
      }

    case PLUGIN_READ:
      {
        String log = F("READ : PPD42NJ/NS ");
        // Actualise les mesures - update measurements
        getPM(DUST_SENSOR_DIGITAL_PIN_PM10, DUST_SENSOR_DIGITAL_PIN_PM25);

        // Use temperature in ppmv calculation if enable
        if (AQI.enableTEMP) {
          // we're checking a var from another task, so calculate that basevar
          byte TaskIndex1    = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
          byte BaseVarIndex1 = TaskIndex1 * VARS_PER_TASK + Settings.TaskDevicePluginConfig[event->TaskIndex][2];
          //float temperature = UserVar[BaseVarIndex1]; // in degrees C
          AQI.temperature = UserVar[BaseVarIndex1] + 273.15; // in Kelvin

          //AQI.ppmvPM10 = concentrationPM10_mgm3(AQI.concentrationPM10) * ((0.08205*AQI.temperature)/28.97);
          AQI.ppmvPM10 = AQI.concentrationPM10 * ((0.08205*AQI.temperature)/28.97);
          //AQI.ppmvPM25 = concentrationPM25_mgm3(AQI.concentrationPM25) * ((0.08205*AQI.temperature)/28.97);
          AQI.ppmvPM25 = AQI.concentrationPM25 * ((0.08205*AQI.temperature)/28.97);

          UserVar[event->BaseVarIndex + 2] = AQI.ppmvPM10;
          UserVar[event->BaseVarIndex + 3] = AQI.ppmvPM25;
        } else {
          UserVar[event->BaseVarIndex + 2] = NAN;
          UserVar[event->BaseVarIndex + 3] = NAN;
        }

        UserVar[event->BaseVarIndex + 0] = AQI.concentrationPM10;
        log = F("PPD42NJ/NS: Concentration PM1.0 (ug/m3) : ");
        log += UserVar[event->BaseVarIndex + 0];
        addLog(LOG_LEVEL_INFO, log);
        log = F("PPD42NJ/NS: Concentration PM1.0 (mili ppmv) : ");
        log += UserVar[event->BaseVarIndex + 2];
        addLog(LOG_LEVEL_INFO, log);
        UserVar[event->BaseVarIndex + 4] = AQI.AqiPM10;
        log = F("PPD42NJ/NS: Air Quality PM1.0 : ");
        log += UserVar[event->BaseVarIndex + 4];
        addLog(LOG_LEVEL_INFO, log);
        UserVar[event->BaseVarIndex + 6] = AQI.AqiStringPM10;
        log = F("PPD42NJ/NS: AQI PM1.0 Status : ");
        log += UserVar[event->BaseVarIndex + 6];
//        log += AQI.AqiStringPM10;
        addLog(LOG_LEVEL_INFO, log);
        UserVar[event->BaseVarIndex + 1] = AQI.concentrationPM25;
        log = F("PPD42NJ/NS: Concentration PM2.5 (ug/m3) : ");
        log += UserVar[event->BaseVarIndex + 1];
        addLog(LOG_LEVEL_INFO, log);
        log = F("PPD42NJ/NS: Concentration PM2.5 (mili ppmv) : ");
        log += UserVar[event->BaseVarIndex + 3];
        addLog(LOG_LEVEL_INFO, log);
        UserVar[event->BaseVarIndex + 5] = AQI.AqiPM25;
        log = F("PPD42NJ/NS: Air Quality PM2.5 : ");
        log += UserVar[event->BaseVarIndex + 5];
        addLog(LOG_LEVEL_INFO, log);
        UserVar[event->BaseVarIndex + 7] = AQI.AqiStringPM25;
        log = F("PPD42NJ/NS: AQI PM2.5 Status : ");
        log += UserVar[event->BaseVarIndex + 7];
//        log += AQI.AqiStringPM25;
        addLog(LOG_LEVEL_INFO, log);

        success = true;
        break;
      }
  }
  return success;
}


void updateAQI() {
  // Actualise les mesures - update measurements
  AQI.endtime = millis();
  float ratio = AQI.lowpulseoccupancyPM10 / (sampletime_ms * 10.0);
  float concentration = 1.1 * pow( ratio, 3) - 3.8 *pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve
  if ( sampletime_ms < 3600000 ) { concentration = concentration * ( sampletime_ms / 3600000.0 ); }
  AQI.lowpulseoccupancyPM10 = 0;
  AQI.concentrationPM10 = concentration;
  //AQI.ppmvPM10 = concentrationPM10_mgm3(AQI.concentrationPM10) * ((0.08205*AQI.temperature)/28.97);

  ratio = AQI.lowpulseoccupancyPM25 / (sampletime_ms * 10.0);
  concentration = 1.1 * pow( ratio, 3) - 3.8 *pow(ratio, 2) + 520 * ratio + 0.62;
  if ( sampletime_ms < 3600000 ) { concentration = concentration * ( sampletime_ms / 3600000.0 ); }
  AQI.lowpulseoccupancyPM25 = 0;
  AQI.concentrationPM25 = concentration;
  //AQI.ppmvPM25 = concentrationPM25_mgm3(AQI.concentrationPM25) * ((0.08205*AQI.temperature)/28.97);

  AQI.starttime = millis();

  // Actualise l'AQI de chaque capteur - update AQI for each sensor 
  getAQILevel();
  // Actualise l'indice AQI - update AQI index
  //updateAQILevel(AQI.AqiPM10, AQI.AqiPM25);
  AQI.AqiStringPM10 = updateAQIDisplay(AQI.AqiPM10);
  AQI.AqiStringPM25 = updateAQIDisplay(AQI.AqiPM25);
}

void getPM(int DUST_SENSOR_DIGITAL_PIN_PM10, int DUST_SENSOR_DIGITAL_PIN_PM25) {
  AQI.lowpulseoccupancyPM10 += pulseIn(DUST_SENSOR_DIGITAL_PIN_PM10, LOW);
  AQI.lowpulseoccupancyPM25 += pulseIn(DUST_SENSOR_DIGITAL_PIN_PM25, LOW);
  timer.run();
}

/*
void updateAQILevel(int aqipm10, int aqipm25) {
  AQI.AqiPM10 = updateAQIDisplay(aqipm10);
  AQI.AqiPM25 = updateAQIDisplay(aqipm25);
}
*/

void getAQILevel() {
/*
  if ( AQI.enableTEMP ) {
    if ( AQI.COUNTRY == 0 ) {
      // France
      AQI.AqiPM25 = getATMO( 0, AQI.ppmvPM25 );
      AQI.AqiPM10 = getATMO( 1, AQI.ppmvPM10 );
    } else if ( AQI.COUNTRY == 1 ) {
      // Europe
      AQI.AqiPM25 = getACQI( 0, AQI.ppmvPM25 );
      AQI.AqiPM10 = getACQI( 1, AQI.ppmvPM10 );
    } else {
      // USA / China
      AQI.AqiPM25 = getAQI( 0, AQI.ppmvPM25 );
      AQI.AqiPM10 = getAQI( 1, AQI.ppmvPM10 );
    }
  } else {
*/
    if ( AQI.COUNTRY == 0 ) {
      // France
      AQI.AqiPM25 = getATMO( 0, AQI.concentrationPM25 );
      AQI.AqiPM10 = getATMO( 1, AQI.concentrationPM10 );
    } else if ( AQI.COUNTRY == 1 ) {
      // Europe
      AQI.AqiPM25 = getACQI( 0, AQI.concentrationPM25 );
      AQI.AqiPM10 = getACQI( 1, AQI.concentrationPM10 );
    } else {
      // USA / China
      AQI.AqiPM25 = getAQI( 0, AQI.concentrationPM25 );
      AQI.AqiPM10 = getAQI( 1, AQI.concentrationPM10 );
    }
//  }
}

int updateAQIDisplay(int AQIpm) {
  // 1 EXCELLENT
  // 2 GOOD
  // 3 ACCEPTABLE
  // 4 MODERATE
  // 5 HEAVY
  // 6 SEVERE
  // 7 HAZARDOUS
  int AqiString;
  if ( AQI.COUNTRY == 0 ) {
    // Système ATMO français - French ATMO AQI system
    switch (AQIpm) {
      case 10:
        return AqiString = SEVERE;
        break;
      case 9:
        return AqiString = HEAVY;
        break;
      case 8:
        return AqiString = HEAVY;
        break;
      case 7:
        return AqiString = MODERATE;
        break;
      case 6:
        return AqiString = MODERATE;
        break;
      case 5:
        return AqiString = ACCEPTABLE;
        break;
      case 4:
        return AqiString = GOOD;
        break;
      case 3:
        return AqiString = GOOD;
        break;
      case 2:
        return AqiString = EXCELLENT;
        break;
      case 1:
        return AqiString = EXCELLENT;
        break;
      }
  } else if ( AQI.COUNTRY == 1 ) {
    // European CAQI
    switch (AQIpm) {
      case 25:
        return AqiString = GOOD;
        break;
      case 50:
        return AqiString = ACCEPTABLE;
        break;
      case 75:
        return AqiString = MODERATE;
        break;
      case 100:
        return AqiString = HEAVY;
        break;
      default:
        return AqiString = SEVERE;
      }
  } else if ( AQI.COUNTRY == 2 ) {
    // USA / CN
    if ( AQIpm <= 50 ) {
      return AqiString = GOOD;
    } else if ( AQIpm > 50 && AQIpm <= 100 ) {
      return AqiString = ACCEPTABLE;
    } else if ( AQIpm > 100 && AQIpm <= 150 ) {
      return AqiString = MODERATE;
    } else if ( AQIpm > 150 && AQIpm <= 200 ) {
      return AqiString = HEAVY;
    } else if ( AQIpm > 200 && AQIpm <= 300 ) {
      return AqiString = SEVERE;
    } else {
      return AqiString = HAZARDOUS;
    }
  }
}

// Calcul l'indice de qualité de l'air français ATMO
// Calculate French ATMO AQI indicator
int getATMO(int sensor, float density ) {
  if ( sensor == 0 ) { //PM2.5
    if ( density <= 11 ) {
      return 1;
    } else if ( density > 11 && density <= 24 ) {
      return 2;
    } else if ( density > 24 && density <= 36 ) {
      return 3;
    } else if ( density > 36 && density <= 41 ) {
      return 4;
    } else if ( density > 41 && density <= 47 ) {
      return 5;
    } else if ( density > 47 && density <= 53 ) {
      return 6;
    } else if ( density > 53 && density <= 58 ) {
      return 7;
    } else if ( density > 58 && density <= 64 ) {
      return 8;
    } else if ( density > 64 && density <= 69 ) {
      return 9;
    } else {
      return 10;
    }
  } else { //PM1.0
    if ( density <= 6 ) {
      return 1;
    } else if ( density > 6 && density <= 13 ) {
      return 2;
    } else if ( density > 13 && density <= 20 ) {
      return 3;
    } else if ( density > 20 && density <= 27 ) {
      return 4;
    } else if ( density > 27 && density <= 34 ) {
      return 5;
    } else if ( density > 34 && density <= 41 ) {
      return 6;
    } else if ( density > 41 && density <= 49 ) {
      return 7;
    } else if ( density > 49 && density <= 64 ) {
      return 8;
    } else if ( density > 64 && density <= 79 ) {
      return 9;
    } else {
      return 10;
    }
  }
}

// CAQI Européen - European CAQI level
// source : http://www.airqualitynow.eu/about_indices_definition.php
int getACQI(int sensor, float density ) {
  if ( sensor == 0 ) {  //PM2.5
    if ( density == 0 ) {
      return 0;
    } else if ( density <= 15 ) {
      return 25 ;
    } else if ( density > 15 && density <= 30 ) {
      return 50;
    } else if ( density > 30 && density <= 55 ) {
      return 75;
    } else if ( density > 55 && density <= 110 ) {
      return 100;
    } else {
      return 150;
    }
  } else {              //PM1.0
    if ( density == 0 ) {
      return 0;
    } else if ( density <= 25 ) {
      return 25 ;
    } else if ( density > 25 && density <= 50 ) {
      return 50;
    } else if ( density > 50 && density <= 90 ) {
      return 75;
    } else if ( density > 90 && density <= 180 ) {
      return 100;
    } else {
      return 150;
    }
  }
}

// AQI formula: https://en.wikipedia.org/wiki/Air_Quality_Index#United_States
// Arduino code https://gist.github.com/nfjinjing/8d63012c18feea3ed04e
// On line AQI calculator https://www.airnow.gov/index.cfm?action=resources.conc_aqi_calc
float calcAQI(float I_high, float I_low, float C_high, float C_low, float C) {
  return (I_high - I_low) * (C - C_low) / (C_high - C_low) + I_low;
}

int getAQI(int sensor, float density) {
  int d10 = (int)(density * 10);
  if ( sensor == 0 ) {  //PM2.5
    if (d10 <= 0) {
      return 0;
    } else if(d10 <= 120) {
      return calcAQI(50, 0, 120, 0, d10);
    } else if (d10 <= 354) {
      return calcAQI(100, 51, 354, 121, d10);
    } else if (d10 <= 554) {
      return calcAQI(150, 101, 554, 355, d10);
    } else if (d10 <= 1504) {
      return calcAQI(200, 151, 1504, 555, d10);
    } else if (d10 <= 2504) {
      return calcAQI(300, 201, 2504, 1505, d10);
    } else if (d10 <= 3504) {
      return calcAQI(400, 301, 3504, 2505, d10);
    } else if (d10 <= 5004) {
      return calcAQI(500, 401, 5004, 3505, d10);
    } else if (d10 <= 10000) {
      return calcAQI(1000, 501, 10000, 5005, d10);
    } else {
      return 1001;
    }
  } else {              //PM1.0
    if (d10 <= 0) {
      return 0;
    } else if(d10 <= 540) {
      return calcAQI(50, 0, 540, 0, d10);
    } else if (d10 <= 1540) {
      return calcAQI(100, 51, 1540, 541, d10);
    } else if (d10 <= 2540) {
      return calcAQI(150, 101, 2540, 1541, d10);
    } else if (d10 <= 3550) {
      return calcAQI(200, 151, 3550, 2541, d10);
    } else if (d10 <= 4250) {
      return calcAQI(300, 201, 4250, 3551, d10);
    } else if (d10 <= 5050) {
      return calcAQI(400, 301, 5050, 4251, d10);
    } else if (d10 <= 6050) {
      return calcAQI(500, 401, 6050, 5051, d10);
    } else {
      return 1001;
    }
  }
}

/*
float concentrationPM25_mgm3(float concentrationPM25) {
  double pi = 3.14159;
  double density = 1.65 * pow (10, 12);
  double r25 = 0.44 * pow (10, -6);
  double vol25 = (4/3) * pi * pow (r25, 3);
  double mass25 = density * vol25;
  double K = 3531.5;
  return (concentrationPM25) * K * mass25;
}

float concentrationPM10_mgm3(float concentrationPM10) {
  double pi = 3.14159;
  double density = 1.65 * pow (10, 12);
  double r10 = 0.44 * pow (10, -6);
  double vol10 = (4/3) * pi * pow (r10, 3);
  double mass10 = density * vol10;
  double K = 3531.5;
  return (concentrationPM10) * K * mass10;
}
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // USES_P134
