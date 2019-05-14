#ifdef USES_P136
//#######################################################################################################
//#################################### Plugin 136: MQ7## #############################################
//#################################### by dony71 ########################################################
//#######################################################################################################
// based on https://github.com/R2D2-2017/R2D2-2017/wiki/MQ-7-gas-sensor

#define PLUGIN_136
#define PLUGIN_ID_136           136
#define PLUGIN_NAME_136         "Gases - MQ7 [TESTING]"
#define PLUGIN_VALUENAME1_136   "CO_ppm"
#define PLUGIN_VALUENAME2_136   "CO_Limit"

/// Analog Sensor Pin
#define SENSOR_PIN  A0

#define CAL_WARMUP   1
#define GET_RZERO    2
#define DAQ_WARMUP   3
#define GET_ANALOG   4
#define GET_DIGITAL  5

byte Plugin_Voltage_Select_Pin;
byte Plugin_MQ7_CO_Limit_Pin;
boolean Plugin_136_init = false;

unsigned long previousMillis;
unsigned long currentMillis;

const long read_interval = 90000;  // 90 seconds
const long heat_interval = 60000;  // 60 seconds
const long flag_interval = 30000;  // 30 seconds

int CurrentState;
int NextState;
int SensorVoltage;
int CO_Limit;

float sensorValue;
float sensor_volt;
float RS_air;
float RZero;
float RS_gas;
float R_ZERO;
float Ratio;
float Exponent;
float CO_PPM;

//==============================================
// PLUGIN
// =============================================

