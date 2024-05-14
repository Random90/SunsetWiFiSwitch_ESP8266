# Sunset WiFi Switch for ESP8266
Turn on/off LED lights based on current localization sunset time using ESP8266

### Features

- sets D3 GPIO to HIGH after sunset based on your geolocation (using https://api.ipgeolocation.io/, registration required for API Key, free plan available)
- sets D3 to LOW just after midnight ( `#define turnoffTimeStr "00:05"`, can be changed)
- before turning led off, it updates data again
- current timezone is hardcoded (not a feature exactly)
- blinks built in led to let you know that it is still working fine (important)

### Building
Install Visual Studio Code and Platformio extension, git clone the repo, open in platformio, compile, upload to your ESP8266. Project is hardcoded for WEMOS D1 Mini board, so you might want to change it inside `platformio.ini`.

