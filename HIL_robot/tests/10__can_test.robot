*** Settings ***
Resource    ../resources/HIL_keywords.robot
Resource    ../user_resources/DUT_keywords.robot
Suite Setup         Run Keywords
...                 Connect To HIL    AND
...                 HIL Power DUT On

Suite Teardown      Disconnect from HIL

*** Test Cases ***
Test CAN send and receive buffer
    HIL Send CAN buffer
    ${data}=    HIL Read CAN Data
    Data Should Increment    data=${data}

Test CAN send and receive string
    # Send string data to DUT via CAN
    ${my_string}=    Set Variable    Hello DUT, this is HIL!
    HIL Send CAN String    text=${my_string}

    # Read data from CAN and verify
    ${received_data}=    HIL Read CAN Data
    HIL Verify CAN String Data    received_data=${received_data}    expected_string=${my_string}
