*** Settings ***
# HIL_ROOT is injected by the CI runner (--variable HIL_ROOT:...).
Resource    ${HIL_ROOT}/HIL_robot/resources/HIL_keywords.robot

Suite Setup         Run Keywords    Connect To HIL    AND    HIL Power DUT On
Suite Teardown      Run Keywords    HIL Power DUT Off    AND    Disconnect from HIL

*** Test Cases ***
DUT Is Powered
    HIL Check is DUT power on
