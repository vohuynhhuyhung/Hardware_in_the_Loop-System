*** Settings ***
Resource    ../resources/HIL_keywords.robot
Resource    ../user_resources/DUT_keywords.robot

Suite Setup         Run Keywords
...                 Connect To HIL          AND
...                 HIL Power DUT On            AND
...                 Connect To DUT

Suite Teardown      Run Keywords
...                 HIL Power DUT Off           AND
...                 Disconnect from HIL     AND
...                 Disconnect from DUT

*** Test Cases ***
EEPROM Validation
    # --- HIL setup ---
    HIL Init EEPROM            addr=0x50    size=1024    page=256
    HIL Activate I2C Device    addr=0x50

    # --- DUT setup ---
    DUT Init EEPROM            addr=0x50    size=1024    page=256

    # --- Timing ---
    ${t}=    DUT Measure EEPROM Write Time    addr=0x50    length=256
    DUT Write Time Should Be Less Than    ${t}    5

    # --- Data validation ---
    ${data}=    DUT Write And Verify EEPROM    addr=0x50    length=256
    Data Should Increment    data=${data}

    # --- HIL deinit ---
    HIL Deinit EEPROM        addr=0x50
