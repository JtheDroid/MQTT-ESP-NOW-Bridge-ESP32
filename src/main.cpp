#define ENABLE_SERIAL

#include <Arduino.h>
#define ESP32_ETH
#include "HA-client.h"
#include "values.h" //defines MQTT_SERVER, MQTT_USER, MQTT_PASS, MQTT_PORT
#include "mac_topic.cpp"

#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER 12

#include "FS.h"
#include "SD_MMC.h"
#include <esp_now.h>

typedef struct mqtt_message_data
{
  char topic[50];
  char message[ESP_NOW_MAX_DATA_LEN - 50];
} mqtt_message_data;

typedef struct espnow_message
{
  mqtt_message_data message;
  uint8_t mac[6];
} espnow_message;

linked_list<mqtt_message_data> messages_mqtt;
linked_list<espnow_message> messages_espnow;
unsigned int send_count{0};
esp_now_peer_num_t peer_num{0, 0};

// const int BUFFER_SIZE = JSON_ARRAY_SIZE(3);
bool sd_available;
void mqttCallback(char *, char *);
void subscribeToTopics(PubSubClient &);

HAClient haClient(MQTT_SERVER,
                  MQTT_USER,
                  MQTT_PASS,
                  MQTT_PORT,
                  "home/" NAME "/available",
                  "home/" NAME "/update",
                  subscribeToTopics,
                  mqttCallback);

void handle_message_espnow()
{
  SERIAL_PRINT("Handling ESP-Now message");
  espnow_message *m{messages_espnow.remove_last()};
  mqtt_message_data &message = m->message;
  uint8_t *mac_addr = m->mac;
  SERIAL_PRINTF(" from %02X:%02X:%02X:%02X:%02X:%02X\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  if (strcmp(message.topic, "REGISTER") == 0)
  {
    SERIAL_PRINTF("Registering ESP");
    add_mac_topic(mac_addr, message.message);
    esp_now_peer_info_t peer_info{{mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]}};
    //esp_err_t code =
    esp_now_add_peer(&peer_info);
    esp_now_get_peer_num(&peer_num);
    /*bool success = code == ESP_OK;
    char str[18 + 5] = {0};
    sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X %s", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], success ? "  OK" : "FAIL");
    haClient.getClient().publish("home/" NAME "/espnow", str);*/
  }
  else if (strcmp(message.topic, "UNREGISTER") == 0)
  {
    SERIAL_PRINTLN("Unregistering ESP");
    remove_mac_topic(mac_addr, message.message);
    //esp_now_del_peer(mac_addr); //only if all topics unregistered
    //esp_now_get_peer_num(&peer_num);
  }
  else
  {
    SERIAL_PRINTLN("Publishing message via MQTT");
    haClient.getClient().publish(message.topic, message.message);
  }
  SERIAL_PRINTLN("Deleting message");
  delete m;
}

void handle_message_mqtt()
{
  SERIAL_PRINTLN("Handling MQTT message");
  mqtt_message_data *msg{messages_mqtt.remove_last()};
  execute_for_topic(msg->topic, [&msg](const uint8_t *mac) {
    if (!esp_now_is_peer_exist(mac))
    {
      SERIAL_PRINTLN("Registering ESP for sending message");
      esp_now_peer_info_t peer_info{{mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]}};
      esp_now_add_peer(&peer_info);
      esp_now_get_peer_num(&peer_num);
    }
    SERIAL_PRINTF("Sending message to ESP %02X:%02X:%02X:%02X:%02X:%02X, ", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    esp_err_t code{esp_now_send(mac, (uint8_t *)msg, sizeof(mqtt_message_data))};
    SERIAL_PRINTF("code: %d\n", code);
    send_count++;
    yield();
  });
  SERIAL_PRINTLN("Deleting message");
  delete msg;
}

void loop_messagelists()
{
  if (!messages_espnow.is_empty())
    handle_message_espnow();

  yield();
  if (send_count == 0) //(esp_msg == nullptr)
    if (!messages_mqtt.is_empty())
      handle_message_mqtt();
}

void loop()
{
  haClient.loop();
  loop_messagelists();
}

