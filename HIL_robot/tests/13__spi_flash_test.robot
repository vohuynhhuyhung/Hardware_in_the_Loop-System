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
${LENGTH_512}    512
${LENGTH_1K}     1024

*** Test Cases ***
DUT Read SPI Flash ID Test
    DUT Read SPI Flash ID

HIL prepare SPI Flash data and DUT read test 512 bytes
    # --- HIL setup ---
    HIL prepare SPI Flash data         length=${LENGTH_512}
    # --- DUT read data ---
    ${data}=    DUT Read SPI Flash Data    length=${LENGTH_512}
    Data Should Increment              data=${data}

DUT erase SPI Flash and read test
    # --- DUT erase data ---
    DUT Erase SPI Flash Data
    # --- DUT read data ---
    ${data}=    DUT Read SPI Flash Data    length=${LENGTH_1K}
    DUT Verify Is SPI Flash Data Erased           data=${data}

DUT write SPI Flash and read test 1024 bytes
    # --- DUT write data ---
    DUT Write SPI Flash Data               length=${LENGTH_1K}
    # --- DUT read data ---
    ${data}=    DUT Read SPI Flash Data    length=${LENGTH_1K}
    Data Should Increment              data=${data}
