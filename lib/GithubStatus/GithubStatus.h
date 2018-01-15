#include <Arduino.h>
#include <ArduinoJson.hpp>

class GithubStatus {
  private:
    String status;
    String message;
    String timestamp;

    void parseJSON(String data);

  public:
    bool update();
    String getStatus();
    String getMessage();
    String getTimestamp();
};
