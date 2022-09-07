#ifndef VAN_CD_CHANGER_COMMANDS_H
#define VAN_CD_CHANGER_COMMANDS_H

enum psa_van_cd_changer_commands
{
    PSA_VAN_CD_CHANGER_POWER_OFF = 0x1101,
    PSA_VAN_CD_CHANGER_POWER_OFF_2 = 0x2101,
    PSA_VAN_CD_CHANGER_PAUSE = 0x1181,
    PSA_VAN_CD_CHANGER_PLAY = 0x1183,
    PSA_VAN_CD_CHANGER_PREVIOUS_TRACK = 0x31FE,
    PSA_VAN_CD_CHANGER_NEXT_TRACK = 0X31FF,
    PSA_VAN_CD_CHANGER_CD_1 = 0X4101,
    PSA_VAN_CD_CHANGER_CD_2 = 0X4102,
    PSA_VAN_CD_CHANGER_CD_3 = 0X4103,
    PSA_VAN_CD_CHANGER_CD_4 = 0X4104,
    PSA_VAN_CD_CHANGER_CD_5 = 0X4105,
    PSA_VAN_CD_CHANGER_CD_6 = 0X4106,
    PSA_VAN_CD_CHANGER_PREVIOUS_CD = 0X41FE,
    PSA_VAN_CD_CHANGER_NEXT_CD = 0X41FF,
};

struct psa_van_cd_changer_command_struct
{
    uint16_t command; // Big endian
};

#endif