boolean Plugin_136(byte function, struct EventStruct *event, String& string)
{
    boolean success = false;

    switch (function)
    {
        case PLUGIN_DEVICE_ADD:
        {
            Device[++deviceCount].Number = PLUGIN_ID_136;
            Device[deviceCount].Type = DEVICE_TYPE_DUAL;
            Device[deviceCount].VType = SENSOR_TYPE_DUAL;
            Device[deviceCount].Ports = 0;
            Device[deviceCount].PullUpOption = false;
            Device[deviceCount].InverseLogicOption = false;
            Device[deviceCount].FormulaOption = true;
            Device[deviceCount].ValueCount = 2;
            Device[deviceCount].SendDataOption = true;
            Device[deviceCount].TimerOption = true;
            Device[deviceCount].GlobalSyncOption = true;
            break;
        }

        case PLUGIN_GET_DEVICENAME:
        {
            string = F(PLUGIN_NAME_136);
            break;
        }

        case PLUGIN_GET_DEVICEVALUENAMES:
        {
            strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_136));
            strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_136));
            break;
        }

        case PLUGIN_WEBFORM_LOAD:
        {
            addFormSeparator(2);
            addFormTextBox(F("R Zero"), F("plugin_136_RZERO"), String(Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0]), 33);
            addUnit(F("260.00"));

            success = true;
            break;
        }

        case PLUGIN_WEBFORM_SAVE:
        {
            Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0] = getFormItemFloat(F("plugin_136_RZERO"));

            success = true;
            break;
        }

        case PLUGIN_INIT:
        {
            Plugin_136_init = true;
            Plugin_MQ7_CO_Limit_Pin = Settings.TaskDevicePin1[event->TaskIndex];
            pinMode(Plugin_MQ7_CO_Limit_Pin, INPUT);
            Plugin_Voltage_Select_Pin = Settings.TaskDevicePin2[event->TaskIndex];
            pinMode(Plugin_Voltage_Select_Pin, OUTPUT);

            digitalWrite(Plugin_Voltage_Select_Pin, LOW);
//            delay(100);
            SensorVoltage = 1;
            CO_Limit = 1;
            UserVar[event->BaseVarIndex + 1] = CO_Limit;
            previousMillis = 0;
            CurrentState = 1;
            NextState = 1;
            success = true;
            break;
        }

        case PLUGIN_READ:
        {
            switch (CurrentState) {
                case CAL_WARMUP: 
                {   // Warm up Sensor for RZero Calibration
                    currentMillis = millis();
/*                    String log = F("Interval Millis ");
                    log += currentMillis - previousMillis;
                    addLog(LOG_LEVEL_INFO, log);
                    log = F("Current State: ");
                    log += CurrentState;
                    addLog(LOG_LEVEL_INFO, log);
                    log = F("Sensor Voltage: ");
                    log += SensorVoltage;
                    addLog(LOG_LEVEL_INFO, log);	*/
                    // Check high voltage to sensor >= 60s
                    if (currentMillis - previousMillis >= heat_interval) {
                        String log = F("MQ7: Sensor already warmed up for 60s, ready for getting RZero");
                        addLog(LOG_LEVEL_INFO, log);
/*                        log = F("Interval Millis ");
                        log += currentMillis - previousMillis;
                        addLog(LOG_LEVEL_INFO, log);
                        log = F("Current State: ");
                        log += CurrentState;
                        addLog(LOG_LEVEL_INFO, log);
                        log = F("Sensor Voltage: ");
                        log += SensorVoltage;
                        addLog(LOG_LEVEL_INFO, log);	*/
                        // Apply low voltage to sensor for getting RZero
                        digitalWrite(Plugin_Voltage_Select_Pin, HIGH);
//                        delay(100);
                        log = F("MQ7: Ready for getting RZero, set Sensor Voltage LOW");
                        addLog(LOG_LEVEL_INFO, log);
                        SensorVoltage = 0;
                        NextState = GET_RZERO;
                        previousMillis = currentMillis;
                    }
                    CurrentState = NextState;
                    break;
                }
                case GET_RZERO: 
                {   // Get RZero
                    currentMillis = millis();
/*                    log = F("Interval Millis ");
                    log += currentMillis - previousMillis;
                    addLog(LOG_LEVEL_INFO, log);
                    log = F("Current State: ");
                    log += CurrentState;
                    addLog(LOG_LEVEL_INFO, log);
                    log = F("Sensor Voltage: ");
                    log += SensorVoltage;
                    addLog(LOG_LEVEL_INFO, log);	*/
                    // To get RZero then check low voltage to sensor >= 90s
                    if (currentMillis - previousMillis >= read_interval) {
                        sensorValue = analogRead(SENSOR_PIN);
//                        delay(20);
                        String log = F("MQ7: Read Analog Sensor for RZero");
                        addLog(LOG_LEVEL_INFO, log);
                        sensor_volt = sensorValue/1024*3.3;
                        RS_air = (3.3-sensor_volt)/sensor_volt;
                        //RZero = RS_air/(26+(1/3));
                        RZero = RS_air/0.26;  // Assume sensor minimum range 10ppm => Rs/R0=0.2559

                        //UserVar[event->BaseVarIndex + 2] = (float)RZero;
                        log = F("MQ7: RZero: ");
                        log += RZero;
                        addLog(LOG_LEVEL_INFO, log);
/*                        log = F("Interval Millis ");
                        log += currentMillis - previousMillis;
                        addLog(LOG_LEVEL_INFO, log);
                        log = F("Current State: ");
                        log += CurrentState;
                        addLog(LOG_LEVEL_INFO, log);
                        log = F("Sensor Voltage: ");
                        log += SensorVoltage;
                        addLog(LOG_LEVEL_INFO, log);	*/
                        // Apply back high voltage to sensor
                        digitalWrite(Plugin_Voltage_Select_Pin, LOW);
//                        delay(100);
                        log = F("MQ7: Heat Sensor for getting concentration, set Sensor Voltage HIGH");
                        addLog(LOG_LEVEL_INFO, log);
                        SensorVoltage = 1;
                        NextState = DAQ_WARMUP;
                        previousMillis = currentMillis;
                    }
                    CurrentState = NextState;
                    break;
                }
                case DAQ_WARMUP: 
                {   // Warm up Sensor for getting concentration
                    currentMillis = millis();
/*                    String log = F("Interval Millis ");
                    log += currentMillis - previousMillis;
                    addLog(LOG_LEVEL_INFO, log);
                    log = F("Current State: ");
                    log += CurrentState;
                    addLog(LOG_LEVEL_INFO, log);
                    log = F("Sensor Voltage: ");
                    log += SensorVoltage;
                    addLog(LOG_LEVEL_INFO, log);	*/
                    // Check high voltage to sensor >= 60s
                    if (currentMillis - previousMillis >= heat_interval) {
                        String log = F("MQ7: Sensor already warmed up for 60s, ready for getting concentration");
                        addLog(LOG_LEVEL_INFO, log);
/*                        log = F("Interval Millis ");
                        log += currentMillis - previousMillis;
                        addLog(LOG_LEVEL_INFO, log);
                        log = F("Current State: ");
                        log += CurrentState;
                        addLog(LOG_LEVEL_INFO, log);
                        log = F("Sensor Voltage: ");
                        log += SensorVoltage;
                        addLog(LOG_LEVEL_INFO, log);	*/
                        // Apply low voltage to sensor to get CO concentration
                        digitalWrite(Plugin_Voltage_Select_Pin, HIGH);
//                        delay(100);
                        log = F("MQ7: Ready for getting concentration, set Sensor Voltage LOW");
                        addLog(LOG_LEVEL_INFO, log);
                        SensorVoltage = 0;
                        NextState = GET_ANALOG;
                        previousMillis = currentMillis;
                    }
                    CurrentState = NextState;
                    break;
                }
                case GET_ANALOG: 
                {   // Get concentration
                    currentMillis = millis();
/*                    log = F("Interval Millis ");
                    log += currentMillis - previousMillis;
                    addLog(LOG_LEVEL_INFO, log);
                    log = F("Current State: ");
                    log += CurrentState;
                    addLog(LOG_LEVEL_INFO, log);
                    log = F("Sensor Voltage: ");
                    log += SensorVoltage;
                    addLog(LOG_LEVEL_INFO, log);	*/
                    // To get CO concentration then check low voltage to sensor >= 90s
                    if (currentMillis - previousMillis >= read_interval) {
                        sensorValue = analogRead(SENSOR_PIN);
//                        delay(20);
                        String log = F("MQ7: Read Analog Sensor for CO concentration");
                        addLog(LOG_LEVEL_INFO, log);
                        sensor_volt = sensorValue/1024*3.3;
                        RS_gas = (3.3-sensor_volt)/sensor_volt;
                        R_ZERO = Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0];
                        Ratio = RS_gas/R_ZERO; //Replace R_ZERO with the value found using the calibration code
                        //CO_PPM = 100 * pow(log10(40)/log10(0.09), Ratio); //Formula for co 2 concentration
                        Exponent = -(log10(1000)-log10(10))*log10(Ratio)/(0.53*2)+log10(0.02)+(0.53*3); //Formula for CO concentration from CO/PPM sensitivity charateristic graph in MQ-7 datasheet
                        CO_PPM = pow(10, Exponent);
                        UserVar[event->BaseVarIndex] = (float)CO_PPM;
                        log = F("MQ7: CO: ");
                        log += CO_PPM;
                        addLog(LOG_LEVEL_INFO, log);
/*                        log = F("Interval Millis ");
                        log += currentMillis - previousMillis;
                        addLog(LOG_LEVEL_INFO, log);
                        log = F("Current State: ");
                        log += CurrentState;
                        addLog(LOG_LEVEL_INFO, log);
                        log = F("Sensor Voltage: ");
                        log += SensorVoltage;
                        addLog(LOG_LEVEL_INFO, log);	*/
                        // Apply back high voltage to sensor
                        digitalWrite(Plugin_Voltage_Select_Pin, LOW);
//                        delay(100);
                        log = F("MQ7: Heat Sensor for getting RZero, set Sensor Voltage HIGH");
                        addLog(LOG_LEVEL_INFO, log);
                        SensorVoltage = 1;
                        NextState = GET_DIGITAL;
                        previousMillis = currentMillis;
                    }
                    CurrentState = NextState;
                    break;
                }
                case GET_DIGITAL: 
                {   // Get CO limit flag
                    currentMillis = millis();
/*                    String log = F("Interval Millis ");
                    log += currentMillis - previousMillis;
                    addLog(LOG_LEVEL_INFO, log);
                    log = F("Current State: ");
                    log += CurrentState;
                    addLog(LOG_LEVEL_INFO, log);
                    log = F("Sensor Voltage: ");
                    log += SensorVoltage;
                    addLog(LOG_LEVEL_INFO, log);	*/
                    // To get CO limit flag then check high voltage to sensor >= 30s
                    if (currentMillis - previousMillis >= flag_interval) {
                        CO_Limit = digitalRead(Plugin_MQ7_CO_Limit_Pin);
//                        delay(20);
                        String log = F("MQ7: Read Digital Sensor for CO limit");
                        addLog(LOG_LEVEL_INFO, log);
                        UserVar[event->BaseVarIndex + 1] = CO_Limit;
                        log = F("MQ7: CO_Limit: ");
                        log += CO_Limit;
                        addLog(LOG_LEVEL_INFO, log);
/*                        log = F("Interval Millis ");
                        log += currentMillis - previousMillis;
                        addLog(LOG_LEVEL_INFO, log);
                        log = F("Current State: ");
                        log += CurrentState;
                        addLog(LOG_LEVEL_INFO, log);
                        log = F("Sensor Voltage: ");
                        log += SensorVoltage;
                        addLog(LOG_LEVEL_INFO, log);	*/
                        NextState = CAL_WARMUP;
                        previousMillis = currentMillis;
                    }
                    CurrentState = NextState;
                    break;
                }
                /*default:
                {
                    CurrentState = CAL_WARMUP;
                    NextState = CAL_WARMUP;
                    break;
                }*/
            }
/*
            String log = F("Current State: ");
            log += CurrentState;
            addLog(LOG_LEVEL_INFO, log);
            log = F("Sensor Voltage: ");
            log += SensorVoltage;
            addLog(LOG_LEVEL_INFO, log);
*/
            success = true;
            break;
        }
    }
    return success;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // USES_P136
