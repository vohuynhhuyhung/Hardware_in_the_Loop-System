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

*** Test Cases ***
Test Connection
    Verify HIL Alive

    Verify DUT Alive
