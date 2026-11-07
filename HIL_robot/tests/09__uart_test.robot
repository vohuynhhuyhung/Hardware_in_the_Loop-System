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
${test_data_1}    Hello HIL from DUT
${test_data_2}    Hello DUT from HIL

*** Test Cases ***
UART Initialization
    HIL Init UART 
    DUT Init UART

UART DUT To HIL Communication
    DUT Send UART String    ${test_data_1}
    Sleep    1s
    ${received_data}=    HIL Read UART Data
    HIL Verify UART String    ${received_data}    ${test_data_1}
    Sleep    1s
    
UART HIL To DUT Communication
    HIL Send UART String    ${test_data_2}
    Sleep    1s
    ${received_data}=    DUT Read UART Data
    DUT Verify UART String    ${received_data}    ${test_data_2}