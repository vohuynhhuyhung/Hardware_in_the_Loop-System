*** Settings ***
Resource    ../resources/HIL_keywords.robot
Resource    ../user_resources/DUT_keywords.robot

Suite Setup         Run Keywords
...                 Connect To HIL               AND
...                 Connect To Logic Analyzer    AND
...                 HIL Power DUT On                 AND
...                 Connect To DUT

Suite Teardown      Run Keywords
...                 HIL Power DUT Off                     AND
...                 Disconnect from HIL               AND
...                 Disconnect from Logic Analyzer    AND
...                 Disconnect from DUT

*** Variables ***
${PWM_FREQUENCY}       1
${PWM_DUTY_CYCLE}      50
${PWM_CHANNEL}         1
${LA_DIGITAL_CHANNEL}  0

*** Test Cases ***
DUT Generate PWM Signal
    DUT Set PWM freq      freq=${PWM_FREQUENCY}
    DUT Set duty cycle    ch=${PWM_CHANNEL}    duty=${PWM_DUTY_CYCLE} 
    DUT Start PWM   ch=${PWM_CHANNEL}

Verify PWM Signal with Logic Analyzer
    # Read data from logic analyzer
    ${data}=    LA Read Data

    # Verify results
    LA Verify Pulse Wave    raw_data=${data}    channel=${LA_DIGITAL_CHANNEL}    expected_freq_hz=${PWM_FREQUENCY}    expected_duty_cycle=${PWM_DUTY_CYCLE}

DUT Stop PWM Signal
    DUT Stop PWM   ch=${PWM_CHANNEL}