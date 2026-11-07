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
${SINE_FREQUENCY}           1000
${TRIANGLE_FREQUENCY}       1000
*** Test Cases ***

DUT Generate Sine Wave and Acquire Data
    DUT Generate Sine Wave    frequency=${SINE_FREQUENCY}
    ${data}=    LA Read Data
    LA Verify Sine Wave    raw_data=${data}    expected_freq_hz=${SINE_FREQUENCY}

DUT Stop Sine Wave
    DUT Stop Sine Wave

DUT Generate Triangle Wave and Acquire Data
    DUT Generate Triangle Wave    frequency=${TRIANGLE_FREQUENCY}
    ${data}=    LA Read Data
    LA Verify Triangle Wave    raw_data=${data}    expected_freq_hz=${TRIANGLE_FREQUENCY}

DUT Stop Triangle Wave
    DUT Stop Triangle Wave