void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  send_count--;
  SERIAL_PRINTF("Send %s: %02X:%02X:%02X:%02X:%02X:%02X, waiting for %d callbacks\n", status == ESP_NOW_SEND_SUCCESS ? "successful" : "failed", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], send_count);
  //if (status == ESP_NOW_SEND_SUCCESS)
  //{

  //}

  //char str[18 + 5] = {0};
  //sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X %s", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], status == ESP_NOW_SEND_SUCCESS ? "SUCC" : "FAIL");
  //haClient.getClient().publish("home/" NAME "/espnow/send_status", str);
}

/*void list_all_mac_topics()
{
  if (root_mac_topic == NULL)
    haClient.getClient().publish("home/" NAME "/list", "NONE");
  int i = 0;
  for (mac_topic *mc = root_mac_topic; mc != NULL; mc = mc->next)
  {
    const uint8_t *mac_addr = mc->mac;
    char str[3 + 18] = {0};
    sprintf(str, "%02d %02X:%02X:%02X:%02X:%02X:%02X", i++, mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    haClient.getClient().publish("home/" NAME "/list", str);
  }
}*/

void recv_cb(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
  SERIAL_PRINTF("ESP-Now message received from %02X:%02X:%02X:%02X:%02X:%02X\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  espnow_message *m = new espnow_message;
  memcpy(&m->message, data, sizeof(mqtt_message_data));
  memcpy(&m->mac, mac_addr, sizeof(uint8_t) * 6);
  SERIAL_PRINTLN("Adding to list");
  messages_espnow.add_to_start(m);
  //list_all_mac_topics();
}

void setup()
{
  setCpuFrequencyMhz(80);
  Serial.begin(115200);
  sd_available = SD_MMC.begin();
  haClient.setup();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_now_init();
  esp_now_register_send_cb(send_cb);
  esp_now_register_recv_cb(recv_cb);
  /*WiFi.onEvent([](WiFiEvent_t event) {
    mqtt_message_data msg{
        "status",
        ""};
    switch (event)
    {
    case SYSTEM_EVENT_ETH_START:
      strcpy(msg.message, "ETH_START");
    case SYSTEM_EVENT_ETH_CONNECTED:
      strcpy(msg.message, "ETH_CONNECTED");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
    {
      String s = "ETH_GOT_IP\n";
      s += ETH.localIP().toString();
      s += "\n";
      s += ETH.macAddress();
      strcpy(msg.message, s.c_str());
    }
    break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      strcpy(msg.message, "ETH_DISCONNECTED");
      break;
    case SYSTEM_EVENT_ETH_STOP:
      strcpy(msg.message, "ETH_STOP");
      break;
    default:
      return;
    }

    esp_now_send(NULL, (uint8_t *)&msg, sizeof(msg));
  });*/
  //esp_now_peer_info_t peer_info{
  //    {0xCC, 0x50, 0xE3, 0x3C, 0x27, 0x60}};
  //esp_now_add_peer(&peer_info);
  /*mqtt_message_data msg{
      "status",
      "Bridge start"};
  esp_now_send(NULL, (uint8_t *)&msg, sizeof(msg));*/
}

void subscribeToTopics(PubSubClient &client)
{
  client.subscribe("#");
  client.publish("home/" NAME "/mac", WiFi.macAddress().c_str());
}

void mqttCallback(char *topic, char *message)
{
  SERIAL_PRINTLN("MQTT message received");
  mqtt_message_data *m{new mqtt_message_data};
  for (size_t i = 0; i < sizeof(m->topic) && topic[i - 1] != '\0'; i++)
  {
    m->topic[i] = topic[i];
  }
  m->topic[sizeof(m->topic) - 1] = '\0';
  for (size_t i = 0; i < sizeof(m->message) && message[i - 1] != '\0'; i++)
  {
    m->message[i] = message[i];
  }
  m->message[sizeof(m->message) - 1] = '\0';
  SERIAL_PRINTLN("Adding to list");
  messages_mqtt.add_to_start(m);
}

/*void logMqttMessage(char *topic, char *message)
{
  String s1("/"), s2(topic);
  s2.replace("/", "_");
  s2 += ".txt";
  File file = SD_MMC.open(s1 + s2, "a");
  if (file)
  {
    file.println(message);
    file.close();
  }
  else
    SERIAL_PRINTF("Error opening file %s%s\n", s1.c_str(), s2.c_str());
  yield();
}*/