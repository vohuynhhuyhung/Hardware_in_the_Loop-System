*** Settings ***
Resource    ../resources/HIL_keywords.robot
Suite Setup         Connect To HIL
Suite Teardown      Disconnect from HIL

*** Test Cases ***
Test HIL Power DUT On
    HIL Power DUT On
    HIL Check is DUT power on

Test HIL Power DUT Off
    HIL Power DUT Off
    HIL Check is DUT power off
