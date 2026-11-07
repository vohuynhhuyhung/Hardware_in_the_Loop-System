*** Settings ***
Library    ../user_lib/dut_lib.py
Library    BuiltIn

*** Variables ***
${DUT_PORT}        COM7
${DUT_BAUDRATE}    921600

*** Keywords ***
Connect To DUT
    Connect DUT     ${DUT_PORT}      ${DUT_BAUDRATE}

Disconnect from DUT
    Disconnect DUT
