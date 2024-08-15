#include "JsonDecoder.h"
#include "JSON.h"
#include "StateMachine.h"
#include "ControlSystem.h"
#include "Debug.h"

/** Private JSON to Structure **/

/**
 * @brief Constructs a MachineConfiguration structure from a JSON string.
 *
 * @param json A JSON string containing a machine configuration.
 */

static bool json_to_self_check_parameters(const json_t *json, SelfCheckParameters *parameters)
{
    bool success = true;
    success &= json_property_to_bool(json, "Charge Pump", &(parameters->chargePump));
    return success;
}

static bool json_to_machine_check_parameters(const json_t *json, MachineCheckParameters *parameters)
{
    bool success = true;
    const char *property = NULL;
    success &= json_property_to_string_ref(json, "ESD Chain", &property);
    parameters->esdChain = string_to_esd_chain(property);
    success &= json_property_to_bool(json, "Servo OK", &(parameters->servoOK));
    success &= json_property_to_bool(json, "Force Gauge Com", &(parameters->forceGaugeCom));
    success &= json_property_to_bool(json, "Servo Com", &(parameters->servoCom));
    return success;
}

static bool json_to_motion_parameters(const json_t *json, MotionParameters *parameters)
{
    bool success = true;
    const char *property = NULL;
    success &= json_property_to_string_ref(json, "Status", &property);
    parameters->status = string_to_motion_status(property);

    success &= json_property_to_string_ref(json, "Condition", &property);
    parameters->condition = string_to_motion_condition(property);

    success &= json_property_to_string_ref(json, "Mode", &property);
    parameters->mode = string_to_motion_mode(property);
    return success;
}

/**
 * @brief Constructs a MachineConfiguration structure from a JSON string.
 *
 * @param json A JSON string containing a machine configuration.
 */

static bool inner_json_to_machine_configuration(const json_t *json, MachineConfiguration *configuration)
{
    bool success = true;
    success &= json_property_to_string(json, "Motor Type", configuration->motorType, sizeof(configuration->motorType));
    success &= json_property_to_int(json, "Max Motor RPM", &(configuration->maxMotorRPM));
    success &= json_property_to_int(json, "Max Motor Torque", &(configuration->maxMotorTorque));
    success &= json_property_to_int(json, "Gear Diameter (mm)", &(configuration->gearDiameter));
    success &= json_property_to_int(json, "Gear Pitch (mm)", &(configuration->gearPitch));
    success &= json_property_to_int(json, "System Inertia", &(configuration->systemIntertia));
    success &= json_property_to_int(json, "Static Torque", &(configuration->staticTorque));
    success &= json_property_to_int(json, "Load", &(configuration->load));
    success &= json_property_to_string(json, "Position Encoder Type", configuration->positionEncoderType, sizeof(configuration->positionEncoderType));
    success &= json_property_to_int(json, "Position Encoder (steps/mm)", &(configuration->encoderStepsPermm));
    success &= json_property_to_int(json, "Servo Step (steps/mm)", &(configuration->servoStepPermm));
    success &= json_property_to_string(json, "Force Gauge Name", configuration->forceGauge, sizeof(configuration->forceGauge));
    success &= json_property_to_int(json, "Force Gauge Gain", &(configuration->forceGaugeGain));
    success &= json_property_to_int(json, "Force Gauge Offset", &(configuration->forceGaugeOffset));
    return success;
}

/**
 * @brief Constructs a MachinePerformance structure from a JSON string.
 *
 * @param json A JSON string containing a machine performance.
 */

