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
DUT Turn On GPIO LED 1 - 5
    DUT Set LED    index=1    state=1
    DUT Set LED    index=2    state=1
    DUT Set LED    index=3    state=1
    DUT Set LED    index=4    state=1
    DUT Set LED    index=5    state=1

HIL Read LED States - Expect ON
    HIL Read GPIO  port=e    pin=0    expected_state=1
    HIL Read GPIO  port=e    pin=1    expected_state=1
    HIL Read GPIO  port=e    pin=2    expected_state=1
    HIL Read GPIO  port=e    pin=3    expected_state=1
    HIL Read GPIO  port=e    pin=4    expected_state=1

DUT Turn Off GPIO LED 1 - 5
    DUT Set LED    index=1    state=0
    DUT Set LED    index=2    state=0
    DUT Set LED    index=3    state=0
    DUT Set LED    index=4    state=0
    DUT Set LED    index=5    state=0

HIL Read LED States - Expect OFF
    HIL Read GPIO  port=e    pin=0    expected_state=0
    HIL Read GPIO  port=e    pin=1    expected_state=0
    HIL Read GPIO  port=e    pin=2    expected_state=0
    HIL Read GPIO  port=e    pin=3    expected_state=0
    HIL Read GPIO  port=e    pin=4    expected_state=0
