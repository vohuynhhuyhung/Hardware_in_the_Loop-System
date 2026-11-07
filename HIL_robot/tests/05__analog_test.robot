*** Settings ***
Resource    ../resources/HIL_keywords.robot
Resource    ../user_resources/DUT_keywords.robot

Suite Setup         Run Keywords
...                 Connect To HIL          AND
...                 HIL Power DUT On            AND
...                 Connect To DUT

Suite Teardown      Run Keywords
...                 Disconnect from HIL     AND
...                 Disconnect from DUT

*** Variables ***
${voltage_1}    1.5
${voltage_2}    2.0
${voltage_3}    3.0
${voltage_4}    3.3

*** Test Cases ***
Test DUT Read 1.5V
    HIL Set DAC Voltage     voltage=${voltage_1}
    DUT Read ADC Voltage    expected_volt=${voltage_1}

Test DUT Read 2.0V
    HIL Set DAC Voltage     voltage=${voltage_2}
    DUT Read ADC Voltage    expected_volt=${voltage_2}

Test DUT Read 3.0V
    HIL Set DAC Voltage     voltage=${voltage_3}
    DUT Read ADC Voltage    expected_volt=${voltage_3}

Test DUT Read 3.3V
    HIL Set DAC Voltage     voltage=${voltage_4}
    DUT Read ADC Voltage    expected_volt=${voltage_4}