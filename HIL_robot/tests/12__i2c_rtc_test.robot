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
RTC Initialization
    # --- HIL setup ---
    HIL Init RTC        addr=0x68
    HIL Activate I2C Device    addr=0x68

    # --- DUT setup ---
    DUT Init RTC

RTC Set and Get Time/Date
    DUT Set RTC Time    hour=14    minute=30    second=45
    ${time}=    DUT Get RTC Time
    RTC Time Should Be Within    rtc_data=${time}    hour=14    minute=30    second=45    tolerance=7

    DUT Set RTC Date    dow=1    date=12    month=4    year=26
    ${date}=    DUT Get RTC Date
    RTC Date Should Be    rtc_data=${date}    dow=1    date=12    month=4    year=26

RTC Time Progression
    DUT Set RTC Date    dow=7    date=31    month=12    year=26
    DUT Set RTC Time    hour=23    minute=59    second=50
    Sleep    10s
    ${date}=    DUT Get RTC Date
    ${time}=    DUT Get RTC Time
    RTC Time Should Be Within    rtc_data=${time}    hour=0    minute=0    second=0    tolerance=7
    RTC Date Should Be    rtc_data=${date}    dow=1    date=1    month=1    year=27
    
