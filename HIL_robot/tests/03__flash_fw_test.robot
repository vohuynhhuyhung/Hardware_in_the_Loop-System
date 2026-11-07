*** Settings ***
Resource    ../resources/HIL_keywords.robot
Resource    ../user_resources/DUT_keywords.robot

Suite Setup         Connect To HIL
Suite Teardown      Disconnect from HIL

*** Variables ***
${FIRMWARE_PATH}    D:/STUDY/00_SCHOOL/DA/0_Project/STM32F4_DUT/Debug/STM32F4_DUT.hex

*** Test Cases ***
Flash Firmware to DUT
    HIL Flash Firmware for DUT    ${FIRMWARE_PATH}

Verify DUT Still Alive with new Firmware
    Connect To DUT
    Verify DUT Alive
    Disconnect from DUT
