*** Settings ***
# Platform-owned stage: bench smoke check, then flash. Not editable by users.
Resource    ${HIL_ROOT}/HIL_robot/resources/HIL_keywords.robot

Suite Setup         Connect To HIL
Suite Teardown      Disconnect from HIL

*** Variables ***
${FIRMWARE_PATH}    ${EMPTY}

*** Test Cases ***
HIL Is Alive
    Verify HIL Alive

HIL Can Switch DUT Power
    HIL Power DUT On
    HIL Check is DUT power on
    HIL Power DUT Off
    HIL Check is DUT power off

Flash Firmware To DUT
    HIL Flash Firmware for DUT    ${FIRMWARE_PATH}