static bool inner_json_to_machine_performance(const json_t *json, MachinePerformance *performance)
{
    bool success = true;
    success &= json_property_to_int(json, "Position Minimum", &(performance->minPosition));
    success &= json_property_to_int(json, "Position Maximum", &(performance->maxPosition));
    success &= json_property_to_int(json, "Velocity Maximum", &(performance->maxVelocity));
    success &= json_property_to_int(json, "Acceleration Maximum", &(performance->maxAcceleration));
    success &= json_property_to_int(json, "Force Tensile Maximum", &(performance->maxForceTensile));
    success &= json_property_to_int(json, "Force Compression Maximum", &(performance->maxForceCompression));
    success &= json_property_to_int(json, "Force gauge Neutral Offset", &(performance->forceGaugeNeutralOffset));
    return success;
}

/**
 * @brief Converts JSON string to a MachineProfile structure.
 *
 * @param json A JSON string containing a machine profile.
 * @return A MachineProfile structure containing the machine profile from JSON.
 */
bool json_to_machine_profile(MachineProfile *profile, char *json)
{
    const json_t *parser = json_create_static(json);
    if (!parser || JSON_OBJ != json_getType(parser))
    {
        DEBUG_ERROR("%s", "Error, the  JSON cannot be parsed.\n");
        return false;
    }

    if (!parser || JSON_OBJ != json_getType(parser))
    {
        DEBUG_ERROR("%s", "Error, the  JSON cannot be parsed.\n");
        return false;
    }

    if (!json_property_to_string(parser, "Name", profile->name, sizeof(profile->name)))
    {
        DEBUG_ERROR("%s", "Error, the  Machine Profile Name  property is not found.\n");
        return false;
    }

    DEBUG_INFO("Machine Profile Name: %s\n", profile->name);

    const json_t *mcParser = json_getProperty(parser, "Configuration");
    const jsonType_t machine_configuration_type = json_getType(mcParser);
    if ((mcParser == NULL) || (machine_configuration_type != JSON_OBJ))
    {
        DEBUG_ERROR("%s", "Error, the  Machine Configuration  property is not found.\n");
        return false;
    }

    if (!inner_json_to_machine_configuration(mcParser, &(profile->configuration)))
    {
        DEBUG_ERROR("%s", "Error, the  Machine Configuration  property is not found.\n");
        return false;
    }

    DEBUG_INFO("Machine Configuration: %d\n", profile->configuration.forceGaugeGain);
    DEBUG_INFO("Machine Configuration: %d\n", profile->configuration.maxMotorRPM);

    const json_t *mpParser = json_getProperty(parser, "Performance");
    if (!mpParser || JSON_OBJ != json_getType(mpParser))
    {
        DEBUG_ERROR("%s", "Machine Performance  property is not found.\n");
    }

    if (!inner_json_to_machine_performance(mpParser, &(profile->performance)))
    {
        DEBUG_ERROR("%s", "Error, the  Machine Performance  property is not found.\n");
        return false;
    }

    return true;
}

bool json_to_machine_configuration(MachineConfiguration *configuration, char *json)
{
    // Use tiny-json to parse the string
    const json_t *parser = json_create_static(json);
    if (!parser || JSON_OBJ != json_getType(parser))
    {
        DEBUG_ERROR("%s", "Error, the  JSON cannot be parsed.\n");
        return false;
    }

    return inner_json_to_machine_configuration(parser, configuration);
}

bool json_to_machine_performance(MachinePerformance *performance, char *json)
{
    // Use tiny-json to parse the string
    const json_t *parser = json_create_static(json);
    if (!parser || JSON_OBJ != json_getType(parser))
    {
        DEBUG_ERROR("%s", "Error, the  JSON cannot be parsed.\n");
        return false;
    }

    return inner_json_to_machine_performance(parser, performance);
}

