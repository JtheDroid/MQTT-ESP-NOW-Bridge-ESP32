#include <Arduino.h>
#define ESP32_ETH
#include "HA-client.h"
#include "values.h"
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

typedef struct mac_topic
{
  const uint8_t *mac;
  const char *topic;
  mac_topic *next;
} mac_topic;

mac_topic *root_mac_topic;

bool mac_equals(const uint8_t *mac1, const uint8_t *mac2)
{
  for (size_t i = 0; i < ESP_NOW_ETH_ALEN; i++)
    if (mac1[i] != mac2[i])
      return false;
  return true;
}

// bool mac_topic_equals(const mac_topic *mc1, const mac_topic *mc2)
// {
//   return mac_equals(mc1->mac, mc2->mac) && strcmp(mc1->topic, mc2->topic) == 0;
// }

void add_mac_topic(const uint8_t *mac, const char *topic)
{
  for (mac_topic *mc = root_mac_topic; mc != NULL; mc = mc->next) //loop through entries, return if exists
    if (mac_equals(mac, mc->mac) && strcmp(topic, mc->topic) == 0)
      return;
  //insert new entry as new root
  mac_topic *added = new mac_topic{
      mac,
      topic,
      root_mac_topic};
  root_mac_topic = added;
}

void remove_mac_topic(const uint8_t *mac, const char *topic)
{
  for (mac_topic *mc = root_mac_topic, *prev = NULL; mc != NULL; mc = mc->next)
  {
    if (mac_equals(mac, mc->mac) && strcmp(topic, mc->topic) == 0)
    {
      if (mc == root_mac_topic)
        root_mac_topic = mc->next;
      else
      {
        prev->next = mc->next;
        delete mc;
      }
      return;
    }
    prev = mc;
  }
}

void execute_for_topic(const char *topic, std::function<void(const uint8_t *mac)> execute)
{
  for (mac_topic *mc = root_mac_topic; mc != NULL; mc = mc->next)
    if (strcmp(topic, mc->topic) == 0 || strcmp("#", mc->topic) == 0)
      execute(mc->mac);
}


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

void loop()
{
  haClient.loop();
}

void send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Send successful" : "Send failed");
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
  mqtt_message_data message;
  memcpy(&message, data, sizeof(message));
  if (strcmp(message.topic, "REGISTER") == 0)
  {
    add_mac_topic(mac_addr, message.message);
    esp_now_peer_info_t peer_info{{mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]}};
    //esp_err_t code =
    esp_now_add_peer(&peer_info);
    /*bool success = code == ESP_OK;
    char str[18 + 5] = {0};
    sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X %s", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], success ? "  OK" : "FAIL");
    haClient.getClient().publish("home/" NAME "/espnow", str);*/
  }
  else if (strcmp(message.topic, "UNREGISTER") == 0)
  {
    remove_mac_topic(mac_addr, message.message);
  }
  else
  {
    haClient.getClient().publish(message.topic, message.message);
  }
  //list_all_mac_topics();
}

void setup()
{
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
  {
    Serial.print("Error opening file ");
    Serial.println(s1 + s2);
  }
  yield();
  /*int len = strlen(message);
  uint8_t msg[len + 1];
  for (size_t i = 0; i < len; i++)
  {
    msg[i] = message[i];
  }
  msg[len] = '\0';*/

  //esp_now_send(NULL, msg, min(len, 250));
  mqtt_message_data msg;
  for (size_t i = 0; i < sizeof(msg.topic) && topic[i - 1] != '\0'; i++)
  {
    msg.topic[i] = topic[i];
  }
  msg.topic[sizeof(msg.topic) - 1] = '\0';
  for (size_t i = 0; i < sizeof(msg.message) && message[i - 1] != '\0'; i++)
  {
    msg.message[i] = message[i];
  }
  msg.message[sizeof(msg.message) - 1] = '\0';

  execute_for_topic(topic, [&msg](const uint8_t *mac) {
    if (!esp_now_is_peer_exist(mac))
    {
      esp_now_peer_info_t peer_info{{mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]}};
      esp_now_add_peer(&peer_info);
    }
    esp_now_send(mac, (uint8_t *)&msg, sizeof(msg));
    yield();
  });
  //esp_now_send(NULL, (uint8_t *)&msg, sizeof(msg));
}