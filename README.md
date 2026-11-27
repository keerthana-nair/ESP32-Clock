# ESP32 Clock
An IoT analogue clock built in C++ using an ESP32 LilyGO TTGO T-Display, featuring real-time NTP synchronisation over the university's eduroam Wi-Fi and a graphical interface adapted from the TFT_eSPI library.

This project was made as part of a coursework and explores the Internet of Things (IoT) through the integration of embedded programming, wireless networking and graphical interfaces. 

The project uses the TFT_Clock example code from the TFT_eSPI library developed by Bodmer as the foundation for its graphical interface. I adapted the example to connect to the eduroam Wi-Fi network and retrieve the current time (BST) from an NTP server rather than relying solely on locally maintained time.

## How it works

1. The ESP32 connects to the eduroam Wi-Fi network.
2. The device retrieves the current time from an NTP server.
3. The retrieved time is used to drive the analogue clock.
4. The TFT_eSPI library renders the clock face and updates the clock hands on the integrated display.

## Future Improvements
- Automatic daylight saving time adjustment
- Integrating weather information at the bottom of the screen
