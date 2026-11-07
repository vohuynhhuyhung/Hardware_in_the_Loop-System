*** Settings ***
Resource    ../resources/HIL_keywords.robot
Resource    ../user_resources/DUT_keywords.robot
Suite Setup         Run Keywords
...                 Connect To HIL               AND
...                 Connect To Logic Analyzer    AND
...                 HIL Power DUT On             AND
...                 Connect To DUT

Suite Teardown      Run Keywords
...                 HIL Power DUT Off                 AND
...                 Disconnect from HIL               AND
...                 Disconnect from Logic Analyzer    AND
...                 Disconnect from DUT

*** Variables ***
${FREQ}    10000

*** Test Cases ***
HIL Generate Sine Wave
    HIL Generate Sine Wave     frequency=${FREQ}

DUT Read Frequency & Re-Generate Sine Wave
    ${freq}=    DUT Read Sine Frequency
    DUT Generate Sine Wave     frequency=${freq}

LA Collect Waveform Data
    ${data}=    LA Read Data
    LA Verify Sine Wave    raw_data=${data}    expected_freq_hz=${FREQ}

HIL Stop Sine Wave
    HIL Stop Sine Wave