bool json_to_sample_profile(SampleProfile *sample, char *json)
{
    // Use tiny-json to parse the string
    const json_t *parser = json_create_static(json);
    if (!parser || JSON_OBJ != json_getType(parser))
    {
        DEBUG_ERROR("%s", "Error, the  JSON cannot be parsed.\n");
        return false;
    }

    bool success = true;
    success &= json_property_to_string(parser, "Name", sample->name, MAX_SAMPLE_PROFILE_NAME);
    success &= json_property_to_int(parser, "Length", &(sample->length));
    success &= json_property_to_int(parser, "Stretch Max", &(sample->stretchMax));
    success &= json_property_to_int(parser, "Max Velocity", &(sample->maxVelocity));
    success &= json_property_to_int(parser, "Max Acceleration", &(sample->maxAcceleration));
    success &= json_property_to_int(parser, "Max Jerk", &(sample->maxJerk));
    success &= json_property_to_int(parser, "Max Force Tensile", &(sample->maxForceTensile));
    success &= json_property_to_int(parser, "Max Force Compression", &(sample->maxForceCompression));
    return success;
}

/**
 * @brief Converts JSON string to a MachineProfile structure.
 *
 * @param json A JSON string containing a machine profile.
 * @return A MachineProfile structure containing the machine profile from JSON.
 */
bool json_to_test_data_request(TestDataRequest *request, char *json)
{
    // Use tiny-json to parse the string
    const json_t *parser = json_create_static(json);
    if (!parser || JSON_OBJ != json_getType(parser))
    {
        DEBUG_ERROR("%s", "Error, the  JSON cannot be parsed.\n");
        return false;
    }

    bool success = true;
    int index = request->index;
    int count = request->count;
    success &= json_property_to_int(parser, "Index", &index);
    success &= json_property_to_int(parser, "Count", &count);
    return success;
}

bool json_to_test_header_name(const char **name, char *json)
{
    // Use tiny-json to parse the string
    const json_t *parser = json_create_static(json);
    if (!parser || JSON_OBJ != json_getType(parser))
    {
        DEBUG_ERROR("%s", "Error, the  JSON cannot be parsed.\n");
        return false;
    }

    return json_property_to_string_ref(parser, "test_name", name);
}

bool json_to_motion_mode(MotionMode *mode, char *json)
{
    // Use tiny-json to parse the string
    const json_t *parser = json_create_static(json);
    if (!parser || JSON_OBJ != json_getType(parser))
    {
        DEBUG_ERROR("%s", "Error, the  JSON cannot be parsed.\n");
        return false;
    }
    bool success = true;
    const char *property;
    success &= json_property_to_string_ref(parser, "Mode", &property);
    *mode = string_to_motion_mode(property);
    return success;
}

bool json_to_motion_status(MotionStatus *status, char *json)
{
    // Use tiny-json to parse the string
    const json_t *parser = json_create_static(json);
    bool success = true;
    if (!parser || JSON_OBJ != json_getType(parser))
    {
        DEBUG_ERROR("%s", "Error, the  JSON cannot be parsed.\n");
        return false;
    }
    const char *property = NULL;
    success &= json_property_to_string_ref(parser, "Status", &property);
    *status = string_to_motion_status(property);
    return success;
}

// {G:0,X:254,F:1000}
bool json_to_move(lib_json_move_S *move, char *json)
{
    // Use tiny-json to parse the string
    // HII riley tmrw, looks like we a parsing floats into an integer pointer
    // should probally make a lib_json struct for the move using floats
    // then we can multiply to um and cast to int in messageSlave
    const json_t *parser = json_create_static(json);
    if (!parser || JSON_OBJ != json_getType(parser))
    {
        DEBUG_ERROR("%s", "Error, the  JSON cannot be parsed.\n");
        return false;
    }

    bool success = true;
    success &= json_property_to_int(parser, "G", &(move->g));
    if (success && (move->g == 0 || move->g == 1))
    {
        DEBUG_INFO("%s", "Parsing G0 or G1 mov\n");
        success &= json_property_to_double(parser, "X", &(move->x));
        success &= json_property_to_double(parser, "F", &(move->f));
    }
    else if (success && (move->g == 4))
    {
        DEBUG_INFO("%s", "Parsing G4 move\n");
        success &= json_property_to_int(parser, "P", &(move->p));
    }
    return success;
}
