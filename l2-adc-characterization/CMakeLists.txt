# L2 — ADC characterization: sample the joystick, print raw counts and volts
es201_app(l2_adc_read main.c)
target_link_libraries(l2_adc_read hardware_adc)
