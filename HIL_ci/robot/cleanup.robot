*** Settings ***
# Always run after a job so the DUT is left powered off.
Resource    ${HIL_ROOT}/HIL_robot/resources/HIL_keywords.robot

Suite Setup         Connect To HIL
Suite Teardown      Disconnect from HIL

*** Test Cases ***
Power Off DUT
    # Leave BOOT0 low so the next power-up runs the firmware, not the ROM bootloader.
    HIL Set DUT BOOT0    0
    HIL Power DUT Off
